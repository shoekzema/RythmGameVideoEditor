#pragma once
#include <SDL.h>
#include "segment/Segment.h"

class Application {
public:
    Application(int width, int height);
    ~Application();

    // Main Application loop
    void run();

private:
    // Initialize SDL and create window/renderer
    bool init();

    // Handle user interaction events
    void handleEvents();

    // Render the window and all active segments
    void render();

    SDL_Window* window;
    SDL_Renderer* renderer;
    EventManager eventManager;
    bool running;
    int screenWidth, screenHeight;

    Segment* rootSegment;

    // Shared variables for dragging between segments
    bool isDragging = false;
    AssetData* draggedAsset = nullptr;
};