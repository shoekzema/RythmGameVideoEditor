#include <SDL.h>
#include <iostream>
#include "WindowHSplit.h"
#include "util.h"

WindowHSplit::WindowHSplit(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Window* parent, SDL_Color color)
    : Window(x, y, w, h, renderer, eventManager, parent, color)
{
    m_divider = { x, y + h / 2 - m_dividerThickness / 2, w, m_dividerThickness };
    m_topWindow    = new Window(x, y,                                  w, h / 2 - m_dividerThickness / 2, renderer, eventManager, this);
    m_bottomWindow = new Window(x, y + h / 2 + m_dividerThickness / 2, w, h / 2 - m_dividerThickness / 2, renderer, eventManager, this);
}

WindowHSplit::~WindowHSplit() {
    if (m_topWindow) delete m_topWindow;
    if (m_bottomWindow) delete m_bottomWindow;
}

void WindowHSplit::setTopWindow(Window* window) {
    SDL_Rect prevRect = m_topWindow->rect;
    m_topWindow = window;
    m_topWindow->update(prevRect.x, prevRect.y, prevRect.w, prevRect.h);
}
void WindowHSplit::setBottomWindow(Window* window) {
    SDL_Rect prevRect = m_bottomWindow->rect;
    m_bottomWindow = window;
    m_bottomWindow->update(prevRect.x, prevRect.y, prevRect.w, prevRect.h);
}

void WindowHSplit::render() {
    Window::render();

    m_topWindow->render();
    m_bottomWindow->render();

    SDL_SetRenderDrawColor(p_renderer, m_dividerColor.r, m_dividerColor.g, m_dividerColor.b, m_dividerColor.a);
    SDL_RenderFillRect(p_renderer, &m_divider);
}

void WindowHSplit::update(int x, int y, int w, int h) {
    int heightDiff = rect.h - h;
    int heightChange = heightDiff * m_topWindow->rect.h / rect.h;
    m_topWindow   ->update(x, y,                    w, m_topWindow->rect.h - heightChange);
    m_bottomWindow->update(x, m_topWindow->rect.h, w, h - m_topWindow->rect.h);
    m_divider = { x, m_divider.y - heightChange, w, m_dividerThickness };
    Window::update(x, y, w, h);
}

void WindowHSplit::handleEvent(SDL_Event& event) {
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
                int newMiddle = std::max(m_dividerThickness / 2, std::min(event.motion.y, appWindowSizeY - m_dividerThickness / 2)) - rect.y - m_divider.h / 2;
                m_divider.y = newMiddle;
                m_topWindow->update(m_topWindow->rect.x, m_topWindow->rect.y, m_topWindow->rect.w, newMiddle);
                m_bottomWindow->update(m_bottomWindow->rect.x, newMiddle + m_dividerThickness, m_bottomWindow->rect.w, rect.h - newMiddle - m_dividerThickness);
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

    m_topWindow->handleEvent(event);
    m_bottomWindow->handleEvent(event);
}

Window* WindowHSplit::findTypeImpl(const std::type_info& type) {
    if (type == typeid(WindowHSplit)) {
        return this;
    }
    if (m_topWindow) {
        if (Window* found = m_topWindow->findTypeImpl(type)) {
            return found;
        }
    }
    if (m_bottomWindow) {
        if (Window* found = m_bottomWindow->findTypeImpl(type)) {
            return found;
        }
    }
    return nullptr;
}
