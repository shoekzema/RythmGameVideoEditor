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

    int draggingThreshold = 15;
    int mouseHoldStartX = 0;

    int lastLegalTrackPos = 0;
    int selectedMaxTrackPos = 0;
    int selectedMinTrackPos = 0;

    Uint32 lastLegalFrame = 0;
    Uint32 lastLegalLeftmostFrame = 0;

    // Resizing state
    enum ResizeSide { RESIZE_NONE = 0, RESIZE_LEFT = 1, RESIZE_RIGHT = 2 };
    bool isResizing = false;
    ResizeSide resizingSide = RESIZE_NONE;
    VideoSegment* resizingVideoSegment = nullptr;
    AudioSegment* resizingAudioSegment = nullptr;

    // Store original values for revert / reference
    Uint32 resizingSourceStartTime = 0;
    Uint32 resizingOriginalTimelinePosition = 0;
    Uint32 resizingOriginalTimelineDuration = 0;

    // Prepare-to-resize (threshold) state
    bool isPreparingResize = false;
    int resizeMouseHoldStartX = 0;

    void clear() {
        selectedVideoSegments.clear();
        selectedAudioSegments.clear();
        isHolding = false;
        isDragging = false;
        isMovingCurrentTime = false;
        isResizing = false;
        isPreparingResize = false;
        resizingSide = RESIZE_NONE;
        resizingVideoSegment = nullptr;
        resizingAudioSegment = nullptr;
    }
};
