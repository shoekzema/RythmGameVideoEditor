#define SDL_MAIN_HANDLED  // This prevents SDL from overriding the main function
#include "Application.h"
#include "util.h"

int main(int argc, char* argv[]) {
    Application app(appWindowSizeX, appWindowSizeY);
    app.run();
    return 0;
}