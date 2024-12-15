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
    void setVideoRect(SDL_Rect* rect);

    // Render video and audio based on the segments in the timeline at the current timeline time.
    void renderTimeline();

    void playAudio();
    void pauseAudioPlayback();

    void renderFrame(VideoSegment* videoSegment);

    // Render the current video frame of a videoSegment to the screen
    void renderFrameToScreen(VideoSegment* videoSegment);

    // Get the current video frame from a videoSegment. The resulting frame is stored inside videoSegment.
    bool getVideoFrame(VideoSegment* videoSegment);
    bool decodeAndProcessFrame(VideoSegment* videoSegment, Uint32 currentFrame);
    bool processFrame(VideoSegment* videoSegment, Uint32 currentFrame);

    // Play the current audio frame from an audioSegment.
    void playAudioSegment(AudioSegment* audioSegment);
private:
    SDL_Texture* m_videoTexture = nullptr; // Texture for the video frame
    VideoData* m_videoData = nullptr; // Holds pointers to all VideoData for ffmpeg to be able to read frames
    Timeline* m_timeline = nullptr; // Pointer towards the timeline segment
    SDL_Rect m_videoRect; // Rectangle to display the video in
    int m_WtoH_ratioW = 16; // Width to height ratio: width (default 1920:1080 = 16:9)
    int m_WtoH_ratioH = 9; // Width to height ratio: height (default 1920:1080 = 16:9)

    SDL_AudioDeviceID m_audioDevice;
    SDL_AudioSpec m_audioSpec;
    uint8_t* m_audioBuffer;
    int m_audioBufferSize;

    VideoSegment* m_lastVideoSegment = nullptr;
    AudioSegment* m_lastAudioSegment = nullptr;
    Uint32 m_lastVideoSegmentFrame = 0;
    double m_frameDropThreshold = 1; // Allow being one frame behind
    int m_framebehindSeekThreshold = 10; // We need to be at least 10 frames behind to use av_seek_frame over just skipping frames one by one.
};