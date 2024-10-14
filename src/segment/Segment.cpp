#include "Segment.h"

// Segment constructor
Segment::Segment(int x, int y, int w, int h, SDL_Renderer* renderer, SDL_Color color)
    : renderer(renderer), rect({ x, y, w, h }), color(color) { }

// Segment destructor
Segment::~Segment() {
    // No need to delete renderer since it is managed elsewhere
}

// Render the segment
void Segment::render() {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
}

// Update the position and size of the segment
void Segment::update(int x, int y, int w, int h) {
    rect = { x, y, w, h };
}

// Handle user events
void Segment::handleEvent(SDL_Event& event) { }