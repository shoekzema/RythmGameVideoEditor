#include "Segment.h"

SegmentHSplit::SegmentHSplit(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Segment* parent, SDL_Color color)
    : Segment(x, y, w, h, renderer, eventManager, parent, color), draggingDivider(false), resizing(false)
{
    divider = { x, y + h / 2 - dividerThickness / 2, w, dividerThickness };
    topSegment    = new Segment(x, y,                                w, h / 2 - dividerThickness / 2, renderer, eventManager, this);
    bottomSegment = new Segment(x, y + h / 2 + dividerThickness / 2, w, h / 2 - dividerThickness / 2, renderer, eventManager, this);
}

SegmentHSplit::~SegmentHSplit() {
    if (topSegment) delete topSegment;
    if (bottomSegment) delete bottomSegment;
}

void SegmentHSplit::setTopSegment(Segment* segment) {
    SDL_Rect prevRect = topSegment->rect;
    topSegment = segment;
    topSegment->update(prevRect.x, prevRect.y, prevRect.w, prevRect.h);
}
void SegmentHSplit::setBottomSegment(Segment* segment) {
    SDL_Rect prevRect = bottomSegment->rect;
    bottomSegment = segment;
    bottomSegment->update(prevRect.x, prevRect.y, prevRect.w, prevRect.h);
}

void SegmentHSplit::render() {
    Segment::render();

    topSegment->render();
    bottomSegment->render();

    SDL_SetRenderDrawColor(renderer, dividerColor.r, dividerColor.g, dividerColor.b, dividerColor.a);
    SDL_RenderFillRect(renderer, &divider);
}

void SegmentHSplit::update(int x, int y, int w, int h) {
    int heightDiff = rect.h - h;
    int heightChange = (int)(heightDiff / (rect.h / (float)topSegment->rect.h));
    topSegment   ->update(x, y,                  w, topSegment->rect.h - heightChange);
    bottomSegment->update(x, topSegment->rect.h, w, h - topSegment->rect.h);
    divider = { x, divider.y - heightChange, w, dividerThickness };
    Segment::update(x, y, w, h);
}

void SegmentHSplit::handleEvent(SDL_Event& event) {
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
                int newMiddle = std::max(dividerThickness/2, std::min(event.motion.y, appWindowSizeY - dividerThickness/2)) - rect.y - divider.h / 2;
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

Segment* SegmentHSplit::findTypeImpl(const std::type_info& type) {
    if (type == typeid(SegmentHSplit)) {
        return this;
    }
    if (topSegment) {
        if (Segment* found = topSegment->findTypeImpl(type)) {
            return found;
        }
    }
    if (bottomSegment) {
        if (Segment* found = bottomSegment->findTypeImpl(type)) {
            return found;
        }
    }
    return nullptr;
}
