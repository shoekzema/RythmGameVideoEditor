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

    // Get the video duration in frames
    Uint32 getVideoDurationInFrames() {
        if (!formatContext) return 0.0; // Return 0 if the format context is invalid

        double durationInSeconds = getVideoDuration();

        // Get the frame rate of the video stream
        AVRational frameRate = formatContext->streams[streamIndex]->avg_frame_rate;
        double framesPerSecond = av_q2d(frameRate);

        // Calculate the total number of frames
        return static_cast<Uint32>(durationInSeconds * framesPerSecond);
    }

    // Get the video duration in frames with a target fps
    Uint32 getVideoDurationInFrames(int targetFramerate) {
        if (!formatContext) return 0.0; // Return 0 if the format context is invalid

        double durationInSeconds = getVideoDuration();

        // Calculate the total number of frames
        return static_cast<Uint32>(durationInSeconds * targetFramerate);
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
        if (!formatContext) return 0.0; // Return 0 if the format context is invalid

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
        if (!formatContext) return 0.0; // Return 0 if the format context is invalid

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