#include <iostream>
#include <filesystem>
#include "AssetsList.h"
#include "util.h"

AssetsList::AssetsList(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Segment* parent, SDL_Color color)
    : Segment(x, y, w, h, renderer, eventManager, parent, color) 
{
    m_altColor = {
        static_cast<Uint8>(std::min(color.r + 8, 255)),
        static_cast<Uint8>(std::min(color.g + 8, 255)),
        static_cast<Uint8>(std::min(color.b + 9, 255)),
        color.a
    };
}

AssetsList::~AssetsList() {}

void AssetsList::render() {
    SDL_SetRenderDrawColor(p_renderer, p_color.r, p_color.g, p_color.b, p_color.a);
    SDL_RenderFillRect(p_renderer, &rect); // Draw background

    SDL_Rect thumbnailRect = {10, 8, 96, 54}; // w:h ratio = 16:9
    for (int i = 0; i < m_assets.size(); i++) {
        // Use alternative background color rect for every second asset
        if (i % 2 == 1) {
            SDL_Rect altBG = { 0, thumbnailRect.y - 1, rect.w, thumbnailRect.h + 1 };
            SDL_SetRenderDrawColor(p_renderer, m_altColor.r, m_altColor.g, m_altColor.b, m_altColor.a);
            SDL_RenderFillRect(p_renderer, &altBG); // Draw background
        }

        SDL_RenderCopy(p_renderer, m_assets[i].videoFrameTexture, nullptr, &thumbnailRect);

        SDL_Color textColor = { 255, 255, 255, 255 };
        SDL_Surface* textSurface = TTF_RenderText_Solid(getFont(), m_assets[i].assetName.c_str(), textColor);
        if (!textSurface) {
            printf("Unable to render text surface! SDL_ttf Error: %s\n", TTF_GetError());
        }
        else {
            // Convert surface to texture
            SDL_Texture* textTexture = SDL_CreateTextureFromSurface(p_renderer, textSurface);
            SDL_FreeSurface(textSurface); // Free the surface now that we have a texture

            if (!textTexture) {
                printf("Unable to create texture from rendered text! SDL Error: %s\n", SDL_GetError());
            }        
            
            // Define the destination rectangle for the text
            SDL_Rect textRect;
            textRect.x = thumbnailRect.x + thumbnailRect.w + 6; // X position
            textRect.y = thumbnailRect.y + 4; // Y position
            SDL_QueryTexture(textTexture, NULL, NULL, &textRect.w, &textRect.h); // Get width and height from the texture

            SDL_RenderCopy(p_renderer, textTexture, NULL, &textRect); // Render text

            SDL_DestroyTexture(textTexture);
        }

        thumbnailRect.y += 2 + thumbnailRect.h;
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
        SDL_free(event.drop.file);
        break;
    }
    }
}

AssetData* AssetsList::getAssetFromAssetList(int mouseX, int mouseY) {
    // Retrieve the asset corresponding to the mouse position
    // TODO: (You will have a list or array of assets in the AssetList class)
    if (m_assets.empty()) return nullptr;

    for (int i = 0; i < m_assets.size(); i++) {
        if (true) {
            return new AssetData(m_assets[i].videoData, m_assets[i].audioData);
        }
    }
    return nullptr;
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

bool AssetsList::loadFile(const char* filepath) {
    Asset newAsset;

    // Open the file and reads its header, populating formatContext.
    newAsset.videoData->formatContext = avformat_alloc_context();
    if (avformat_open_input(&newAsset.videoData->formatContext, filepath, nullptr, nullptr) != 0) {
        std::cerr << "Could not open input file: " << filepath << std::endl;
        delete newAsset.videoData;
        return false;
    }
    newAsset.audioData->formatContext = avformat_alloc_context();
    if (avformat_open_input(&newAsset.audioData->formatContext, filepath, nullptr, nullptr) != 0) {
        std::cerr << "Could not open input file: " << filepath << std::endl;
        delete newAsset.audioData;
        return false;
    }

    // Find information about streams (audio, video) within the file.
    if (avformat_find_stream_info(newAsset.videoData->formatContext, NULL) < 0) {
        std::cerr << "Could not find stream information." << std::endl;
        delete newAsset.videoData;
        return false;
    }
    if (avformat_find_stream_info(newAsset.audioData->formatContext, NULL) < 0) {
        std::cerr << "Could not find stream information." << std::endl;
        delete newAsset.audioData;
        return false;
    }

    // Initialize stream indices as invalid
    newAsset.videoData->streamIndex = -1;
    newAsset.audioData->streamIndex = -1;

    // Find the first video and audio streams
    for (unsigned int i = 0; i < newAsset.videoData->formatContext->nb_streams; i++) {
        AVMediaType codecType = newAsset.videoData->formatContext->streams[i]->codecpar->codec_type;
        if (codecType == AVMEDIA_TYPE_VIDEO && newAsset.videoData->streamIndex == -1) {
            // Check for album art characteristics
            AVStream* stream = newAsset.videoData->formatContext->streams[i];
            if (stream->nb_frames <= 1 || stream->r_frame_rate.num < 2) {
                // This is likely album art; ignore it as a video stream
                continue;
            }
            newAsset.videoData->streamIndex = i;
        }
        if (codecType == AVMEDIA_TYPE_AUDIO && newAsset.audioData->streamIndex == -1) {
            newAsset.audioData->streamIndex = i;
        }
    }

    // Check for the presence of video and/or audio streams
    bool hasVideo = newAsset.videoData->streamIndex != -1;
    bool hasAudio = newAsset.audioData->streamIndex != -1;

    if (!hasVideo && !hasAudio) {
        std::cerr << "Could not find audio or video stream in file." << std::endl;
        delete newAsset.videoData;
        delete newAsset.audioData;
        return false;
    }

    // If a video stream is present, set up the video decoder
    if (hasVideo) {
        // Get the video codec parameters
        AVCodecParameters* videoCodecParams = newAsset.videoData->formatContext->streams[newAsset.videoData->streamIndex]->codecpar;
        const AVCodec* videoCodec = avcodec_find_decoder(videoCodecParams->codec_id); // A specific codec for decoding video (e.g., H.264).
        if (!videoCodec) {
            std::cerr << "Unsupported video codec!" << std::endl;
            delete newAsset.videoData;
            delete newAsset.audioData;
            return false;
        }

        // Allocate video codec context
        newAsset.videoData->codecContext = avcodec_alloc_context3(videoCodec);
        if (!newAsset.videoData->codecContext) {
            std::cerr << "Could not allocate video codec context." << std::endl;
            delete newAsset.videoData;
            delete newAsset.audioData;
            return false;
        }

        // Copy codec parameters to the video codec context
        if (avcodec_parameters_to_context(newAsset.videoData->codecContext, videoCodecParams) < 0) {
            std::cerr << "Could not copy video codec parameters to context." << std::endl;
            delete newAsset.videoData;
            delete newAsset.audioData;
            return false;
        }

        // Open the video codec
        if (avcodec_open2(newAsset.videoData->codecContext, videoCodec, nullptr) < 0) {
            std::cerr << "Could not open video codec." << std::endl;
            delete newAsset.videoData;
            delete newAsset.audioData;
            return false;
        }

        // Allocate memory for video frames for decoded video and converted RGB format
        newAsset.videoData->frame = av_frame_alloc();
        newAsset.videoData->rgbFrame = av_frame_alloc();

        // Set up SwsContext for frame conversion (YUV -> RGB)
        // Initializes the scaling / conversion context, used to convert the decoded frame(YUV format) to RGB format.
        newAsset.videoData->swsContext = sws_getContext(newAsset.videoData->codecContext->width, newAsset.videoData->codecContext->height, 
            newAsset.videoData->codecContext->pix_fmt,
            newAsset.videoData->codecContext->width, newAsset.videoData->codecContext->height, 
            AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);
    }
    else {
        delete newAsset.videoData;
        newAsset.videoData = nullptr;
    }

    // If an audio stream is present, set up the audio decoder
    if (hasAudio) {
        // Get the codec parameters for audio
        AVCodecParameters* audioCodecParams = newAsset.audioData->formatContext->streams[newAsset.audioData->streamIndex]->codecpar;
        const AVCodec* audioCodec = avcodec_find_decoder(audioCodecParams->codec_id);
        if (!audioCodec) {
            std::cerr << "Unsupported audio codec!" << std::endl;
            delete newAsset.audioData;
            return false;
        }

        // Allocate audio codec context
        newAsset.audioData->codecContext = avcodec_alloc_context3(audioCodec);
        if (!newAsset.audioData->codecContext) {
            std::cerr << "Could not allocate audio codec context." << std::endl;
            delete newAsset.audioData;
            return false;
        }

        // Copy codec parameters to the codec context
        if (avcodec_parameters_to_context(newAsset.audioData->codecContext, audioCodecParams) < 0) {
            std::cerr << "Could not copy audio codec parameters to context." << std::endl;
            delete newAsset.audioData;
            return false;
        }

        // Set the packet timebase
        newAsset.audioData->codecContext->pkt_timebase = newAsset.audioData->formatContext->streams[newAsset.audioData->streamIndex]->time_base;

        // Open the audio codec
        if (avcodec_open2(newAsset.audioData->codecContext, audioCodec, NULL) < 0) {
            std::cerr << "Could not open audio codec." << std::endl;
            delete newAsset.audioData;
            return false;
        }

        // Set up the SwrContext for audio resampling
        newAsset.audioData->swrContext = swr_alloc();
        if (!newAsset.audioData->swrContext) {
            std::cerr << "Could not allocate SwrContext." << std::endl;
            delete newAsset.audioData;
            return false;
        }

        // Set the options for the SwrContext
        AVChannelLayout outChannelLayout = AV_CHANNEL_LAYOUT_STEREO;
        AVChannelLayout inChannelLayout;
        inChannelLayout.order = AV_CHANNEL_ORDER_NATIVE;
        inChannelLayout.nb_channels = av_get_channel_layout_nb_channels(newAsset.audioData->codecContext->channels);
        if (inChannelLayout.nb_channels > 0) {
            inChannelLayout.u.mask = (1ULL << inChannelLayout.nb_channels) - 1; // Assuming all channels are used
        }

        if (swr_alloc_set_opts2(
            &newAsset.audioData->swrContext,
            &outChannelLayout,              // Output channel layout
            AV_SAMPLE_FMT_S16,              // Output sample format (for SDL)
            44100,                          // Output sample rate
            &inChannelLayout,               // Input channel layout
            (AVSampleFormat)newAsset.audioData->codecContext->sample_fmt, // Input sample format (FFmpeg decoded format)
            newAsset.audioData->codecContext->sample_rate,                // Input sample rate
            0,                              // No additional options
            nullptr                         // No logging context
        ) < 0) {
            std::cerr << "Failed to set options for SwrContext." << std::endl;
            swr_free(&newAsset.audioData->swrContext);
            return false;
        }

        // Initialize the SwrContext
        if (swr_init(newAsset.audioData->swrContext) < 0) {
            std::cerr << "Failed to initialize the SwrContext." << std::endl;
            delete newAsset.audioData;
            return false;
        }

        // Allocate memory for audio frames
        newAsset.audioData->frame = av_frame_alloc();
    }
    else {
        delete newAsset.audioData;
        newAsset.audioData = nullptr;
    }

    // Set a video/audio thumbnail texture
    newAsset.videoFrameTexture = getFrameTexture(newAsset.videoData);

    // Set the asset name
    newAsset.assetName = std::filesystem::path(filepath).filename().string();

    m_assets.push_back(newAsset);
    return true; // Successfully loaded the video/audio file
}

AVFrame* AssetsList::getFrame(VideoData* videoData, int frameIndex) {
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
                    return videoData->rgbFrame; // Return the RGB frame
                }
                currentFrame++;
            }
        }
        av_packet_unref(&packet);
    }

    return nullptr;  // Frame not found
}

SDL_Texture* AssetsList::getFrameTexture(VideoData* videoData) {
#ifdef _WIN32
    // If on a windows machine and m_useWindowsThumbnail is true, get the same thumbnail as windows shows
    if (m_useWindowsThumbnail) {
        std::wstring wideFilePath = to_wstring(videoData->formatContext->url);
        return getWindowsThumbnail(wideFilePath.c_str());
    }
#endif // _WIN32
    // Get the first video frame and use it as a thumbnail
    AVFrame* frame = getFrame(videoData, 0); // Get the first frame
    if (!frame) {
        return nullptr;
    }

    // Create an SDL_Texture from the frame's RGB data
    SDL_Texture* texture = SDL_CreateTexture(p_renderer,
        SDL_PIXELFORMAT_RGB24,
        SDL_TEXTUREACCESS_STREAMING,
        videoData->codecContext->width,
        videoData->codecContext->height);

    // Lock the texture for pixel access
    void* pixels;
    int pitch;
    SDL_LockTexture(texture, nullptr, &pixels, &pitch);

    // Copy the frame's RGB data into the texture
    for (int y = 0; y < videoData->codecContext->height; y++) {
        memcpy((uint8_t*)pixels + y * pitch, videoData->rgbFrame->data[0] + y * videoData->rgbFrame->linesize[0], videoData->codecContext->width * 3);
    }

    SDL_UnlockTexture(texture);
    return texture;
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
                texture = SDL_CreateTexture(p_renderer,
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