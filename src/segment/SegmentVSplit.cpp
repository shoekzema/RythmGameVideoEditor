#include "Segment.h"

SegmentVSplit::SegmentVSplit(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Segment* parent, SDL_Color color)
    : Segment(x, y, w, h, renderer, eventManager, parent, color)
{
    m_divider = { x + w / 2 - m_dividerThickness / 2, y, m_dividerThickness, h };
    m_leftSegment  = new Segment(x,                                  y, w / 2 - m_dividerThickness / 2, h, renderer, eventManager, this);
    m_rightSegment = new Segment(x + w / 2 + m_dividerThickness / 2, y, w / 2 - m_dividerThickness / 2, h, renderer, eventManager, this);
}

SegmentVSplit::~SegmentVSplit() {
    if (m_leftSegment) delete m_leftSegment;
    if (m_rightSegment) delete m_rightSegment;
}

void SegmentVSplit::setLeftSegment(Segment* segment) {
    SDL_Rect prevRect = m_leftSegment->rect;
    m_leftSegment = segment;
    m_leftSegment->update(prevRect.x, prevRect.y, prevRect.w, prevRect.h);
}
void SegmentVSplit::setRightSegment(Segment* segment) {
    SDL_Rect prevRect = m_rightSegment->rect;
    m_rightSegment = segment;
    m_rightSegment->update(prevRect.x, prevRect.y, prevRect.w, prevRect.h);
}

void SegmentVSplit::render() {
    m_leftSegment->render();
    m_rightSegment->render();

    SDL_SetRenderDrawColor(p_renderer, m_dividerColor.r, m_dividerColor.g, m_dividerColor.b, m_dividerColor.a);
    SDL_RenderFillRect(p_renderer, &m_divider);
}

void SegmentVSplit::update(int x, int y, int w, int h) {
    int widthDiff = rect.w - w;
    int widthChange = widthDiff / (rect.w / (float)m_leftSegment->rect.w);
    m_leftSegment ->update(x,                   y, m_leftSegment->rect.w - widthChange, h);
    m_rightSegment->update(m_leftSegment->rect.w, y, w - m_leftSegment->rect.w,           h);
    m_divider = { m_divider.x - widthChange, y, m_dividerThickness, h };
    Segment::update(x, y, w, h);
}

void SegmentVSplit::handleEvent(SDL_Event& event) {
    if (event.type == SDL_MOUSEBUTTONDOWN) {
        SDL_Point mousePoint = { event.button.x, event.button.y };
        if (SDL_PointInRect(&mousePoint, &m_divider)) {
            m_draggingDivider = true;
        }
    }
    else if (event.type == SDL_MOUSEMOTION) {
        SDL_Point mouseMotion = { event.motion.x, event.motion.y };

        // If not in this Segment, we ignore it 
        if (SDL_PointInRect(&mouseMotion, &rect)) {
            if (m_draggingDivider) {
                // Resize the segments dynamically by dragging the divider
                int newMiddle = std::max(m_dividerThickness/2, std::min(event.motion.x, appWindowSizeX - m_dividerThickness/2)) - rect.x - m_divider.w / 2;
                m_divider.x = newMiddle;
                m_leftSegment->update(m_leftSegment->rect.x, m_leftSegment->rect.y, newMiddle, m_leftSegment->rect.h);
                m_rightSegment->update(newMiddle, m_rightSegment->rect.y, rect.w - newMiddle, m_rightSegment->rect.h);
            }
            else if (SDL_PointInRect(&mouseMotion, &m_divider)) {
                SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE));
            }
            else {
                SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW));
            }
        }
    }
    else if (event.type == SDL_MOUSEBUTTONUP) {
        SDL_Point mousePoint = { event.button.x, event.button.y };
        m_draggingDivider = false;
    }

    m_leftSegment->handleEvent(event);
    m_rightSegment->handleEvent(event);
}

Segment* SegmentVSplit::findTypeImpl(const std::type_info& type) {
    if (type == typeid(SegmentVSplit)) {
        return this;
    }
    if (m_leftSegment) {
        if (Segment* found = m_leftSegment->findTypeImpl(type)) {
            return found;
        }
    }
    if (m_rightSegment) {
        if (Segment* found = m_rightSegment->findTypeImpl(type)) {
            return found;
        }
    }
    return nullptr;
}
