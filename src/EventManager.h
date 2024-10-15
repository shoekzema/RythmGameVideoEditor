#pragma once
#include <functional>
#include <vector>
#include <map>

enum class EventType {
    VideoSelected,
    VideoStopped
};

//struct VideoData {
//    AVFormatContext* formatContext;
//    AVCodecContext* codecContext;
//    AVFrame* frame;
//    AVFrame* rgbFrame;
//    SwsContext* swsContext;
//    int videoStreamIndex;
//
//    VideoData()
//        : formatContext(nullptr), codecContext(nullptr), frame(nullptr), rgbFrame(nullptr), swsContext(nullptr), videoStreamIndex(-1) {}
//
//    ~VideoData() {
//        // Cleanup video data when it's no longer used
//        if (frame) av_frame_free(&frame);
//        if (rgbFrame) av_frame_free(&rgbFrame);
//        if (codecContext) avcodec_free_context(&codecContext);
//        if (formatContext) avformat_close_input(&formatContext);
//        if (swsContext) sws_freeContext(swsContext);
//    }
//};

class EventManager {
public:
    using EventCallback = std::function<void(const std::string&)>;

    // Register an event listener
    void subscribe(EventType eventType, EventCallback callback) {
        listeners[eventType].push_back(callback);
    }

    // Broadcast an event
    void emit(EventType eventType, const std::string& data) {
        for (auto& listener : listeners[eventType]) {
            listener(data);
        }
    }

private:
    std::map<EventType, std::vector<EventCallback>> listeners;
};
