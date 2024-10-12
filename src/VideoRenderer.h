#pragma once

#include <SDL.h>
#include "Frame.h"

/**
 * @class VideoRenderer
 *
 * @brief Responsible for initializing SDL, creating the window, and rendering each frame.
 */
class VideoRenderer {
public:
    VideoRenderer(int width, int height);
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
};
