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
private:
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    EventManager m_eventManager;
    bool m_running = false;
    int m_screenWidth, m_screenHeight;
    Segment* m_rootSegment; // Root of the window segment hierarchy
    bool m_isDragging = false;
    AssetData* m_draggedAsset = nullptr; // Shared variables for dragging between segments
};