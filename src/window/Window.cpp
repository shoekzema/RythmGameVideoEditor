#include <SDL.h>
#include "Window.h"

Window::Window(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Window* parent, SDL_Color color)
    : p_renderer(renderer), p_eventManager(eventManager), rect({ x, y, w, h }), parent(parent), p_color(color) { }

Window::~Window() {
    // No need to delete renderer since it is managed elsewhere
}

void Window::render() {
    SDL_SetRenderDrawColor(p_renderer, p_color.r, p_color.g, p_color.b, p_color.a);
    SDL_RenderFillRect(p_renderer, &rect);
}

void Window::update(int x, int y, int w, int h) {
    rect = { x, y, w, h };
}

void Window::handleEvent(SDL_Event& event) { }

Window* Window::findTypeImpl(const std::type_info& type) {
    // Base implementation: return this if the type matches
    if (type == typeid(Window)) {
        return this;
    }
    return nullptr;
}