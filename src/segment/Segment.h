#pragma once
#include <SDL.h>
#include <iostream>
#include "util.h"
#include "EventManager.h"

/**
 * @class Segment
 * @brief Basic empty window segment.
 */
class Segment {
public:
    Segment* parent;

    Segment(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Segment* parent = nullptr, SDL_Color color = { 0, 0, 0, 255 });
    virtual ~Segment();

    virtual void render();
    virtual void update(int x, int y, int w, int h);

    /**
     * @brief Handle user events.
     * @param event User interaction event code.
     */
    virtual void handleEvent(SDL_Event& event);

    SDL_Rect rect;

    template <typename T>
    T* findType();

    virtual Segment* findTypeImpl(const std::type_info& type);
protected:
    SDL_Renderer* renderer;
    EventManager* eventManager;
    SDL_Color color;
};

// Template method definition
template<typename T>
T* Segment::findType() {
    return dynamic_cast<T*>(this->findTypeImpl(typeid(T)));
}

/**
 * @class SegmentHSplit
 * @brief Window segment split into a top and bottom segment with a movable horizontal divider between that allows for resizing.
 */
class SegmentHSplit : public Segment {
public:
    SegmentHSplit(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Segment* parent = nullptr, SDL_Color color = { 0, 255, 0, 255 });
    ~SegmentHSplit();

    void render() override;
    void update(int x, int y, int w, int h) override;

    /**
     * @brief Handle user events, like dragging the divider.
     * @param event User interaction event code.
     */
    void handleEvent(SDL_Event& event) override;

    Segment* findTypeImpl(const std::type_info& type) override;
private:
    // Divider between segments
    SDL_Rect divider;
    SDL_Color dividerColor = { 255, 255, 255, 255 }; // white
    bool draggingDivider;

    // For resize handles
    bool resizing;
    int dividerThickness = 8;

    Segment* topSegment;
    Segment* bottomSegment;
};

/**
 * @class SegmentVSplit
 * @brief Window segment split into a left and right segment with a movable vertical divider between that allows for resizing.
 */
class SegmentVSplit : public Segment {
public:
    SegmentVSplit(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Segment* parent = nullptr, SDL_Color color = { 0, 255, 0, 255 });
    ~SegmentVSplit();

    void render() override;
    void update(int x, int y, int w, int h) override;

    /**
     * @brief Handle user events, like dragging the divider.
     * @param event User interaction event code.
     */
    void handleEvent(SDL_Event& event) override;

    Segment* findTypeImpl(const std::type_info& type) override;
private:
    // Divider between segments
    SDL_Rect divider;
    SDL_Color dividerColor = { 255, 255, 255, 255 }; // white
    bool draggingDivider;

    // For resize handles
    bool resizing;
    int dividerThickness = 8;

    Segment* leftSegment;
    Segment* rightSegment;
};

/**
 * @class AssetsList
 * @brief Window segment containing the list of assets loaded in the current project.
 */
class AssetsList : public Segment {
public:
    AssetsList(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Segment* parent = nullptr, SDL_Color color = { 0, 0, 0, 255 });
    ~AssetsList();

    void render() override;
    void update(int x, int y, int w, int h) override;

    /**
     * @brief Handle user events, like mouse clicks and drag-and-drop.
     * @param event User interaction event code.
     */
    void handleEvent(SDL_Event& event) override;

    VideoData* getAssetFromAssetList(int mouseX, int mouseY);
    double getVideoDuration();

    Segment* findTypeImpl(const std::type_info& type) override;
private:
    SDL_Texture* videoFrameTexture; // The texture to render
    VideoData* videoData; // Holds all VideoData that ffmpeg needs to processing

    /**
     * @brief Opens the video file, finds the stream with video data and set up the codec context to decode video.
     * @param filepath The path to the video file.
     * @return True if successful, otherwise false.
     */
    bool loadVideo(const char* filepath);

    /**
     * @brief Get a specific frame from a video.
     * @param frameIndex The index of the video frame to return.
     * @return The chosen video frame.
     */
    AVFrame* getFrame(int frameIndex);

    // Get the total number of frames of the current video.
    int getFrameCount();

    /**
     * @brief Return a texture for a video thumbail.
     * @param filepath The path to the video file.
     * @return The texture with the video's thumbnail.
     */
    SDL_Texture* getFrameTexture(const char* filepath);

#ifdef _WIN32
    /**
     * @brief Return the texture of the same video thumbail as windows shows.
     * @param wfilepath The path to the video file in windows format.
     * @return The texture with the video's thumbnail.
     */
    SDL_Texture* getWindowsThumbnail(const wchar_t* wfilepath);
#endif // _WIN32
};

struct VideoSegment {
    VideoData* videoData;      // Reference to the video data
    double sourceStartTime;    // Start time in the original video file
    double duration;           // Duration of this segment
    double timelinePosition;   // Position in the overall timeline
};
struct AudioSegment {
    VideoData* audioData;      // Reference to the audio data
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
    bool playing;

    Timeline(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Segment* parent = nullptr, SDL_Color color = { 0, 0, 0, 255 });
    ~Timeline();

    void render() override;
    void update(int x, int y, int w, int h) override;

    /**
     * @brief Handle user events, like mouse clicks, drag-and-frop, etc.
     * @param event User interaction event code.
     */
    void handleEvent(SDL_Event& event) override;

    VideoSegment* getCurrentVideoSegment();
    AudioSegment* getCurrentAudioSegment();
    double getCurrentTime();
    void setCurrentTime(double time);

    void addVideoSegment(VideoData* data);
    void addAudioSegment(VideoData* data);

    Segment* findTypeImpl(const std::type_info& type) override;
private:
    std::vector<VideoSegment> videoSegments;
    std::vector<AudioSegment> audioSegments;
    double currentTime;   // The current time (and position) of the timeline
    double startPlayTime; // The time in the timeline where playing starts from
    Uint32 startTime = 0; // Absolute start time of playback (milliseconds)
};

/**
 * @class VideoPlayer
 * @brief Window segment that can render videos.
 */
class VideoPlayer : public Segment {
public:
    VideoPlayer(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Segment* parent = nullptr, SDL_Color color = { 0, 0, 0, 255 });
    ~VideoPlayer();

    void render() override;
    void update(int x, int y, int w, int h) override;

    /**
     * @brief Handle user events, like mouse clicks.
     * @param event User interaction event code.
     */
    void handleEvent(SDL_Event& event) override;

    bool getVideoFrameAtTime(double timeInSeconds, VideoSegment* videoSegment);
    int getAudioFrameAtTime(double timeInSeconds, AudioSegment* audioSegment);
    void playTimeline(Timeline* timeline);

    Segment* findTypeImpl(const std::type_info& type) override;
private:
    SDL_Texture* videoTexture; // Texture for the video frame
    VideoData* videoData; // Holds pointers to all VideoData for ffmpeg to be able to read frames
    Timeline* timeline; // Pointer towards the timeline segment

    bool playing = false;
    Uint32 lastFrameTime = 0;      // The time when the last frame was updated
    double frameDurationMs = 0;    // Time per video frame in milliseconds

    SDL_AudioDeviceID audioDevice;
    uint8_t* audioBuffer;
    int audioBufferSize;

    /**
     * @brief Save a video's data and start playing it.
     * @param videoData The processed ffmpeg video data.
     */
    void loadAndPlayVideo(VideoData* videoData);
    void playVideo();
    void playAudioFrame(VideoData* audioData);

    AVFrame* getNextFrame();
    void updateTextureFromFrame(AVFrame* frame);
};
