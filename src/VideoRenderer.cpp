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
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);

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
        else if (event.type == SDL_MOUSEBUTTONDOWN) {
            if (event.button.button == SDL_BUTTON_LEFT) {
                SDL_Point mousePoint = { event.button.x, event.button.y };

                std::vector<SDL_Rect> dividers;

                // Create vertical dividers
                dividers.push_back({ segments[0].x + segments[0].w, segments[0].y, borderThickness, segments[0].h }); // Between top-left and top-right
                dividers.push_back({ segments[2].x + segments[2].w, segments[2].y, borderThickness, segments[2].h }); // Between bottom-left and bottom-right

                // Create horizontal dividers
                dividers.push_back({ segments[0].x, segments[0].y + segments[0].h, segments[0].w, borderThickness }); // Between top-left and bottom-left
                dividers.push_back({ segments[1].x, segments[1].y + segments[1].h, segments[1].w, borderThickness }); // Between top-right and bottom-right

                for (size_t i = 0; i < dividers.size(); ++i) {
                    if (SDL_PointInRect(&mousePoint, &dividers[i])) {
                        resizing = true; // Start resizing
                        lastMouseX = event.button.x;
                        lastMouseY = event.button.y;
                        // Determine if it is a vertical or horizontal divider
                        resizingVertical = (i < 2); // Assuming the first two are vertical
                        SDL_SetCursor(SDL_CreateSystemCursor(resizingVertical ? SDL_SYSTEM_CURSOR_SIZEWE : SDL_SYSTEM_CURSOR_SIZENS)); // Change cursor to resize
                        break;
                    }
                }
                break;
            }
        }
        else if (event.type == SDL_MOUSEMOTION) {
            if (resizing) {
                int deltaX = event.motion.x - lastMouseX;
                int deltaY = event.motion.y - lastMouseY;

                if (resizingVertical) {
                    // Update the widths of the left and right segments
                    segments[0].w += deltaX; // Top left
                    segments[1].x += deltaX; // Top right
                    segments[1].w -= deltaX;
                    segments[2].w += deltaX; // Bottom left
                    segments[3].x += deltaX; // Bottom right
                    segments[3].w -= deltaX;
                }
                else {
                    // Update the heights of the top and bottom segments
                    segments[0].h += deltaY; // Top left
                    segments[1].h += deltaY; // Top right
                    segments[2].y += deltaY; // Bottom left
                    segments[2].h -= deltaY;
                    segments[3].y += deltaY; // Bottom right
                    segments[3].h -= deltaY;
                }

                // Update the last mouse position
                lastMouseX = event.motion.x;
                lastMouseY = event.motion.y;
            }
            else {
                SDL_Point mousePoint = { event.motion.x, event.motion.y };
                bool cursorChanged = false;

                std::vector<SDL_Rect> dividers;

                // Create vertical dividers
                dividers.push_back({ segments[0].x + segments[0].w, segments[0].y, borderThickness, segments[0].h }); // Between top-left and top-right
                dividers.push_back({ segments[2].x + segments[2].w, segments[2].y, borderThickness, segments[2].h }); // Between bottom-left and bottom-right

                // Create horizontal dividers
                dividers.push_back({ segments[0].x, segments[0].y + segments[0].h, segments[0].w, borderThickness }); // Between top-left and bottom-left
                dividers.push_back({ segments[1].x, segments[1].y + segments[1].h, segments[1].w, borderThickness }); // Between top-right and bottom-right

                // Check if the mouse is over any dividers
                for (size_t i = 0; i < dividers.size(); ++i) {
                    if (SDL_PointInRect(&mousePoint, &dividers[i])) {
                        std::cout << "check" << std::endl;

                        // Change cursor to resize (either horizontal or vertical)
                        SDL_SetCursor(SDL_CreateSystemCursor((i < 2) ? SDL_SYSTEM_CURSOR_SIZEWE : SDL_SYSTEM_CURSOR_SIZENS));
                        cursorChanged = true;
                        break;
                    }
                }

                // Reset cursor if not hovering over any borders
                if (!cursorChanged) {
                    SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW));
                }
            }
            break;
        }
        else if (event.type == SDL_MOUSEBUTTONUP) {
            if (event.button.button == SDL_BUTTON_LEFT) {
                resizing = false; // Stop resizing
                SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW));
            }
        }
    }
}
