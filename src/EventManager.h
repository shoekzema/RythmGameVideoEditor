#pragma once
#include <functional>
#include <vector>
#include <map>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

enum class EventType {
    VideoSelected,
    VideoStopped,
};

struct VideoData {
    AVFormatContext* formatContext; // Manages the media container, holds info about streams, formats, etc.
    AVCodecContext* codecContext; // Manages decoding of the video stream.
    AVFrame* frame; // Holds decoded video frame data.
    AVFrame* rgbFrame; // Holds video frame data converted to RGB format for easier processing.
    SwsContext* swsContext; // Used for converting the frame to the desired format (e.g., YUV to RGB).
    int videoStreamIndex; // The index of the video stream (because a file might have multiple streams).

    VideoData() : formatContext(nullptr), codecContext(nullptr), frame(nullptr), rgbFrame(nullptr), swsContext(nullptr), videoStreamIndex(-1) {}

    // Cleanup video data when it's no longer used
    ~VideoData() {
        // Free the frames
        if (frame) av_frame_free(&frame);
        if (rgbFrame) av_frame_free(&rgbFrame);

        // Close codec and free context
        if (codecContext) avcodec_free_context(&codecContext);

        // Close format context
        if (formatContext) avformat_close_input(&formatContext);
        if (swsContext) sws_freeContext(swsContext);
    }
};

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
