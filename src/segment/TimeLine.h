#pragma once
#include <SDL.h>
#include "Segment.h"
#include "EventManager.h"

// Segment in the timeline with a pointer to the corresponding video data and data on what of that video is to be played.
struct VideoSegment {
    VideoData* videoData;      // Reference to the video data
    double sourceStartTime;    // Start time in the original video file
    double duration;           // Duration of this segment
    double timelinePosition;   // Position in the overall timeline
};

// Segment in the timeline with a pointer to the corresponding audio data and data on what of that audio is to be played.
struct AudioSegment {
    AudioData* audioData;      // Reference to the audio data
    double sourceStartTime;    // Start time in the original audio file
    double duration;           // Duration of this segment
    double timelinePosition;   // Position in the overall timeline
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

    // Get the current time in the timeline (in seconds)
    double getCurrentTime();

    // Set the current time (in seconds) in the timeline
    void setCurrentTime(double time);

    // Add a video segment to the timeline video track
    void addVideoSegment(VideoData* data);

    // Add an audio segment to the timeline audio track
    void addAudioSegment(AudioData* data);
private:
    bool m_playing = false;
    std::vector<std::vector<VideoSegment>> m_videoTracks; // List of all videoTracks with for every videoTrack a list of all VideoSegments on it.
    std::vector<std::vector<AudioSegment>> m_audioTracks; // List of all audioTracks with for every audioTrack a list of all AudioSegments on it.
    double m_currentTime = 0.0;   // The current time (and position) of the timeline (in seconds)
    double m_startPlayTime = 0.0; // The time in the timeline where playing starts from (in seconds)
    Uint32 m_startTime = 0; // Absolute start time of playback (in milliseconds)

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
};