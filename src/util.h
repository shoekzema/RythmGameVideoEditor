#pragma once
#include <iostream>
#include <SDL.h>
#include <SDL_ttf.h>

static int appWindowSizeX = 1280;
static int appWindowSizeY = 800;

// Function to convert a char* (UTF-8) to std::wstring (UTF-16)
std::wstring to_wstring(const char* str);

void setFont(TTF_Font* font); // Function to set the font
void setFontBig(TTF_Font* font);
void setFontSmall(TTF_Font* font);
TTF_Font* getFont(); // Function to get the font
TTF_Font* getFontBig();
TTF_Font* getFontSmall();

/**
 * @brief Renders some text to the screen at a position.
 * @param renderer The SDL_Renderer to use.
 * @param xPos The text's top-left x-position on screen.
 * @param yPos The text's top-left y-position on screen.
 * @param font The font (includes size) of the text.
 * @param text The text to render.
 * @param color The text color.
 * @returns The boundary rectangle of the resulting text.
 */
SDL_Rect renderText(SDL_Renderer* renderer, int xPos, int yPos, TTF_Font* font, const char* text, SDL_Color color = { 255, 255, 255, 255 });

/**
 * @brief Renders some text to the screen at a position with custom spacing between characters.
 * @param renderer The SDL_Renderer to use.
 * @param xPos The text's top-left x-position on screen.
 * @param yPos The text's top-left y-position on screen.
 * @param font The font (includes size) of the text.
 * @param text The text to render.
 * @param customSpacing The amount of pixels to add or subtract from the normal space in between characters.
 * @param color The text color.
 * @returns The boundary rectangle of the resulting text.
 */
SDL_Rect renderTextWithCustomSpacing(SDL_Renderer* renderer, int xPos, int yPos, TTF_Font* font, std::string text, int customSpacing, SDL_Color color = { 255, 255, 255, 255 });

/**
 * @brief Format a double with a time into hh:mm:ss:ff format. (Hours, Minutes, seconds, frames)
 * @param timeInSeconds The time in seconds to format.
 * @param fps The amount of frames per second.
 * @returns The formatted time as a String.
 */
std::string formatTime(double timeInSeconds, int fps);

/**
 * @brief Format an Uint32 with a time into hh:mm:ss:ff format. (Hours, Minutes, seconds, frames)
 * @param timeInFrames The time in a number of frames to format.
 * @param fps The amount of frames per second.
 * @returns The formatted time as a String.
 */
std::string formatTime(Uint32 timeInFrames, int fps);