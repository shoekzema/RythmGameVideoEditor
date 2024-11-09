#pragma once
#include <SDL.h>
#include "Segment.h"
#include "EventManager.h"

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
    SDL_Color m_dividerColor = { 95, 98, 101, 255 };
    bool m_draggingDivider = false;
    bool m_resizing = false; // For resize handles
    int m_dividerThickness = 2;

    Segment* m_topSegment;
    Segment* m_bottomSegment;
};