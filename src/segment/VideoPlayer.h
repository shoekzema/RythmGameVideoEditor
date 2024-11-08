#pragma once
#include <SDL.h>
#include "Segment.h"
#include "Timeline.h"
#include "EventManager.h"
#include "VideoData.h"

/**
 * @class VideoPlayer
 * @brief Window segment that can render videos.
 */
class VideoPlayer : public Segment {
public:
    VideoPlayer(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Segment* parent = nullptr, SDL_Color color = { 83, 83, 83, 255 });
    ~VideoPlayer();

    void render() override;
    void update(int x, int y, int w, int h) override;
    void handleEvent(SDL_Event& event) override;
    Segment* findTypeImpl(const std::type_info& type) override;
private:
    // Render video and audio based on the segments in the timeline at the current timeline time.
    void playTimeline(Timeline* timeline);

    // Get the current video frame from a videoSegment. The resulting frame is stored inside videoSegment.
    bool getVideoFrame(VideoSegment* videoSegment);

    // Play the current audio frame from an audioSegment.
    void playAudioSegment(AudioSegment* audioSegment);
private:
    SDL_Texture* m_videoTexture = nullptr; // Texture for the video frame
    VideoData* m_videoData = nullptr; // Holds pointers to all VideoData for ffmpeg to be able to read frames
    Timeline* m_timeline = nullptr; // Pointer towards the timeline segment

    SDL_AudioDeviceID m_audioDevice;
    SDL_AudioSpec m_audioSpec;
    uint8_t* m_audioBuffer;
    int m_audioBufferSize;

    VideoSegment* m_lastVideoSegment = nullptr;
    AudioSegment* m_lastAudioSegment = nullptr;
    double m_frameDropThreshold = 1.0 / 60.0; // 60 fps
};