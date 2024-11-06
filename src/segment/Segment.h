#pragma once
#include <SDL.h>
#include "EventManager.h"

/**
 * @class Segment
 * @brief Basic empty window segment.
 */
class Segment {
public:
    Segment(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Segment* parent = nullptr, SDL_Color color = { 0, 0, 0, 255 });
    virtual ~Segment();

    virtual void render();
    virtual void update(int x, int y, int w, int h);
    /**
     * @brief Handle user events.
     * @param event User interaction event code.
     */
    virtual void handleEvent(SDL_Event& event);
    /**
     * @brief Find a segment with type T. (Best to call from the root segment)
     * @returns The first segment in the hierarchy with type T.
     */
    template <typename T>
    T* findType();

    // The virtual implementation method for findType(). Should only be called from findType() or any overwritten findTypeImpl().
    virtual Segment* findTypeImpl(const std::type_info& type);
public:
    Segment* parent;
    SDL_Rect rect;
protected:
    SDL_Renderer* p_renderer;
    EventManager* p_eventManager;
    SDL_Color p_color;
};

template<typename T>
T* Segment::findType() {
    return dynamic_cast<T*>(this->findTypeImpl(typeid(T)));
}
