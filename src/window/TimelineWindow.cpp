#include <iostream>
#include <string>
#include <algorithm>    
#include <vector>
#include <unordered_map>
#include <SDL_ttf.h>
#include "TimelineWindow.h"
#include "util.h"
#include "ContextMenu.h"

TimeLineWindow::TimeLineWindow(Timeline* timeline, int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Window* parent, SDL_Color color)
    : Window(x, y, w, h, renderer, eventManager, parent, color)
{
    m_timeline = timeline;
    m_rendererImpl = new TimelineRenderer(m_timeline, p_renderer);
    m_controller = new TimelineController(m_timeline, &m_selection, &m_view, p_renderer);
}

TimeLineWindow::~TimeLineWindow() {
    // No need to delete renderer since it is managed elsewhere
    delete m_rendererImpl;
    delete m_controller;
}

void TimeLineWindow::render() {
    m_rendererImpl->render(rect, m_view, m_selection);
}

void TimeLineWindow::handleEvent(SDL_Event& event) {
    m_controller->handleEvent(event, rect);
}

void TimeLineWindow::update(int x, int y, int w, int h) {
    rect = { x, y, w, h };
}

void TimeLineWindow::deleteSelectedSegments() {
    m_timeline->deleteSegments(&m_selection.selectedVideoSegments, &m_selection.selectedAudioSegments);
    m_selection.clear();
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
    for (int i = 0; i < m_timeline->getVideoTrackCount(); ++i) {
        if (mousePoint.y >= rect.y + m_view.topBarheight + m_view.trackHeight * i && mousePoint.y <= rect.y + m_view.topBarheight + m_view.trackHeight * (i + 1)) {
            track.trackID = m_timeline->getVideoTrackID(m_timeline->getVideoTrackCount() - 1 - i);
            track.trackType = VIDEO;
            return track;
        }
    }
    for (int i = 0; i < m_timeline->getAudioTrackCount(); ++i) {
        if (mousePoint.y >= rect.y + m_view.topBarheight + m_view.trackHeight * (m_timeline->getVideoTrackCount() + i) &&
            mousePoint.y <= rect.y + m_view.topBarheight + m_view.trackHeight * (m_timeline->getAudioTrackCount() + i + 1))
        {
            track.trackID = m_timeline->getAudioTrackID(i);
            track.trackType = AUDIO;
            return track;
        }
    }
    return track;
}

int TimeLineWindow::getTrackPos(int y) {
    if (y < rect.y + m_view.topBarheight) return -1;

    int selectedTrackPos = (y - rect.y - m_view.topBarheight) / m_view.trackHeight;
    int videoTrackCount = m_timeline->getVideoTrackCount();
    if (selectedTrackPos < videoTrackCount) return videoTrackCount - 1 - selectedTrackPos;

    int audioTrackCount = m_timeline->getAudioTrackCount();
    if (selectedTrackPos < videoTrackCount + audioTrackCount) return selectedTrackPos - videoTrackCount;
    return -1;
}

bool TimeLineWindow::addAssetSegments(AssetData* data, int mouseX, int mouseY) {
    return m_controller->addAssetSegments(data, mouseX, mouseY, rect);
}
