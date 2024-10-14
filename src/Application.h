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
    void update();
    void render();

    SDL_Window* window;
    SDL_Renderer* renderer;
    bool running;
    int screenWidth, screenHeight;

    Segment* rootSegment;
};