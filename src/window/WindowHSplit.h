#pragma once
#include <iostream>
#include <SDL.h>
#include "Window.h"
#include "EventManager.h"

/**
 * @class WindowHSplit
 * @brief Window segment split into a top and bottom window with a movable horizontal divider between that allows for resizing.
 */
class WindowHSplit : public Window {
public:
    WindowHSplit(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Window* parent = nullptr, SDL_Color color = { 0, 255, 0, 255 });
    ~WindowHSplit();

    void render() override;
    void update(int x, int y, int w, int h) override;
    void handleEvent(SDL_Event& event) override;
    Window* findTypeImpl(const std::type_info& type) override;

    void setTopWindow(Window* window);
    void setBottomWindow(Window* window);
private:
    SDL_Rect m_divider;
    SDL_Color m_dividerColor = { 95, 98, 101, 255 };
    bool m_draggingDivider = false;
    bool m_resizing = false; // For resize handles
    int m_dividerThickness = 2;

    Window* m_topWindow;
    Window* m_bottomWindow;
};