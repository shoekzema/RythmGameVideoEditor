#pragma once
#include <SDL.h>
#include <iostream>
#include <vector>
#include <unordered_map>
#include "Window.h"
#include "EventManager.h"
#include "VideoData.h"
#include "Timeline.h"

/**
 * @class TimeLineWindow
 * @brief Window segment that shows the timeline. Editing is mostly done here.
 */
class TimeLineWindow : public Window {
public:
    TimeLineWindow(Timeline* timeline, int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Window* parent = nullptr, SDL_Color color = { 42, 46, 50, 255 });
    ~TimeLineWindow();

    void render() override;
    void update(int x, int y, int w, int h) override;
    void handleEvent(SDL_Event& event) override;
    Window* findTypeImpl(const std::type_info& type) override;

    // Add a video and/or audio segment to the timeline at the mouse position
    bool addAssetSegments(AssetData* data, int mouseX, int mouseY);

    Timeline* tempGetTimeline() { return m_timeline; };
private:
    // Get the trackID and type the mouse is in
    Track getTrackID(SDL_Point mousePoint);

    // Get the track order position the mouse is in
    int getTrackPos(int y);

    // Delete all currently selected segments
    void deleteSelectedSegments();
private:
    Timeline* m_timeline;
    bool tempAddedVid = false;

    // Interaction variables
    std::vector<VideoSegment*> m_selectedVideoSegments;
    std::vector<AudioSegment*> m_selectedAudioSegments;
    bool m_isHolding = false;
    bool m_isDragging = false;
    bool m_isMovingCurrentTime = false;
    int m_draggingThreshold = 20; // Pixel threshold for dragging segments
    int m_mouseHoldStartX = 0;
    int m_lastLegalTrackPos = 0; // The last track order position to which moving the selected segments was legal
    int m_selectedMaxTrackPos = 0; // Highest trackPos of the selected segments (for vertical movement)
    int m_selectedMinTrackPos = 0; // Lowest trackPos of the selected segments (for vertical movement)
    Uint32 m_lastLegalFrame = 0; // The last frame selected to which moving the selected segments was legal
    Uint32 m_lastLegalLeftmostFrame = 0; // The last earliest/leftmost frame of all selected segments, to which moving was legal

    // UI magic numbers
    Uint32 m_zoom = 512; // The amount to zoom out (minimum = 2, meaning a distance of 2 frame between timeline labels)
    Uint32 m_scrollOffset = 0; // The leftmost frame on screen
    int m_scrollSpeed = 10;
    int m_timeLabelInterval = 70;
    int m_topBarheight = 30;
    Uint8 m_indicatorFrameDisplayThreshold = 8;
    int m_trackDataWidth = 100;
    int m_trackStartXPos = m_trackDataWidth + 2;
    int m_trackHeight = 64;
    int m_rowHeight = m_trackHeight + 2; // Includes a pixel below and above

    // Colors
    SDL_Color m_videoTrackBGColor      = { 35,  38,  41, 255 }; // Desaturated Blueish
    SDL_Color m_videoTrackDataColor    = { 33,  36,  39, 255 }; // Darker Desaturated Blueish
    SDL_Color m_videoTrackSegmentColor = { 19, 102, 162, 255 }; // Blue

    SDL_Color m_audioTrackBGColor      = { 40, 37, 45, 255 }; // Desaturated Purplish
    SDL_Color m_audioTrackDataColor    = { 38, 35, 43, 255 }; // Darker Desaturated Purplish
    SDL_Color m_audioTrackSegmentColor = { 13, 58, 32, 255 }; // Dark Green

    SDL_Color m_segmentOutlineColor   = {  61, 174, 233, 255 }; // Light Blue
    SDL_Color m_segmentHighlightColor = { 246, 116,   0, 255 }; // Light Orange
    SDL_Color m_timeIndicatorColor    = { 255, 255, 255, 255 }; // White
    SDL_Color m_betweenLineColor      = {  30,  33,  36, 255 }; // Dark Gray
    SDL_Color m_timeLabelColor        = { 180, 180, 180, 255 }; // Light Gray
};