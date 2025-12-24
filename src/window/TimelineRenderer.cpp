#include "TimelineRenderer.h"
#include "util.h"
#include <algorithm>
#include <string>

TimelineRenderer::TimelineRenderer(Timeline* timeline, SDL_Renderer* renderer)
    : m_timeline(timeline), m_renderer(renderer) {}

void TimelineRenderer::render(const SDL_Rect& rect, const TimelineView& view, const TimelineSelectionManager& selection) {
    // Background
    SDL_SetRenderDrawColor(m_renderer, 42, 46, 50, 255);
    SDL_RenderFillRect(m_renderer, &rect);

    renderTopBar(rect, view);
    renderVideoTracks(rect, view);
    renderVideoSegments(rect, view, selection);
    renderAudioTracks(rect, view);
    renderAudioSegments(rect, view, selection);
    renderTimeIndicator(rect, view);
}

void TimelineRenderer::renderTopBar(const SDL_Rect& rect, const TimelineView& view) {
    int xPos = rect.x + view.trackStartXPos;
    int yPos = rect.y;
    Uint32 timeLabel = view.scrollOffset;
    SDL_SetRenderDrawColor(m_renderer, view.timeLabelColor.r, view.timeLabelColor.g, view.timeLabelColor.b, view.timeLabelColor.a);

    while (xPos < rect.x + rect.w) {
        SDL_Rect textRect = renderTextWithCustomSpacing(m_renderer, xPos, yPos,
            getFont(), formatTime(timeLabel, m_timeline->getFPS()).c_str(),
            -1, view.timeLabelColor);

        SDL_RenderDrawLine(m_renderer, xPos, rect.y + textRect.h, xPos, rect.y + view.topBarheight);
        SDL_RenderDrawLine(m_renderer, xPos + view.timeLabelInterval / 2, rect.y + (int)(textRect.h * 1.5), xPos + view.timeLabelInterval / 2, rect.y + view.topBarheight);

        xPos += view.timeLabelInterval;
        timeLabel += view.zoom;
    }
}

void TimelineRenderer::renderVideoTracks(const SDL_Rect& rect, const TimelineView& view) {
    for (int i = 0; i < m_timeline->getVideoTrackCount(); i++) {
        int trackYpos = rect.y + view.topBarheight + (m_timeline->getVideoTrackCount() - 1 - i) * view.rowHeight;

        SDL_Rect backgroundRect = { rect.x, trackYpos, rect.w, view.rowHeight };
        SDL_SetRenderDrawColor(m_renderer, view.betweenLineColor.r, view.betweenLineColor.g, view.betweenLineColor.b, view.betweenLineColor.a);
        SDL_RenderFillRect(m_renderer, &backgroundRect);

        SDL_Rect videoTrackDataRect = { rect.x, trackYpos + 1, view.trackDataWidth, view.trackHeight };
        SDL_SetRenderDrawColor(m_renderer, view.videoTrackDataColor.r, view.videoTrackDataColor.g, view.videoTrackDataColor.b, view.videoTrackDataColor.a);
        SDL_RenderFillRect(m_renderer, &videoTrackDataRect);
        {
            std::string label = std::string("V") + std::to_string(i);
            renderText(m_renderer,
                videoTrackDataRect.x + videoTrackDataRect.w / 4,
                videoTrackDataRect.y + videoTrackDataRect.h / 4,
                getFontBig(),
                label.c_str(), view.timeLabelColor);
        }

        SDL_Rect videoTrackRect = { rect.x + view.trackStartXPos, trackYpos + 1, rect.w - view.trackStartXPos, view.trackHeight };
        SDL_SetRenderDrawColor(m_renderer, view.videoTrackBGColor.r, view.videoTrackBGColor.g, view.videoTrackBGColor.b, view.videoTrackBGColor.a);
        SDL_RenderFillRect(m_renderer, &videoTrackRect);
    }
}

void TimelineRenderer::renderVideoSegments(const SDL_Rect& rect, const TimelineView& view, const TimelineSelectionManager& selection) {
    const auto* segments = m_timeline->getAllVideoSegments();
    if (!segments) return;

    for (const VideoSegment& segment : *segments) {
        if (view.scrollOffset > segment.timelinePosition + segment.timelineDuration) continue;

        Uint32 xPos = segment.timelinePosition - view.scrollOffset;
        int diff = 0;
        if (view.scrollOffset > segment.timelinePosition) {
            xPos = 0;
            diff = view.scrollOffset - segment.timelinePosition;
        }

        int renderXPos = rect.x + view.trackStartXPos + xPos * view.timeLabelInterval / view.zoom;
        int trackPos = m_timeline->getVideoTrackPos(segment.trackID);
        int renderYPos = rect.y + view.topBarheight + (m_timeline->getVideoTrackCount() - 1 - trackPos) * view.rowHeight;
        int renderWidth = (segment.timelineDuration - diff) * view.timeLabelInterval / view.zoom;

        SDL_Rect outlineRect = { renderXPos - 1, renderYPos - 1, renderWidth + 2, view.trackHeight + 2 };
        if (std::find(selection.selectedVideoSegments.begin(), selection.selectedVideoSegments.end(), &segment) != selection.selectedVideoSegments.end()) {
            SDL_SetRenderDrawColor(m_renderer, view.segmentHighlightColor.r, view.segmentHighlightColor.g, view.segmentHighlightColor.b, view.segmentHighlightColor.a);
        }
        else {
            SDL_SetRenderDrawColor(m_renderer, view.segmentOutlineColor.r, view.segmentOutlineColor.g, view.segmentOutlineColor.b, view.segmentOutlineColor.a);
        }
        SDL_RenderFillRect(m_renderer, &outlineRect);

        SDL_Rect segmentRect = { renderXPos + 1, renderYPos + 1, renderWidth - 2, view.trackHeight - 2 };
        SDL_SetRenderDrawColor(m_renderer, view.videoTrackSegmentColor.r, view.videoTrackSegmentColor.g, view.videoTrackSegmentColor.b, view.videoTrackSegmentColor.a);
        SDL_RenderFillRect(m_renderer, &segmentRect);

        int videoFrameWidth = 0, videoFrameHeight = 0;
        SDL_QueryTexture(segment.firstFrame, nullptr, nullptr, &videoFrameWidth, &videoFrameHeight);
        if (videoFrameWidth > 0 && videoFrameHeight > 0) {
            int frameInTrackWidth = (view.trackHeight - 2) * videoFrameWidth / videoFrameHeight;
            int firstFrameWidth = std::min(renderWidth - 2, frameInTrackWidth);
            SDL_Rect firstFrameRect = { renderXPos + 1, renderYPos + 1, firstFrameWidth, view.trackHeight - 2 };
            int sourceWidth = videoFrameWidth * firstFrameWidth / frameInTrackWidth;
            SDL_Rect sourceRect = { 0, 0, sourceWidth, videoFrameHeight };
            SDL_RenderCopy(m_renderer, segment.firstFrame, &sourceRect, &firstFrameRect);

            int renderWidthLeftOver = renderWidth - 2 - firstFrameWidth;
            if (renderWidthLeftOver > 0) {
                int lastFrameWidth = std::min(renderWidthLeftOver, frameInTrackWidth);
                SDL_Rect lastFrameRect = { renderXPos + renderWidth - 1 - lastFrameWidth, renderYPos + 1, lastFrameWidth, view.trackHeight - 2 };
                sourceRect.w = videoFrameWidth * lastFrameWidth / frameInTrackWidth;
                SDL_RenderCopy(m_renderer, segment.lastFrame, &sourceRect, &lastFrameRect);
            }
        }
    }
}

void TimelineRenderer::renderAudioTracks(const SDL_Rect& rect, const TimelineView& view) {
    for (int i = 0; i < m_timeline->getAudioTrackCount(); i++) {
        int trackYpos = rect.y + view.topBarheight + (m_timeline->getVideoTrackCount() + i) * view.rowHeight;

        SDL_Rect backgroundRect = { rect.x, trackYpos, rect.w, view.rowHeight };
        SDL_SetRenderDrawColor(m_renderer, view.betweenLineColor.r, view.betweenLineColor.g, view.betweenLineColor.b, view.betweenLineColor.a);
        SDL_RenderFillRect(m_renderer, &backgroundRect);

        SDL_Rect audioTrackDataRect = { rect.x, trackYpos + 1, view.trackDataWidth, view.trackHeight };
        SDL_SetRenderDrawColor(m_renderer, view.audioTrackDataColor.r, view.audioTrackDataColor.g, view.audioTrackDataColor.b, view.audioTrackDataColor.a);
        SDL_RenderFillRect(m_renderer, &audioTrackDataRect);
        {
            std::string label = std::string("A") + std::to_string(i);
            renderText(m_renderer,
                audioTrackDataRect.x + audioTrackDataRect.w / 4,
                audioTrackDataRect.y + audioTrackDataRect.h / 4,
                getFontBig(),
                label.c_str(), view.timeLabelColor);
        }

        SDL_Rect audioTrackRect = { rect.x + view.trackStartXPos, trackYpos + 1, rect.w - view.trackStartXPos, view.trackHeight };
        SDL_SetRenderDrawColor(m_renderer, view.audioTrackBGColor.r, view.audioTrackBGColor.g, view.audioTrackBGColor.b, view.audioTrackBGColor.a);
        SDL_RenderFillRect(m_renderer, &audioTrackRect);
    }
}

void TimelineRenderer::renderAudioSegments(const SDL_Rect& rect, const TimelineView& view, const TimelineSelectionManager& selection) {
    const auto* segments = m_timeline->getAllAudioSegments();
    if (!segments) return;

    for (const AudioSegment& segment : *segments) {
        if (view.scrollOffset > segment.timelinePosition + segment.timelineDuration) continue;

        Uint32 xPos = segment.timelinePosition - view.scrollOffset;
        int diff = 0;
        if (view.scrollOffset > segment.timelinePosition) {
            xPos = 0;
            diff = view.scrollOffset - segment.timelinePosition;
        }

        int renderXPos = rect.x + view.trackStartXPos + xPos * view.timeLabelInterval / view.zoom;
        int trackPos = m_timeline->getAudioTrackPos(segment.trackID);
        int renderYPos = rect.y + view.topBarheight + (m_timeline->getVideoTrackCount() + trackPos) * view.rowHeight;
        int renderWidth = (segment.timelineDuration - diff) * view.timeLabelInterval / view.zoom;

        SDL_Rect outlineRect = { renderXPos - 1, renderYPos - 1, renderWidth + 2, view.trackHeight + 2 };
        if (std::find(selection.selectedAudioSegments.begin(), selection.selectedAudioSegments.end(), &segment) != selection.selectedAudioSegments.end()) {
            SDL_SetRenderDrawColor(m_renderer, view.segmentHighlightColor.r, view.segmentHighlightColor.g, view.segmentHighlightColor.b, view.segmentHighlightColor.a);
        } else {
            SDL_SetRenderDrawColor(m_renderer, view.segmentOutlineColor.r, view.segmentOutlineColor.g, view.segmentOutlineColor.b, view.segmentOutlineColor.a);
        }
        SDL_RenderFillRect(m_renderer, &outlineRect);

        SDL_Rect segmentRect = { renderXPos + 1, renderYPos + 1, renderWidth - 2, view.trackHeight - 2 };
        SDL_SetRenderDrawColor(m_renderer, view.audioTrackSegmentColor.r, view.audioTrackSegmentColor.g, view.audioTrackSegmentColor.b, view.audioTrackSegmentColor.a);
        SDL_RenderFillRect(m_renderer, &segmentRect);
    }
}

void TimelineRenderer::renderTimeIndicator(const SDL_Rect& rect, const TimelineView& view) {
    if (view.scrollOffset <= m_timeline->getCurrentTime()) {
        int indicatorX = view.trackStartXPos + (m_timeline->getCurrentTime() - view.scrollOffset) * view.timeLabelInterval / view.zoom;
        SDL_SetRenderDrawColor(m_renderer, view.timeIndicatorColor.r, view.timeIndicatorColor.g, view.timeIndicatorColor.b, view.timeIndicatorColor.a);

        SDL_RenderDrawLine(m_renderer, rect.x + indicatorX, rect.y, rect.x + indicatorX, rect.y + view.topBarheight + (m_timeline->getVideoTrackCount() + m_timeline->getAudioTrackCount()) * view.rowHeight);

        if (view.zoom <= view.indicatorFrameDisplayThreshold) {
            int indicatorXplus1 = indicatorX + view.timeLabelInterval / view.zoom;
            SDL_RenderDrawLine(m_renderer, rect.x + indicatorX, rect.y + view.topBarheight, rect.x + indicatorXplus1, rect.y + view.topBarheight);
        }
    }
}
