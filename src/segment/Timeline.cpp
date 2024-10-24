#include "Segment.h"

Timeline::Timeline(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, SDL_Color color)
    : Segment(x, y, w, h, renderer, eventManager, color), currentTime(0.0), playing(false), lastUpdateTime(0) { }

Timeline::~Timeline() {
    // No need to delete renderer since it is managed elsewhere
}

void Timeline::render() {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);

    // Draw the video and audio track lines (example dimensions)
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect videoTrack = { rect.x + 50, rect.h + 50, 800, 20 }; // Video track
    SDL_Rect audioTrack = { rect.x + 50, rect.h + 100, 800, 20 }; // Audio track
    SDL_RenderFillRect(renderer, &videoTrack);
    SDL_RenderFillRect(renderer, &audioTrack);

    // Draw video and audio segments
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    for (auto &segment : videoSegments) {
        SDL_Rect segmentRect = { videoTrack.x + segment.timelinePosition, videoTrack.y, segment.timelinePosition + (int)segment.duration, 20 };
        SDL_RenderFillRect(renderer, &segmentRect);
    }
    for (auto& segment : audioSegments) {
        SDL_Rect segmentRect = { audioTrack.x + segment.timelinePosition, audioTrack.y, segment.timelinePosition + (int)segment.duration, 20 };
        SDL_RenderFillRect(renderer, &segmentRect);
    }

    // Draw the current time indicator (a vertical line)
    float timePercent = 0; //(float)vs->current_time / (float)vs->duration; // Percent of video duration
    int indicatorX = 50 + (int)(timePercent * 800); // Map to screen width
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red line for current time
    SDL_RenderDrawLine(renderer, rect.x + indicatorX, rect.h + 40, rect.x + indicatorX, rect.h + 130);
}

void Timeline::update(int x, int y, int w, int h) {
    rect = { x, y, w, h };
}

void Timeline::handleEvent(SDL_Event& event) {
}

VideoSegment* Timeline::getCurrentVideoSegment() {
    double currentTime = getCurrentTime();

    // Iterate over video segments to find which one is active at currentTime
    for (auto& segment : videoSegments) {
        if (currentTime >= segment.timelinePosition &&
            currentTime <= segment.timelinePosition + segment.duration) {
            return &segment;  // Return the active video segment
        }
    }

    return nullptr;  // No segment found at the current time
}

AudioSegment* Timeline::getCurrentAudioSegment() {
    double currentTime = getCurrentTime();

    // Iterate over audio segments to find which one is active at currentTime
    for (auto& segment : audioSegments) {
        if (currentTime >= segment.timelinePosition &&
            currentTime <= segment.timelinePosition + segment.duration) {
            return &segment;  // Return the active audio segment
        }
    }

    return nullptr;  // No segment found at the current time
}

double Timeline::getCurrentTime() {
    if (playing) {
        // If playing, calculate the current time based on how long it's been playing
        Uint32 now = SDL_GetTicks();
        currentTime += (now - lastUpdateTime) / 1000.0; // Update time in seconds
        lastUpdateTime = now;
    }
    return currentTime;
}

void Timeline::setCurrentTime(double time) {
    currentTime = time;
    lastUpdateTime = SDL_GetTicks();  // Reset the time tracking
}

Segment* Timeline::findTypeImpl(const std::type_info& type) {
    if (type == typeid(Timeline)) {
        return this;
    }
    return nullptr;
}

void Timeline::addVideoSegment(VideoData* data) {
    double position = 0.0;
    // Get the x-position on the right of the last video segment
    if (!videoSegments.empty()) {
        VideoSegment lastSegment = videoSegments.back();
        position = lastSegment.timelinePosition + lastSegment.duration;
    }

    // Create and add a new videoSegment
    VideoSegment videoSegment = {
        .videoData = data,
        .sourceStartTime = 0.0,
        .duration = data->getVideoDuration(),
        .timelinePosition = position
    };
    videoSegments.push_back(videoSegment);
}

void Timeline::addAudioSegment(VideoData* data) {
    double position = 0.0;
    // Get the x-position on the right of the last audio segment
    if (!audioSegments.empty()) {
        AudioSegment lastSegment = audioSegments.back();
        position = lastSegment.timelinePosition + lastSegment.duration;
    }

    // Create and add a new audioSegment
    AudioSegment audioSegment = {
        .videoData = data,
        .sourceStartTime = 0.0,
        .duration = data->getVideoDuration(),
        .timelinePosition = position
    };
    audioSegments.push_back(audioSegment);
}
