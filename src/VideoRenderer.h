#pragma once

#include <SDL.h>
#include <vector>
#include "Frame.h"

/**
 * @class VideoRenderer
 *
 * @brief Responsible for initializing SDL, creating the window, and rendering each frame.
 */
class VideoRenderer {
public:
    VideoRenderer(int width, int height, int segmentCount, int borderThickness);
    ~VideoRenderer();

    bool init();
    void renderFrame(const Frame& frame);
    void handleEvents(bool& quit);

private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    int width;
    int height;
    std::vector<SDL_Rect> segments;
    int segmentCount;
    int borderThickness;
};
