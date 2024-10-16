#pragma once
#include <SDL.h>
#include <iostream>
#include "util.h"
#include "EventManager.h"


class Segment {
public:
    Segment(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, SDL_Color color = { 0, 0, 0, 255 });
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
    SegmentHSplit(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, SDL_Color color = { 0, 255, 0, 255 });
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
    SegmentVSplit(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, SDL_Color color = { 0, 255, 0, 255 });
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
    SDL_Texture* videoFrameTexture; // The texture to render
    VideoData* videoData; // Holds all VideoData that ffmpeg needs to processing

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
    VideoPlayer(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, SDL_Color color = { 0, 0, 0, 255 });

    // Destructor
    ~VideoPlayer();

    // Render the thumbnails
    void render() override;

    // Handle events such as mouse clicks and drag-and-drop
    void handleEvent(SDL_Event& event) override;

    // Update the container dimensions
    void update(int x, int y, int w, int h) override;
private:
    SDL_Texture* videoTexture; // Texture for the video frame
    VideoData* videoData; // Holds pointers to all VideoData for ffmpeg to be able to read frames

    bool playing = false;
    Uint32 lastFrameTime = 0;      // The time when the last frame was updated
    double frameDurationMs = 0;    // Time per video frame in milliseconds

    void loadAndPlayVideo(VideoData* videoData);
    void playVideo();

    AVFrame* getNextFrame();
    void updateTextureFromFrame(AVFrame* frame);
};