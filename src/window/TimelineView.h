#pragma once

#include <SDL.h>

// Simple struct to hold view/UI state for the timeline window
struct TimelineView {
    Uint32 zoom = 512; // The amount to zoom out
    Uint32 scrollOffset = 0; // The leftmost frame on screen
    int scrollSpeed = 10;
    int timeLabelInterval = 70;
    int topBarheight = 30;
    Uint8 indicatorFrameDisplayThreshold = 8;
    int trackDataWidth = 100;
    int trackStartXPos = trackDataWidth + 2;
    int trackHeight = 64;
    int rowHeight = trackHeight + 2;

    // Colors
    SDL_Color videoTrackBGColor      = { 35,  38,  41, 255 }; // Desaturated Blueish
    SDL_Color videoTrackDataColor    = { 33,  36,  39, 255 }; // Darker Desaturated Blueish
    SDL_Color videoTrackSegmentColor = { 19, 102, 162, 255 }; // Blue

    SDL_Color audioTrackBGColor      = { 40, 37, 45, 255 }; // Desaturated Purplish
    SDL_Color audioTrackDataColor    = { 38, 35, 43, 255 }; // Darker Desaturated Purplish
    SDL_Color audioTrackSegmentColor = { 13, 58, 32, 255 }; // Dark Green

    SDL_Color segmentOutlineColor   = {  61, 174, 233, 255 }; // Light Blue
    SDL_Color segmentHighlightColor = { 246, 116,   0, 255 }; // Light Orange
    SDL_Color timeIndicatorColor    = { 255, 255, 255, 255 }; // White
    SDL_Color betweenLineColor      = {  30,  33,  36, 255 }; // Dark Gray
    SDL_Color timeLabelColor        = { 180, 180, 180, 255 }; // Light Gray
};
