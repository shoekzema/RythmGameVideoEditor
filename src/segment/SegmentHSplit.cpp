#include "Segment.h"

// Segment constructor
SegmentHSplit::SegmentHSplit(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, SDL_Color color)
    : Segment(x, y, w, h, renderer, eventManager, color), draggingDivider(false), resizing(false) 
{
    divider = { x, y + h / 2 - dividerThickness / 2, w, dividerThickness };
    topSegment    = new SegmentVSplit(x, y,                          w, h / 2 - dividerThickness / 2, renderer, eventManager, color);
    bottomSegment = new Segment(x, y + h / 2 + dividerThickness / 2, w, h / 2 - dividerThickness / 2, renderer, eventManager, color);
}

// Segment destructor
SegmentHSplit::~SegmentHSplit() {
    if (topSegment) delete topSegment;
    if (bottomSegment) delete bottomSegment;
}

// Render the segment and its split children
void SegmentHSplit::render() {
    topSegment->render();
    bottomSegment->render();

    SDL_SetRenderDrawColor(renderer, dividerColor.r, dividerColor.g, dividerColor.b, dividerColor.a);
    SDL_RenderFillRect(renderer, &divider);
}

// Update the position and size of the segment
void SegmentHSplit::update(int x, int y, int w, int h) {
    Segment::update(x, y, w, h);
    int heightDiff = h - rect.h;
    topSegment   ->update(x, y,                  w, topSegment->rect.h - heightDiff / 2);
    bottomSegment->update(x, topSegment->rect.h, w, h - topSegment->rect.h);
    divider = { x, divider.y - heightDiff / 2, w, dividerThickness };
}

// Handle user events
void SegmentHSplit::handleEvent(SDL_Event& event) {
    if (event.type == SDL_MOUSEBUTTONDOWN) {
        SDL_Point mousePoint = { event.button.x, event.button.y };
        if (SDL_PointInRect(&mousePoint, &divider)) {
            draggingDivider = true;
        }
    }
    else if (event.type == SDL_MOUSEMOTION) {
        SDL_Point mouseMotion = { event.motion.x, event.motion.y };
        if (SDL_PointInRect(&mouseMotion, &rect)) { // Not in this Segment, so we ignore it 
            if (draggingDivider) {
                // Resize the segments dynamically by dragging the divider
                int newMiddle = std::max(0, std::min(event.motion.y, appWindowSizeY)) - rect.y - divider.h / 2;
                divider.y = newMiddle;
                topSegment   ->update(topSegment   ->rect.x, topSegment->rect.y, topSegment   ->rect.w, newMiddle);
                bottomSegment->update(bottomSegment->rect.x, newMiddle,          bottomSegment->rect.w, rect.h - newMiddle);
            }
            else if (SDL_PointInRect(&mouseMotion, &divider)) {
                SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS));
            }
            else {
                SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW));
            }
        }
    }
    else if (event.type == SDL_MOUSEBUTTONUP) {
        SDL_Point mousePoint = { event.button.x, event.button.y };
        draggingDivider = false;
    }

    topSegment->handleEvent(event);
    bottomSegment->handleEvent(event);
}