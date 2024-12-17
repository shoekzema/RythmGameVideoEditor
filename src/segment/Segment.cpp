#include <SDL.h>
#include "Segment.h"

Segment::Segment(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Segment* parent, SDL_Color color)
    : p_renderer(renderer), p_eventManager(eventManager), rect({ x, y, w, h }), parent(parent), p_color(color) { }

Segment::~Segment() {
    // No need to delete renderer since it is managed elsewhere
}

void Segment::render() {
    SDL_SetRenderDrawColor(p_renderer, p_color.r, p_color.g, p_color.b, p_color.a);
    SDL_RenderFillRect(p_renderer, &rect);
}

void Segment::update(int x, int y, int w, int h) {
    rect = { x, y, w, h };
}

void Segment::handleEvent(SDL_Event& event) { }

Segment* Segment::findTypeImpl(const std::type_info& type) {
    // Base implementation: return this if the type matches
    if (type == typeid(Segment)) {
        return this;
    }
    return nullptr;
}