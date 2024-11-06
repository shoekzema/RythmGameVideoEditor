#pragma once

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

    // Get the video duration in seconds
    double getVideoDuration() {
        if (!formatContext) return 0.0; // Return 0 if the format context is invalid

        // FFmpeg stores the duration in AVFormatContext::duration in AV_TIME_BASE units
        int64_t durationInMicroseconds = formatContext->duration;

        // Convert to seconds: AV_TIME_BASE is 1,000,000 (microseconds per second)
        return (double)durationInMicroseconds / AV_TIME_BASE;
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