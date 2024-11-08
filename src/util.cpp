#include <filesystem>
#include "util.h"

std::wstring to_wstring(const char* str) {
    std::mbstate_t state = std::mbstate_t();
    std::size_t len = 1 + std::mbsrtowcs(nullptr, &str, 0, &state); // Get length required
    std::wstring wstr(len, L'\0');  // Reserve space for wide string

    std::mbsrtowcs(&wstr[0], &str, len, &state); // Convert to wide string
    return wstr;
}

std::string getFullPath(const std::string& internalPath) {
    char* basePath = SDL_GetBasePath();
    if (!basePath) {
        std::cerr << "Failed to get base path: " << SDL_GetError() << std::endl;
        return internalPath;
    }
    std::filesystem::path currentPath(basePath);
    SDL_free(basePath);

    // Traverse up the directory tree to find the "RythmGameVideoEditor" folder
    while (!currentPath.empty()) {
        if (currentPath.filename() == "RythmGameVideoEditor") {
            // We've found the project root
            return currentPath.string() + "/" + internalPath;
        }
        currentPath = currentPath.parent_path();  // Go up one level
    }

    std::cerr << "Could not find the project root (RythmGameVideoEditor) folder." << std::endl;
    return "";
}

std::string getFullPath(const char* internalPath) {
    return getFullPath(std::string(internalPath));
}

static TTF_Font* s_font;

void setFont(TTF_Font* font) { s_font = font; }
TTF_Font* getFont() { return s_font; }