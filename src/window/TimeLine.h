#pragma once
#include <SDL.h>
#include <iostream>
#include <vector>
#include <unordered_map>
#include "Window.h"
#include "EventManager.h"
#include "VideoData.h"

// Segment in the timeline with a pointer to the corresponding video data and data on what of that video is to be played.
struct VideoSegment {
    VideoData* videoData;    // Reference to the video data
    Uint32 sourceStartTime;  // Start time in the original video file
    Uint32 duration;         // Duration of this segment in its own fps
    Uint32 timelinePosition; // Position in the overall timeline
    Uint32 timelineDuration; // Duration of this segment in the timeline's fps
    AVRational fps;          // Source frames per second as an AVRational (use av_q2d to convert to double)
    int trackID;             // The video track this segment is on
    SDL_Texture* firstFrame; // First video frame
    SDL_Texture* lastFrame;  // First video frame

    // Checks if two VideoSegments on the same track overlap
    bool overlapsWith(VideoSegment* other) {
        if (this->trackID != other->trackID) return false;
        if (this->timelinePosition  + this->timelineDuration  < other->timelinePosition) return false;
        if (other->timelinePosition + other->timelineDuration < this->timelinePosition)  return false;
        return true;
    }
};

// Segment in the timeline with a pointer to the corresponding audio data and data on what of that audio is to be played.
struct AudioSegment {
    AudioData* audioData;    // Reference to the audio data
    Uint32 sourceStartTime;  // Start time in the original audio file
    Uint32 duration;         // Duration of this segment
    Uint32 timelinePosition; // Position in the overall timeline
    Uint32 timelineDuration; // Duration of this segment in the timeline's fps
    int trackID;             // The audio track this segment is on

    // Checks if two VideoSegments on the same track overlap
    bool overlapsWith(AudioSegment* other) {
        if (this->trackID != other->trackID) return false;
        if (this->timelinePosition  + this->timelineDuration  < other->timelinePosition) return false;
        if (other->timelinePosition + other->timelineDuration < this->timelinePosition)  return false;
        return true;
    }
};

enum TrackType {
    VIDEO = 0,
    AUDIO = 1
};

struct Track {
    int trackID;
    TrackType trackType;
};

/**
 * @class TimeLine
 * @brief Window segment that shows the timeline. Editing is mostly done here.
 */
class Timeline : public Window {
public:
    Timeline(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Window* parent = nullptr, SDL_Color color = { 42, 46, 50, 255 });
    ~Timeline();

    void render() override;
    void update(int x, int y, int w, int h) override;
    void handleEvent(SDL_Event& event) override;
    Window* findTypeImpl(const std::type_info& type) override;

    // Get the video segment that should currently be playing
    VideoSegment* getCurrentVideoSegment();

    // Get the audio segment that should currently be playing
    AudioSegment* getCurrentAudioSegment();

    // Returns if the timeline is playing
    bool isPlaying();

    // Get the timeline's target frames per second
    int getFPS();

    // Set the timeline's target frames per second
    void setFPS(int fps);

    // Get the current time in the timeline (as a frameIndex)
    Uint32 getCurrentTime();

    // Set the current time (in seconds) in the timeline
    void setCurrentTime(Uint32 time);

    // Add a video and/or audio segment to the timeline at the mouse position
    bool addAssetSegments(AssetData* data, int mouseX, int mouseY);
private:
    // Get the video segment at the mouse position (nullptr if none)
    VideoSegment* getVideoSegmentAtPos(int x, int y);

    // Get the audio segment at the mouse position (nullptr if none)
    AudioSegment* getAudioSegmentAtPos(int x, int y);

    // Check if a video segment is colliding with another
    bool isCollidingWithOtherSegments(VideoSegment* videoSegment);

    // Check if an audio segment is colliding with another
    bool isCollidingWithOtherSegments(AudioSegment* audioSegment);

    // Get the trackID and type the mouse is in
    Track getTrackID(SDL_Point mousePoint);

    // Get the track order position and the mouse is in
    int getTrackPos(int y);

    /**
     * @brief Add a new track to the timeline.
     * @param trackID The track from which we relatively add a new track.
     * @param trackType The type of track of trackID.
     * @param above Where to put the new track relative to the selected Track.
     * @param videoOrAudio What type of track to add. 0 for video, 1 for audio, 2 for both (AV).
     */
    void addTrack(Track track, int videoOrAudio, bool above = true);

    // Delete a track from the timeline
    void deleteTrack(Track track);

    // Delete all currently selected segments
    void deleteSelectedSegments();
private:
    bool m_playing = false;
    std::vector<VideoSegment> m_videoSegments; // List of all VideoSegments in the timeline.
    std::vector<AudioSegment> m_audioSegments; // List of all AudioSegments in the timeline.
    std::unordered_map<int, int> m_videoTrackIDtoPosMap; // Maps videoTrackID to its position order
    std::unordered_map<int, int> m_audioTrackIDtoPosMap; // Maps audioTrackID to its position order
    std::unordered_map<int, int> m_videoTrackPosToIDMap; // Maps position order to videoTrackID
    std::unordered_map<int, int> m_audioTrackPosToIDMap; // Maps position order audioTrackID
    int m_nextVideoTrackID; // Keeps track of the next available videoTrackID
    int m_nextAudioTrackID; // Keeps track of the next available audioTrackID
    Uint32 m_currentTime = 0;     // The current time (and position) of the timeline (in frames)
    Uint32 m_startPlayTime = 0; // The time in the timeline where playing starts from (in frames)
    Uint32 m_startTime = 0; // Absolute start time of playback (in milliseconds)
    int m_fps = 60; // Target frames per second to render in.

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