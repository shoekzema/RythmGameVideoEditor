#include "TimelineController.h"
#include "ContextMenu.h"
#include "util.h"
#include <algorithm>

TimelineController::TimelineController(Timeline* timeline, TimelineSelectionManager* selection, TimelineView* view, SDL_Renderer* renderer)
    : m_timeline(timeline), m_selection(selection), m_view(view), m_renderer(renderer) {}

void TimelineController::handleEvent(SDL_Event& event, const SDL_Rect& rect) {
    static bool mouseInThisWindow = false;

    switch (event.type) {
    case SDL_KEYDOWN:
        handleKeyDown(event);
        break;
    case SDL_MOUSEBUTTONDOWN:
        handleMouseButtonDown(event, rect);
        break;
    case SDL_MOUSEMOTION:
        {
            SDL_Point mousePoint = { event.motion.x, event.motion.y };
            mouseInThisWindow = SDL_PointInRect(&mousePoint, &rect) ? true : false;
        }
        handleMouseMotion(event, rect);
        break;
    case SDL_MOUSEBUTTONUP:
        handleMouseButtonUp(event);
        break;
    case SDL_MOUSEWHEEL:
        if (!mouseInThisWindow) break;
        handleMouseWheel(event);
        break;
    default:
        break;
    }
}

void TimelineController::handleKeyDown(const SDL_Event& event) {
    switch (event.key.keysym.sym) {
        case SDLK_SPACE:
            m_timeline->togglePlaying();
            break;
        case SDLK_DELETE:
            m_timeline->deleteSegments(&m_selection->selectedVideoSegments, &m_selection->selectedAudioSegments);
            m_selection->clear();
            break;
        case SDLK_RIGHT:
            m_timeline->setCurrentTime(m_timeline->getCurrentTime() + 1);
            break;
        case SDLK_LEFT: {
            Uint32 currentTime = m_timeline->getCurrentTime();
            if (currentTime != 0) m_timeline->setCurrentTime(currentTime - 1);
            break;
        }
        default:
            break;
    }
}

Uint32 TimelineController::frameFromMouseX(int mouseX, const SDL_Rect& rect) const {
    if (mouseX <= rect.x + m_view->trackStartXPos) return 0;
    return (mouseX - rect.x - m_view->trackStartXPos) * m_view->zoom / m_view->timeLabelInterval + m_view->scrollOffset;
}

Track TimelineController::getTrackID(SDL_Point mousePoint, const SDL_Rect& rect) {
    Track track;
    track.trackID = -1;
    for (int i = 0; i < m_timeline->getVideoTrackCount(); ++i) {
        if (mousePoint.y >= rect.y + m_view->topBarheight + m_view->trackHeight * i && mousePoint.y <= rect.y + m_view->topBarheight + m_view->trackHeight * (i + 1)) {
            track.trackID = m_timeline->getVideoTrackID(m_timeline->getVideoTrackCount() - 1 - i);
            track.trackType = VIDEO;
            return track;
        }
    }
    for (int i = 0; i < m_timeline->getAudioTrackCount(); ++i) {
        if (mousePoint.y >= rect.y + m_view->topBarheight + m_view->trackHeight * (m_timeline->getVideoTrackCount() + i) &&
            mousePoint.y <= rect.y + m_view->topBarheight + m_view->trackHeight * (m_timeline->getVideoTrackCount() + i + 1))
        {
            track.trackID = m_timeline->getAudioTrackID(i);
            track.trackType = AUDIO;
            return track;
        }
    }
    return track;
}

int TimelineController::getTrackPos(int y, const SDL_Rect& rect) {
    if (y < rect.y + m_view->topBarheight) return -1;

    int selectedTrackPos = (y - rect.y - m_view->topBarheight) / m_view->trackHeight;
    int videoTrackCount = m_timeline->getVideoTrackCount();
    if (selectedTrackPos < videoTrackCount) return videoTrackCount - 1 - selectedTrackPos;
    int audioTrackCount = m_timeline->getAudioTrackCount();
    if (selectedTrackPos < videoTrackCount + audioTrackCount) return selectedTrackPos - videoTrackCount;
    return -1;
}

void TimelineController::handleMouseButtonDown(const SDL_Event& event, const SDL_Rect& rect) {
    SDL_Point mouseButton = { event.button.x, event.button.y };
    if (!SDL_PointInRect(&mouseButton, &rect)) return;
    if (m_selection->isDragging) return;

    if (mouseButton.x < m_view->trackStartXPos) {
        if (event.button.button == SDL_BUTTON_RIGHT) {
            Track track = getTrackID(mouseButton, rect);
            if (track.trackID >= 0) {
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
        return;
    }

    Uint32 clickedFrame = frameFromMouseX(mouseButton.x, rect);

    // Click in video area?
    if (mouseButton.y > rect.y + m_view->topBarheight && mouseButton.y < rect.y + m_view->topBarheight + m_timeline->getVideoTrackCount() * m_view->trackHeight) {
        int selectedTrackPos = m_timeline->getVideoTrackCount() - 1 - ((mouseButton.y - rect.y - m_view->topBarheight) / m_view->trackHeight);
        VideoSegment* clickedVideoSegment = m_timeline->getVideoSegment(selectedTrackPos, clickedFrame);

        if (clickedVideoSegment) {
            if (event.button.button == SDL_BUTTON_LEFT) {
                if (SDL_GetModState() & KMOD_SHIFT) {
                    if (std::find(m_selection->selectedVideoSegments.begin(), m_selection->selectedVideoSegments.end(), clickedVideoSegment) == m_selection->selectedVideoSegments.end()) {
                        m_selection->selectedVideoSegments.push_back(clickedVideoSegment);
                    }
                    else {
                        m_selection->selectedVideoSegments.erase(std::remove(m_selection->selectedVideoSegments.begin(), m_selection->selectedVideoSegments.end(), clickedVideoSegment), m_selection->selectedVideoSegments.end());
                    }
                }
                else {
                    if (std::find(m_selection->selectedVideoSegments.begin(), m_selection->selectedVideoSegments.end(), clickedVideoSegment) == m_selection->selectedVideoSegments.end()) {
                        m_selection->selectedVideoSegments.clear();
                        m_selection->selectedAudioSegments.clear();
                        m_selection->selectedVideoSegments.push_back(clickedVideoSegment);
                    }
                    m_selection->isHolding = true;
                    m_selection->mouseHoldStartX = mouseButton.x;
                    m_selection->lastLegalTrackPos = selectedTrackPos;
                    m_selection->selectedMaxTrackPos = selectedTrackPos;
                    m_selection->selectedMinTrackPos = selectedTrackPos;
                    m_selection->lastLegalFrame = clickedFrame;
                    m_selection->lastLegalLeftmostFrame = clickedFrame;

                    for (auto* vs : m_selection->selectedVideoSegments) {
                        if (vs->timelinePosition < m_selection->lastLegalLeftmostFrame) m_selection->lastLegalLeftmostFrame = vs->timelinePosition;
                        int segmentTrackPos = m_timeline->getVideoTrackPos(vs->trackID);
                            m_selection->selectedMaxTrackPos = std::max(m_selection->selectedMaxTrackPos, segmentTrackPos);
                            m_selection->selectedMinTrackPos = std::min(m_selection->selectedMinTrackPos, segmentTrackPos);
                        }
                    for (auto* as : m_selection->selectedAudioSegments) {
                        if (as->timelinePosition < m_selection->lastLegalLeftmostFrame) m_selection->lastLegalLeftmostFrame = as->timelinePosition;
                        int segmentTrackPos = m_timeline->getVideoTrackPos(as->trackID);
                            m_selection->selectedMaxTrackPos = std::max(m_selection->selectedMaxTrackPos, segmentTrackPos);
                            m_selection->selectedMinTrackPos = std::min(m_selection->selectedMinTrackPos, segmentTrackPos);
                        }
                }
            }
            else if (event.button.button == SDL_BUTTON_RIGHT) {
                if (std::find(m_selection->selectedVideoSegments.begin(), m_selection->selectedVideoSegments.end(), clickedVideoSegment) == m_selection->selectedVideoSegments.end()) {
                    m_selection->selectedVideoSegments.clear();
                    m_selection->selectedAudioSegments.clear();
                    m_selection->selectedVideoSegments.push_back(clickedVideoSegment);
                }
            }
            return;
        }
    }
    // Otherwise, if clicked in the audioTrack area
    else if (mouseButton.y < rect.y + m_view->topBarheight + (m_timeline->getVideoTrackCount() + m_timeline->getAudioTrackCount()) * m_view->trackHeight)
    {
        int selectedTrackPos = (mouseButton.y - rect.y - m_view->topBarheight) / m_view->trackHeight - m_timeline->getVideoTrackCount();
        AudioSegment* clickedAudioSegment = m_timeline->getAudioSegment(selectedTrackPos, clickedFrame);

        if (clickedAudioSegment) {
            if (event.button.button == SDL_BUTTON_LEFT) {
                if (SDL_GetModState() & KMOD_SHIFT) {
                    if (std::find(m_selection->selectedAudioSegments.begin(), m_selection->selectedAudioSegments.end(), clickedAudioSegment) == m_selection->selectedAudioSegments.end()) {
                        m_selection->selectedAudioSegments.push_back(clickedAudioSegment);
                    }
                    else {
                        m_selection->selectedAudioSegments.erase(std::remove(m_selection->selectedAudioSegments.begin(), m_selection->selectedAudioSegments.end(), clickedAudioSegment), m_selection->selectedAudioSegments.end());
                    }
                }
                else {
                    if (std::find(m_selection->selectedAudioSegments.begin(), m_selection->selectedAudioSegments.end(), clickedAudioSegment) == m_selection->selectedAudioSegments.end()) {
                        m_selection->selectedVideoSegments.clear();
                        m_selection->selectedAudioSegments.clear();
                        m_selection->selectedAudioSegments.push_back(clickedAudioSegment);
                    }
                    m_selection->isHolding = true;
                    m_selection->mouseHoldStartX = mouseButton.x;
                    m_selection->lastLegalTrackPos = selectedTrackPos;
                    m_selection->selectedMaxTrackPos = selectedTrackPos;
                    m_selection->selectedMinTrackPos = selectedTrackPos;
                    m_selection->lastLegalFrame = clickedFrame;
                    m_selection->lastLegalLeftmostFrame = clickedFrame;

                    for (auto* vs : m_selection->selectedVideoSegments) {
                        if (vs->timelinePosition < m_selection->lastLegalLeftmostFrame) m_selection->lastLegalLeftmostFrame = vs->timelinePosition;
                        int segmentTrackPos = m_timeline->getVideoTrackPos(vs->trackID);
                        m_selection->selectedMaxTrackPos = std::max(m_selection->selectedMaxTrackPos, segmentTrackPos);
                        m_selection->selectedMinTrackPos = std::min(m_selection->selectedMinTrackPos, segmentTrackPos);
                    }
                    for (auto* as : m_selection->selectedAudioSegments) {
                        if (as->timelinePosition < m_selection->lastLegalLeftmostFrame) m_selection->lastLegalLeftmostFrame = as->timelinePosition;
                        int segmentTrackPos = m_timeline->getVideoTrackPos(as->trackID);
                        m_selection->selectedMaxTrackPos = std::max(m_selection->selectedMaxTrackPos, segmentTrackPos);
                        m_selection->selectedMinTrackPos = std::min(m_selection->selectedMinTrackPos, segmentTrackPos);
                    }
                }
            }
            else if (event.button.button == SDL_BUTTON_RIGHT) {
                if (std::find(m_selection->selectedAudioSegments.begin(), m_selection->selectedAudioSegments.end(), clickedAudioSegment) == m_selection->selectedAudioSegments.end()) {
                    m_selection->selectedVideoSegments.clear();
                    m_selection->selectedAudioSegments.clear();
                    m_selection->selectedAudioSegments.push_back(clickedAudioSegment);
                }
            }
            return;
        }
    }

    if (event.button.button == SDL_BUTTON_LEFT) {
        if (m_timeline->isPlaying()) m_timeline->togglePlaying();
        m_selection->isMovingCurrentTime = true;
        m_timeline->setCurrentTime(clickedFrame);
        m_selection->selectedVideoSegments.clear();
        m_selection->selectedAudioSegments.clear();
    }
    else if (event.button.button == SDL_BUTTON_RIGHT) {
        std::vector<ContextMenu::MenuItem> contextMenuOptions = {
            { "Delete Selected Item(s)", [this]() { m_timeline->deleteSegments(&m_selection->selectedVideoSegments, &m_selection->selectedAudioSegments); m_selection->clear(); } }
        };
        ContextMenu::show(mouseButton.x, mouseButton.y, contextMenuOptions);
    }
}

void TimelineController::handleMouseMotion(const SDL_Event& event, const SDL_Rect& rect) {
    SDL_Point mousePoint = { event.motion.x, event.motion.y };

    if (m_selection->isMovingCurrentTime) {
        Uint32 hoveredFrame = frameFromMouseX(mousePoint.x, rect);
        m_timeline->setCurrentTime(hoveredFrame);
    }

    if (m_selection->isHolding) {
        int mouseTrackPos = getTrackPos(mousePoint.y, rect);
        int deltaTrackPos = mouseTrackPos - m_selection->lastLegalTrackPos;
        if (deltaTrackPos != 0) {
            bool impossibleMove = false;
            if (deltaTrackPos > 0) {
                if (m_selection->selectedVideoSegments.empty()) {
                    if (m_selection->selectedMaxTrackPos + deltaTrackPos >= m_timeline->getAudioTrackCount()) impossibleMove = true;
                }
                else if (m_selection->selectedAudioSegments.empty()) {
                    if (m_selection->selectedMaxTrackPos + deltaTrackPos >= m_timeline->getVideoTrackCount()) impossibleMove = true;
                }
                else if (m_selection->selectedMaxTrackPos + deltaTrackPos >= std::min(m_timeline->getVideoTrackCount(), m_timeline->getAudioTrackCount())) {
                    impossibleMove = true;
                }
            }
            else if (deltaTrackPos < 0) {
                if (m_selection->selectedMinTrackPos + deltaTrackPos < 0) impossibleMove = true;
            }

            if (!impossibleMove) {
                if (m_timeline->segmentsChangeTrack(&m_selection->selectedVideoSegments, &m_selection->selectedAudioSegments, deltaTrackPos)) {
                    m_selection->lastLegalTrackPos = mouseTrackPos;
                    m_selection->selectedMaxTrackPos += deltaTrackPos;
                    m_selection->selectedMinTrackPos += deltaTrackPos;
                }
            }
        }
    }

    if (m_selection->isHolding && !m_selection->isDragging) {
        if (std::abs(mousePoint.x - m_selection->mouseHoldStartX) > m_selection->draggingThreshold) {
            m_selection->isDragging = true;
        }
    }

    if (m_selection->isDragging && (!m_selection->selectedVideoSegments.empty() || !m_selection->selectedAudioSegments.empty())) {
        Uint32 currentFrame = frameFromMouseX(mousePoint.x, rect);
        if (mousePoint.x < rect.x + m_view->trackStartXPos) currentFrame = 0;

        Uint32 deltaFrames;
        if (currentFrame < m_selection->lastLegalFrame) {
            Uint32 absDiff = m_selection->lastLegalFrame - currentFrame;
            if (absDiff > m_selection->lastLegalLeftmostFrame) {
                deltaFrames = UINT32_MAX - m_selection->lastLegalLeftmostFrame + 1;
            }
            else {
                deltaFrames = UINT32_MAX - absDiff + 1;
            }
        }
        else {
            deltaFrames = currentFrame - m_selection->lastLegalFrame;
        }

        if (deltaFrames == 0) return;

        if (m_timeline->segmentsMoveFrames(&m_selection->selectedVideoSegments, &m_selection->selectedAudioSegments, deltaFrames)) {
            m_selection->lastLegalFrame = currentFrame;
            m_selection->lastLegalLeftmostFrame += deltaFrames;
        }
    }
}

void TimelineController::handleMouseButtonUp(const SDL_Event& /*event*/) {
    m_selection->isHolding = false;
    m_selection->isDragging = false;
    m_selection->isMovingCurrentTime = false;
}

void TimelineController::handleMouseWheel(const SDL_Event& event) {
    SDL_Keymod mod = SDL_GetModState();

    int scrollDirection = (event.wheel.y > 0) ? 1 : (event.wheel.y < 0) ? -1 : 0;

    if (mod & KMOD_CTRL) {
        if (scrollDirection > 0 && m_view->zoom > 2) m_view->zoom /= 2;
        if (scrollDirection < 0 && m_view->zoom < UINT16_MAX / 2) m_view->zoom *= 2;
    } else if (mod & KMOD_SHIFT) {
        // Vertical scrolling (not implemented yet)
    }
    else {
        if ((int32_t)m_view->scrollOffset < scrollDirection * (int32_t)m_view->zoom) {
            m_view->scrollOffset = 0;
        }
        else {
            m_view->scrollOffset -= scrollDirection * m_view->zoom;
        }
    }
}

bool TimelineController::addAssetSegments(AssetData* data, int mouseX, int mouseY, const SDL_Rect& rect) {
    SDL_Point mousePoint = { mouseX, mouseY };
    if (!SDL_PointInRect(&mousePoint, &rect)) return false;
    if (mousePoint.x < m_view->trackStartXPos) return false;

    Uint32 selectedFrame = frameFromMouseX(mousePoint.x, rect);
    Track track = getTrackID(mousePoint, rect);
    SegmentPointer segmentPointer = m_timeline->addAssetSegments(m_renderer, data, selectedFrame, track);

    if (!segmentPointer.videoSegment && !segmentPointer.audioSegment) return false;

    m_selection->selectedVideoSegments.clear();
    m_selection->selectedAudioSegments.clear();

    if (data->videoData) m_selection->selectedVideoSegments.push_back(segmentPointer.videoSegment);
    if (data->audioData) m_selection->selectedAudioSegments.push_back(segmentPointer.audioSegment);

    m_selection->isHolding = true;
    m_selection->isDragging = true;
    int trackPos = getTrackPos(mousePoint.y, rect);
    m_selection->lastLegalTrackPos = trackPos;
    m_selection->selectedMaxTrackPos = trackPos;
    m_selection->selectedMinTrackPos = trackPos;
    m_selection->lastLegalFrame = selectedFrame;
    m_selection->lastLegalLeftmostFrame = selectedFrame;

    for (auto* vs : m_selection->selectedVideoSegments) {
        if (vs->timelinePosition < m_selection->lastLegalLeftmostFrame) m_selection->lastLegalLeftmostFrame = vs->timelinePosition;
    }
    for (auto* as : m_selection->selectedAudioSegments) {
        if (as->timelinePosition < m_selection->lastLegalLeftmostFrame) m_selection->lastLegalLeftmostFrame = as->timelinePosition;
    }

    return true;
}
