#pragma once
#include <SDL.h>
#include <iostream>
#include <unordered_map>
#include <vector>
#include "VideoData.h"

// Segment in the timeline with a pointer to the corresponding video data and data on what of that video is to be played.
struct VideoSegment {
    VideoData* videoData;    // Reference to the video data
    Uint32 sourceStartTime;  // Start time in the original video file
    Uint32 sourceDuration;   // End time in the original video file in the timeline's fps
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
        if (this->timelinePosition + this->timelineDuration < other->timelinePosition) return false;
        if (other->timelinePosition + other->timelineDuration < this->timelinePosition)  return false;
        return true;
    }
};

// Segment in the timeline with a pointer to the corresponding audio data and data on what of that audio is to be played.
struct AudioSegment {
    AudioData* audioData;    // Reference to the audio data
    Uint32 sourceStartTime;  // Start time in the original audio file
    Uint32 sourceDuration;   // End time in the original audio file in the timeline's fps
    Uint32 duration;         // Duration of this segment
    Uint32 timelinePosition; // Position in the overall timeline
    Uint32 timelineDuration; // Duration of this segment in the timeline's fps
    int trackID;             // The audio track this segment is on

    // Checks if two VideoSegments on the same track overlap
    bool overlapsWith(AudioSegment* other) {
        if (this->trackID != other->trackID) return false;
        if (this->timelinePosition + this->timelineDuration < other->timelinePosition) return false;
        if (other->timelinePosition + other->timelineDuration < this->timelinePosition)  return false;
        return true;
    }
};

struct SegmentPointer {
    VideoSegment* videoSegment = nullptr;
    AudioSegment* audioSegment = nullptr;
};

enum TrackType {
    VIDEO = 0,
    AUDIO = 1,
};

struct Track {
    int trackID;
    TrackType trackType;
};

/**
 * @class Timeline
 * @brief The editors timeline. Contains everything that will be played at what time, in what order and for how long.
 */
class Timeline {
public:
    Timeline();
    ~Timeline();

    // Get / Toggle the timeline's playing status
    bool isPlaying(); void togglePlaying();

    // Get / Set the timeline's target frames per second
    int getFPS(); void setFPS(int fps);

    // Get / Set the current time in the timeline (as a frameIndex)
    Uint32 getCurrentTime(); void setCurrentTime(Uint32 time);

    // Get the amount of video / audio tracks
    int getVideoTrackCount(); int getAudioTrackCount();

    // Get the video / audio trackID of the track at position trackPos
    int getVideoTrackID(int trackPos); int getAudioTrackID(int trackPos);

    // Get the video / audio trackPos of a given trackID
    int getVideoTrackPos(int trackID); int getAudioTrackPos(int trackID);

    // Get all video / audio segments
    const std::vector<VideoSegment>* getAllVideoSegments(); const std::vector<AudioSegment>* getAllAudioSegments();

    // Get the video / audio segment that should currently be playing
    VideoSegment* getCurrentVideoSegment(); AudioSegment* getCurrentAudioSegment();

    // Get the video / audio segment from a track at a given position (nullptr if none)
    VideoSegment* getVideoSegment(int trackPos, Uint32 frame); AudioSegment* getAudioSegment(int trackPos, Uint32 frame);

    // Add a video and/or audio segment to the timeline at a track at frame, returns a pointer to the added segments
    SegmentPointer addAssetSegments(SDL_Renderer* renderer, AssetData* data, Uint32 frame, Track track);

    // Move all given video and audio segments by deltaTrackPos (up-down, track position order)
    bool segmentsChangeTrack(std::vector<VideoSegment*>* videoSegments, std::vector<AudioSegment*>* audioSegments, int deltaTrackPos);

    // Move all given video and audio segments by deltaFrames (left-right, timeline position)
    bool segmentsMoveFrames(std::vector<VideoSegment*>* videoSegments, std::vector<AudioSegment*>* audioSegments, int deltaFrames);

    // Delete the given video and audio segments from the timeline
    void deleteSegments(std::vector<VideoSegment*>* videoSegments, std::vector<AudioSegment*>* audioSegments);

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

    // Expose collision checks publicly for editor operations (resizing)
    bool isCollidingWithOtherSegments(VideoSegment* videoSegment);
    bool isCollidingWithOtherSegments(AudioSegment* audioSegment);

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
};
