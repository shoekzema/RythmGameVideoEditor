#pragma once
#include <SDL.h>
#include "Segment.h"
#include "EventManager.h"

// Segment in the timeline with a pointer to the corresponding video data and data on what of that video is to be played.
struct VideoSegment {
    VideoData* videoData;      // Reference to the video data
    Uint32 sourceStartTime;    // Start time in the original video file
    Uint32 duration;           // Duration of this segment in its own fps
    Uint32 timelinePosition;   // Position in the overall timeline
    Uint32 timelineDuration;   // Duration of this segment in the timeline's fps
};

// Segment in the timeline with a pointer to the corresponding audio data and data on what of that audio is to be played.
struct AudioSegment {
    AudioData* audioData;      // Reference to the audio data
    Uint32 sourceStartTime;    // Start time in the original audio file
    Uint32 duration;           // Duration of this segment
    Uint32 timelinePosition;   // Position in the overall timeline
    Uint32 timelineDuration;   // Duration of this segment in the timeline's fps
};

/**
 * @class TimeLine
 * @brief Window segment that shows the timeline. Editing is mostly done here.
 */
class Timeline : public Segment {
public:
    Timeline(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Segment* parent = nullptr, SDL_Color color = { 42, 46, 50, 255 });
    ~Timeline();

    void render() override;
    void update(int x, int y, int w, int h) override;
    void handleEvent(SDL_Event& event) override;
    Segment* findTypeImpl(const std::type_info& type) override;

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

    // Add a video segment to the timeline video track
    void addVideoSegment(VideoData* data);

    // Add an audio segment to the timeline audio track
    void addAudioSegment(AudioData* data);
private:
    bool m_playing = false;
    std::vector<std::vector<VideoSegment>> m_videoTracks; // List of all videoTracks with for every videoTrack a list of all VideoSegments on it.
    std::vector<std::vector<AudioSegment>> m_audioTracks; // List of all audioTracks with for every audioTrack a list of all AudioSegments on it.
    Uint32 m_currentTime = 0;     // The current time (and position) of the timeline (in frames)
    Uint32 m_startPlayTime = 0.0; // The time in the timeline where playing starts from (in frames)
    Uint32 m_startTime = 0; // Absolute start time of playback (in milliseconds)
    int m_fps = 60; // Target frames per second to render in.

    Uint32 m_zoom = 512; // The amount to zoom out (minimum = 2, meaning a distance of 2 frame between timeline labels)
    int m_timeLabelInterval = 70;
    int m_topBarheight = 30;
    int m_trackDataWidth = 100;
    int m_trackStartXPos = m_trackDataWidth + 2;
    int m_trackHeight = 64;
    int m_rowHeight = m_trackHeight + 2; // Includes a pixel below and above

    SDL_Color m_videoTrackBGColor      = { 35,  38,  41, 255 };
    SDL_Color m_videoTrackDataColor    = { 33,  36,  39, 255 };
    SDL_Color m_videoTrackSegmentColor = { 19, 102, 162, 255 };

    SDL_Color m_audioTrackBGColor      = { 40, 37, 45, 255 };
    SDL_Color m_audioTrackDataColor    = { 38, 35, 43, 255 };
    SDL_Color m_audioTrackSegmentColor = { 13, 58, 32, 255 };

    SDL_Color m_segmentOutlineColor = {  61, 174, 233, 255 };
    SDL_Color m_timeIndicatorColor  = { 255, 255, 255, 255 };
    SDL_Color m_betweenLineColor    = {  30,  33,  36, 255 };
    SDL_Color m_timeLabelColor      = { 180, 180, 180, 255 };
};