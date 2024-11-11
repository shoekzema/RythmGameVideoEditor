#include <iostream>
#include <string> 
#include <SDL_ttf.h>
#include "Timeline.h"
#include "util.h"

Timeline::Timeline(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Segment* parent, SDL_Color color)
    : Segment(x, y, w, h, renderer, eventManager, parent, color) 
{
    // Start with one empty video track and one empty audio track
    m_videoTracks.push_back({});
    m_audioTracks.push_back({});
}

Timeline::~Timeline() {
    // No need to delete renderer since it is managed elsewhere
}

void Timeline::render() {
    SDL_SetRenderDrawColor(p_renderer, p_color.r, p_color.g, p_color.b, p_color.a);
    SDL_RenderFillRect(p_renderer, &rect);

    // Draw the top bar
    int xPos = rect.x + m_trackStartXPos;
    int yPos = rect.y;
    double timeLabel = 0.0;
    SDL_SetRenderDrawColor(p_renderer, m_timeLabelColor.r, m_timeLabelColor.g, m_timeLabelColor.b, m_timeLabelColor.a);
    while (xPos < rect.x + rect.w) {
        SDL_Rect textRect = renderTextWithCustomSpacing(p_renderer, xPos, yPos,
            getFont(), formatTime(timeLabel, m_fps).c_str(),
            -1, m_timeLabelColor);

        SDL_RenderDrawLine(p_renderer, xPos, rect.h + textRect.h, xPos, rect.h + m_topBarheight); // Big label
        SDL_RenderDrawLine(p_renderer, xPos + m_timeLabelInterval / 2, rect.h + textRect.h * 1.5, xPos + m_timeLabelInterval / 2, rect.h + m_topBarheight); // Small halfway label

        xPos += m_timeLabelInterval;
        timeLabel += m_timeLabelInterval / m_zoom;
    }

    int trackYpos = rect.y + m_topBarheight; // y-position for the next track

    // Draw all video tracks
    SDL_SetRenderDrawColor(p_renderer, 0, 0, 255, 255);
    for (int i = 0; i < m_videoTracks.size(); i++) {
        // Draw the background that everything will be overlayed on. What is left are small lines in between
        SDL_Rect backgroundRect = { rect.x, trackYpos, rect.w, m_rowHeight };
        SDL_SetRenderDrawColor(p_renderer, m_betweenLineColor.r, m_betweenLineColor.g, m_betweenLineColor.b, m_betweenLineColor.a);
        SDL_RenderFillRect(p_renderer, &backgroundRect);

        // Draw the track data
        SDL_Rect videoTrackDataRect = { rect.x, trackYpos + 1, m_trackDataWidth, m_trackHeight };
        SDL_SetRenderDrawColor(p_renderer, m_videoTrackDataColor.r, m_videoTrackDataColor.g, m_videoTrackDataColor.b, m_videoTrackDataColor.a);
        SDL_RenderFillRect(p_renderer, &videoTrackDataRect);
        renderText(p_renderer,
            videoTrackDataRect.x + videoTrackDataRect.w / 4, // At 1/4 of the left of the track data Rect
            videoTrackDataRect.y + videoTrackDataRect.h / 4, // At 1/4 of the top of the track data Rect
            getFontBig(),
            ("V" + std::to_string(i)).c_str());

        // Draw the track background
        SDL_Rect videoTrackRect = { rect.x + m_trackStartXPos, trackYpos + 1, rect.w - m_trackStartXPos, m_trackHeight };
        SDL_SetRenderDrawColor(p_renderer, m_videoTrackBGColor.r, m_videoTrackBGColor.g, m_videoTrackBGColor.b, m_videoTrackBGColor.a);
        SDL_RenderFillRect(p_renderer, &videoTrackRect);

        // Draw all video segments on the track
        for (VideoSegment& segment : m_videoTracks[i]) {
            // Draw the outlines
            SDL_Rect outlineRect = { videoTrackRect.x + (int)(segment.timelinePosition * m_zoom) - 1, videoTrackRect.y - 1, (int)(segment.duration * m_zoom) + 2, m_trackHeight + 2 };
            SDL_SetRenderDrawColor(p_renderer, m_segmentOutlineColor.r, m_segmentOutlineColor.g, m_segmentOutlineColor.b, m_segmentOutlineColor.a);
            SDL_RenderFillRect(p_renderer, &outlineRect);

            // Draw the inside BG
            SDL_Rect segmentRect = { videoTrackRect.x + (int)(segment.timelinePosition * m_zoom) + 1, videoTrackRect.y + 1, (int)(segment.duration * m_zoom) - 2, m_trackHeight - 2 };
            SDL_SetRenderDrawColor(p_renderer, m_videoTrackSegmentColor.r, m_videoTrackSegmentColor.g, m_videoTrackSegmentColor.b, m_videoTrackSegmentColor.a);
            SDL_RenderFillRect(p_renderer, &segmentRect);
        }
        trackYpos += m_rowHeight;
    }
    // Draw all audio tracks
    for (int i = 0; i < m_audioTracks.size(); i++) {
        // Draw the background that everything will be overlayed on. What is left are small lines in between
        SDL_Rect backgroundRect = { rect.x, trackYpos, rect.w, m_rowHeight };
        SDL_SetRenderDrawColor(p_renderer, m_betweenLineColor.r, m_betweenLineColor.g, m_betweenLineColor.b, m_betweenLineColor.a);
        SDL_RenderFillRect(p_renderer, &backgroundRect);

        // Draw the track data
        SDL_Rect audioTrackDataRect = { rect.x, trackYpos + 1, m_trackDataWidth, m_trackHeight };
        SDL_SetRenderDrawColor(p_renderer, m_audioTrackDataColor.r, m_audioTrackDataColor.g, m_audioTrackDataColor.b, m_audioTrackDataColor.a);
        SDL_RenderFillRect(p_renderer, &audioTrackDataRect);
        renderText(p_renderer, 
            audioTrackDataRect.x + audioTrackDataRect.w / 4, // At 1/4 of the left of the track data Rect
            audioTrackDataRect.y + audioTrackDataRect.h / 4, // At 1/4 of the top of the track data Rect
            getFontBig(), 
            ("A" + std::to_string(i)).c_str());

        // Draw the track background
        SDL_Rect audioTrackRect = { rect.x + m_trackStartXPos, trackYpos + 1, rect.w - m_trackStartXPos, m_trackHeight };
        SDL_SetRenderDrawColor(p_renderer, m_audioTrackBGColor.r, m_audioTrackBGColor.g, m_audioTrackBGColor.b, m_audioTrackBGColor.a);
        SDL_RenderFillRect(p_renderer, &audioTrackRect);

        // Draw all audio segments on the track
        for (AudioSegment& segment : m_audioTracks[i]) {
            // Draw the outlines
            SDL_Rect outlineRect = { audioTrackRect.x + (int)(segment.timelinePosition * m_zoom) - 1, audioTrackRect.y - 1, (int)(segment.duration * m_zoom) + 2, m_trackHeight + 2 };
            SDL_SetRenderDrawColor(p_renderer, m_segmentOutlineColor.r, m_segmentOutlineColor.g, m_segmentOutlineColor.b, m_segmentOutlineColor.a);
            SDL_RenderFillRect(p_renderer, &outlineRect);

            // Draw the inside BG
            SDL_Rect segmentRect = { audioTrackRect.x + (int)(segment.timelinePosition * m_zoom) + 1, audioTrackRect.y + 1, (int)(segment.duration * m_zoom) - 2, m_trackHeight - 2 };
            SDL_SetRenderDrawColor(p_renderer, m_audioTrackSegmentColor.r, m_audioTrackSegmentColor.g, m_audioTrackSegmentColor.b, m_audioTrackSegmentColor.a);
            SDL_RenderFillRect(p_renderer, &segmentRect);
        }
        trackYpos += m_rowHeight;
    }

    // Draw the current time indicator (a vertical line)
    int indicatorX = m_trackStartXPos + (int)(m_currentTime * m_zoom);
    SDL_SetRenderDrawColor(p_renderer, m_timeIndicatorColor.r, m_timeIndicatorColor.g, m_timeIndicatorColor.b, m_timeIndicatorColor.a);
    SDL_RenderDrawLine(p_renderer, rect.x + indicatorX, rect.y, rect.x + indicatorX, trackYpos);
}

void Timeline::update(int x, int y, int w, int h) {
    rect = { x, y, w, h };
}

void Timeline::handleEvent(SDL_Event& event) {
    switch (event.type) {
    case SDL_KEYDOWN:
        // Check if the key pressed was the spacebar
        if (event.key.keysym.sym == SDLK_SPACE) {
            if (m_playing) {
                m_playing = false;
                m_currentTime = m_startPlayTime;
            }
            else {
                m_playing = true;
                m_startTime = SDL_GetTicks() - static_cast<Uint32>(m_currentTime * 1000);
            }
        }
    }
}

VideoSegment* Timeline::getCurrentVideoSegment() {
    if (m_videoTracks.empty()) return nullptr;

    double currentTime = getCurrentTime();

    // Iterate over video tracks 
    for (auto& track : m_videoTracks) {
        // Iterate over video segments to find which one is active at currentTime
        for (VideoSegment& segment : track) {
            if (currentTime >= segment.timelinePosition &&
                currentTime <= segment.timelinePosition + segment.duration) {
                return &segment;  // Return the active video segment
            }
        }
    }
    return nullptr;  // No segment found at the current time
}

AudioSegment* Timeline::getCurrentAudioSegment() {
    double currentTime = getCurrentTime();

    // Iterate over audio tracks (TODO: merge audio if multiple tracks have a audioSegment to play at this time)
    for (auto& track : m_audioTracks) {
        // Iterate over audio segments to find which one is active at currentTime
        for (AudioSegment& segment : track) {
            if (currentTime >= segment.timelinePosition &&
                currentTime <= segment.timelinePosition + segment.duration) {
                return &segment;  // Return the active audio segment
            }
        }
    }

    return nullptr;  // No segment found at the current time
}

bool Timeline::isPlaying() {
    return m_playing;
}

double Timeline::getCurrentTime() {
    if (m_playing) {
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
    if (m_videoTracks.empty()) return;

    double position = 0.0;
    // Get the x-position on the right of the last video segment
    if (!m_videoTracks[0].empty()) {
        VideoSegment lastSegment = m_videoTracks[0].back();
        position = lastSegment.timelinePosition + lastSegment.duration;
    }

    // Create and add a new videoSegment
    VideoSegment videoSegment = {
        .videoData = data,
        .sourceStartTime = 0.0,
        .duration = data->getVideoDuration(),
        .timelinePosition = position
    };
    m_videoTracks[0].push_back(videoSegment);
}

void Timeline::addAudioSegment(AudioData* data) {
    if (m_audioTracks.empty()) return;

    double position = 0.0;
    // Get the x-position on the right of the last audio segment
    if (!m_audioTracks[0].empty()) {
        AudioSegment lastSegment = m_audioTracks[0].back();
        position = lastSegment.timelinePosition + lastSegment.duration;
    }

    // Create and add a new audioSegment
    AudioSegment audioSegment = {
        .audioData = data,
        .sourceStartTime = 0.0,
        .duration = data->getAudioDuration(),
        .timelinePosition = position
    };
    m_audioTracks[0].push_back(audioSegment);
}
