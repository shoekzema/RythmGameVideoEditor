#pragma once

#include <vector>
#include <SDL.h>
#include "Timeline.h"

// Simple manager for selection and drag state extracted from TimeLineWindow
struct TimelineSelectionManager {
    std::vector<VideoSegment*> selectedVideoSegments;
    std::vector<AudioSegment*> selectedAudioSegments;

    bool isHolding = false;
    bool isDragging = false;
    bool isMovingCurrentTime = false;

    int draggingThreshold = 20;
    int mouseHoldStartX = 0;

    int lastLegalTrackPos = 0;
    int selectedMaxTrackPos = 0;
    int selectedMinTrackPos = 0;

    Uint32 lastLegalFrame = 0;
    Uint32 lastLegalLeftmostFrame = 0;

    void clear() {
        selectedVideoSegments.clear();
        selectedAudioSegments.clear();
        isHolding = false;
        isDragging = false;
        isMovingCurrentTime = false;
    }
};
