#pragma once
#include <SDL.h>
#include "Segment.h"
#include "EventManager.h"

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