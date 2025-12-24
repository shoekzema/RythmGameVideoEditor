#pragma once
#include <SDL.h>
#include <iostream>
#include <vector>
#include <unordered_map>
#include "Window.h"
#include "EventManager.h"
#include "VideoData.h"
#include "Timeline.h"
#include "TimelineSelectionManager.h"
#include "TimelineView.h"
#include "TimelineRenderer.h"
#include "TimelineController.h"

/**
 * @class TimeLineWindow
 * @brief Window segment that shows the timeline. Editing is mostly done here.
 */
class TimeLineWindow : public Window {
public:
    TimeLineWindow(Timeline* timeline, int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Window* parent = nullptr, SDL_Color color = { 42, 46, 50, 255 });
    ~TimeLineWindow();

    void render() override;
    void update(int x, int y, int w, int h) override;
    void handleEvent(SDL_Event& event) override;
    Window* findTypeImpl(const std::type_info& type) override;

    // Add a video and/or audio segment to the timeline at the mouse position
    bool addAssetSegments(AssetData* data, int mouseX, int mouseY);

    Timeline* tempGetTimeline() { return m_timeline; };

private:
    // Delegated helpers
    Track getTrackID(SDL_Point mousePoint);
    int getTrackPos(int y);
    void deleteSelectedSegments();

private:
    Timeline* m_timeline;
    bool tempAddedVid = false;

    TimelineSelectionManager m_selection;
    TimelineView m_view;
    TimelineRenderer* m_rendererImpl = nullptr;
    TimelineController* m_controller = nullptr;
};