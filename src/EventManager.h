#pragma once
#include <functional>
#include <map>
#include "VideoData.h"

// Type of event to emit or subscribe to
enum class EventType {
    VideoSelected,
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
