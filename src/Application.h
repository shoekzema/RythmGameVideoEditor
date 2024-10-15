#pragma once
#include <SDL.h>
#include "segment/Segment.h"

class Application {
public:
    Application(int width, int height);
    ~Application();
    void run();

private:
    bool init();
    void handleEvents();
    void render();

    SDL_Window* window;
    SDL_Renderer* renderer;
    EventManager eventManager;
    bool running;
    int screenWidth, screenHeight;

    Segment* rootSegment;
};