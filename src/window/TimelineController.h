#pragma once

#include <SDL.h>
#include "Timeline.h"
#include "TimelineSelectionManager.h"
#include "TimelineView.h"

class TimelineController {
public:
    TimelineController(Timeline* timeline, TimelineSelectionManager* selection, TimelineView* view, SDL_Renderer* renderer);

    void handleEvent(SDL_Event& event, const SDL_Rect& rect);
    bool addAssetSegments(AssetData* data, int mouseX, int mouseY, const SDL_Rect& rect);

private:
    Timeline* m_timeline;
    TimelineSelectionManager* m_selection;
    TimelineView* m_view;
    SDL_Renderer* m_renderer;

    void handleKeyDown(const SDL_Event& event);
    void handleMouseButtonDown(const SDL_Event& event, const SDL_Rect& rect);
    void handleMouseMotion(const SDL_Event& event, const SDL_Rect& rect);
    void handleMouseButtonUp(const SDL_Event& event);
    void handleMouseWheel(const SDL_Event& event);

    Uint32 frameFromMouseX(int mouseX, const SDL_Rect& rect) const;
    Track getTrackID(SDL_Point mousePoint, const SDL_Rect& rect);
    int getTrackPos(int y, const SDL_Rect& rect);
};
