#include <SDL.h>
#include <iostream>
#include <vector>
#include <filesystem>
#include "AssetsList.h"
#include "VideoData.h"
#include "util.h"

AssetsList::AssetsList(SDL_Renderer* renderer) { 
    m_renderer = renderer;
}

AssetsList::~AssetsList() { }

bool AssetsList::IsEmpty() { return m_assets.empty(); }

int AssetsList::getAssetCount() { return static_cast<int>(m_assets.size()); }

const std::vector<Asset>* AssetsList::getAllAssets() {
    return &m_assets;
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

    bool fakeVideoStream = false; // If the video Stream is a single frame (e.g. album covers), use it to get the texture and then make videoData null

    // Find the first video and audio streams
    for (unsigned int i = 0; i < newAsset.videoData->formatContext->nb_streams; i++) {
        AVMediaType codecType = newAsset.videoData->formatContext->streams[i]->codecpar->codec_type;
        if (codecType == AVMEDIA_TYPE_VIDEO && newAsset.videoData->streamIndex == -1) {
            // Check for album art characteristics
            AVStream* stream = newAsset.videoData->formatContext->streams[i];
            if (stream->nb_frames <= 1 || stream->r_frame_rate.num < 2) {
                // This is likely album art
                fakeVideoStream = true;
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
            AV_PIX_FMT_RGB24, SWS_BILINEAR, nullptr, nullptr, nullptr);
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
        AVChannelLayout inChannelLayout = newAsset.audioData->codecContext->ch_layout;

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
    newAsset.assetFrameTexture = getThumbnail(newAsset.videoData);

    // Now that we used the video stream, throw it all away, cause we won't ever use it again
    if (fakeVideoStream) {
        delete newAsset.videoData;
        newAsset.videoData = nullptr;
    }

    // Set the asset name
    newAsset.assetName = std::filesystem::path(filepath).filename().string();

    m_assets.push_back(newAsset);
    return true; // Successfully loaded the video/audio file
}

SDL_Texture* AssetsList::getThumbnail(VideoData* videoData) {
#ifdef _WIN32
    // If on a windows machine and m_useWindowsThumbnail is true, get the same thumbnail as windows shows
    if (m_useWindowsThumbnail) {
        std::wstring wideFilePath = to_wstring(videoData->formatContext->url);
        return getWindowsThumbnail(wideFilePath.c_str());
    }
#endif // _WIN32
    // Otherwise, get the first video frame as the thumbnail
    return videoData->getFrameTexture(m_renderer, 0);
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
    hr = SHCreateItemFromParsingName(wfilepath, nullptr, IID_PPV_ARGS(&imageFactory));
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
                texture = SDL_CreateTexture(m_renderer,
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
                if (SDL_LockTexture(texture, nullptr, &pixels, &pitch) != 0) {
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