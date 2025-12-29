#pragma once
#include <SDL.h>
#include "EventManager.h"

/**
 * @class Window
 * @brief Basic empty window segment.
 */
class Window {
public:
    Window(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Window* parent = nullptr, SDL_Color color = { 0, 0, 0, 255 });
    virtual ~Window();

    virtual void render();
    virtual void update(int x, int y, int w, int h);
    /**
     * @brief Handle user events.
     * @param event User interaction event code.
     */
    virtual void handleEvent(SDL_Event& event);
    /**
     * @brief Find a window with type T. (Best to call from the root window)
     * @returns The first window in the hierarchy with type T.
     */
    template <typename T>
    T* findType();

    // The virtual implementation method for findType(). Should only be called from findType() or any overwritten findTypeImpl().
    virtual Window* findTypeImpl(const std::type_info& type);

public:
    Window* parent;
    SDL_Rect rect;

protected:
    SDL_Renderer* p_renderer;
    EventManager* p_eventManager;
    SDL_Color p_color;
};

template<typename T>
T* Window::findType() {
    return dynamic_cast<T*>(this->findTypeImpl(typeid(T)));
}
