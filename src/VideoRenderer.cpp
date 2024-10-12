#include "VideoRenderer.h"
#include <iostream>

/**
 * @brief Constructs a VideoRenderer object, initializing the window size.
 *
 * @param width The initial width of the window.
 * @param height The initial height of the window.
 */
VideoRenderer::VideoRenderer(int width, int height)
    : window(nullptr), renderer(nullptr), texture(nullptr), width(width), height(height) {}

/**
 * @brief Destructor for the VideoRenderer class.
 *
 * Cleans up and frees all SDL resources
 */
VideoRenderer::~VideoRenderer() {
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit(); // Shut down SDL subsystems.
}

/**
 * @brief Initializes the SDL window, renderer, and texture for video rendering.
 *
 * This function initializes the SDL video subsystem, creates a window, a renderer, and
 * a texture to store video frames. If any step fails, it returns false.
 *
 * @return true if initialization succeeds, false if there is an error.
 */
bool VideoRenderer::init() {
    // Initialize the SDL video subsystem
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Create an SDL window
    window = SDL_CreateWindow("FFmpeg Video Player",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN);

    if (!window) {
        std::cerr << "Window could not be created! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Create an SDL renderer for the window
    renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer) {
        std::cerr << "Renderer could not be created! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Create an SDL texture for rendering video frames (RGB format, streaming access)
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!texture) {
        std::cerr << "Texture could not be created! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }

    return true;
}

/**
 * @brief Renders a video frame using the SDL renderer and texture.
 *
 * This function updates the SDL texture with the video frame's pixel data, clears the
 * renderer, copies the texture to the renderer, and presents the final image on the window.
 *
 * @param frame The video frame to be rendered.
 */
void VideoRenderer::renderFrame(const Frame& frame) {
    // Update the texture with the RGB frame data
    SDL_UpdateTexture(texture, NULL, frame.data.data(), frame.linesize);

    // Clear the renderer to prepare for new frame rendering
    SDL_RenderClear(renderer);

    // Copy the texture to the renderer
    SDL_RenderCopy(renderer, texture, NULL, NULL);

    // Present the updated renderer (show the frame)
    SDL_RenderPresent(renderer);
}

/**
 * @brief Handles SDL events, such as quitting the application.
 *
 * This function polls for SDL events (e.g., window close event) and sets the quit flag
 * to true if the user requests to quit the application (e.g., by closing the window).
 *
 * @param quit A boolean flag that is set to true when an SDL_QUIT event is received.
 */
void VideoRenderer::handleEvents(bool& quit) {
    SDL_Event event;

    // Poll for events (e.g., window close)
    while (SDL_PollEvent(&event)) {
        // Check if the event is a quit event
        if (event.type == SDL_QUIT) {
            quit = true; // Set the quit flag to true to terminate the main loop
        }
    }
}
