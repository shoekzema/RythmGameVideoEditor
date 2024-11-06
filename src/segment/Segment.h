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
    Segment(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Segment* parent = nullptr, SDL_Color color = { 0, 0, 0, 255 });
    virtual ~Segment();

    virtual void render();
    virtual void update(int x, int y, int w, int h);
    /**
     * @brief Handle user events.
     * @param event User interaction event code.
     */
    virtual void handleEvent(SDL_Event& event);
    /**
     * @brief Find a segment with type T. (Best to call from the root segment)
     * @returns The first segment in the hierarchy with type T.
     */
    template <typename T>
    T* findType();

    // The virtual implementation method for findType(). Should only be called from findType() or any overwritten findTypeImpl().
    virtual Segment* findTypeImpl(const std::type_info& type);
public:
    Segment* parent;
    SDL_Rect rect;
protected:
    SDL_Renderer* p_renderer;
    EventManager* p_eventManager;
    SDL_Color p_color;
};

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
    void handleEvent(SDL_Event& event) override;
    Segment* findTypeImpl(const std::type_info& type) override;

    void setTopSegment(Segment* segment);
    void setBottomSegment(Segment* segment);
private:
    SDL_Rect m_divider;
    SDL_Color m_dividerColor = { 255, 255, 255, 255 }; // white
    bool m_draggingDivider = false;
    bool m_resizing = false; // For resize handles
    int m_dividerThickness = 8;

    Segment* m_topSegment;
    Segment* m_bottomSegment;
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
    void handleEvent(SDL_Event& event) override;
    Segment* findTypeImpl(const std::type_info& type) override;

    void setLeftSegment(Segment* segment);
    void setRightSegment(Segment* segment);
private:
    SDL_Rect m_divider;
    SDL_Color m_dividerColor = { 255, 255, 255, 255 }; // white
    bool m_draggingDivider = false;
    bool m_resizing = false; // For resize handles
    int m_dividerThickness = 8;

    Segment* m_leftSegment;
    Segment* m_rightSegment;
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
    void handleEvent(SDL_Event& event) override;
    Segment* findTypeImpl(const std::type_info& type) override;

    /**
     * @brief Retrieve the asset data corresponding to the mouse position
     * @return AssetData with VideoData and/or AudioData
     */
    AssetData* getAssetFromAssetList(int mouseX, int mouseY);
private:
    /**
     * @brief Opens the video file, finds the stream with video data and set up the codec context to decode video.
     * @param filepath The path to the video or audio file.
     * @return True if successful, otherwise false.
     */
    bool loadFile(const char* filepath);

    /**
     * @brief Get a specific frame from a video.
     * @param frameIndex The index of the video frame to return.
     * @return The chosen video frame.
     */
    AVFrame* getFrame(int frameIndex);

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
private:
    SDL_Texture* m_videoFrameTexture = nullptr; // The texture to render
    VideoData* m_videoData = new VideoData(); // Holds all VideoData that ffmpeg needs for processing video
    AudioData* m_audioData = new AudioData(); // Holds all AudioData that ffmpeg needs for processing audio
};

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
    Timeline(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Segment* parent = nullptr, SDL_Color color = { 0, 0, 0, 255 });
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
