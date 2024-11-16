#pragma once
#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
}

// Structure holding all preprocessed ffmpeg data to be able to quickly process videos
struct VideoData {
    AVFormatContext* formatContext = nullptr; // Manages the media container, holds info about streams, formats, etc.
    AVCodecContext* codecContext = nullptr; // Manages decoding of the video stream.
    SwsContext* swsContext = nullptr; // Used for converting the frame to the desired format (e.g., YUV to RGB).
    AVFrame* frame = nullptr; // Holds decoded video frame data.
    AVFrame* rgbFrame = nullptr; // Holds video frame data converted to RGB format for easier processing.
    int streamIndex = -1; // The index of the video stream.

    VideoData() {}

    // Cleanup video data when it's no longer used
    ~VideoData() {
        if (frame) { 
            av_frame_free(&frame);
            frame = nullptr;
        }
        if (rgbFrame) {
            av_frame_free(&rgbFrame);
            rgbFrame = nullptr;
        }
        if (codecContext) {
            avcodec_free_context(&codecContext);
            codecContext = nullptr;
        }
        if (formatContext) {
            avformat_close_input(&formatContext);
            formatContext = nullptr;
        }
        if (swsContext) {
            sws_freeContext(swsContext);
            swsContext = nullptr;
        }
    }

    // Get the videos framerate as an AVRational (use av_q2d to convert to double)
    AVRational getFPS() {
        return formatContext->streams[streamIndex]->avg_frame_rate;
    }

    // Get the video duration in seconds
    double getVideoDuration() {
        if (!formatContext) return 0.0; // Return 0 if the format context is invalid

        // FFmpeg stores the duration in AVFormatContext::duration in AV_TIME_BASE units
        int64_t durationInMicroseconds = formatContext->duration;

        // Convert to seconds: AV_TIME_BASE is 1,000,000 (microseconds per second)
        return (double)durationInMicroseconds / AV_TIME_BASE;
    }

    // Get the video duration in frames
    Uint32 getVideoDurationInFrames() {
        if (!formatContext) return 0; // Return 0 if the format context is invalid

        double durationInSeconds = getVideoDuration();
        double framesPerSecond = av_q2d(getFPS());

        // Calculate the total number of frames
        return static_cast<Uint32>(durationInSeconds * framesPerSecond);
    }

    // Get the video duration in frames with a target fps
    Uint32 getVideoDurationInFrames(int targetFramerate) {
        if (!formatContext) return 0; // Return 0 if the format context is invalid

        double durationInSeconds = getVideoDuration();

        // Calculate the total number of frames
        return static_cast<Uint32>(durationInSeconds * targetFramerate);
    }

    // Get a specific frame from a video.
    AVFrame* getFrame(Uint32 frameIndex) {
        AVPacket packet;

        // If we do not want the first frame, seek the frame we want
        if (frameIndex != 0) {
            // Get the timestamp in the stream's time base
            AVRational timeBase = formatContext->streams[streamIndex]->time_base;

            // Convert the desired frame number to a timestamp
            int64_t targetTimestamp = av_rescale_q(frameIndex, getFPS(), timeBase);

            if (av_seek_frame(formatContext, streamIndex, targetTimestamp, AVSEEK_FLAG_BACKWARD) < 0) {
                std::cerr << "Error seeking video to timestamp: " << targetTimestamp << std::endl;
                return nullptr;
            }

            // Flush the codec context buffers to clear any data from previous frames.
            avcodec_flush_buffers(codecContext);
        }

        // Read packets from the media file. Each packet corresponds to a small chunk of data (e.g., a frame).
        while (av_read_frame(formatContext, &packet) >= 0) {
            if (packet.stream_index == streamIndex) {
                // Send the packet to the codec for decoding
                avcodec_send_packet(codecContext, &packet);

                // Receive the decoded frame from the codec
                int ret = avcodec_receive_frame(codecContext, frame);
                if (ret >= 0) {
                    // Convert the frame to RGB
                    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, codecContext->width, codecContext->height, 1);
                    uint8_t* buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));

                    // Perform the conversion to RGB.
                    av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, buffer, AV_PIX_FMT_RGB24, codecContext->width, codecContext->height, 1);
                    sws_scale(swsContext, frame->data, frame->linesize, 0, codecContext->height, rgbFrame->data, rgbFrame->linesize);

                    av_packet_unref(&packet);
                    return rgbFrame; // Return the RGB frame
                }
            }
            av_packet_unref(&packet);
        }

        return nullptr; // Frame not found
    }

    // Get the texture of a specific video frame.
    SDL_Texture* getFrameTexture(SDL_Renderer* renderer, Uint32 frameIndex) {
        AVFrame* frame = getFrame(frameIndex);
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
        SDL_LockTexture(texture, nullptr, &pixels, &pitch);

        // Copy the frame's RGB data into the texture
        for (int y = 0; y < codecContext->height; y++) {
            memcpy((uint8_t*)pixels + y * pitch, rgbFrame->data[0] + y * rgbFrame->linesize[0], codecContext->width * 3);
        }

        SDL_UnlockTexture(texture);
        return texture;
    }
};

// Structure holding all preprocessed ffmpeg data to be able to quickly process videos
struct AudioData {
    AVFormatContext* formatContext = nullptr; // Manages the media container, holds info about streams, formats, etc.
    AVCodecContext* codecContext = nullptr; // Manages decoding of the audio stream.
    SwrContext* swrContext = nullptr; // Used for resampling and converting audio to SDL format.
    AVFrame* frame = nullptr; // Holds decoded audio frame data.
    int streamIndex = -1; // The index of the audio stream.

    AudioData() {}

    // Cleanup audio data when it's no longer used
    ~AudioData() {
        if (frame) {
            av_frame_free(&frame);
            frame = nullptr;
        }
        if (codecContext) {
            avcodec_free_context(&codecContext);
            codecContext = nullptr;
        }
        if (formatContext) {
            avformat_close_input(&formatContext);
            formatContext = nullptr;
        }
        if (swrContext) {
            swr_free(&swrContext);
            swrContext = nullptr;
        }
    }

    // Get the audio duration in seconds
    double getAudioDuration() {
        if (!formatContext) return 0.0; // Return 0 if the format context is invalid

        // FFmpeg stores the duration in AVFormatContext::duration in AV_TIME_BASE units
        int64_t durationInMicroseconds = formatContext->duration;

        // Convert to seconds: AV_TIME_BASE is 1,000,000 (microseconds per second)
        return (double)durationInMicroseconds / AV_TIME_BASE;
    }

    // Get the audio duration in frames
    Uint32 getAudioDurationInFrames() {
        if (!formatContext) return 0; // Return 0 if the format context is invalid

        double durationInSeconds = getAudioDuration();

        // Get the sample rate (samples per second) from the audio stream's codec parameters
        int sampleRate = formatContext->streams[streamIndex]->codecpar->sample_rate;
        if (sampleRate <= 0) return 0; // Return 0 if the sample rate is invalid

        AVStream* audioStream = formatContext->streams[streamIndex];

        // Calculate the audio duration in frames
        double durationInFrames = static_cast<double>(audioStream->duration) * audioStream->time_base.num / audioStream->time_base.den;

        // Calculate the total number of frames
        return static_cast<Uint32>(durationInSeconds * sampleRate);
    }

    // Get the audio duration in frames with a target fps
    Uint32 getAudioDurationInFrames(int targetFramerate) {
        if (!formatContext) return 0; // Return 0 if the format context is invalid

        double durationInSeconds = getAudioDuration();

        // Calculate the total number of frames
        return static_cast<Uint32>(durationInSeconds * targetFramerate);
    }
};

// Struct holding a pointer to videoData and/or audioData (used as generic datatype (mp4, mp3, etc.)
struct AssetData {
    VideoData* videoData;
    AudioData* audioData;

    AssetData(VideoData* videoData, AudioData* audioData) : videoData(videoData), audioData(audioData) {}

    ~AssetData() {
        if (videoData) delete videoData;
        if (audioData) delete audioData;
    }
};