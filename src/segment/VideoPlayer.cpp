#include "Segment.h"

// Constructor
VideoPlayer::VideoPlayer(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, SDL_Color color)
    : Segment(x, y, w, h, renderer, eventManager, color), videoData(nullptr), videoTexture(nullptr)
{
    // Subscribe to the VideoSelected event
    eventManager->subscribe(EventType::VideoSelected, [this](VideoData* videoData) {
        loadAndPlayVideo(videoData);
    });
}

// Destructor to clean up textures
VideoPlayer::~VideoPlayer() {
}

void VideoPlayer::loadAndPlayVideo(VideoData* videoData) {
    if (videoData) {
        VideoPlayer::videoData = videoData;

        // Set frame timing based on video FPS
        frameDurationMs = (1.0 / videoData->codecContext->framerate.num) * 1000.0; // ms per frame
        lastFrameTime = 0; //SDL_GetTicks(); // Record the start time
        playing = true;

        playVideo();
    }
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

    while (av_read_frame(videoData->formatContext, &packet) >= 0) {
        if (packet.stream_index == videoData->videoStreamIndex) {
            // Send the packet to the codec for decoding
            avcodec_send_packet(videoData->codecContext, &packet);

            // Receive the decoded frame from the codec
            int ret = avcodec_receive_frame(videoData->codecContext, videoData->frame);
            if (ret >= 0) {
                av_packet_unref(&packet);
                return videoData->frame;  // Return the decoded frame
            }
        }
        av_packet_unref(&packet);
    }

    return nullptr; // No more frames available
}

void VideoPlayer::updateTextureFromFrame(AVFrame* frame) {
    // Convert the frame to RGB (similar to your original method)
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, videoData->codecContext->width, videoData->codecContext->height, 1);
    uint8_t* buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));

    av_image_fill_arrays(videoData->rgbFrame->data, videoData->rgbFrame->linesize, buffer, AV_PIX_FMT_RGB24, videoData->codecContext->width, videoData->codecContext->height, 1);
    sws_scale(videoData->swsContext, frame->data, frame->linesize, 0, videoData->codecContext->height, videoData->rgbFrame->data, videoData->rgbFrame->linesize);

    // Update the SDL texture with the new frame
    if (!videoTexture) {
        videoTexture = SDL_CreateTexture(renderer,
            SDL_PIXELFORMAT_RGB24,
            SDL_TEXTUREACCESS_STREAMING,
            videoData->codecContext->width,
            videoData->codecContext->height);
    }

    // Lock the texture to update pixel data
    void* pixels;
    int pitch;
    SDL_LockTexture(videoTexture, NULL, &pixels, &pitch);

    for (int y = 0; y < videoData->codecContext->height; y++) {
        memcpy((uint8_t*)pixels + y * pitch, videoData->rgbFrame->data[0] + y * videoData->rgbFrame->linesize[0], videoData->codecContext->width * 3);
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