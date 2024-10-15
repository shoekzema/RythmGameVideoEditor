#pragma once
#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include <vector>
#include <string>
#include "util.h"
#include "EventManager.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

class Segment {
public:
    Segment(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, SDL_Color color = { 0, 0, 255, 255 }); // blue (easy to find problems)
    virtual ~Segment();

    // Render the segment
    virtual void render();

    // Update position and size
    virtual void update(int x, int y, int w, int h);

    // Handle resizing events, etc.
    virtual void handleEvent(SDL_Event& event);

    SDL_Rect rect;
protected:
    SDL_Renderer* renderer;
    EventManager* eventManager;
    SDL_Color color;
};


class SegmentHSplit : public Segment {
public:
    SegmentHSplit(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, SDL_Color color = { 0, 0, 0, 255 });
    ~SegmentHSplit();

    // Render the segment
    void render() override;

    // Update position and size
    void update(int x, int y, int w, int h) override;

    // Handle resizing events, etc.
    void handleEvent(SDL_Event& event) override;
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


class SegmentVSplit : public Segment {
public:
    SegmentVSplit(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, SDL_Color color = { 0, 0, 0, 255 });
    ~SegmentVSplit();

    // Render the segment
    void render() override;

    // Update position and size
    void update(int x, int y, int w, int h) override;

    // Handle resizing events, etc.
    void handleEvent(SDL_Event& event) override;
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


class AssetsList : public Segment {
public:
    // Constructor
    AssetsList(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, SDL_Color color = { 0, 0, 0, 255 });

    // Destructor
    ~AssetsList();

    // Add a video and create a thumbnail (for now, static image)
    void addAsset(const char* filename);

    // Render the thumbnails
    void render() override;

    // Handle events such as mouse clicks and drag-and-drop
    void handleEvent(SDL_Event& event) override;

    // Update the container dimensions
    void update(int x, int y, int w, int h) override;

private:
    AVFormatContext* formatContext; // Manages the media container, holds info about streams, formats, etc.
    AVCodecContext* codecContext; // Manages decoding of the video stream.
    const AVCodec* codec; // A specific codec for decoding video (e.g., H.264).
    AVFrame* frame; // Holds decoded video frame data.
    AVFrame* rgbFrame; // Holds video frame data converted to RGB format for easier processing.
    SwsContext* swsContext; // Used for converting the frame to the desired format (e.g., YUV to RGB).
    int videoStreamIndex; // The index of the video stream (because a file might have multiple streams).
    SDL_Texture* videoFrameTexture; // The texture to render
    std::string currentVideoFile;

    // Load a video file
    bool loadVideo(const char* filepath);

    // Get a specific frame
    AVFrame* getFrame(int frameIndex);

    // Get the total frame count
    int getFrameCount();

    // Return a Texture for a video thumbail
    SDL_Texture* getFrameTexture(const char* filepath);

    // Return a Texture for a video thumbail
    SDL_Texture* getWindowsThumbnail(const wchar_t* wfilepath);
};


class VideoPlayer : public Segment {
public:
    // Constructor
    VideoPlayer(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, SDL_Color color = { 0, 0, 255, 255 });

    // Destructor
    ~VideoPlayer();

    // Render the thumbnails
    void render() override;

    // Handle events such as mouse clicks and drag-and-drop
    void handleEvent(SDL_Event& event) override;

    // Update the container dimensions
    void update(int x, int y, int w, int h) override;
private:
    AVFormatContext* formatContext; // Manages the media container, holds info about streams, formats, etc.
    AVCodecContext* codecContext; // Manages decoding of the video stream.
    const AVCodec* codec; // A specific codec for decoding video (e.g., H.264).
    AVFrame* frame; // Holds decoded video frame data.
    AVFrame* rgbFrame; // Holds video frame data converted to RGB format for easier processing.
    SwsContext* swsContext; // Used for converting the frame to the desired format (e.g., YUV to RGB).
    int videoStreamIndex; // The index of the video stream (because a file might have multiple streams).
    SDL_Texture* videoTexture; // Texture for the video frame

    bool playing = false;
    Uint32 lastFrameTime = 0;      // The time when the last frame was updated
    double frameDurationMs = 0;    // Time per video frame in milliseconds

    void loadAndPlayVideo(const char* videoPath);
    bool loadVideo(const char* filename);
    void playVideo();

    AVFrame* getNextFrame();
    void updateTextureFromFrame(AVFrame* frame);
};