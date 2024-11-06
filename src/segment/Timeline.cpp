#include "Segment.h"

Timeline::Timeline(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Segment* parent, SDL_Color color)
    : Segment(x, y, w, h, renderer, eventManager, parent, color) { }

Timeline::~Timeline() {
    // No need to delete renderer since it is managed elsewhere
}

void Timeline::render() {
    SDL_SetRenderDrawColor(p_renderer, p_color.r, p_color.g, p_color.b, p_color.a);
    SDL_RenderFillRect(p_renderer, &rect);

    // Draw the video and audio track lines (example dimensions)
    SDL_SetRenderDrawColor(p_renderer, 255, 255, 255, 255);
    SDL_Rect videoTrack = { rect.x + 50, rect.h + 50, rect.w - 100, 20 }; // Video track
    SDL_Rect audioTrack = { rect.x + 50, rect.h + 100, rect.w - 100, 20 }; // Audio track
    SDL_RenderFillRect(p_renderer, &videoTrack);
    SDL_RenderFillRect(p_renderer, &audioTrack);

    // Draw video and audio segments
    SDL_SetRenderDrawColor(p_renderer, 0, 0, 255, 255);
    for (auto &segment : m_videoSegments) {
        SDL_Rect segmentRect = { videoTrack.x + segment.timelinePosition, videoTrack.y, segment.timelinePosition + (int)segment.duration, 20 };
        SDL_RenderFillRect(p_renderer, &segmentRect);
    }
    for (auto& segment : m_audioSegments) {
        SDL_Rect segmentRect = { audioTrack.x + segment.timelinePosition, audioTrack.y, segment.timelinePosition + (int)segment.duration, 20 };
        SDL_RenderFillRect(p_renderer, &segmentRect);
    }

    // Draw the current time indicator (a vertical line)
    int indicatorX = 50 + (int)m_currentTime;
    SDL_SetRenderDrawColor(p_renderer, 255, 0, 0, 255); // Red line for current time
    SDL_RenderDrawLine(p_renderer, rect.x + indicatorX, rect.h + 40, rect.x + indicatorX, rect.h + 130);
}

void Timeline::update(int x, int y, int w, int h) {
    rect = { x, y, w, h };
}

void Timeline::handleEvent(SDL_Event& event) {
    switch (event.type) {
    case SDL_KEYDOWN:
        // Check if the key pressed was the spacebar
        if (event.key.keysym.sym == SDLK_SPACE) {
            if (playing) {
                playing = false;
                m_currentTime = m_startPlayTime;
            }
            else {
                playing = true;
                m_startTime = SDL_GetTicks() - static_cast<Uint32>(m_currentTime * 1000);
            }
        }
    }
}

VideoSegment* Timeline::getCurrentVideoSegment() {
    double currentTime = getCurrentTime();

    // Iterate over video segments to find which one is active at currentTime
    for (auto& segment : m_videoSegments) {
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
    for (auto& segment : m_audioSegments) {
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
        m_currentTime = (now - m_startTime) / 1000.0; // Update time in seconds
    }
    return m_currentTime;
}

void Timeline::setCurrentTime(double time) {
    m_currentTime = time;
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
    if (!m_videoSegments.empty()) {
        VideoSegment lastSegment = m_videoSegments.back();
        position = lastSegment.timelinePosition + lastSegment.duration;
    }

    // Create and add a new videoSegment
    VideoSegment videoSegment = {
        .videoData = data,
        .sourceStartTime = 0.0,
        .duration = data->getVideoDuration(),
        .timelinePosition = position
    };
    m_videoSegments.push_back(videoSegment);
}

void Timeline::addAudioSegment(AudioData* data) {
    double position = 0.0;
    // Get the x-position on the right of the last audio segment
    if (!m_audioSegments.empty()) {
        AudioSegment lastSegment = m_audioSegments.back();
        position = lastSegment.timelinePosition + lastSegment.duration;
    }

    // Create and add a new audioSegment
    AudioSegment audioSegment = {
        .audioData = data,
        .sourceStartTime = 0.0,
        .duration = data->getAudioDuration(),
        .timelinePosition = position
    };
    m_audioSegments.push_back(audioSegment);
}
