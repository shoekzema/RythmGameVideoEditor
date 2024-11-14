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

        SDL_RenderDrawLine(p_renderer, xPos, rect.y + textRect.h, xPos, rect.y + m_topBarheight); // Big label
        SDL_RenderDrawLine(p_renderer, xPos + m_timeLabelInterval / 2, rect.y + textRect.h * 1.5, xPos + m_timeLabelInterval / 2, rect.y + m_topBarheight); // Small halfway label

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
    static bool mouseInThisSegment = false;

    switch (event.type) {
    case SDL_MOUSEMOTION: {
        SDL_Point mousePoint = { event.motion.x, event.motion.y };
        mouseInThisSegment = SDL_PointInRect(&mousePoint, &rect) ? true : false;
        break;
    }
    case SDL_KEYDOWN: {
        // Check if the key pressed was the spacebar
        if (event.key.keysym.sym == SDLK_SPACE) {
            if (m_playing) {
                m_playing = false;
                m_currentTime = m_startPlayTime;
            }
            else {
                m_playing = true;
                m_startTime = SDL_GetTicks() - static_cast<Uint32>(m_currentTime * 1000 / m_fps);
            }
        }
        break;
    }
    case SDL_MOUSEBUTTONDOWN: {
        SDL_Point mousePoint = { event.button.x, event.button.y };

        // If clicked outside this segment, do nothing
        if (!SDL_PointInRect(&mousePoint, &rect)) break;

        // If clicked in the left column, do nothing
        if (mousePoint.x < m_trackStartXPos) break;

        Uint32 selectedFrame = (mousePoint.x - rect.x - m_trackStartXPos) * m_zoom / m_timeLabelInterval + m_scrollOffset;

        VideoSegment* selectedVideoSegment = getVideoSegmentAtPos(mousePoint.x, mousePoint.y);
        AudioSegment* selectedAudioSegment = getAudioSegmentAtPos(mousePoint.x, mousePoint.y);

        if (selectedVideoSegment) break; // If we selected a video segment, do nothing FOR NOW ... TODO: select it
        if (selectedAudioSegment) break; // If we selected an audio segment, do nothing FOR NOW ... TODO: select it

        // If nothing is selected, then we want to move the currentTime to the selected time (and, if playing, pause)
        m_playing = false;
        setCurrentTime(selectedFrame);

        break;
    }
    case SDL_MOUSEWHEEL: {
        if (!mouseInThisSegment) break;

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
}

VideoSegment* Timeline::getVideoSegmentAtPos(int x, int y) {
    Uint32 selectedFrame = (x - rect.x - m_trackStartXPos) * m_zoom / m_timeLabelInterval + m_scrollOffset;

    // Iterate over video tracks
    for (int i = 0; i < m_videoTracks.size(); i++) {
        // If inside this track
        if (y >= rect.y + m_topBarheight + m_trackHeight * i && y <= rect.y + m_topBarheight + m_trackHeight * (i + 1)) {
            // Iterate over video segments to find which one is active at currentTime
            for (VideoSegment& segment : m_videoTracks[i]) {
                if (selectedFrame >= segment.timelinePosition && selectedFrame <= segment.timelineDuration) {
                    return &segment;
                }
            }
        }
    }
    return nullptr;
}

AudioSegment* Timeline::getAudioSegmentAtPos(int x, int y) {
    Uint32 selectedFrame = (x - rect.x - m_trackStartXPos) * m_zoom / m_timeLabelInterval + m_scrollOffset;

    // Iterate over audio tracks
    for (int i = 0; i < m_audioTracks.size(); i++) {
        // If inside this track
        if (y >= rect.y + m_topBarheight + m_trackHeight * (m_videoTracks.size() + i) && y <= rect.y + m_topBarheight + m_trackHeight * (m_videoTracks.size() + i + 1)) {
            // Iterate over video segments to find which one is active at currentTime
            for (AudioSegment& segment : m_audioTracks[i]) {
                if (selectedFrame >= segment.timelinePosition && selectedFrame <= segment.timelineDuration) {
                    return &segment;
                }
            }
        }
    }
    return nullptr;
}

bool Timeline::isCollidingWithOtherSegment(VideoSegment* videoSegment, int trackID) {
    // Iterate over video segments to find which one overlaps with the input segment
    for (VideoSegment& segment : m_videoTracks[trackID]) {
        if (videoSegment->overlapsWith(&segment)) return true;
    }
    return false;
}

bool Timeline::isCollidingWithOtherSegment(AudioSegment* audioSegment, int trackID) {
    // Iterate over video segments to find which one overlaps with the input segment
    for (AudioSegment& segment : m_audioTracks[trackID]) {
        if (audioSegment->overlapsWith(&segment)) return true;
    }
    return false;
}

VideoSegment* Timeline::getCurrentVideoSegment() {
    if (m_videoTracks.empty()) return nullptr;

    Uint32 currentTime = getCurrentTime();

    // Iterate over video tracks 
    for (auto& track : m_videoTracks) {
        // Iterate over video segments to find which one is active at currentTime
        for (VideoSegment& segment : track) {
            if (currentTime >= segment.timelinePosition &&
                currentTime <= segment.timelinePosition + segment.timelineDuration) {
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
                currentTime <= segment.timelinePosition + segment.timelineDuration) {
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
    m_startPlayTime = time;
}

Segment* Timeline::findTypeImpl(const std::type_info& type) {
    if (type == typeid(Timeline)) {
        return this;
    }
    return nullptr;
}

void Timeline::addAssetSegments(AssetData* data, int mouseX, int mouseY) {
    SDL_Point mousePoint = { mouseX, mouseY };

    // If outside this segment, do nothing
    if (!SDL_PointInRect(&mousePoint, &rect)) return;

    // If in the left column, do nothing
    if (mousePoint.x < m_trackStartXPos) return;

    Uint32 selectedFrame = (mousePoint.x - rect.x - m_trackStartXPos) * m_zoom / m_timeLabelInterval + m_scrollOffset;

    int trackID = 0; // If video -> videoTrackID   |   If audio -> audioTrackID   |   If video with audio -> videoTrackID == audioTrackID
    // Iterate over video and audio tracks to find the trackID
    for (int i = 0; i < m_videoTracks.size(); i++) {
        if (mousePoint.y >= rect.y + m_topBarheight + m_trackHeight * i && mousePoint.y <= rect.y + m_topBarheight + m_trackHeight * (i + 1)) {
            trackID = i;
        }
    }
    // Iterate over audio tracks
    for (int i = 0; i < m_audioTracks.size(); i++) {
        if (mousePoint.y >= rect.y + m_topBarheight + m_trackHeight * (m_videoTracks.size() + i) && mousePoint.y <= rect.y + m_topBarheight + m_trackHeight * (m_videoTracks.size() + i + 1)) {
            trackID = i;
        }
    }

    VideoSegment videoSegment;
    AudioSegment audioSegment;

    // In case the asset has video
    if (data->videoData) {
        if (m_videoTracks.empty()) return;

        // If the asset also has audio, check if both can be placed
        if (data->audioData) {
            if (m_audioTracks.empty()) return;
        }

        // Create and add a new videoSegment
        videoSegment = {
            .videoData = data->videoData,
            .sourceStartTime = 0,
            .duration = data->videoData->getVideoDurationInFrames(),
            .timelinePosition = selectedFrame,
            .timelineDuration = data->videoData->getVideoDurationInFrames(m_fps),
            .fps = data->videoData->getFPS()
        };

        // Cannot drop here, because it would overlap with another segment
        if (isCollidingWithOtherSegment(&videoSegment, trackID)) return;
    }
    // In case the asset has audio
    if (data->audioData) {
        if (m_audioTracks.empty()) return;

        // Create and add a new audioSegment
        audioSegment = {
            .audioData = data->audioData,
            .sourceStartTime = 0,
            .duration = data->audioData->getAudioDurationInFrames(),
            .timelinePosition = selectedFrame,
            .timelineDuration = data->audioData->getAudioDurationInFrames(m_fps)
        };

        // Cannot drop here, because it would overlap with another segment
        if (isCollidingWithOtherSegment(&audioSegment, trackID)) return;
    }

    // If we reached here, then the new video segment and/or audio segment can be added
    if (data->videoData) {
        m_videoTracks[trackID].push_back(videoSegment);
    }
    if (data->audioData) {
        m_audioTracks[trackID].push_back(audioSegment);
    }
}
