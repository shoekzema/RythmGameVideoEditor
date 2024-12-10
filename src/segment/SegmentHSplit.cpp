#include <iostream>
#include "SegmentHSplit.h"
#include "util.h"

SegmentHSplit::SegmentHSplit(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Segment* parent, SDL_Color color)
    : Segment(x, y, w, h, renderer, eventManager, parent, color)
{
    m_divider = { x, y + h / 2 - m_dividerThickness / 2, w, m_dividerThickness };
    m_topSegment    = new Segment(x, y,                                  w, h / 2 - m_dividerThickness / 2, renderer, eventManager, this);
    m_bottomSegment = new Segment(x, y + h / 2 + m_dividerThickness / 2, w, h / 2 - m_dividerThickness / 2, renderer, eventManager, this);
}

SegmentHSplit::~SegmentHSplit() {
    if (m_topSegment) delete m_topSegment;
    if (m_bottomSegment) delete m_bottomSegment;
}

void SegmentHSplit::setTopSegment(Segment* segment) {
    SDL_Rect prevRect = m_topSegment->rect;
    m_topSegment = segment;
    m_topSegment->update(prevRect.x, prevRect.y, prevRect.w, prevRect.h);
}
void SegmentHSplit::setBottomSegment(Segment* segment) {
    SDL_Rect prevRect = m_bottomSegment->rect;
    m_bottomSegment = segment;
    m_bottomSegment->update(prevRect.x, prevRect.y, prevRect.w, prevRect.h);
}

void SegmentHSplit::render() {
    Segment::render();

    m_topSegment->render();
    m_bottomSegment->render();

    SDL_SetRenderDrawColor(p_renderer, m_dividerColor.r, m_dividerColor.g, m_dividerColor.b, m_dividerColor.a);
    SDL_RenderFillRect(p_renderer, &m_divider);
}

void SegmentHSplit::update(int x, int y, int w, int h) {
    int heightDiff = rect.h - h;
    int heightChange = heightDiff * m_topSegment->rect.h / rect.h;
    m_topSegment   ->update(x, y,                    w, m_topSegment->rect.h - heightChange);
    m_bottomSegment->update(x, m_topSegment->rect.h, w, h - m_topSegment->rect.h);
    m_divider = { x, m_divider.y - heightChange, w, m_dividerThickness };
    Segment::update(x, y, w, h);
}

void SegmentHSplit::handleEvent(SDL_Event& event) {
    switch (event.type) {
    case SDL_MOUSEBUTTONDOWN: {
        SDL_Point mousePoint = { event.button.x, event.button.y };
        if (SDL_PointInRect(&mousePoint, &m_divider)) {
            m_draggingDivider = true;
        }
        break;
    }
    case SDL_MOUSEMOTION: {
        SDL_Point mouseMotion = { event.motion.x, event.motion.y };

        // If not in this Segment, we ignore it 
        if (SDL_PointInRect(&mouseMotion, &rect)) {
            if (m_draggingDivider) {
                // Resize the segments dynamically by dragging the divider
                int newMiddle = std::max(m_dividerThickness / 2, std::min(event.motion.y, appWindowSizeY - m_dividerThickness / 2)) - rect.y - m_divider.h / 2;
                m_divider.y = newMiddle;
                m_topSegment->update(m_topSegment->rect.x, m_topSegment->rect.y, m_topSegment->rect.w, newMiddle);
                m_bottomSegment->update(m_bottomSegment->rect.x, newMiddle + m_dividerThickness, m_bottomSegment->rect.w, rect.h - newMiddle - m_dividerThickness);
            }
            else if (SDL_PointInRect(&mouseMotion, &m_divider)) {
                SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS));
            }
            else {
                SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW));
            }
        }
        break;
    }
    case SDL_MOUSEBUTTONUP: {
        SDL_Point mousePoint = { event.button.x, event.button.y };
        m_draggingDivider = false;
        break;
    }
    }

    m_topSegment->handleEvent(event);
    m_bottomSegment->handleEvent(event);
}

Segment* SegmentHSplit::findTypeImpl(const std::type_info& type) {
    if (type == typeid(SegmentHSplit)) {
        return this;
    }
    if (m_topSegment) {
        if (Segment* found = m_topSegment->findTypeImpl(type)) {
            return found;
        }
    }
    if (m_bottomSegment) {
        if (Segment* found = m_bottomSegment->findTypeImpl(type)) {
            return found;
        }
    }
    return nullptr;
}
