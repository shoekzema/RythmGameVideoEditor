#pragma once
#include <iostream>
#include <SDL_ttf.h>

static int appWindowSizeX = 1280;
static int appWindowSizeY = 800;

// Function to convert a char* (UTF-8) to std::wstring (UTF-16)
std::wstring to_wstring(const char* str);

// Function to add the path to the root of the project on top of the path inside the project
std::string getFullPath(const std::string& internalPath);
std::string getFullPath(const char* internalPath);

void setFont(TTF_Font* font); // Function to set the font
TTF_Font* getFont(); // Function to get the font