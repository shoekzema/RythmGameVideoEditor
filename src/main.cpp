#define SDL_MAIN_HANDLED  // This prevents SDL from overriding the main function
#include <SDL.h>
#include "VideoPlayer.h"

int main() {
    const char* filename = "FilePath";

    try {
        VideoPlayer player(filename);
        player.play();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';  // Print the error message
        return 1;  // Return a non-zero exit code to indicate failure
    }

    return 0;  // Return 0 to indicate successful execution
}
