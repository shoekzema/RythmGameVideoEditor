#include <SDL.h>
#include <iostream>
#include "WindowVSplit.h"
#include "util.h"

WindowVSplit::WindowVSplit(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Window* parent, SDL_Color color)
    : Window(x, y, w, h, renderer, eventManager, parent, color)
{
    m_divider = { x + w / 2 - m_dividerThickness / 2, y, m_dividerThickness, h };
    m_leftWindow  = new Window(x,                                  y, w / 2 - m_dividerThickness / 2, h, renderer, eventManager, this);
    m_rightWindow = new Window(x + w / 2 + m_dividerThickness / 2, y, w / 2 - m_dividerThickness / 2, h, renderer, eventManager, this);
}

WindowVSplit::~WindowVSplit() {
    if (m_leftWindow) delete m_leftWindow;
    if (m_rightWindow) delete m_rightWindow;
}

void WindowVSplit::setLeftWindow(Window* window) {
    SDL_Rect prevRect = m_leftWindow->rect;
    m_leftWindow = window;
    m_leftWindow->update(prevRect.x, prevRect.y, prevRect.w, prevRect.h);
}
void WindowVSplit::setRightWindow(Window* window) {
    SDL_Rect prevRect = m_rightWindow->rect;
    m_rightWindow = window;
    m_rightWindow->update(prevRect.x, prevRect.y, prevRect.w, prevRect.h);
}

void WindowVSplit::render() {
    m_leftWindow->render();
    m_rightWindow->render();

    SDL_SetRenderDrawColor(p_renderer, m_dividerColor.r, m_dividerColor.g, m_dividerColor.b, m_dividerColor.a);
    SDL_RenderFillRect(p_renderer, &m_divider);
}

void WindowVSplit::update(int x, int y, int w, int h) {
    int widthDiff = rect.w - w;
    int widthChange = widthDiff * m_leftWindow->rect.w / rect.w;
    m_leftWindow ->update(x, y, m_leftWindow->rect.w - widthChange, h);
    m_rightWindow->update(m_leftWindow->rect.w, y, w - m_leftWindow->rect.w,           h);
    m_divider = { m_divider.x - widthChange, y, m_dividerThickness, h };
    Window::update(x, y, w, h);
}

void WindowVSplit::handleEvent(SDL_Event& event) {
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

        // If not in this window, we ignore it 
        if (SDL_PointInRect(&mouseMotion, &rect)) {
            if (m_draggingDivider) {
                // Resize the windows dynamically by dragging the divider
                int newMiddle = std::max(m_dividerThickness / 2, std::min(event.motion.x, appWindowSizeX - m_dividerThickness / 2)) - m_divider.w / 2;
                m_divider.x = newMiddle;
                m_leftWindow->update(m_leftWindow->rect.x, m_leftWindow->rect.y, newMiddle - rect.x, m_leftWindow->rect.h);
                m_rightWindow->update(newMiddle + m_dividerThickness, m_rightWindow->rect.y, rect.w - m_leftWindow->rect.w - m_dividerThickness, m_rightWindow->rect.h);
            }
            else if (SDL_PointInRect(&mouseMotion, &m_divider)) {
                SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE));
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

    m_leftWindow->handleEvent(event);
    m_rightWindow->handleEvent(event);
}

Window* WindowVSplit::findTypeImpl(const std::type_info& type) {
    if (type == typeid(WindowVSplit)) {
        return this;
    }
    if (m_leftWindow) {
        if (Window* found = m_leftWindow->findTypeImpl(type)) {
            return found;
        }
    }
    if (m_rightWindow) {
        if (Window* found = m_rightWindow->findTypeImpl(type)) {
            return found;
        }
    }
    return nullptr;
}
