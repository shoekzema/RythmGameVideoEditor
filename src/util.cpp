#include <filesystem>
#include <iostream>
#include "util.h"

std::wstring to_wstring(const char* str) {
    if (!str) throw std::invalid_argument("Input string is null");

    std::mbstate_t state = std::mbstate_t();
    const char* src = str;
    std::size_t len = 0;

    // Get the required length (including the null terminator)
    errno_t err = mbsrtowcs_s(&len, nullptr, 0, &src, 0, &state);
    if (err != 0) throw std::runtime_error("Failed to calculate the length of the wide string.");

    std::wstring wstr(len, L'\0'); // Reserve space for wide string

    // Convert to wide string
    src = str; // Reset src pointer for the conversion
    err = mbsrtowcs_s(nullptr, &wstr[0], len, &src, len, &state);
    if (err != 0) throw std::runtime_error("Failed to convert to a wide string.");

    // Remove the trailing null character (mbsrtowcs_s adds it)
    wstr.resize(len - 1);

    return wstr;
}

static TTF_Font* s_font;
static TTF_Font* s_fontBig;
static TTF_Font* s_fontSmall;

void setFont(TTF_Font* font) { s_font = font; }
void setFontBig(TTF_Font* font) { s_fontBig = font; }
void setFontSmall(TTF_Font* font) { s_fontSmall = font; }

TTF_Font* getFont() { return s_font; }
TTF_Font* getFontBig() { return s_fontBig; }
TTF_Font* getFontSmall() { return s_fontSmall; }

SDL_Rect renderText(SDL_Renderer* renderer, int xPos, int yPos, TTF_Font* font, const char* text, SDL_Color color) {
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, text, color);
    if (!textSurface) {
        printf("Unable to render text surface! SDL_ttf Error: %s\n", TTF_GetError());
        return { xPos, yPos, 0, 0 };
    }
    // Convert surface to texture
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface); // Free the surface now that we have a texture

    if (!textTexture) printf("Unable to create texture from rendered text! SDL Error: %s\n", SDL_GetError());

    // Define the destination rectangle for the text
    SDL_Rect textRect;
    textRect.x = xPos;
    textRect.y = yPos;
    SDL_QueryTexture(textTexture, nullptr, nullptr, &textRect.w, &textRect.h); // Get width and height from the texture

    SDL_RenderCopy(renderer, textTexture, nullptr, &textRect); // Render text

    SDL_DestroyTexture(textTexture);

    return textRect;
}

SDL_Rect renderTextWithCustomSpacing(SDL_Renderer* renderer, int xPos, int yPos, TTF_Font* font, std::string text, int customSpacing, SDL_Color color) {
    if (text.empty()) return { xPos, yPos, 0, 0 };

    int offsetX = 0;

    // Render each character (number) individually with custom spacing
    int charWidth = 0;
    int charHeight = 0;
    for (char c : text) {
        std::string character(1, c);  // Convert char to string
        SDL_Surface* surface = TTF_RenderText_Solid(font, character.c_str(), color);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

        TTF_SizeText(font, character.c_str(), &charWidth, &charHeight);

        SDL_Rect dstRect = { xPos + offsetX, yPos, charWidth, charHeight };
        SDL_RenderCopy(renderer, texture, nullptr, &dstRect);

        offsetX += charWidth + customSpacing; // Move to the right, applying custom spacing

        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }
    return { xPos, yPos, offsetX, charHeight };
}

std::string formatTime(double timeInSeconds, int fps) {
    int hours = static_cast<int>(timeInSeconds) / 3600;
    int minutes = (static_cast<int>(timeInSeconds) % 3600) / 60;
    int seconds = static_cast<int>(timeInSeconds) % 60;

    // Calculate frames based on the fractional part and frame rate
    int frames = static_cast<int>((timeInSeconds - static_cast<int>(timeInSeconds)) * fps);

    // Handle case where rounding pushes frames to fps
    if (frames >= fps) {
        frames = 0;
        seconds++;
        if (seconds == 60) {
            seconds = 0;
            minutes++;
            if (minutes == 60) {
                minutes = 0;
                hours++;
            }
        }
    }

    // Format the output with zero-padded values
    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << hours << ":"
        << std::setw(2) << std::setfill('0') << minutes << ":"
        << std::setw(2) << std::setfill('0') << seconds << ":"
        << std::setw(2) << std::setfill('0') << frames;

    return oss.str();
}

std::string formatTime(Uint32 timeInFrames, int fps) {
    int frames = timeInFrames % fps;
    int seconds = (timeInFrames - frames) / fps % 60;
    int minutes = (timeInFrames - seconds - frames) / fps / 60 % 60;
    int hours = (timeInFrames - minutes - seconds - frames) / fps / 3600;

    // Format the output with zero-padded values
    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << hours << ":"
        << std::setw(2) << std::setfill('0') << minutes << ":"
        << std::setw(2) << std::setfill('0') << seconds << ":"
        << std::setw(2) << std::setfill('0') << frames;

    return oss.str();
}
