#include "Application.h"

// Application constructor
Application::Application(int width, int height)
    : window(nullptr), renderer(nullptr), running(false),
    screenWidth(width), screenHeight(height) {
    if (init()) {
        rootSegment = new SegmentHSplit(0, 0, screenWidth, screenHeight, renderer, &eventManager);
        running = true;
    }
}

// Initialize SDL and create window/renderer
bool Application::init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return false;
    }
    window = SDL_CreateWindow("RythmGameVideoEditor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenWidth, screenHeight, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        return false;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
        return false;
    }
    return true;
}

// Application destructor
Application::~Application() {
    if (rootSegment) delete rootSegment;
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

// Handle user events
void Application::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            running = false;
        }
        else if (rootSegment) {
            rootSegment->handleEvent(event);
        }
    }
}

// Render the frame
void Application::render() {
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // red (easy to find problems)
    SDL_RenderClear(renderer);

    // Render segments
    if (rootSegment) {
        rootSegment->render();
    }

    SDL_RenderPresent(renderer);
}

// Main Application loop
void Application::run() {
    const int targetFrameTime = 1000 / 60;  // 16.67 ms per frame for 60 FPS
    Uint32 frameStart, frameEnd, frameDuration;

    while (running) {
        // Record the time at the start of the frame
        frameStart = SDL_GetTicks();

        handleEvents(); // Process input events
        render();       // Render everything

        // Record the time at the end of the frame
        frameEnd = SDL_GetTicks();

        // Calculate how long the frame took
        frameDuration = frameEnd - frameStart;

        // If the frame took less time than the target frame duration, delay the difference
        if (frameDuration < targetFrameTime) {
            SDL_Delay(targetFrameTime - frameDuration);
        }
        // If the frame took longer, we don't delay and the game continues running
    }
}