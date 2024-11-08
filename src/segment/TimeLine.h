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
    std::vector<VideoSegment> m_videoSegments; // List of all VideoSegments on the video track
    std::vector<AudioSegment> m_audioSegments; // List of all AudioSegments on the audio track
    double m_currentTime = 0.0;   // The current time (and position) of the timeline (in seconds)
    double m_startPlayTime = 0.0; // The time in the timeline where playing starts from (in seconds)
    Uint32 m_startTime = 0; // Absolute start time of playback (in milliseconds)
};