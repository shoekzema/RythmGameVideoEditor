#include "Segment.h"

AssetsList::AssetsList(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Segment* parent, SDL_Color color)
    : Segment(x, y, w, h, renderer, eventManager, parent, color), videoData(new VideoData()), audioData(new AudioData()), videoFrameTexture(nullptr) { }

AssetsList::~AssetsList() {
    if (videoData) delete videoData;
    if (audioData) delete audioData;
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
        loadFile(droppedFile);
        videoFrameTexture = getFrameTexture(droppedFile);
        SDL_free(event.drop.file);
        break;
    }
    }
}

AssetData* AssetsList::getAssetFromAssetList(int mouseX, int mouseY) {
    // Retrieve the asset corresponding to the mouse position
    // TODO: (You will have a list or array of assets in the AssetList class)
    return new AssetData(videoData, audioData);
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

bool AssetsList::loadFile(const char* filename) {
    // Open the file and reads its header, populating formatContext.
    videoData->formatContext = avformat_alloc_context();
    if (avformat_open_input(&videoData->formatContext, filename, nullptr, nullptr) != 0) {
        std::cerr << "Could not open input file: " << filename << std::endl;
        delete videoData;
        return false;
    }
    audioData->formatContext = avformat_alloc_context();
    if (avformat_open_input(&audioData->formatContext, filename, nullptr, nullptr) != 0) {
        std::cerr << "Could not open input file: " << filename << std::endl;
        delete audioData;
        return false;
    }

    // Find information about streams (audio, video) within the file.
    if (avformat_find_stream_info(videoData->formatContext, NULL) < 0) {
        std::cerr << "Could not find stream information." << std::endl;
        delete videoData;
        return false;
    }
    if (avformat_find_stream_info(audioData->formatContext, NULL) < 0) {
        std::cerr << "Could not find stream information." << std::endl;
        delete audioData;
        return false;
    }

    // Initialize stream indices as invalid
    videoData->streamIndex = -1;
    audioData->streamIndex = -1;

    // Find the first video and audio streams
    for (unsigned int i = 0; i < videoData->formatContext->nb_streams; i++) {
        AVMediaType codecType = videoData->formatContext->streams[i]->codecpar->codec_type;
        if (codecType == AVMEDIA_TYPE_VIDEO && videoData->streamIndex == -1) {
            // Check for album art characteristics
            AVStream* stream = videoData->formatContext->streams[i];
            if (stream->nb_frames <= 1 || stream->r_frame_rate.num < 2) {
                // This is likely album art; ignore it as a video stream
                continue;
            }
            videoData->streamIndex = i;
        }
        if (codecType == AVMEDIA_TYPE_AUDIO && audioData->streamIndex == -1) {
            audioData->streamIndex = i;
        }
    }

    // Check for the presence of video and/or audio streams
    bool hasVideo = videoData->streamIndex != -1;
    bool hasAudio = audioData->streamIndex != -1;

    if (!hasVideo && !hasAudio) {
        std::cerr << "Could not find audio or video stream in file." << std::endl;
        delete videoData;
        delete audioData;
        return false;
    }

    // If a video stream is present, set up the video decoder
    if (hasVideo) {
        // Get the video codec parameters
        AVCodecParameters* videoCodecParams = videoData->formatContext->streams[videoData->streamIndex]->codecpar;
        const AVCodec* videoCodec = avcodec_find_decoder(videoCodecParams->codec_id); // A specific codec for decoding video (e.g., H.264).
        if (!videoCodec) {
            std::cerr << "Unsupported video codec!" << std::endl;
            delete videoData;
            delete audioData;
            return false;
        }

        // Allocate video codec context
        videoData->codecContext = avcodec_alloc_context3(videoCodec);
        if (!videoData->codecContext) {
            std::cerr << "Could not allocate video codec context." << std::endl;
            delete videoData;
            delete audioData;
            return false;
        }

        // Copy codec parameters to the video codec context
        if (avcodec_parameters_to_context(videoData->codecContext, videoCodecParams) < 0) {
            std::cerr << "Could not copy video codec parameters to context." << std::endl;
            delete videoData;
            delete audioData;
            return false;
        }

        // Open the video codec
        if (avcodec_open2(videoData->codecContext, videoCodec, nullptr) < 0) {
            std::cerr << "Could not open video codec." << std::endl;
            delete videoData;
            delete audioData;
            return false;
        }

        // Allocate memory for video frames for decoded video and converted RGB format
        videoData->frame = av_frame_alloc();
        videoData->rgbFrame = av_frame_alloc();

        // Set up SwsContext for frame conversion (YUV -> RGB)
        // Initializes the scaling / conversion context, used to convert the decoded frame(YUV format) to RGB format.
        videoData->swsContext = sws_getContext(videoData->codecContext->width, videoData->codecContext->height, videoData->codecContext->pix_fmt,
            videoData->codecContext->width, videoData->codecContext->height, AV_PIX_FMT_RGB24,
            SWS_BILINEAR, NULL, NULL, NULL);
    }
    else {
        delete videoData;
        videoData = nullptr;
    }

    // If an audio stream is present, set up the audio decoder
    if (hasAudio) {
        // Get the codec parameters for audio
        AVCodecParameters* audioCodecParams = audioData->formatContext->streams[audioData->streamIndex]->codecpar;
        const AVCodec* audioCodec = avcodec_find_decoder(audioCodecParams->codec_id);
        if (!audioCodec) {
            std::cerr << "Unsupported audio codec!" << std::endl;
            delete audioData;
            return false;
        }

        // Allocate audio codec context
        audioData->codecContext = avcodec_alloc_context3(audioCodec);
        if (!audioData->codecContext) {
            std::cerr << "Could not allocate audio codec context." << std::endl;
            delete audioData;
            return false;
        }

        // Copy codec parameters to the codec context
        if (avcodec_parameters_to_context(audioData->codecContext, audioCodecParams) < 0) {
            std::cerr << "Could not copy audio codec parameters to context." << std::endl;
            delete audioData;
            return false;
        }

        // Set the packet timebase
        audioData->codecContext->pkt_timebase = audioData->formatContext->streams[audioData->streamIndex]->time_base;

        // Open the audio codec
        if (avcodec_open2(audioData->codecContext, audioCodec, NULL) < 0) {
            std::cerr << "Could not open audio codec." << std::endl;
            delete audioData;
            return false;
        }

        // Set up the SwrContext for audio resampling
        audioData->swrContext = swr_alloc();
        if (!audioData->swrContext) {
            std::cerr << "Could not allocate SwrContext." << std::endl;
            delete audioData;
            return false;
        }

        // Set the options for the SwrContext
        AVChannelLayout outChannelLayout = AV_CHANNEL_LAYOUT_STEREO;
        AVChannelLayout inChannelLayout;
        inChannelLayout.order = AV_CHANNEL_ORDER_NATIVE;
        inChannelLayout.nb_channels = av_get_channel_layout_nb_channels(audioData->codecContext->channels);
        if (inChannelLayout.nb_channels > 0) {
            inChannelLayout.u.mask = (1ULL << inChannelLayout.nb_channels) - 1; // Assuming all channels are used
        }

        if (swr_alloc_set_opts2(
            &audioData->swrContext,
            &outChannelLayout,              // Output channel layout
            AV_SAMPLE_FMT_S16,              // Output sample format (for SDL)
            44100,                          // Output sample rate
            &inChannelLayout,               // Input channel layout
            (AVSampleFormat)audioData->codecContext->sample_fmt, // Input sample format (FFmpeg decoded format)
            audioData->codecContext->sample_rate,                // Input sample rate
            0,                              // No additional options
            nullptr                         // No logging context
        ) < 0) {
            std::cerr << "Failed to set options for SwrContext." << std::endl;
            swr_free(&audioData->swrContext);
            return false;
        }

        // Initialize the SwrContext
        if (swr_init(audioData->swrContext) < 0) {
            std::cerr << "Failed to initialize the SwrContext." << std::endl;
            delete audioData;
            return false;
        }

        // Allocate memory for audio frames
        audioData->frame = av_frame_alloc();
    }
    else {
        delete audioData;
        audioData = nullptr;
    }

    return true; // Successfully loaded the video/audio file
}

AVFrame* AssetsList::getFrame(int frameIndex) {
    AVPacket packet;
    int frameFinished = 0;
    int currentFrame = 0;

    // Read packets from the media file. Each packet corresponds to a small chunk of data (e.g., a frame).
    while (av_read_frame(videoData->formatContext, &packet) >= 0) {
        if (packet.stream_index == videoData->streamIndex) {
            // Send the packet to the codec for decoding
            avcodec_send_packet(videoData->codecContext, &packet);

            // Receive the decoded frame from the codec
            int ret = avcodec_receive_frame(videoData->codecContext, videoData->frame);
            if (ret >= 0) {
                if (currentFrame == frameIndex) {
                    // Convert the frame to RGB
                    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, videoData->codecContext->width, videoData->codecContext->height, 1);
                    uint8_t* buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));

                    // Perform the conversion to RGB.
                    av_image_fill_arrays(videoData->rgbFrame->data, videoData->rgbFrame->linesize, buffer, AV_PIX_FMT_RGB24, videoData->codecContext->width, videoData->codecContext->height, 1);
                    sws_scale(videoData->swsContext, videoData->frame->data, videoData->frame->linesize, 0, videoData->codecContext->height, videoData->rgbFrame->data, videoData->rgbFrame->linesize);

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