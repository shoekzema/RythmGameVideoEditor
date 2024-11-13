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
    Uint32 timeLabel = m_scrollOffset;
    SDL_SetRenderDrawColor(p_renderer, m_timeLabelColor.r, m_timeLabelColor.g, m_timeLabelColor.b, m_timeLabelColor.a);
    while (xPos < rect.x + rect.w) {
        SDL_Rect textRect = renderTextWithCustomSpacing(p_renderer, xPos, yPos,
            getFont(), formatTime(timeLabel, m_fps).c_str(),
            -1, m_timeLabelColor);

        SDL_RenderDrawLine(p_renderer, xPos, rect.h + textRect.h, xPos, rect.h + m_topBarheight); // Big label
        SDL_RenderDrawLine(p_renderer, xPos + m_timeLabelInterval / 2, rect.h + textRect.h * 1.5, xPos + m_timeLabelInterval / 2, rect.h + m_topBarheight); // Small halfway label

        xPos += m_timeLabelInterval;
        timeLabel += m_zoom;
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
            // If fully outside render view, do not render
            if (m_scrollOffset > segment.timelinePosition + segment.timelineDuration) continue;

            Uint32 xPos = segment.timelinePosition - m_scrollOffset;
            int diff = 0;
            if (m_scrollOffset > segment.timelinePosition) {
                xPos = 0; // If xPos should be negative, make it 0
                diff = m_scrollOffset - segment.timelinePosition; // keep the difference to subtract it from the width
            }

            int renderXPos = videoTrackRect.x + xPos * m_timeLabelInterval / m_zoom;
            int renderWidth = (segment.timelineDuration - diff) * m_timeLabelInterval / m_zoom;

            // Draw the outlines
            SDL_Rect outlineRect = { renderXPos - 1, videoTrackRect.y - 1, renderWidth + 2, m_trackHeight + 2 };
            SDL_SetRenderDrawColor(p_renderer, m_segmentOutlineColor.r, m_segmentOutlineColor.g, m_segmentOutlineColor.b, m_segmentOutlineColor.a);
            SDL_RenderFillRect(p_renderer, &outlineRect);

            // Draw the inside BG
            SDL_Rect segmentRect = { renderXPos + 1, videoTrackRect.y + 1, renderWidth - 2, m_trackHeight - 2 };
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
            // If fully outside render view, do not render
            if (m_scrollOffset > segment.timelinePosition + segment.timelineDuration) continue;

            Uint32 xPos = segment.timelinePosition - m_scrollOffset;
            int diff = 0;
            if (m_scrollOffset > segment.timelinePosition) {
                xPos = 0; // If xPos should be negative, make it 0
                diff = m_scrollOffset - segment.timelinePosition; // keep the difference to subtract it from the width
            }

            int renderXPos = audioTrackRect.x + xPos * m_timeLabelInterval / m_zoom;
            int renderWidth = (segment.timelineDuration - diff) * m_timeLabelInterval / m_zoom;

            // Draw the outlines
            SDL_Rect outlineRect = { renderXPos - 1, audioTrackRect.y - 1, renderWidth + 2, m_trackHeight + 2 };
            SDL_SetRenderDrawColor(p_renderer, m_segmentOutlineColor.r, m_segmentOutlineColor.g, m_segmentOutlineColor.b, m_segmentOutlineColor.a);
            SDL_RenderFillRect(p_renderer, &outlineRect);

            // Draw the inside BG
            SDL_Rect segmentRect = { renderXPos + 1, audioTrackRect.y + 1, renderWidth - 2, m_trackHeight - 2 };
            SDL_SetRenderDrawColor(p_renderer, m_audioTrackSegmentColor.r, m_audioTrackSegmentColor.g, m_audioTrackSegmentColor.b, m_audioTrackSegmentColor.a);
            SDL_RenderFillRect(p_renderer, &segmentRect);
        }
        trackYpos += m_rowHeight;
    }

    // If the currentTime is higher than the scroll offset (leftmost frame)
    if (m_scrollOffset <= m_currentTime) {
        // Draw the current time indicator (a vertical line)
        int indicatorX = m_trackStartXPos + (m_currentTime - m_scrollOffset) * m_timeLabelInterval / m_zoom;
        SDL_SetRenderDrawColor(p_renderer, m_timeIndicatorColor.r, m_timeIndicatorColor.g, m_timeIndicatorColor.b, m_timeIndicatorColor.a);
        SDL_RenderDrawLine(p_renderer, rect.x + indicatorX, rect.y, rect.x + indicatorX, trackYpos);
    }
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
        break;
    case SDL_MOUSEWHEEL:
        // Get the current modifier state (Shift, Ctrl, etc.)
        SDL_Keymod mod = SDL_GetModState();

        // Normalize event.wheel.y to -1, 0, or 1
        int scrollDirection = (event.wheel.y > 0) ? 1 : (event.wheel.y < 0) ? -1 : 0;

        if (mod & KMOD_CTRL) { // Control is held down
            // Zooming in
            if (scrollDirection > 0 && m_zoom > 2) {
                m_zoom /= 2;
            }
            // Zooming out
            if (scrollDirection < 0 && m_zoom < UINT16_MAX / 2) {
                m_zoom *= 2;
            }
        }
        else if (mod & KMOD_SHIFT) { // Shift is held down
            // Vertical scrolling

        }
        else {
            // Horizontal scrolling
            if ((int32_t)m_scrollOffset < scrollDirection * (int32_t)m_zoom) {
                // Cannot go into negative -> make zero instead.
                m_scrollOffset = 0;
            }
            else {
                m_scrollOffset -= scrollDirection * m_zoom;
            }
        }
        break;
    }
}

VideoSegment* Timeline::getCurrentVideoSegment() {
    if (m_videoTracks.empty()) return nullptr;

    Uint32 currentTime = getCurrentTime();

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
    if (m_audioTracks.empty()) return nullptr;

    Uint32 currentTime = getCurrentTime();

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

int Timeline::getFPS() {
    return m_fps;
}

void Timeline::setFPS(int fps) {
    m_fps = fps;
}

Uint32 Timeline::getCurrentTime() {
    if (m_playing) {
        // If playing, calculate the current time based on how long it's been playing
        Uint32 now = SDL_GetTicks();
        m_currentTime = static_cast<Uint32>((now - m_startTime) / 1000.0 * m_fps);
    }
    return m_currentTime;
}

void Timeline::setCurrentTime(Uint32 time) {
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

    Uint32 position = 0;
    // Get the x-position on the right of the last video segment
    if (!m_videoTracks[0].empty()) {
        VideoSegment lastSegment = m_videoTracks[0].back();
        position = lastSegment.timelinePosition + lastSegment.timelineDuration;
    }

    // Create and add a new videoSegment
    VideoSegment videoSegment = {
        .videoData = data,
        .sourceStartTime = 0,
        .duration = data->getVideoDurationInFrames(),
        .timelinePosition = position,
        .timelineDuration = data->getVideoDurationInFrames(m_fps)
    };
    m_videoTracks[0].push_back(videoSegment);
}

void Timeline::addAudioSegment(AudioData* data) {
    if (m_audioTracks.empty()) return;

    Uint32 position = 0;
    // Get the x-position on the right of the last audio segment
    if (!m_audioTracks[0].empty()) {
        AudioSegment lastSegment = m_audioTracks[0].back();
        position = lastSegment.timelinePosition + lastSegment.timelineDuration;
    }

    // Create and add a new audioSegment
    AudioSegment audioSegment = {
        .audioData = data,
        .sourceStartTime = 0,
        .duration = data->getAudioDurationInFrames(),
        .timelinePosition = position,
        .timelineDuration = data->getAudioDurationInFrames(m_fps)
    };
    m_audioTracks[0].push_back(audioSegment);
}
