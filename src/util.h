#pragma once
#include <iostream>
#include <SDL.h>
#include <SDL_ttf.h>

static int appWindowSizeX = 1280;
static int appWindowSizeY = 800;

// Function to convert a char* (UTF-8) to std::wstring (UTF-16)
std::wstring to_wstring(const char* str);

// Function to add the path to the root of the project on top of the path inside the project
std::string getFullPath(const std::string& internalPath);
std::string getFullPath(const char* internalPath);

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
 */
void renderText(SDL_Renderer* renderer, int xPos, int yPos, TTF_Font* font, const char* text, SDL_Color color = { 255, 255, 255, 255 });