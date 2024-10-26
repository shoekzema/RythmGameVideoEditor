#include "Segment.h"

AssetsList::AssetsList(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Segment* parent, SDL_Color color)
    : Segment(x, y, w, h, renderer, eventManager, parent, color), videoData(new VideoData()), videoFrameTexture(nullptr) { }

AssetsList::~AssetsList() {
    if (videoData) delete videoData;
    if (videoFrameTexture) SDL_DestroyTexture(videoFrameTexture);
}

void AssetsList::render() {
    if (videoFrameTexture) {
        SDL_RenderCopy(renderer, videoFrameTexture, NULL, &rect);
    }
    else {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_RenderFillRect(renderer, &rect); // Draw background
    }
}

void AssetsList::handleEvent(SDL_Event& event) {
    switch (event.type) {
    case SDL_MOUSEBUTTONDOWN:
        if (event.button.button == SDL_BUTTON_LEFT) {
            SDL_Point mouseButton = { event.button.x, event.button.y };

            // If not in this Segment, we ignore it 
            if (SDL_PointInRect(&mouseButton, &rect)) {
                // Broadcast the VideoData object to other segments
                //eventManager->emit(EventType::VideoSelected, videoData);
            }
        }
        break;

    case SDL_MOUSEWHEEL:
        break;

    case SDL_DROPFILE: {
        const char* droppedFile = event.drop.file;
        loadVideo(droppedFile);
        videoFrameTexture = getFrameTexture(droppedFile);
        SDL_free(event.drop.file);
        break;
    }
    }
}

VideoData* AssetsList::getAssetFromAssetList(int mouseX, int mouseY) {
    // Retrieve the asset corresponding to the mouse position
    // TODO: (You will have a list or array of assets in the AssetList class)
    return videoData;
}

double AssetsList::getVideoDuration() {
    if (!videoData->formatContext) {
        return 0.0;  // Return 0 if the asset or format context is invalid
    }

    // FFmpeg stores the duration in AVFormatContext::duration in AV_TIME_BASE units
    int64_t durationInMicroseconds = videoData->formatContext->duration;

    // Convert to seconds: AV_TIME_BASE is 1,000,000 (microseconds per second)
    double durationInSeconds = (double)durationInMicroseconds / AV_TIME_BASE;

    return durationInSeconds;
}

Segment* AssetsList::findTypeImpl(const std::type_info& type) {
    if (type == typeid(AssetsList)) {
        return this;
    }
    return nullptr;
}

void AssetsList::update(int x, int y, int w, int h) {
    rect = { x, y, w, h };
}

bool AssetsList::loadVideo(const char* filename) {
    // Open the file and reads its header, populating formatContext.
    videoData->formatContext = avformat_alloc_context();
    if (avformat_open_input(&videoData->formatContext, filename, NULL, NULL) != 0) {
        return false;  // Couldn't open the video file
    }

    // Find information about streams (audio, video) within the file.
    if (avformat_find_stream_info(videoData->formatContext, NULL) < 0) {
        return false;  // Couldn't find stream info
    }

    // Find the first video and audio stream index
    videoData->videoStreamIndex = -1;
    videoData->audioStreamIndex = -1;

    for (unsigned int i = 0; i < videoData->formatContext->nb_streams; i++) {
        AVMediaType codecType = videoData->formatContext->streams[i]->codecpar->codec_type;
        if (codecType == AVMEDIA_TYPE_VIDEO && videoData->videoStreamIndex == -1) {
            videoData->videoStreamIndex = i;
        }
        if (codecType == AVMEDIA_TYPE_AUDIO && videoData->audioStreamIndex == -1) {
            videoData->audioStreamIndex = i;
        }
    }

    if (videoData->videoStreamIndex == -1) {
        return false;  // Couldn't find a video stream
    }
    if (videoData->audioStreamIndex == -1) {
        return false;  // Couldn't find an audio stream
    }

    // Get the codec and set up the codec context
    AVCodecParameters* codecParams = videoData->formatContext->streams[videoData->videoStreamIndex]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id); // A specific codec for decoding video (e.g., H.264).
    if (!codec) {
        return false;  // Codec not found
    }

    // Allocate and set up the codec context.
    videoData->videoCodecContext = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(videoData->videoCodecContext, codecParams);

    // Open the codec for decoding.
    if (avcodec_open2(videoData->videoCodecContext, codec, NULL) < 0) {
        return false;  // Couldn't open codec
    }

    // Allocate memory for video frames for decoded video and converted RGB format
    videoData->frame = av_frame_alloc();
    videoData->rgbFrame = av_frame_alloc();

    // Set up SwsContext for frame conversion (YUV -> RGB)
    // Initializes the scaling / conversion context, used to convert the decoded frame(YUV format) to RGB format.
    videoData->swsContext = sws_getContext(videoData->videoCodecContext->width, videoData->videoCodecContext->height, videoData->videoCodecContext->pix_fmt,
        videoData->videoCodecContext->width, videoData->videoCodecContext->height, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, NULL, NULL, NULL);


    // Set up the audio codec and context
    AVCodecParameters* audioCodecParams = videoData->formatContext->streams[videoData->audioStreamIndex]->codecpar;
    const AVCodec* audioCodec = avcodec_find_decoder(audioCodecParams->codec_id);
    if (!audioCodec) {
        return false;  // Audio codec not found
    }

    // Allocate and set up the audio codec context
    videoData->audioCodecContext = avcodec_alloc_context3(audioCodec);
    avcodec_parameters_to_context(videoData->audioCodecContext, audioCodecParams);

    // Open the audio codec for decoding
    if (avcodec_open2(videoData->audioCodecContext, audioCodec, NULL) < 0) {
        return false;  // Couldn't open audio codec
    }

    // Allocate memory for audio frames
    videoData->audioFrame = av_frame_alloc();

    return true;  // Successfully loaded the video
}

AVFrame* AssetsList::getFrame(int frameIndex) {
    AVPacket packet;
    int frameFinished = 0;
    int currentFrame = 0;

    // Read packets from the media file. Each packet corresponds to a small chunk of data (e.g., a frame).
    while (av_read_frame(videoData->formatContext, &packet) >= 0) {
        if (packet.stream_index == videoData->videoStreamIndex) {
            // Send the packet to the codec for decoding
            avcodec_send_packet(videoData->videoCodecContext, &packet);

            // Receive the decoded frame from the codec
            int ret = avcodec_receive_frame(videoData->videoCodecContext, videoData->frame);
            if (ret >= 0) {
                if (currentFrame == frameIndex) {
                    // Convert the frame to RGB
                    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, videoData->videoCodecContext->width, videoData->videoCodecContext->height, 1);
                    uint8_t* buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));

                    // Perform the conversion to RGB.
                    av_image_fill_arrays(videoData->rgbFrame->data, videoData->rgbFrame->linesize, buffer, AV_PIX_FMT_RGB24, videoData->videoCodecContext->width, videoData->videoCodecContext->height, 1);
                    sws_scale(videoData->swsContext, videoData->frame->data, videoData->frame->linesize, 0, videoData->videoCodecContext->height, videoData->rgbFrame->data, videoData->rgbFrame->linesize);

                    av_packet_unref(&packet);
                    return videoData->rgbFrame;  // Return the RGB frame
                }
                currentFrame++;
            }
        }
        av_packet_unref(&packet);
    }

    return NULL;  // Frame not found
}

int AssetsList::getFrameCount()
{
    return 0; // TODO
}

SDL_Texture* AssetsList::getFrameTexture(const char* filepath) {
#ifdef _WIN32
    // If on a windows machine, get the same thumbnail as windows shows
    std::wstring wideFilePath = to_wstring(filepath);
    return getWindowsThumbnail(wideFilePath.c_str());
#else
    // If not on a windows machine, get the first video frame and use it as a thumbnail
    AVFrame* frame = getFrame(0);  // Get the first frame
    if (!frame) {
        return nullptr;
    }

    // Create an SDL_Texture from the frame's RGB data
    SDL_Texture* texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_RGB24,
        SDL_TEXTUREACCESS_STREAMING,
        codecContext->width,
        codecContext->height);

    // Lock the texture for pixel access
    void* pixels;
    int pitch;
    SDL_LockTexture(texture, NULL, &pixels, &pitch);

    // Copy the frame's RGB data into the texture
    for (int y = 0; y < codecContext->height; y++) {
        memcpy((uint8_t*)pixels + y * pitch, rgbFrame->data[0] + y * rgbFrame->linesize[0], codecContext->width * 3);
    }

    SDL_UnlockTexture(texture);
    return texture;
#endif // _WIN32
}


#ifdef _WIN32

#include <Shobjidl.h> // Windows shell (explorer) library
#include <comdef.h> // For COM error handling

SDL_Texture* AssetsList::getWindowsThumbnail(const wchar_t* wfilepath) {
    HRESULT hr;
    IShellItemImageFactory* imageFactory = nullptr;
    HBITMAP hBitmap = nullptr;
    SDL_Texture* texture = nullptr;

    // Initialize COM
    hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        return nullptr;
    }

    // Create a ShellItem from the video file path
    hr = SHCreateItemFromParsingName(wfilepath, NULL, IID_PPV_ARGS(&imageFactory));
    if (SUCCEEDED(hr) && imageFactory != nullptr) {
        // Request the thumbnail (let's say 256x256 for a medium-sized thumbnail)
        SIZE size = { 256, 256 };
        hr = imageFactory->GetImage(size, SIIGBF_BIGGERSIZEOK, &hBitmap);
        if (SUCCEEDED(hr) && hBitmap != nullptr) {
            // Get bitmap information
            BITMAP bm;
            if (GetObject(hBitmap, sizeof(bm), &bm) != 0) {
                // Ensure the bitmap is 32 bits (ARGB)
                if (bm.bmBitsPixel != 32) {
                    std::cerr << "Unsupported bitmap format!" << std::endl;
                    DeleteObject(hBitmap);
                    imageFactory->Release();
                    CoUninitialize();
                    return nullptr;
                }

                // Check if bm.bmBits is valid
                if (!bm.bmBits) {
                    std::cerr << "Bitmap bits are NULL!" << std::endl;
                    DeleteObject(hBitmap);
                    imageFactory->Release();
                    CoUninitialize();
                    return nullptr;
                }

                // Create SDL_Texture from the bitmap data
                texture = SDL_CreateTexture(renderer,
                    SDL_PIXELFORMAT_ARGB8888,
                    SDL_TEXTUREACCESS_STREAMING,
                    bm.bmWidth,
                    bm.bmHeight);

                if (!texture) {
                    std::cerr << "Failed to create texture: " << SDL_GetError() << std::endl;
                    return nullptr;
                }

                // Lock the texture for pixel access
                void* pixels;
                int pitch;
                if (SDL_LockTexture(texture, NULL, &pixels, &pitch) != 0) {
                    std::cerr << "Failed to lock texture: " << SDL_GetError() << std::endl;
                    SDL_DestroyTexture(texture);
                    return nullptr;
                }

                // Copy the bitmap data into the texture
                // Each row in the HBITMAP might be padded, so we need to use bm.bmWidthBytes
                for (int y = 0; y < bm.bmHeight; y++) {
                    // Copy each row of pixels from the HBITMAP to the SDL_Texture
                    memcpy((uint8_t*)pixels + y * pitch, (uint8_t*)bm.bmBits + y * bm.bmWidthBytes, bm.bmWidthBytes);
                }

                // Unlock the texture
                SDL_UnlockTexture(texture);
            }
            else {
                std::cerr << "Failed to get bitmap object: " << GetLastError() << std::endl;
            }

            // Free the HBITMAP once done
            DeleteObject(hBitmap);
        }
        else {
            std::cerr << "Failed to get image from shell item." << std::endl;
        }

        // Release the image factory
        imageFactory->Release();
    }
    else {
        std::cerr << "Failed to create shell item from parsing name." << std::endl;
    }

    // Uninitialize COM
    CoUninitialize();

    return texture;
}
#endif // _WIN32