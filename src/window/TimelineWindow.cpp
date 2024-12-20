#include <iostream>
#include <string> 
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <SDL_ttf.h>
#include "TimelineWindow.h"
#include "util.h"
#include "ContextMenu.h"

TimeLineWindow::TimeLineWindow(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Window* parent, SDL_Color color)
    : Window(x, y, w, h, renderer, eventManager, parent, color)
{
    m_timeline = new Timeline();
}

TimeLineWindow::~TimeLineWindow() {
    // No need to delete renderer since it is managed elsewhere
}

void TimeLineWindow::render() {
    if (tempAddedVid && m_selectedVideoSegments.empty()) {
        int ijk = 0;
    }
    SDL_SetRenderDrawColor(p_renderer, p_color.r, p_color.g, p_color.b, p_color.a);
    SDL_RenderFillRect(p_renderer, &rect);

    // Draw the top bar
    int xPos = rect.x + m_trackStartXPos;
    int yPos = rect.y;
    Uint32 timeLabel = m_scrollOffset;
    SDL_SetRenderDrawColor(p_renderer, m_timeLabelColor.r, m_timeLabelColor.g, m_timeLabelColor.b, m_timeLabelColor.a);
    while (xPos < rect.x + rect.w) {
        SDL_Rect textRect = renderTextWithCustomSpacing(p_renderer, xPos, yPos,
            getFont(), formatTime(timeLabel, m_timeline->getFPS()).c_str(),
            -1, m_timeLabelColor);

        SDL_RenderDrawLine(p_renderer, xPos, rect.y + textRect.h, xPos, rect.y + m_topBarheight); // Big label
        SDL_RenderDrawLine(p_renderer, xPos + m_timeLabelInterval / 2, rect.y + (int)(textRect.h * 1.5), xPos + m_timeLabelInterval / 2, rect.y + m_topBarheight); // Small halfway label

        xPos += m_timeLabelInterval;
        timeLabel += m_zoom;
    }

    // Draw all video tracks
    for (int i = 0; i < m_timeline->getVideoTrackCount(); i++) {
        int trackYpos = rect.y + m_topBarheight + (m_timeline->getVideoTrackCount() - 1 - i) * m_rowHeight;

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
    for (VideoSegment& segment : *m_timeline->getAllVideoSegments()) {
        // If fully outside render view, do not render
        if (m_scrollOffset > segment.timelinePosition + segment.timelineDuration) continue;

        Uint32 xPos = segment.timelinePosition - m_scrollOffset;
        int diff = 0;
        if (m_scrollOffset > segment.timelinePosition) {
            xPos = 0; // If xPos should be negative, make it 0
            diff = m_scrollOffset - segment.timelinePosition; // keep the difference to subtract it from the width
        }

        int renderXPos = rect.x + m_trackStartXPos + xPos * m_timeLabelInterval / m_zoom;
        int trackPos = m_timeline->getVideoTrackPos(segment.trackID);
        int renderYPos = rect.y + m_topBarheight + (m_timeline->getVideoTrackCount() - 1 - trackPos) * m_rowHeight;
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
    for (int i = 0; i < m_timeline->getAudioTrackCount(); i++) {
        int trackYpos = rect.y + m_topBarheight + (m_timeline->getVideoTrackCount() + i) * m_rowHeight;

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
    for (AudioSegment& segment : *m_timeline->getAllAudioSegments()) {
        // If fully outside render view, do not render
        if (m_scrollOffset > segment.timelinePosition + segment.timelineDuration) continue;

        Uint32 xPos = segment.timelinePosition - m_scrollOffset;
        int diff = 0;
        if (m_scrollOffset > segment.timelinePosition) {
            xPos = 0; // If xPos should be negative, make it 0
            diff = m_scrollOffset - segment.timelinePosition; // keep the difference to subtract it from the width
        }

        int renderXPos = rect.x + m_trackStartXPos + xPos * m_timeLabelInterval / m_zoom;
        int trackPos = m_timeline->getAudioTrackPos(segment.trackID);
        int renderYPos = rect.y + m_topBarheight + (m_timeline->getVideoTrackCount() + trackPos) * m_rowHeight;
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
    if (m_scrollOffset <= m_timeline->getCurrentTime()) {
        // Draw the current time indicator (a vertical line)
        int indicatorX = m_trackStartXPos + (m_timeline->getCurrentTime() - m_scrollOffset) * m_timeLabelInterval / m_zoom;
        SDL_SetRenderDrawColor(p_renderer, m_timeIndicatorColor.r, m_timeIndicatorColor.g, m_timeIndicatorColor.b, m_timeIndicatorColor.a);
        SDL_RenderDrawLine(p_renderer, rect.x + indicatorX, rect.y, rect.x + indicatorX, rect.y + m_topBarheight + (m_timeline->getVideoTrackCount() + m_timeline->getAudioTrackCount()) * m_rowHeight);
        // If zoomed in enough, also draw a small horizontal line (to show the frame length)
        if (m_zoom <= m_indicatorFrameDisplayThreshold) {
            int indicatorXplus1 = indicatorX + m_timeLabelInterval / m_zoom; // indicatorX + 1 frame
            SDL_RenderDrawLine(p_renderer, rect.x + indicatorX, rect.y + m_topBarheight, rect.x + indicatorXplus1, rect.y + m_topBarheight);
        }
    }
}

void TimeLineWindow::update(int x, int y, int w, int h) {
    rect = { x, y, w, h };
}

void TimeLineWindow::handleEvent(SDL_Event& event) {
    static bool mouseInThisWindow = false;

    switch (event.type) {
    case SDL_KEYDOWN: {
        switch (event.key.keysym.sym) {
        // Play / Pause
        case SDLK_SPACE: {
            m_timeline->togglePlaying();
            break;
        }
        // Deleting segments / tracks
        case SDLK_DELETE: {
            deleteSelectedSegments();
            break;
        }
        // Move one frame up the timeline
        case SDLK_RIGHT: {
            m_timeline->setCurrentTime(m_timeline->getCurrentTime() + 1);
            break;
        }
        // Move one frame down the timeline
        case SDLK_LEFT: {
            Uint32 currentTime = m_timeline->getCurrentTime();
            if (currentTime != 0) m_timeline->setCurrentTime(currentTime - 1);
            break;
        }
        }
        break;
    }
    case SDL_MOUSEBUTTONDOWN: {
        SDL_Point mouseButton = { event.button.x, event.button.y };
        // If clicked outside this window, do nothing
        if (!SDL_PointInRect(&mouseButton, &rect)) break;

        if (m_isDragging) break;

        // If clicked in the left column
        if (mouseButton.x < m_trackStartXPos) {
            // If right clicked
            if (event.button.button == SDL_BUTTON_RIGHT) {
                Track track = getTrackID(mouseButton);
                // If in a track, show Track options
                if (track.trackID >= 0) {
                    if (track.trackID < 0) break;
                    if (track.trackID < 0) break;
                    std::vector<ContextMenu::MenuItem> contextMenuOptions = {
                        { "Add Track", nullptr, {
                            { "Add AV Track Above",    [this, track]() { m_timeline->addTrack(track, 2, true);  } },
                            { "Add AV Track Below",    [this, track]() { m_timeline->addTrack(track, 2, false); } },
                            { "Add Video Track Above", [this, track]() { m_timeline->addTrack(track, 0, true);  } },
                            { "Add Video Track Below", [this, track]() { m_timeline->addTrack(track, 0, false); } },
                            { "Add Audio Track Above", [this, track]() { m_timeline->addTrack(track, 1, true);  } },
                            { "Add Audio Track Below", [this, track]() { m_timeline->addTrack(track, 1, false); } }
                        }},
                        { "Delete Track", [this, track]() { m_timeline->deleteTrack(track); } }
                    };
                    ContextMenu::show(mouseButton.x, mouseButton.y, contextMenuOptions);
                }
            }
            break;
        }

        Uint32 clickedFrame = (mouseButton.x - rect.x - m_trackStartXPos) * m_zoom / m_timeLabelInterval + m_scrollOffset;

        // If clicked in the videoTrack area
        if (mouseButton.y > rect.y + m_topBarheight && mouseButton.y < rect.y + m_topBarheight + m_timeline->getVideoTrackCount() * m_trackHeight)
        {
            int selectedTrackPos = m_timeline->getVideoTrackCount() - 1 - ((mouseButton.y - rect.y - m_topBarheight) / m_trackHeight);
            VideoSegment* clickedVideoSegment = m_timeline->getVideoSegment(selectedTrackPos, clickedFrame);

            // If we clicked on a video segment
            if (clickedVideoSegment) {
                if (event.button.button == SDL_BUTTON_LEFT) {
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
                        m_lastLegalTrackPos = selectedTrackPos;
                        m_selectedMaxTrackPos = selectedTrackPos;
                        m_selectedMinTrackPos = selectedTrackPos;
                        m_lastLegalFrame = clickedFrame;
                        m_lastLegalLeftmostFrame = clickedFrame;

                        // Find the earliest/leftmost frame position of all selected segments
                        // Also find the highest and lowest track order positions
                        for (int i = 0; i < m_selectedVideoSegments.size(); i++) {
                            if (m_selectedVideoSegments[i]->timelinePosition < m_lastLegalLeftmostFrame) m_lastLegalLeftmostFrame = m_selectedVideoSegments[i]->timelinePosition;
                            int segmentTrackPos = m_timeline->getVideoTrackPos(m_selectedVideoSegments[i]->trackID);
                            m_selectedMaxTrackPos = std::max(m_selectedMaxTrackPos, segmentTrackPos);
                            m_selectedMinTrackPos = std::min(m_selectedMinTrackPos, segmentTrackPos);
                        }
                        for (int i = 0; i < m_selectedAudioSegments.size(); i++) {
                            if (m_selectedAudioSegments[i]->timelinePosition < m_lastLegalLeftmostFrame) m_lastLegalLeftmostFrame = m_selectedAudioSegments[i]->timelinePosition;
                            int segmentTrackPos = m_timeline->getVideoTrackPos(m_selectedAudioSegments[i]->trackID);
                            m_selectedMaxTrackPos = std::max(m_selectedMaxTrackPos, segmentTrackPos);
                            m_selectedMinTrackPos = std::min(m_selectedMinTrackPos, segmentTrackPos);
                        }
                    }
                    break;
                }
                else if (event.button.button == SDL_BUTTON_RIGHT) {
                    // If not already selected, unselect everything else and select this one
                    if (std::find(m_selectedVideoSegments.begin(), m_selectedVideoSegments.end(), clickedVideoSegment) == m_selectedVideoSegments.end()) {
                        m_selectedVideoSegments.clear();
                        m_selectedAudioSegments.clear();
                        m_selectedVideoSegments.push_back(clickedVideoSegment);
                    }
                }
            }
        }
        // Otherwise, if clicked in the audioTrack area
        else if (mouseButton.y < rect.y + m_topBarheight + (m_timeline->getVideoTrackCount() + m_timeline->getAudioTrackCount()) * m_trackHeight)
        {
            int selectedTrackPos = (mouseButton.y - rect.y - m_topBarheight) / m_trackHeight - m_timeline->getVideoTrackCount();
            AudioSegment* clickedAudioSegment = m_timeline->getAudioSegment(selectedTrackPos, clickedFrame);

            // If we selected an audio segment
            if (clickedAudioSegment) {
                if (event.button.button == SDL_BUTTON_LEFT) {
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
                        m_lastLegalTrackPos = selectedTrackPos;
                        m_selectedMaxTrackPos = selectedTrackPos;
                        m_selectedMinTrackPos = selectedTrackPos;
                        m_lastLegalFrame = clickedFrame;
                        m_lastLegalLeftmostFrame = clickedFrame;

                        // Find the earliest/leftmost frame position of all selected segments
                        for (int i = 0; i < m_selectedVideoSegments.size(); i++) {
                            if (m_selectedVideoSegments[i]->timelinePosition < m_lastLegalLeftmostFrame) m_lastLegalLeftmostFrame = m_selectedVideoSegments[i]->timelinePosition;
                            int segmentTrackPos = m_timeline->getVideoTrackPos(m_selectedVideoSegments[i]->trackID);
                            m_selectedMaxTrackPos = std::max(m_selectedMaxTrackPos, segmentTrackPos);
                            m_selectedMinTrackPos = std::min(m_selectedMinTrackPos, segmentTrackPos);
                        }
                        for (int i = 0; i < m_selectedAudioSegments.size(); i++) {
                            if (m_selectedAudioSegments[i]->timelinePosition < m_lastLegalLeftmostFrame) m_lastLegalLeftmostFrame = m_selectedAudioSegments[i]->timelinePosition;
                            int segmentTrackPos = m_timeline->getVideoTrackPos(m_selectedAudioSegments[i]->trackID);
                            m_selectedMaxTrackPos = std::max(m_selectedMaxTrackPos, segmentTrackPos);
                            m_selectedMinTrackPos = std::min(m_selectedMinTrackPos, segmentTrackPos);
                        }
                    }
                    break;
                }
                else if (event.button.button == SDL_BUTTON_RIGHT) {
                    // If not already selected, unselect everything else and select this one
                    if (std::find(m_selectedAudioSegments.begin(), m_selectedAudioSegments.end(), clickedAudioSegment) == m_selectedAudioSegments.end()) {
                        m_selectedVideoSegments.clear();
                        m_selectedAudioSegments.clear();
                        m_selectedAudioSegments.push_back(clickedAudioSegment);
                    }
                }
            }
        }

        if (event.button.button == SDL_BUTTON_LEFT) {
            // If nothing is selected, then we want to move the currentTime to the selected time (and, if playing, pause)
            if (m_timeline->isPlaying()) m_timeline->togglePlaying();
            m_isMovingCurrentTime = true;
            m_timeline->setCurrentTime(clickedFrame);
            m_selectedVideoSegments.clear();
            m_selectedAudioSegments.clear();
        }
        else if (event.button.button == SDL_BUTTON_RIGHT) {
            // Show selected segments options
            std::vector<ContextMenu::MenuItem> contextMenuOptions = {
                { "Delete Selected Item(s)", [this]() { deleteSelectedSegments(); } }
            };
            ContextMenu::show(mouseButton.x, mouseButton.y, contextMenuOptions);
        }
        break;
    }
    case SDL_MOUSEMOTION: {
        SDL_Point mousePoint = { event.motion.x, event.motion.y };
        mouseInThisWindow = SDL_PointInRect(&mousePoint, &rect) ? true : false;

        // Changing current time
        if (m_isMovingCurrentTime) {
            Uint32 hoveredFrame = (mousePoint.x <= m_trackStartXPos) ? 0 : (mousePoint.x - rect.x - m_trackStartXPos) * m_zoom / m_timeLabelInterval + m_scrollOffset;
            m_timeline->setCurrentTime(hoveredFrame);
        }

        // Vertical movement:

        if (m_isHolding) {
            int mouseTrackPos = getTrackPos(mousePoint.y);
            int deltaTrackPos = mouseTrackPos - m_lastLegalTrackPos;
            if (deltaTrackPos != 0) {
                // Check if there are enough tracks to move in the desired trackPos direction
                bool impossibleMove = false;
                if (deltaTrackPos > 0) {
                    // Only audio
                    if (m_selectedVideoSegments.empty()) {
                        if (m_selectedMaxTrackPos + deltaTrackPos >= m_timeline->getAudioTrackCount()) {
                            impossibleMove = true;
                        }
                    }
                    // Only video
                    else if (m_selectedAudioSegments.empty()) {
                        if (m_selectedMaxTrackPos + deltaTrackPos >= m_timeline->getVideoTrackCount()) {
                            impossibleMove = true;
                        }
                    }
                    // Both
                    else if (m_selectedMaxTrackPos + deltaTrackPos >= std::min(m_timeline->getVideoTrackCount(), m_timeline->getAudioTrackCount())) {
                        impossibleMove = true;
                    }
                }
                else if (deltaTrackPos < 0) {
                    if (m_selectedMinTrackPos + deltaTrackPos < 0) impossibleMove = true;
                }

                // If it is possible to move all selected segments to different tracks, move them and check for collisions
                if (!impossibleMove) {
                    // Move all selected segments up/down
                    if (m_timeline->segmentsChangeTrack(&m_selectedVideoSegments, &m_selectedAudioSegments, deltaTrackPos)) {
                        // Update the lastLegalTrackPos and the selected Max and Min TrackPos
                        m_lastLegalTrackPos = mouseTrackPos;
                        m_selectedMaxTrackPos += deltaTrackPos;
                        m_selectedMinTrackPos += deltaTrackPos;
                    }
                }
            }
        }

        // Horizontal movement:

        // If we moved above a threshold amount of pixels from the starting X-location of the mouse since holding the left mouse button, start dragging
        if (m_isHolding && !m_isDragging) {
            if (std::abs(mousePoint.x - m_mouseHoldStartX) > m_draggingThreshold) {
                m_isDragging = true; 
            }
        }

        // If we are dragging and we have selected some segments
        if (m_isDragging && (!m_selectedVideoSegments.empty() || !m_selectedAudioSegments.empty())) {
            Uint32 currentFrame = (mousePoint.x - rect.x - m_trackStartXPos) * m_zoom / m_timeLabelInterval + m_scrollOffset;
            if (mousePoint.x < rect.x + m_trackStartXPos) currentFrame = 0;
            
            Uint32 deltaFrames; // Amount of frames to move
            if (currentFrame < m_lastLegalFrame) { // Moving left
                deltaFrames = m_lastLegalFrame - currentFrame; // Absolute difference
                if (deltaFrames > m_lastLegalLeftmostFrame) { // Would cause uint underflow
                    deltaFrames = UINT32_MAX - m_lastLegalLeftmostFrame + 1; // equivalent to -m_lastLegalLeftmostFrame
                }
                else {
                    deltaFrames = UINT32_MAX - deltaFrames + 1; // equivalent to -deltaFrames
                }
            }
            else { // Moving right
                deltaFrames = currentFrame - m_lastLegalFrame;
            }
            if (deltaFrames == 0) break; // No movement, so just stop here

            // Move all selected segments left/right
            if (m_timeline->segmentsMoveFrames(&m_selectedVideoSegments, &m_selectedAudioSegments, deltaFrames)) {
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
        m_isMovingCurrentTime = false;
        break;
    }
    case SDL_MOUSEWHEEL: {
        if (!mouseInThisWindow) break;

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

void TimeLineWindow::deleteSelectedSegments() {
    m_timeline->deleteSegments(&m_selectedVideoSegments, &m_selectedAudioSegments);
    m_selectedVideoSegments.clear();
    m_selectedAudioSegments.clear();
}

Window* TimeLineWindow::findTypeImpl(const std::type_info& type) {
    if (type == typeid(TimeLineWindow)) {
        return this;
    }
    return nullptr;
}

Track TimeLineWindow::getTrackID(SDL_Point mousePoint) {
    Track track;
    track.trackID = -1;
    // Iterate over video and audio tracks to find the trackID
    for (int i = 0; i < m_timeline->getVideoTrackCount(); i++) {
        if (mousePoint.y >= rect.y + m_topBarheight + m_trackHeight * i && mousePoint.y <= rect.y + m_topBarheight + m_trackHeight * (i + 1)) {
            track.trackID = m_timeline->getVideoTrackID(m_timeline->getVideoTrackCount() - 1 - i);
            track.trackType = VIDEO;
            return track;
        }
    }
    for (int i = 0; i < m_timeline->getAudioTrackCount(); i++) {
        if (mousePoint.y >= rect.y + m_topBarheight + m_trackHeight * (m_timeline->getVideoTrackCount() + i) &&
            mousePoint.y <= rect.y + m_topBarheight + m_trackHeight * (m_timeline->getAudioTrackCount() + i + 1))
        {
            track.trackID = m_timeline->getAudioTrackID(i);
            track.trackType = AUDIO;
            return track;
        }
    }
    return track;
}

int TimeLineWindow::getTrackPos(int y) {
    if (y < rect.y + m_topBarheight) return -1;

    int selectedTrackPos = m_timeline->getVideoTrackCount() - 1 - ((y - rect.y - m_topBarheight) / m_trackHeight);
    if (selectedTrackPos < m_timeline->getVideoTrackCount()) return selectedTrackPos;

    selectedTrackPos = (y - rect.y - m_topBarheight) / m_trackHeight - m_timeline->getAudioTrackCount();
    if (selectedTrackPos < m_timeline->getAudioTrackCount()) return selectedTrackPos;
    return -1;
}

bool TimeLineWindow::addAssetSegments(AssetData* data, int mouseX, int mouseY) {
    SDL_Point mousePoint = { mouseX, mouseY };

    // If outside this window, do nothing
    if (!SDL_PointInRect(&mousePoint, &rect)) return false;

    // If in the left column, do nothing
    if (mousePoint.x < m_trackStartXPos) return false;

    Uint32 selectedFrame = (mousePoint.x - rect.x - m_trackStartXPos) * m_zoom / m_timeLabelInterval + m_scrollOffset;

    Track track = getTrackID(mousePoint);

    SegmentPointer segmentPointer = m_timeline->addAssetSegments(p_renderer, data, selectedFrame, track);

    if (!segmentPointer.videoSegment && !segmentPointer.audioSegment) return false; // if both are nullptr, it failed

    // Unselect any selected segments
    m_selectedVideoSegments.clear();
    m_selectedAudioSegments.clear();

    // Select the new assets (so we can immediately move them until we release the mouse)
    if (data->videoData) {
        m_selectedVideoSegments.push_back(segmentPointer.videoSegment);
    }
    if (data->audioData) {
        m_selectedAudioSegments.push_back(segmentPointer.audioSegment);
    }

    tempAddedVid = true;

    // Start dragging logic
    m_isHolding = true;
    m_isDragging = true;
    int trackPos = 0;
    trackPos = getTrackPos(mousePoint.y);
    m_lastLegalTrackPos = trackPos;
    m_selectedMaxTrackPos = trackPos;
    m_selectedMinTrackPos = trackPos;
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
