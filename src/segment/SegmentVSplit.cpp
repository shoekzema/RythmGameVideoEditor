#include "Segment.h"

SegmentVSplit::SegmentVSplit(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, SDL_Color color)
    : Segment(x, y, w, h, renderer, eventManager, color), draggingDivider(false), resizing(false)
{
    divider = { x + w / 2 - dividerThickness / 2, y, dividerThickness, h };
    leftSegment  = new AssetsList(x,                                y, w / 2 - dividerThickness / 2, h, renderer, eventManager);
    rightSegment = new VideoPlayer(x + w / 2 + dividerThickness / 2, y, w / 2 - dividerThickness / 2, h, renderer, eventManager);
}

SegmentVSplit::~SegmentVSplit() {
    if (leftSegment) delete leftSegment;
    if (rightSegment) delete rightSegment;
}

void SegmentVSplit::render() {
    leftSegment->render();
    rightSegment->render();

    SDL_SetRenderDrawColor(renderer, dividerColor.r, dividerColor.g, dividerColor.b, dividerColor.a);
    SDL_RenderFillRect(renderer, &divider);
}

void SegmentVSplit::update(int x, int y, int w, int h) {
    int widthDiff = rect.w - w;
    int widthChange = widthDiff / (rect.w / (float)leftSegment->rect.w);
    leftSegment ->update(x,                   y, leftSegment->rect.w - widthChange, h);
    rightSegment->update(leftSegment->rect.w, y, w - leftSegment->rect.w,           h);
    divider = { divider.x - widthChange, y, dividerThickness, h };
    Segment::update(x, y, w, h);
}

void SegmentVSplit::handleEvent(SDL_Event& event) {
    if (event.type == SDL_MOUSEBUTTONDOWN) {
        SDL_Point mousePoint = { event.button.x, event.button.y };
        if (SDL_PointInRect(&mousePoint, &divider)) {
            draggingDivider = true;
        }
    }
    else if (event.type == SDL_MOUSEMOTION) {
        SDL_Point mouseMotion = { event.motion.x, event.motion.y };

        // If not in this Segment, we ignore it 
        if (SDL_PointInRect(&mouseMotion, &rect)) {
            if (draggingDivider) {
                // Resize the segments dynamically by dragging the divider
                int newMiddle = std::max(dividerThickness/2, std::min(event.motion.x, appWindowSizeX - dividerThickness/2)) - rect.x - divider.w / 2;
                divider.x = newMiddle;
                leftSegment->update(leftSegment->rect.x, leftSegment->rect.y, newMiddle, leftSegment->rect.h);
                rightSegment->update(newMiddle, rightSegment->rect.y, rect.w - newMiddle, rightSegment->rect.h);
            }
            else if (SDL_PointInRect(&mouseMotion, &divider)) {
                SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE));
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

    leftSegment->handleEvent(event);
    rightSegment->handleEvent(event);
}