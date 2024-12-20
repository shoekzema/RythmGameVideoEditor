#include "Timeline.h"

Timeline::Timeline() {
    m_videoTrackIDtoPosMap[0] = 0;
    m_videoTrackIDtoPosMap[1] = 1;

    m_audioTrackIDtoPosMap[0] = 0;
    m_audioTrackIDtoPosMap[1] = 1;

    m_videoTrackPosToIDMap[0] = 0;
    m_videoTrackPosToIDMap[1] = 1;

    m_audioTrackPosToIDMap[0] = 0;
    m_audioTrackPosToIDMap[1] = 1;

    m_nextVideoTrackID = 2;
    m_nextAudioTrackID = 2;
}

Timeline::~Timeline()
{
}

bool Timeline::isPlaying() {
    return m_playing;
}

void Timeline::togglePlaying() {
    m_playing = !m_playing;

    if (m_playing) {
        m_startTime = SDL_GetTicks() - static_cast<Uint32>(m_currentTime * 1000 / m_fps);
    }
    else {
        m_currentTime = m_startPlayTime;
    }
}

int Timeline::getFPS() {
    return m_fps;
}

void Timeline::setFPS(int fps) {
    m_fps = fps;
}

Uint32 Timeline::getCurrentTime() {
    if (m_playing) {
        // If playing, calculate the current time based on how long it's been playing
        Uint32 now = SDL_GetTicks();
        m_currentTime = static_cast<Uint32>((now - m_startTime) / 1000.0 * m_fps);
    }
    return m_currentTime;
}

void Timeline::setCurrentTime(Uint32 time) {
    m_currentTime = time;
    m_startPlayTime = time;
}

int Timeline::getVideoTrackCount() { return static_cast<int>(m_videoTrackIDtoPosMap.size()); }
int Timeline::getAudioTrackCount() { return static_cast<int>(m_audioTrackIDtoPosMap.size()); }

int Timeline::getVideoTrackID(int trackPos) { return m_videoTrackPosToIDMap[trackPos]; }
int Timeline::getAudioTrackID(int trackPos) { return m_audioTrackPosToIDMap[trackPos]; }

int Timeline::getVideoTrackPos(int trackID) { return m_videoTrackIDtoPosMap[trackID]; }
int Timeline::getAudioTrackPos(int trackID) { return m_audioTrackIDtoPosMap[trackID]; }

std::vector<VideoSegment>* Timeline::getAllVideoSegments() { return &m_videoSegments; }
std::vector<AudioSegment>* Timeline::getAllAudioSegments() { return &m_audioSegments; }

// TODO: return only the video segment with the highest trackID at this time
VideoSegment* Timeline::getCurrentVideoSegment() {
    Uint32 currentTime = getCurrentTime();
    VideoSegment* currentVideoSegment = nullptr;

    // Iterate over video segments
    for (VideoSegment& segment : m_videoSegments) {
        // Check if active at the current time
        if (currentTime >= segment.timelinePosition &&
            currentTime < segment.timelinePosition + segment.timelineDuration) {
            // Check if the found segment is higher on the track ordering
            if (!currentVideoSegment || (m_videoTrackPosToIDMap[segment.trackID] > m_videoTrackPosToIDMap[currentVideoSegment->trackID])) {
                currentVideoSegment = &segment; // Set this segment to be returned
            }
        }
    }
    return currentVideoSegment;
}

// TODO: merge audio if multiple tracks have a audioSegment to play at this time
AudioSegment* Timeline::getCurrentAudioSegment() {
    Uint32 currentTime = getCurrentTime();

    for (AudioSegment& segment : m_audioSegments) {
        if (currentTime >= segment.timelinePosition &&
            currentTime < segment.timelinePosition + segment.timelineDuration) {
            return &segment;  // Return the active audio segment
        }
    }
    return nullptr;  // No segment found at the current time
}

VideoSegment* Timeline::getVideoSegment(int trackPos, Uint32 frame) {
    if (trackPos >= getVideoTrackCount()) return nullptr;
    int trackID = m_videoTrackPosToIDMap[trackPos];

    // Iterate over video segments to find which one is active at the given frame
    for (VideoSegment& segment : m_videoSegments) {
        if (segment.trackID != trackID) continue;
        if (frame >= segment.timelinePosition && frame <= segment.timelinePosition + segment.timelineDuration) {
            return &segment;
        }
    }
    return nullptr;
}

AudioSegment* Timeline::getAudioSegment(int trackPos, Uint32 frame) {
    if (trackPos >= getAudioTrackCount()) return nullptr;
    int trackID = m_audioTrackPosToIDMap[trackPos];

    // Iterate over audio segments to find which one is active at the given frame
    for (AudioSegment& segment : m_audioSegments) {
        if (segment.trackID != trackID) continue;
        if (frame >= segment.timelinePosition && frame <= segment.timelinePosition + segment.timelineDuration) {
            return &segment;
        }
    }
    return nullptr;
}

SegmentPointer Timeline::addAssetSegments(SDL_Renderer* renderer, AssetData* data, Uint32 frame, Track track) {
    int videoTrackID = track.trackID;
    int audioTrackID = track.trackID;
    SegmentPointer segmentPointer;

    if (videoTrackID < 0) {
        return segmentPointer; // Not in a legitimate track (above first or below last track)
    }
    if (!data->videoData && track.trackType == VIDEO) return segmentPointer; // Cannot drop audio only files in video tracks
    if (!data->audioData && track.trackType == AUDIO) return segmentPointer; // Cannot drop video only files in audio tracks
    if (data->videoData && data->audioData) {
        int trackPos = 0;
        if (track.trackType == VIDEO) {
            trackPos = m_videoTrackIDtoPosMap[videoTrackID];
            audioTrackID = m_audioTrackPosToIDMap[trackPos];
        }
        else if (track.trackType == AUDIO) {
            trackPos = m_audioTrackIDtoPosMap[audioTrackID];
            videoTrackID = m_videoTrackPosToIDMap[trackPos];
        }

        if (trackPos >= std::min(m_videoTrackIDtoPosMap.size(), m_audioTrackIDtoPosMap.size())) return segmentPointer; // Cannot drop AV files in if the target trackPos doesn't exist for both video and audio
    }

    VideoSegment videoSegment;
    AudioSegment audioSegment;

    // In case the asset has video
    if (data->videoData) {
        // Create and add a new videoSegment
        videoSegment = {
            .videoData = data->videoData,
            .sourceStartTime = 0,
            .duration = data->videoData->getVideoDurationInFrames(),
            .timelinePosition = frame,
            .timelineDuration = data->videoData->getVideoDurationInFrames(m_fps),
            .fps = data->videoData->getFPS(),
            .trackID = videoTrackID
        };
        videoSegment.firstFrame = videoSegment.videoData->getFrameTexture(renderer, 0);
        videoSegment.lastFrame = videoSegment.videoData->getFrameTexture(renderer, videoSegment.duration - 1);

        // Cannot drop here, because it would overlap with another segment
        if (isCollidingWithOtherSegments(&videoSegment)) return segmentPointer;
    }
    // In case the asset has audio
    if (data->audioData) {
        // Create and add a new audioSegment
        audioSegment = {
            .audioData = data->audioData,
            .sourceStartTime = 0,
            .duration = data->audioData->getAudioDurationInFrames(),
            .timelinePosition = frame,
            .timelineDuration = data->audioData->getAudioDurationInFrames(m_fps),
            .trackID = audioTrackID
        };

        // Cannot drop here, because it would overlap with another segment
        if (isCollidingWithOtherSegments(&audioSegment)) return segmentPointer;
    }

    // If we reached here, then the new video segment and/or audio segment can be added
    // Also add pointers to the new segments to the return value
    if (data->videoData) {
        m_videoSegments.push_back(videoSegment);
        segmentPointer.videoSegment = &m_videoSegments.back();
    }
    if (data->audioData) {
        m_audioSegments.push_back(audioSegment);
        segmentPointer.audioSegment = &m_audioSegments.back();
    }

    return segmentPointer;
}

bool Timeline::segmentsChangeTrack(std::vector<VideoSegment*>* videoSegments, std::vector<AudioSegment*>* audioSegments, int deltaTrackPos) {
    // Move all segments by deltaTrackPos (no collision checks yet)
    for (int i = 0; i < videoSegments->size(); i++) {
        (*videoSegments)[i]->trackID = m_videoTrackPosToIDMap[m_videoTrackIDtoPosMap[(*videoSegments)[i]->trackID] + deltaTrackPos];
    }
    for (int i = 0; i < audioSegments->size(); i++) {
        (*audioSegments)[i]->trackID = m_audioTrackPosToIDMap[m_audioTrackIDtoPosMap[(*audioSegments)[i]->trackID] + deltaTrackPos];
    }

    // For every moved segment, check for collisions
    bool illegalMove = false;
    for (int i = 0; i < videoSegments->size(); i++) {
        // If it collides, undo every move
        if (isCollidingWithOtherSegments((*videoSegments)[i])) {
            illegalMove = true;
            break; // Exit loop
        }
    }
    if (!illegalMove) {
        for (int i = 0; i < audioSegments->size(); i++) {
            // If it collides, undo every move
            if (isCollidingWithOtherSegments((*audioSegments)[i])) {
                illegalMove = true;
                break; // Exit loop
            }
        }
    }

    // If any of the moved segments is colliding with something, undo it all
    if (illegalMove) {
        // Move all selected segments back
        for (int i = 0; i < videoSegments->size(); i++) {
            (*videoSegments)[i]->trackID = m_videoTrackPosToIDMap[m_videoTrackIDtoPosMap[(*videoSegments)[i]->trackID] - deltaTrackPos];
        }
        for (int i = 0; i < audioSegments->size(); i++) {
            (*audioSegments)[i]->trackID = m_audioTrackPosToIDMap[m_audioTrackIDtoPosMap[(*audioSegments)[i]->trackID] - deltaTrackPos];
        }
        return false;
    }
    return true;
}

bool Timeline::segmentsMoveFrames(std::vector<VideoSegment*>* videoSegments, std::vector<AudioSegment*>* audioSegments, int deltaFrames) {
    // Move all selected segments by deltaFrames (no collision checks yet)
    for (int i = 0; i < videoSegments->size(); i++) {
        (*videoSegments)[i]->timelinePosition += deltaFrames;
    }
    for (int i = 0; i < audioSegments->size(); i++) {
        (*audioSegments)[i]->timelinePosition += deltaFrames;
    }

    // For every moved segment, check for collisions
    bool illegalMove = false;
    for (int i = 0; i < videoSegments->size(); i++) {
        // If it collides, undo every move
        if (isCollidingWithOtherSegments((*videoSegments)[i])) {
            illegalMove = true;
            break; // Exit loop
        }
    }
    if (!illegalMove) {
        for (int i = 0; i < audioSegments->size(); i++) {
            // If it collides, undo every move
            if (isCollidingWithOtherSegments((*audioSegments)[i])) {
                illegalMove = true;
                break; // Exit loop
            }
        }
    }

    // If any of the moved segments is colliding with something, undo it all
    if (illegalMove) {
        // Move all selected segments back
        for (int i = 0; i < videoSegments->size(); i++) {
            (*videoSegments)[i]->timelinePosition -= deltaFrames;
        }
        for (int i = 0; i < audioSegments->size(); i++) {
            (*audioSegments)[i]->timelinePosition -= deltaFrames;
        }
        return false;
    }
    return true;
}

void Timeline::deleteSegments(std::vector<VideoSegment*>* videoSegments, std::vector<AudioSegment*>* audioSegments) {
    if (!videoSegments->empty()) {
        // Predicate to check if a VideoSegment exists in m_selectedVideoSegments
        auto isSelected = [videoSegments](const VideoSegment& segment) {
            return std::find((*videoSegments).begin(), (*videoSegments).end(), &segment) != (*videoSegments).end();
            };

        // Remove all selected segments from m_videoSegments
        m_videoSegments.erase(
            std::remove_if(m_videoSegments.begin(), m_videoSegments.end(), isSelected),
            m_videoSegments.end()
        );
    }
    if (!audioSegments->empty()) {
        // Predicate to check if an AudioSegment exists in m_selectedAudioSegments
        auto isSelected = [audioSegments](const AudioSegment& segment) {
            return std::find((*audioSegments).begin(), (*audioSegments).end(), &segment) != (*audioSegments).end();
            };

        // Remove all selected segments from m_videoSegments
        m_audioSegments.erase(
            std::remove_if(m_audioSegments.begin(), m_audioSegments.end(), isSelected),
            m_audioSegments.end()
        );
    }
}

void Timeline::addTrack(Track track, int videoOrAudio, bool above) {
    if (videoOrAudio == 0 || videoOrAudio == 2) {
        int newVideoTrackID = m_nextVideoTrackID++;

        // Find the position to insert the new track
        auto it = m_videoTrackIDtoPosMap.find(track.trackID);
        if (it != m_videoTrackIDtoPosMap.end()) {
            int pos = it->second; // Get position of the existing track
            if (above) pos++;

            // Insert the new track relative to existing one, shift positions of others
            for (auto& entry : m_videoTrackIDtoPosMap) {
                if (entry.second >= pos) {
                    m_videoTrackPosToIDMap.erase(entry.second); // Remove old position entry in reverse map
                    entry.second++; // Shift the position of tracks after the insertion point
                    m_videoTrackPosToIDMap[entry.second] = entry.first; // Update reverse map with the new position
                }
            }
            m_videoTrackIDtoPosMap[newVideoTrackID] = pos;
            m_videoTrackPosToIDMap[pos] = newVideoTrackID;
        }
        else {
            // Insert at the end: new track gets the next available position
            m_videoTrackIDtoPosMap[newVideoTrackID] = static_cast<int>(m_videoTrackIDtoPosMap.size());
            m_videoTrackPosToIDMap[static_cast<int>(m_videoTrackIDtoPosMap.size())] = newVideoTrackID;
        }
    }
    if (videoOrAudio == 1 || videoOrAudio == 2) {
        int newAudioTrackID = m_nextAudioTrackID++;

        // Find the position to insert the new track
        auto it = m_audioTrackIDtoPosMap.find(track.trackID);
        if (it != m_audioTrackIDtoPosMap.end()) {
            int pos = it->second; // Get position of the existing track
            if (!above) pos++;

            // Insert the new track relative to existing one, shift positions of others
            for (auto& entry : m_audioTrackIDtoPosMap) {
                if (entry.second >= pos) {
                    m_audioTrackPosToIDMap.erase(entry.second); // Remove old position entry in reverse map
                    entry.second++; // Shift the position of tracks after the insertion point
                    m_audioTrackPosToIDMap[entry.second] = entry.first; // Update reverse map with the new position
                }
            }
            m_audioTrackIDtoPosMap[newAudioTrackID] = pos;
            m_audioTrackPosToIDMap[pos] = newAudioTrackID;
        }
        else {
            // Insert at the end: new track gets the next available position
            m_audioTrackIDtoPosMap[newAudioTrackID] = static_cast<int>(m_audioTrackIDtoPosMap.size());
            m_audioTrackPosToIDMap[static_cast<int>(m_audioTrackIDtoPosMap.size())] = newAudioTrackID;
        }
    }
}

void Timeline::deleteTrack(Track track) {
    if (track.trackType == VIDEO) {
        if (getVideoTrackCount() <= 1) return; // Cannot remove the track if it is the only video track

        // Remove video segments 
        m_videoSegments.erase(
            std::remove_if(m_videoSegments.begin(), m_videoSegments.end(),
                [track](const VideoSegment& item) {
                    return item.trackID == track.trackID;
                }),
            m_videoSegments.end()
        );

        // Remove the track from the maps
        int trackPos = m_videoTrackIDtoPosMap[track.trackID];
        if (!m_videoTrackIDtoPosMap.erase(track.trackID)) {
            std::cout << "Track ID " << track.trackID << " not found.\n";
        }
        if (!m_videoTrackPosToIDMap.erase(trackPos)) {
            std::cout << "Track Pos " << trackPos << " not found.\n";
        }

        // Shift track positions down
        std::unordered_map<int, int> updatedTrackPosToIDmap;
        for (const auto& [pos, id] : m_videoTrackPosToIDMap) {
            if (pos > trackPos) {
                updatedTrackPosToIDmap[pos - 1] = id;
                m_videoTrackIDtoPosMap[id] = pos - 1;
            }
            else {
                updatedTrackPosToIDmap[pos] = id;
            }
        }
        m_videoTrackPosToIDMap = std::move(updatedTrackPosToIDmap);
    }
    else if (track.trackType == AUDIO) {
        if (getAudioTrackCount() <= 1) return; // Cannot remove the track if it is the only audio track

        // Remove audio segments 
        m_audioSegments.erase(
            std::remove_if(m_audioSegments.begin(), m_audioSegments.end(),
                [track](const AudioSegment& item) {
                    return item.trackID == track.trackID;
                }),
            m_audioSegments.end()
        );

        // Remove the track from the maps
        int trackPos = m_audioTrackIDtoPosMap[track.trackID];
        if (!m_audioTrackIDtoPosMap.erase(track.trackID)) {
            std::cout << "Track ID " << track.trackID << " not found.\n";
        }
        if (!m_audioTrackPosToIDMap.erase(trackPos)) {
            std::cout << "Track Pos " << trackPos << " not found.\n";
        }

        // Shift track positions down
        std::unordered_map<int, int> updatedTrackPosToIDmap;
        for (const auto& [pos, id] : m_audioTrackPosToIDMap) {
            if (pos > trackPos) {
                updatedTrackPosToIDmap[pos - 1] = id;
                m_audioTrackIDtoPosMap[id] = pos - 1;
            }
            else {
                updatedTrackPosToIDmap[pos] = id;
            }
        }
        m_audioTrackPosToIDMap = std::move(updatedTrackPosToIDmap);
    }
}

bool Timeline::isCollidingWithOtherSegments(VideoSegment* videoSegment) {
    // Iterate over video segments to find which one overlaps with the input segment
    for (VideoSegment& segment : m_videoSegments) {
        if (&segment == videoSegment) continue; // Skip self-collision check
        if (videoSegment->overlapsWith(&segment)) return true;
    }
    return false;
}

bool Timeline::isCollidingWithOtherSegments(AudioSegment* audioSegment) {
    // Iterate over video segments to find which one overlaps with the input segment
    for (AudioSegment& segment : m_audioSegments) {
        if (&segment == audioSegment) continue; // Skip self-collision check
        if (audioSegment->overlapsWith(&segment)) return true;
    }
    return false;
}
