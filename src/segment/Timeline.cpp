#include <iostream>
#include <string> 
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <SDL_ttf.h>
#include "Timeline.h"
#include "util.h"
#include "PopupMenu.h"

Timeline::Timeline(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Segment* parent, SDL_Color color)
    : Segment(x, y, w, h, renderer, eventManager, parent, color) 
{
    m_videoTrackPositionMap[0] = 0;
    m_videoTrackPositionMap[1] = 1;

    m_audioTrackPositionMap[0] = 0;
    m_audioTrackPositionMap[1] = 1;

    m_reverseVideoTrackPositionMap[0] = 0;
    m_reverseVideoTrackPositionMap[1] = 1;

    m_reverseAudioTrackPositionMap[0] = 0;
    m_reverseAudioTrackPositionMap[1] = 1;

    m_nextVideoTrackID = 2;
    m_nextAudioTrackID = 2;
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
        SDL_RenderDrawLine(p_renderer, xPos + m_timeLabelInterval / 2, rect.y + (int)(textRect.h * 1.5), xPos + m_timeLabelInterval / 2, rect.y + m_topBarheight); // Small halfway label

        xPos += m_timeLabelInterval;
        timeLabel += m_zoom;
    }

    // Draw all video tracks
    SDL_SetRenderDrawColor(p_renderer, 0, 0, 255, 255);
    for (int i = 0; i < m_videoTrackPositionMap.size(); i++) {
        int trackYpos = rect.y + m_topBarheight + (static_cast<int>(m_videoTrackPositionMap.size()) - 1 - i) * m_rowHeight;

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
    }

    // Draw all video segments on the video tracks
    for (VideoSegment& segment : m_videoSegments) {
        // If fully outside render view, do not render
        if (m_scrollOffset > segment.timelinePosition + segment.timelineDuration) continue;

        Uint32 xPos = segment.timelinePosition - m_scrollOffset;
        int diff = 0;
        if (m_scrollOffset > segment.timelinePosition) {
            xPos = 0; // If xPos should be negative, make it 0
            diff = m_scrollOffset - segment.timelinePosition; // keep the difference to subtract it from the width
        }

        int renderXPos = rect.x + m_trackStartXPos + xPos * m_timeLabelInterval / m_zoom;
        int trackPos = m_videoTrackPositionMap[segment.trackID];
        int renderYPos = rect.y + m_topBarheight + (static_cast<int>(m_videoTrackPositionMap.size()) - 1 - trackPos) * m_rowHeight;
        int renderWidth = (segment.timelineDuration - diff) * m_timeLabelInterval / m_zoom;

        // Draw the outlines
        SDL_Rect outlineRect = { renderXPos - 1, renderYPos - 1, renderWidth + 2, m_trackHeight + 2 };
        if (std::find(m_selectedVideoSegments.begin(), m_selectedVideoSegments.end(), &segment) != m_selectedVideoSegments.end()) {
            SDL_SetRenderDrawColor(p_renderer, m_segmentHighlightColor.r, m_segmentHighlightColor.g, m_segmentHighlightColor.b, m_segmentHighlightColor.a);
        }
        else {
            SDL_SetRenderDrawColor(p_renderer, m_segmentOutlineColor.r, m_segmentOutlineColor.g, m_segmentOutlineColor.b, m_segmentOutlineColor.a);
        }
        SDL_RenderFillRect(p_renderer, &outlineRect);

        // Draw the inside BG
        SDL_Rect segmentRect = { renderXPos + 1, renderYPos + 1, renderWidth - 2, m_trackHeight - 2 };
        SDL_SetRenderDrawColor(p_renderer, m_videoTrackSegmentColor.r, m_videoTrackSegmentColor.g, m_videoTrackSegmentColor.b, m_videoTrackSegmentColor.a);
        SDL_RenderFillRect(p_renderer, &segmentRect);

        // Get the width and height of the original video texture
        int videoFrameWidth, videoFrameHeight;
        SDL_QueryTexture(segment.firstFrame, nullptr, nullptr, &videoFrameWidth, &videoFrameHeight);

        // Draw the first and last frame of the video
        int frameInTrackWidth = (m_trackHeight - 2) * videoFrameWidth / videoFrameHeight; // How wide we want the frames

        // Draw the first frame
        int firstFrameWidth = std::min(renderWidth - 2, frameInTrackWidth); // How wide can we actually make it
        SDL_Rect firstFrameRect = { renderXPos + 1, renderYPos + 1, firstFrameWidth, m_trackHeight - 2 };
        int sourceWidth = videoFrameWidth * firstFrameWidth / frameInTrackWidth; // How much to take from the original frame texture
        SDL_Rect sourceRect = { 0, 0, sourceWidth, videoFrameHeight }; // Crop the rest out (if necessary)
        SDL_RenderCopy(p_renderer, segment.firstFrame, &sourceRect, &firstFrameRect);

        int renderWidthLeftOver = renderWidth - 2 - firstFrameWidth;
        if (renderWidthLeftOver > 0) {
            // Draw the last frame
            int lastFrameWidth = std::min(renderWidthLeftOver, frameInTrackWidth); // How wide can we actually make it
            SDL_Rect lastFrameRect = { renderXPos + renderWidth - 1 - lastFrameWidth, renderYPos + 1, lastFrameWidth, m_trackHeight - 2 };
            sourceRect.w = videoFrameWidth * lastFrameWidth / frameInTrackWidth; // How much to take from the original frame texture
            SDL_RenderCopy(p_renderer, segment.lastFrame, &sourceRect, &lastFrameRect);
        }
    }

    // Draw all audio tracks
    for (int i = 0; i < m_audioTrackPositionMap.size(); i++) {
        int trackYpos = rect.y + m_topBarheight + (static_cast<int>(m_videoTrackPositionMap.size()) + i) * m_rowHeight;

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
    }

    // Draw all audio segments on the audio tracks
    for (AudioSegment& segment : m_audioSegments) {
        // If fully outside render view, do not render
        if (m_scrollOffset > segment.timelinePosition + segment.timelineDuration) continue;

        Uint32 xPos = segment.timelinePosition - m_scrollOffset;
        int diff = 0;
        if (m_scrollOffset > segment.timelinePosition) {
            xPos = 0; // If xPos should be negative, make it 0
            diff = m_scrollOffset - segment.timelinePosition; // keep the difference to subtract it from the width
        }

        int renderXPos = rect.x + m_trackStartXPos + xPos * m_timeLabelInterval / m_zoom;
        int trackPos = m_audioTrackPositionMap[segment.trackID];
        int renderYPos = rect.y + m_topBarheight + (static_cast<int>(m_videoTrackPositionMap.size()) + trackPos) * m_rowHeight;
        int renderWidth = (segment.timelineDuration - diff) * m_timeLabelInterval / m_zoom;

        // Draw the outlines
        SDL_Rect outlineRect = { renderXPos - 1, renderYPos - 1, renderWidth + 2, m_trackHeight + 2 };
        if (std::find(m_selectedAudioSegments.begin(), m_selectedAudioSegments.end(), &segment) != m_selectedAudioSegments.end()) {
            SDL_SetRenderDrawColor(p_renderer, m_segmentHighlightColor.r, m_segmentHighlightColor.g, m_segmentHighlightColor.b, m_segmentHighlightColor.a);
        }
        else {
            SDL_SetRenderDrawColor(p_renderer, m_segmentOutlineColor.r, m_segmentOutlineColor.g, m_segmentOutlineColor.b, m_segmentOutlineColor.a);
        }
        SDL_RenderFillRect(p_renderer, &outlineRect);

        // Draw the inside BG
        SDL_Rect segmentRect = { renderXPos + 1, renderYPos + 1, renderWidth - 2, m_trackHeight - 2 };
        SDL_SetRenderDrawColor(p_renderer, m_audioTrackSegmentColor.r, m_audioTrackSegmentColor.g, m_audioTrackSegmentColor.b, m_audioTrackSegmentColor.a);
        SDL_RenderFillRect(p_renderer, &segmentRect);
    }

    // If the currentTime is higher than the scroll offset (leftmost frame)
    if (m_scrollOffset <= m_currentTime) {
        // Draw the current time indicator (a vertical line)
        int indicatorX = m_trackStartXPos + (m_currentTime - m_scrollOffset) * m_timeLabelInterval / m_zoom;
        SDL_SetRenderDrawColor(p_renderer, m_timeIndicatorColor.r, m_timeIndicatorColor.g, m_timeIndicatorColor.b, m_timeIndicatorColor.a);
        SDL_RenderDrawLine(p_renderer, rect.x + indicatorX, rect.y, rect.x + indicatorX, rect.y + m_topBarheight + static_cast<int>(m_videoTrackPositionMap.size() + m_audioTrackPositionMap.size()) * m_rowHeight);
    }
}

void Timeline::update(int x, int y, int w, int h) {
    rect = { x, y, w, h };
}

void Timeline::handleEvent(SDL_Event& event) {
    static bool mouseInThisSegment = false;

    switch (event.type) {
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
        // Check if the key pressed was delete
        if (event.key.keysym.sym == SDLK_DELETE) {
            if (!m_selectedVideoSegments.empty()) {
                // Predicate to check if a VideoSegment exists in m_selectedVideoSegments
                auto isSelected = [this](const VideoSegment& segment) {
                    return std::find(m_selectedVideoSegments.begin(), m_selectedVideoSegments.end(), &segment) != m_selectedVideoSegments.end();
                    };

                // Remove all selected segments from m_videoSegments
                m_videoSegments.erase(
                    std::remove_if(m_videoSegments.begin(), m_videoSegments.end(), isSelected),
                    m_videoSegments.end()
                );
            }
            if (!m_selectedAudioSegments.empty()) {
                // Predicate to check if an AudioSegment exists in m_selectedAudioSegments
                auto isSelected = [this](const AudioSegment& segment) {
                    return std::find(m_selectedAudioSegments.begin(), m_selectedAudioSegments.end(), &segment) != m_selectedAudioSegments.end();
                    };

                // Remove all selected segments from m_videoSegments
                m_audioSegments.erase(
                    std::remove_if(m_audioSegments.begin(), m_audioSegments.end(), isSelected),
                    m_audioSegments.end()
                );
            }
        }
        break;
    }
    case SDL_MOUSEBUTTONDOWN: {
        SDL_Point mouseButton = { event.button.x, event.button.y };
        // If clicked outside this segment, do nothing
        if (!SDL_PointInRect(&mouseButton, &rect)) break;

        if (event.button.button == SDL_BUTTON_LEFT) {
            if (m_isDragging) break;

            // If clicked in the left column, do nothing
            if (mouseButton.x < m_trackStartXPos) break;

            Uint32 clickedFrame = (mouseButton.x - rect.x - m_trackStartXPos) * m_zoom / m_timeLabelInterval + m_scrollOffset;

            VideoSegment* clickedVideoSegment = getVideoSegmentAtPos(mouseButton.x, mouseButton.y);
            AudioSegment* clickedAudioSegment = getAudioSegmentAtPos(mouseButton.x, mouseButton.y);

            // If we clicked on a video segment
            if (clickedVideoSegment) {
                // If shift is held, toggle selection of the clicked segment
                if (SDL_GetModState() & KMOD_SHIFT) {
                    if (std::find(m_selectedVideoSegments.begin(), m_selectedVideoSegments.end(), clickedVideoSegment) == m_selectedVideoSegments.end()) {
                        m_selectedVideoSegments.push_back(clickedVideoSegment);
                    }
                    else {
                        m_selectedVideoSegments.erase(std::remove(m_selectedVideoSegments.begin(), m_selectedVideoSegments.end(), clickedVideoSegment), m_selectedVideoSegments.end());
                    }
                }
                else {
                    // If shift is not held and the clicked segment was not already selected
                    if (std::find(m_selectedVideoSegments.begin(), m_selectedVideoSegments.end(), clickedVideoSegment) == m_selectedVideoSegments.end()) {
                        m_selectedVideoSegments.clear();
                        m_selectedAudioSegments.clear();
                        m_selectedVideoSegments.push_back(clickedVideoSegment);
                    }
                    // Start holding/dragging logic
                    m_isHolding = true;
                    m_mouseHoldStartX = mouseButton.x;
                    m_mouseHoldStartTrackID = 0; // TODO
                    m_lastLegalFrame = clickedFrame;
                    m_lastLegalLeftmostFrame = clickedFrame;

                    // Find the earliest/leftmost frame position of all selected segments
                    for (int i = 0; i < m_selectedVideoSegments.size(); i++) {
                        if (m_selectedVideoSegments[i]->timelinePosition < m_lastLegalLeftmostFrame) m_lastLegalLeftmostFrame = m_selectedVideoSegments[i]->timelinePosition;
                    }
                    for (int i = 0; i < m_selectedAudioSegments.size(); i++) {
                        if (m_selectedAudioSegments[i]->timelinePosition < m_lastLegalLeftmostFrame) m_lastLegalLeftmostFrame = m_selectedAudioSegments[i]->timelinePosition;
                    }
                }
                break;
            }
            // If we selected an audio segment
            else if (clickedAudioSegment) {
                // If shift is held, toggle selection of the clicked segment
                if (SDL_GetModState() & KMOD_SHIFT) {
                    if (std::find(m_selectedAudioSegments.begin(), m_selectedAudioSegments.end(), clickedAudioSegment) == m_selectedAudioSegments.end()) {
                        m_selectedAudioSegments.push_back(clickedAudioSegment);
                    }
                    else {
                        m_selectedAudioSegments.erase(std::remove(m_selectedAudioSegments.begin(), m_selectedAudioSegments.end(), clickedAudioSegment), m_selectedAudioSegments.end());
                    }
                }
                else {
                    // If shift is not held and the clicked segment was not already selected
                    if (std::find(m_selectedAudioSegments.begin(), m_selectedAudioSegments.end(), clickedAudioSegment) == m_selectedAudioSegments.end()) {
                        m_selectedVideoSegments.clear();
                        m_selectedAudioSegments.clear();
                        m_selectedAudioSegments.push_back(clickedAudioSegment);
                    }
                    m_isHolding = true;
                    m_mouseHoldStartX = mouseButton.x;
                    m_mouseHoldStartTrackID = 0; // TODO
                    m_lastLegalFrame = clickedFrame;
                    m_lastLegalLeftmostFrame = clickedFrame;

                    // Find the earliest/leftmost frame position of all selected segments
                    for (int i = 0; i < m_selectedVideoSegments.size(); i++) {
                        if (m_selectedVideoSegments[i]->timelinePosition < m_lastLegalLeftmostFrame) m_lastLegalLeftmostFrame = m_selectedVideoSegments[i]->timelinePosition;
                    }
                    for (int i = 0; i < m_selectedAudioSegments.size(); i++) {
                        if (m_selectedAudioSegments[i]->timelinePosition < m_lastLegalLeftmostFrame) m_lastLegalLeftmostFrame = m_selectedAudioSegments[i]->timelinePosition;
                    }
                }
                break;
            }

            // If nothing is selected, then we want to move the currentTime to the selected time (and, if playing, pause)
            m_playing = false;
            setCurrentTime(clickedFrame);
            m_selectedVideoSegments.clear();
            m_selectedAudioSegments.clear();
        }
        else if (event.button.button == SDL_BUTTON_RIGHT) {
            // If clicked in the left column
            if (mouseButton.x < m_trackStartXPos) {
                Track track = GetTrackID(mouseButton);
                // If in a track, show Track options
                if (track.trackID >= 0) {
                    if (track.trackID < 0) break;
                    if (track.trackID < 0) break;
                    std::vector<PopupMenu::MenuItem> contextMenuOptions = {
                        { "Add AV Track Above",    [this, track]() { addTrack(track, 2, true);  } },
                        { "Add AV Track Below",    [this, track]() { addTrack(track, 2, false); } },
                        { "Add Video Track Above", [this, track]() { addTrack(track, 0, true);  } },
                        { "Add Video Track Below", [this, track]() { addTrack(track, 0, false); } },
                        { "Add Audio Track Above", [this, track]() { addTrack(track, 1, true);  } },
                        { "Add Audio Track Below", [this, track]() { addTrack(track, 1, false); } }
                    };
                    PopupMenu::show(mouseButton.x, mouseButton.y, contextMenuOptions);
                }
            }
        }
        break;
    }
    case SDL_MOUSEMOTION: {
        SDL_Point mousePoint = { event.motion.x, event.motion.y };
        mouseInThisSegment = SDL_PointInRect(&mousePoint, &rect) ? true : false;

        if (m_isHolding && !m_isDragging) {
            // If we moved above a threshold amount of pixels from the starting X-location of the mouse since holding the left mouse button, start dragging
            if (std::abs(mousePoint.x - m_mouseHoldStartX) > m_draggingThreshold) {
                m_isDragging = true; 
            }
        }

        // If we are dragging and we have selected some segments
        if (m_isDragging && (!m_selectedVideoSegments.empty() || !m_selectedAudioSegments.empty())) {
            Uint32 currentFrame = (mousePoint.x - rect.x - m_trackStartXPos) * m_zoom / m_timeLabelInterval + m_scrollOffset;
            if (mousePoint.x < rect.x + m_trackStartXPos) currentFrame = 0;

            // TODO: allow vertical movement / moving between tracks

            Uint32 deltaFrames; // Amount of frames to move
            if (currentFrame < m_lastLegalFrame) { // Moving left
                deltaFrames = m_lastLegalFrame - currentFrame; // Absolute difference
                if (deltaFrames > m_lastLegalLeftmostFrame) { // Would cause uint underflow
                    deltaFrames = -m_lastLegalLeftmostFrame;
                }
                else {
                    deltaFrames = -deltaFrames;
                }
            }
            else { // Moving right
                deltaFrames = currentFrame - m_lastLegalFrame;
            }
            if (deltaFrames == 0) break; // No movement, so just stop here

            // Move all selected segments by deltaFrames (no collision checks yet)
            for (int i = 0; i < m_selectedVideoSegments.size(); i++) {
                m_selectedVideoSegments[i]->timelinePosition += deltaFrames;
            }
            for (int i = 0; i < m_selectedAudioSegments.size(); i++) {
                m_selectedAudioSegments[i]->timelinePosition += deltaFrames;
            }

            // For every moved segment, check for collisions
            bool illegalMove = false;
            for (int i = 0; i < m_selectedVideoSegments.size(); i++) {
                // If it collides, undo every move
                if (isCollidingWithOtherSegments(m_selectedVideoSegments[i])) {
                    illegalMove = true;
                    break; // Exit loop
                }
            }
            if (!illegalMove) {
                for (int i = 0; i < m_selectedAudioSegments.size(); i++) {
                    // If it collides, undo every move
                    if (isCollidingWithOtherSegments(m_selectedAudioSegments[i])) {
                        illegalMove = true;
                        break; // Exit loop
                    }
                }
            }
            if (illegalMove) {
                // Move all selected segments back
                for (int i = 0; i < m_selectedVideoSegments.size(); i++) {
                    m_selectedVideoSegments[i]->timelinePosition -= deltaFrames;
                }
                for (int i = 0; i < m_selectedAudioSegments.size(); i++) {
                    m_selectedAudioSegments[i]->timelinePosition -= deltaFrames;
                }
            }
            else {
                // Update lastLegalFrame and -leftMostFrame
                m_lastLegalFrame = currentFrame;
                m_lastLegalLeftmostFrame += deltaFrames;
            }
        }
        break;
    }
    case SDL_MOUSEBUTTONUP: {
        m_isHolding = false;
        m_isDragging = false;
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
    if (x < rect.x + m_trackStartXPos) return nullptr;
    if (y < rect.y + m_topBarheight) return nullptr;
    int selectedTrackPos = static_cast<int>(m_videoTrackPositionMap.size() - 1) - ((y - rect.y - m_topBarheight) / m_trackHeight);
    if (selectedTrackPos >= m_videoTrackPositionMap.size()) return nullptr;
    int selectedTrack = m_reverseVideoTrackPositionMap[selectedTrackPos];

    Uint32 selectedFrame = (x - rect.x - m_trackStartXPos) * m_zoom / m_timeLabelInterval + m_scrollOffset;

    // Iterate over video segments to find which one is active at currentTime
    for (VideoSegment& segment : m_videoSegments) {
        if (segment.trackID != selectedTrack) continue;
        if (selectedFrame >= segment.timelinePosition && selectedFrame <= segment.timelinePosition + segment.timelineDuration) {
            return &segment;
        }
    }
    return nullptr;
}

AudioSegment* Timeline::getAudioSegmentAtPos(int x, int y) {
    if (x < rect.x + m_trackStartXPos) return nullptr;
    if (y < rect.y + m_topBarheight + m_videoTrackPositionMap.size() * m_trackHeight) return nullptr;
    int selectedTrackPos = (y - rect.y - m_topBarheight) / m_trackHeight - static_cast<int>(m_videoTrackPositionMap.size());
    if (selectedTrackPos >= m_audioTrackPositionMap.size()) return nullptr;
    int selectedTrack = m_reverseAudioTrackPositionMap[selectedTrackPos];

    Uint32 selectedFrame = (x - rect.x - m_trackStartXPos) * m_zoom / m_timeLabelInterval + m_scrollOffset;

    // Iterate over video segments to find which one is active at currentTime
    for (AudioSegment& segment : m_audioSegments) {
        if (segment.trackID != selectedTrack) continue;
        if (selectedFrame >= segment.timelinePosition && selectedFrame <= segment.timelinePosition + segment.timelineDuration) {
            return &segment;
        }
    }
    return nullptr;
}

bool Timeline::isCollidingWithOtherSegments(VideoSegment* videoSegment) {
    // Iterate over video segments to find which one overlaps with the input segment
    for (VideoSegment& segment : m_videoSegments) {
        if (&segment == videoSegment) continue; // Skip self-collision check
        if (videoSegment->overlapsWith(&segment)) return true;
    }
    return false;
}

bool Timeline::isCollidingWithOtherSegments(AudioSegment* audioSegment) {
    // Iterate over video segments to find which one overlaps with the input segment
    for (AudioSegment& segment : m_audioSegments) {
        if (&segment == audioSegment) continue; // Skip self-collision check
        if (audioSegment->overlapsWith(&segment)) return true;
    }
    return false;
}

void Timeline::addTrack(Track track, int videoOrAudio, bool above) {
    if (videoOrAudio == 0 || videoOrAudio == 2) {
        int newVideoTrackID = m_nextVideoTrackID++;

        // Find the position to insert the new track
        auto it = m_videoTrackPositionMap.find(track.trackID);
        if (it != m_videoTrackPositionMap.end()) {
            int pos = it->second; // Get position of the existing track
            if (above) pos++;

            // Insert the new track relative to existing one, shift positions of others
            for (auto& entry : m_videoTrackPositionMap) {
                if (entry.second >= pos) {
                    m_reverseVideoTrackPositionMap.erase(entry.second); // Remove old position entry in reverse map
                    entry.second++; // Shift the position of tracks after the insertion point
                    m_reverseVideoTrackPositionMap[entry.second] = entry.first; // Update reverse map with the new position
                }
            }
            m_videoTrackPositionMap[newVideoTrackID] = pos;
            m_reverseVideoTrackPositionMap[pos] = newVideoTrackID;
        }
        else {
            // Insert at the end: new track gets the next available position
            m_videoTrackPositionMap[newVideoTrackID] = static_cast<int>(m_videoTrackPositionMap.size());
            m_reverseVideoTrackPositionMap[static_cast<int>(m_videoTrackPositionMap.size())] = newVideoTrackID;
        }
    }
    if (videoOrAudio == 1 || videoOrAudio == 2) {
        int newAudioTrackID = m_nextAudioTrackID++;

        // Find the position to insert the new track
        auto it = m_audioTrackPositionMap.find(track.trackID);
        if (it != m_audioTrackPositionMap.end()) {
            int pos = it->second; // Get position of the existing track
            if (above) pos++;

            // Insert the new track relative to existing one, shift positions of others
            for (auto& entry : m_audioTrackPositionMap) {
                if (entry.second >= pos) {
                    m_reverseAudioTrackPositionMap.erase(entry.second); // Remove old position entry in reverse map
                    entry.second++; // Shift the position of tracks after the insertion point
                    m_reverseAudioTrackPositionMap[entry.second] = entry.first; // Update reverse map with the new position
                }
            }
            m_audioTrackPositionMap[newAudioTrackID] = pos;
            m_reverseAudioTrackPositionMap[pos] = newAudioTrackID;
        }
        else {
            // Insert at the end: new track gets the next available position
            m_audioTrackPositionMap[newAudioTrackID] = static_cast<int>(m_audioTrackPositionMap.size());
            m_reverseAudioTrackPositionMap[static_cast<int>(m_audioTrackPositionMap.size())] = newAudioTrackID;
        }
    }
}

// TODO: return only the video segment with the highest trackID at this time
VideoSegment* Timeline::getCurrentVideoSegment() {
    Uint32 currentTime = getCurrentTime();
    VideoSegment* currentVideoSegment = nullptr;

    // Iterate over video segments
    for (VideoSegment& segment : m_videoSegments) {
        // Check if active at the current time
        if (currentTime >= segment.timelinePosition &&
            currentTime < segment.timelinePosition + segment.timelineDuration) {
            // Check if the found segment is higher on the track ordering
            if (!currentVideoSegment || (m_reverseVideoTrackPositionMap[segment.trackID] > m_reverseVideoTrackPositionMap[currentVideoSegment->trackID])) {
                currentVideoSegment = &segment; // Set this segment to be returned
            }
        }
    }
    return currentVideoSegment;
}

// TODO: merge audio if multiple tracks have a audioSegment to play at this time
AudioSegment* Timeline::getCurrentAudioSegment() {
    Uint32 currentTime = getCurrentTime();

    for (AudioSegment& segment : m_audioSegments) {
        if (currentTime >= segment.timelinePosition &&
            currentTime < segment.timelinePosition + segment.timelineDuration) {
            return &segment;  // Return the active audio segment
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

Track Timeline::GetTrackID(SDL_Point mousePoint) {
    Track track;
    track.trackID = -1;
    // Iterate over video and audio tracks to find the trackID
    for (int i = 0; i < m_videoTrackPositionMap.size(); i++) {
        if (mousePoint.y >= rect.y + m_topBarheight + m_trackHeight * i && mousePoint.y <= rect.y + m_topBarheight + m_trackHeight * (i + 1)) {
            track.trackID = m_reverseVideoTrackPositionMap[m_videoTrackPositionMap.size() - 1 - i];
            track.trackType = VIDEO;
            return track;
        }
    }
    for (int i = 0; i < m_audioTrackPositionMap.size(); i++) {
        if (mousePoint.y >= rect.y + m_topBarheight + m_trackHeight * (m_videoTrackPositionMap.size() + i) && mousePoint.y <= rect.y + m_topBarheight + m_trackHeight * (m_videoTrackPositionMap.size() + i + 1)) {
            track.trackID = m_reverseAudioTrackPositionMap[i];
            track.trackType = AUDIO;
            return track;
        }
    }
}

bool Timeline::addAssetSegments(AssetData* data, int mouseX, int mouseY) {
    SDL_Point mousePoint = { mouseX, mouseY };

    // If outside this segment, do nothing
    if (!SDL_PointInRect(&mousePoint, &rect)) return false;

    // If in the left column, do nothing
    if (mousePoint.x < m_trackStartXPos) return false;

    Uint32 selectedFrame = (mousePoint.x - rect.x - m_trackStartXPos) * m_zoom / m_timeLabelInterval + m_scrollOffset;

    // If video -> videoTrackID   |   If audio -> audioTrackID   |   If video with audio -> videoTrackID == audioTrackID
    Track track = GetTrackID(mousePoint);
    int trackID = track.trackID;

    if (trackID < 0) {
        return false; // Not in a legitimate track (above first or below last track)
    }

    VideoSegment videoSegment;
    AudioSegment audioSegment;

    // In case the asset has video
    if (data->videoData) {
        // Create and add a new videoSegment
        videoSegment = {
            .videoData = data->videoData,
            .sourceStartTime = 0,
            .duration = data->videoData->getVideoDurationInFrames(),
            .timelinePosition = selectedFrame,
            .timelineDuration = data->videoData->getVideoDurationInFrames(m_fps),
            .fps = data->videoData->getFPS(),
            .trackID = trackID
        };
        videoSegment.firstFrame = videoSegment.videoData->getFrameTexture(p_renderer, 0);
        videoSegment.lastFrame  = videoSegment.videoData->getFrameTexture(p_renderer, videoSegment.duration - 1);

        // Cannot drop here, because it would overlap with another segment
        if (isCollidingWithOtherSegments(&videoSegment)) return false;
    }
    // In case the asset has audio
    if (data->audioData) {
        // Create and add a new audioSegment
        audioSegment = {
            .audioData = data->audioData,
            .sourceStartTime = 0,
            .duration = data->audioData->getAudioDurationInFrames(),
            .timelinePosition = selectedFrame,
            .timelineDuration = data->audioData->getAudioDurationInFrames(m_fps),
            .trackID = trackID
        };

        // Cannot drop here, because it would overlap with another segment
        if (isCollidingWithOtherSegments(&audioSegment)) return false;
    }

    // Unselect any selected segments
    m_selectedVideoSegments.clear();
    m_selectedAudioSegments.clear();

    // If we reached here, then the new video segment and/or audio segment can be added
    // Also immediately select the new assets (so we can immediately move them until we release the mouse)
    if (data->videoData) {
        m_videoSegments.push_back(videoSegment);
        m_selectedVideoSegments.push_back(&m_videoSegments.back());
    }
    if (data->audioData) {
        m_audioSegments.push_back(audioSegment);
        m_selectedAudioSegments.push_back(&m_audioSegments.back());
    }

    // Start dragging logic
    m_isHolding = true;
    m_isDragging = true;
    m_mouseHoldStartTrackID = 0; // TODO
    m_lastLegalFrame = selectedFrame;
    m_lastLegalLeftmostFrame = selectedFrame;

    // Find the earliest/leftmost frame position of all selected segments
    for (int i = 0; i < m_selectedVideoSegments.size(); i++) {
        if (m_selectedVideoSegments[i]->timelinePosition < m_lastLegalLeftmostFrame) m_lastLegalLeftmostFrame = m_selectedVideoSegments[i]->timelinePosition;
    }
    for (int i = 0; i < m_selectedAudioSegments.size(); i++) {
        if (m_selectedAudioSegments[i]->timelinePosition < m_lastLegalLeftmostFrame) m_lastLegalLeftmostFrame = m_selectedAudioSegments[i]->timelinePosition;
    }

    return true;
}
