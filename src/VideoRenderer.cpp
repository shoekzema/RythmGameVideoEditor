#include "VideoRenderer.h"
#include <iostream>

/**
 * @brief Constructs a VideoRenderer object, initializing the window size.
 *
 * @param width The initial width of the window.
 * @param height The initial height of the window.
 * TODO
 */
VideoRenderer::VideoRenderer(int width, int height, int segmentCount, int borderThickness)
    : window(nullptr), renderer(nullptr), texture(nullptr), width(width), height(height), 
    segmentCount(segmentCount), borderThickness(borderThickness)
{
    // Calculate segment dimensions with borders
    int segmentWidth = (width - borderThickness) / 2;
    int segmentHeight = (height - borderThickness) / 2;

    // Create segments (2 horizontal, each with 2 vertical)
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            SDL_Rect segment;
            segment.x = j * (segmentWidth + borderThickness);
            segment.y = i * (segmentHeight + borderThickness);
            segment.w = segmentWidth;
            segment.h = segmentHeight;
            segments.push_back(segment);
        }
    }
}

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

    // Render the frame in each segment
    for (const auto& segment : segments) {
        SDL_RenderCopy(renderer, texture, NULL, &segment);
    }

    // Set the border color (you can customize this)
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White border

    // Draw borders
    for (const auto& segment : segments) {
        // Draw the right border
        SDL_Rect borderRect = { segment.x + segment.w, segment.y, borderThickness, segment.h };
        SDL_RenderFillRect(renderer, &borderRect);

        // Draw the bottom border
        borderRect = { segment.x, segment.y + segment.h, segment.w + borderThickness, borderThickness };
        SDL_RenderFillRect(renderer, &borderRect);
    }

    // Present the updated renderer (show the frames)
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
        else if (event.type == SDL_WINDOWEVENT) {
            if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                width = event.window.data1;
                height = event.window.data2;
                // Update segment sizes considering the border
                int segmentWidth = (width - borderThickness) / 2;
                int segmentHeight = (height - borderThickness) / 2;
                segments.clear(); // Clear existing segments
                for (int i = 0; i < 2; ++i) {
                    for (int j = 0; j < 2; ++j) {
                        SDL_Rect segment;
                        segment.x = j * (segmentWidth + borderThickness);
                        segment.y = i * (segmentHeight + borderThickness);
                        segment.w = segmentWidth;
                        segment.h = segmentHeight;
                        segments.push_back(segment);
                    }
                }
            }
        }
    }
}
