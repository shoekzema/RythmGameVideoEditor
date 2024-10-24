#include "Segment.h"

Segment::Segment(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, SDL_Color color)
    : renderer(renderer), eventManager(eventManager), rect({ x, y, w, h }), color(color) { }

Segment::~Segment() {
    // No need to delete renderer since it is managed elsewhere
}

void Segment::render() {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
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