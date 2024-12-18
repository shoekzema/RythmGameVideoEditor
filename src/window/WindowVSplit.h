#pragma once
#include <iostream>
#include <SDL.h>
#include "Window.h"
#include "EventManager.h"

/**
 * @class WindowVSplit
 * @brief Window segment split into a left and right window with a movable vertical divider between that allows for resizing.
 */
class WindowVSplit : public Window {
public:
    WindowVSplit(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Window* parent = nullptr, SDL_Color color = { 0, 255, 0, 255 });
    ~WindowVSplit();

    void render() override;
    void update(int x, int y, int w, int h) override;
    void handleEvent(SDL_Event& event) override;
    Window* findTypeImpl(const std::type_info& type) override;

    void setLeftWindow(Window* window);
    void setRightWindow(Window* window);
private:
    SDL_Rect m_divider;
    SDL_Color m_dividerColor = { 95, 98, 101, 255 };
    bool m_draggingDivider = false;
    bool m_resizing = false; // For resize handles
    int m_dividerThickness = 2;

    Window* m_leftWindow;
    Window* m_rightWindow;
};