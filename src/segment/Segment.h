#pragma once
#include <SDL.h>
#include <iostream>
#include "util.h"

class Segment {
public:
    Segment(int x, int y, int w, int h, SDL_Renderer* renderer, SDL_Color color = { 0, 0, 255, 255 }); // blue (easy to find problems)
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
    SDL_Color color;
};


class SegmentHSplit : public Segment {
public:
    SegmentHSplit(int x, int y, int w, int h, SDL_Renderer* renderer, SDL_Color color = { 0, 0, 0, 255 });
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
    SegmentVSplit(int x, int y, int w, int h, SDL_Renderer* renderer, SDL_Color color = { 0, 0, 0, 255 });
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