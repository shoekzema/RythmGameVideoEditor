#include "Segment.h"

// Constructor
VideoPlayer::VideoPlayer(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, SDL_Color color)
    : Segment(x, y, w, h, renderer, eventManager, color), codec(nullptr), codecContext(nullptr), formatContext(nullptr), frame(nullptr), rgbFrame(nullptr),
    swsContext(nullptr), videoStreamIndex(0), videoTexture(nullptr)
{
    // Subscribe to the VideoSelected event
    eventManager->subscribe(EventType::VideoSelected, [this](const std::string& videoPath) {
        loadAndPlayVideo(videoPath.c_str());
    });
}

// Destructor to clean up textures
VideoPlayer::~VideoPlayer() {
}

void VideoPlayer::loadAndPlayVideo(const char* videoPath) {
    if (loadVideo(videoPath)) {
        // Set frame timing based on video FPS
        frameDurationMs = (1.0 / codecContext->framerate.num) * 1000.0; // ms per frame
        lastFrameTime = 0; //SDL_GetTicks(); // Record the start time
        playing = true;

        playVideo();
    }
}

bool VideoPlayer::loadVideo(const char* filename) {
    // Video loading logic (similar to what we had in the AssetsList)

    formatContext = avformat_alloc_context();
    if (avformat_open_input(&formatContext, filename, NULL, NULL) != 0) {
        return false;  // Couldn't open the video file
    }
    if (avformat_find_stream_info(formatContext, NULL) < 0) {
        return false;  // Couldn't find stream info
    }
    videoStreamIndex = -1;
    for (unsigned int i = 0; i < formatContext->nb_streams; i++) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            break;
        }
    }
    if (videoStreamIndex == -1) {
        return false;  // Couldn't find a video stream
    }
    AVCodecParameters* codecParams = formatContext->streams[videoStreamIndex]->codecpar;
    codec = avcodec_find_decoder(codecParams->codec_id);
    if (!codec) {
        return false;  // Codec not found
    }
    codecContext = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecContext, codecParams);
    if (avcodec_open2(codecContext, codec, NULL) < 0) {
        return false;  // Couldn't open codec
    }
    frame = av_frame_alloc();
    rgbFrame = av_frame_alloc();

    return true;  // Successfully loaded the video
}

void VideoPlayer::playVideo() {
    playing = true;
}

// Render the thumbnails
void VideoPlayer::render() {
    if (!playing) {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_RenderFillRect(renderer, &rect); // Draw background
    }
    else {
        // Get current time
        Uint32 currentTime = SDL_GetTicks();

        // Check if enough time has passed to display the next frame
        if (currentTime - lastFrameTime >= frameDurationMs) {
            // Update to the next video frame
            AVFrame* frame = getNextFrame();
            if (frame) {
                updateTextureFromFrame(frame);
            }
            lastFrameTime = currentTime;
        }

        // Render the current texture (even if it's the same frame)
        SDL_RenderCopy(renderer, videoTexture, NULL, &rect);
    }
}

// Get the next frame in the video
AVFrame* VideoPlayer::getNextFrame() {
    AVPacket packet;
    int frameFinished = 0;

    while (av_read_frame(formatContext, &packet) >= 0) {
        if (packet.stream_index == videoStreamIndex) {
            // Send the packet to the codec for decoding
            avcodec_send_packet(codecContext, &packet);

            // Receive the decoded frame from the codec
            int ret = avcodec_receive_frame(codecContext, frame);
            if (ret >= 0) {
                av_packet_unref(&packet);
                return frame;  // Return the decoded frame
            }
        }
        av_packet_unref(&packet);
    }

    return nullptr; // No more frames available
}

void VideoPlayer::updateTextureFromFrame(AVFrame* frame) {
    // Convert the frame to RGB (similar to your original method)
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, codecContext->width, codecContext->height, 1);
    uint8_t* buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));

    swsContext = sws_getContext(codecContext->width, codecContext->height, codecContext->pix_fmt,
        codecContext->width, codecContext->height, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, NULL, NULL, NULL);

    av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, buffer, AV_PIX_FMT_RGB24, codecContext->width, codecContext->height, 1);
    sws_scale(swsContext, frame->data, frame->linesize, 0, codecContext->height, rgbFrame->data, rgbFrame->linesize);

    // Update the SDL texture with the new frame
    if (!videoTexture) {
        videoTexture = SDL_CreateTexture(renderer,
            SDL_PIXELFORMAT_RGB24,
            SDL_TEXTUREACCESS_STREAMING,
            codecContext->width,
            codecContext->height);
    }

    // Lock the texture to update pixel data
    void* pixels;
    int pitch;
    SDL_LockTexture(videoTexture, NULL, &pixels, &pitch);

    for (int y = 0; y < codecContext->height; y++) {
        memcpy((uint8_t*)pixels + y * pitch, rgbFrame->data[0] + y * rgbFrame->linesize[0], codecContext->width * 3);
    }

    SDL_UnlockTexture(videoTexture);
}

// Handle events like clicks
void VideoPlayer::handleEvent(SDL_Event& event) {
    switch (event.type) {
    case SDL_MOUSEBUTTONDOWN:
        if (event.button.button == SDL_BUTTON_LEFT) {
        }
        break;
    }
}

// Handle resizing of the container
void VideoPlayer::update(int x, int y, int w, int h) {
    rect = { x, y, w, h };
}