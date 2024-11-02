#pragma once
#include <functional>
#include <vector>
#include <map>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
}

// Type of event to emit or subscribe to
enum class EventType {
    VideoSelected,
};

// Structure holding all preprocessed ffmpeg data to be able to quickly process videos
struct VideoData {
    AVFormatContext* formatContext; // Manages the media container, holds info about streams, formats, etc.
    AVCodecContext* videoCodecContext; // Manages decoding of the video stream.
    AVCodecContext* audioCodecContext; // Manages decoding of the audio stream.
    AVFrame* frame; // Holds decoded video frame data.
    AVFrame* audioFrame; // Holds decoded audio frame data.
    AVFrame* rgbFrame; // Holds video frame data converted to RGB format for easier processing.
    SwsContext* swsContext; // Used for converting the frame to the desired format (e.g., YUV to RGB).
    int videoStreamIndex; // The index of the video stream (because a file might have multiple streams).
    int audioStreamIndex; // The index of the audio stream (because a file might have multiple streams).
    SwrContext* swrCtx;

    VideoData() : formatContext(nullptr), videoCodecContext(nullptr), audioCodecContext(nullptr), 
        frame(nullptr), audioFrame(nullptr), rgbFrame(nullptr), 
        swsContext(nullptr), videoStreamIndex(-1), audioStreamIndex(-1), swrCtx(nullptr) {}

    // Cleanup video data when it's no longer used
    ~VideoData() {
        // Free the frames
        if (frame) av_frame_free(&frame);
        if (audioFrame) av_frame_free(&audioFrame);
        if (rgbFrame) av_frame_free(&rgbFrame);

        // Close codec and free context
        if (videoCodecContext) avcodec_free_context(&videoCodecContext);
        if (audioCodecContext) avcodec_free_context(&audioCodecContext);

        // Close format context
        if (formatContext) avformat_close_input(&formatContext);
        if (swsContext) sws_freeContext(swsContext);

        // Close audio context
        if (swrCtx) swr_free(&swrCtx);
    }

    double getVideoDuration() {
        if (!formatContext) return 0.0; // Return 0 if the format context is invalid

        // FFmpeg stores the duration in AVFormatContext::duration in AV_TIME_BASE units
        int64_t durationInMicroseconds = formatContext->duration;

        // Convert to seconds: AV_TIME_BASE is 1,000,000 (microseconds per second)
        return (double)durationInMicroseconds / AV_TIME_BASE;
    }
};

/**
 * @class Eventmanager
 * @brief Manages communication between classes using events. Classes can emit event signals or subscribe to a event listener.
 */
class EventManager {
public:
    using EventCallback = std::function<void(VideoData*)>;

    // Register an event listener
    void subscribe(EventType eventType, EventCallback callback) {
        listeners[eventType].push_back(callback);
    }

    // Broadcast an event
    void emit(EventType eventType, VideoData* data) {
        for (auto& listener : listeners[eventType]) {
            listener(data);
        }
    }

private:
    std::map<EventType, std::vector<EventCallback>> listeners;
};
