#pragma once

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
        if (this->timelinePosition + this->timelineDuration < other->timelinePosition) return false;
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
        if (this->timelinePosition + this->timelineDuration < other->timelinePosition) return false;
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
