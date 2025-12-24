#pragma once

#include <SDL.h>
#include "Timeline.h"
#include "TimelineSelectionManager.h"
#include "TimelineView.h"

class TimelineRenderer {
public:
    TimelineRenderer(Timeline* timeline, SDL_Renderer* renderer);
    void render(const SDL_Rect& rect, const TimelineView& view, const TimelineSelectionManager& selection);

private:
    Timeline* m_timeline;
    SDL_Renderer* m_renderer;

    void renderTopBar(const SDL_Rect& rect, const TimelineView& view);
    void renderVideoTracks(const SDL_Rect& rect, const TimelineView& view);
    void renderVideoSegments(const SDL_Rect& rect, const TimelineView& view, const TimelineSelectionManager& selection);
    void renderAudioTracks(const SDL_Rect& rect, const TimelineView& view);
    void renderAudioSegments(const SDL_Rect& rect, const TimelineView& view, const TimelineSelectionManager& selection);
    void renderTimeIndicator(const SDL_Rect& rect, const TimelineView& view);
};
