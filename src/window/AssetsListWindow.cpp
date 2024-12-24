#include <iostream>
#include <SDL.h>
#include <SDL_ttf.h>
#include "AssetsListWindow.h"
#include "util.h"

AssetsListWindow::AssetsListWindow(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Window* parent, SDL_Color color)
    : Window(x, y, w, h, renderer, eventManager, parent, color)
{
    m_assetsList = new AssetsList(renderer);

    m_altColor = {
        static_cast<Uint8>(std::min(color.r + 8, 255)),
        static_cast<Uint8>(std::min(color.g + 8, 255)),
        static_cast<Uint8>(std::min(color.b + 9, 255)),
        color.a
    };
}

AssetsListWindow::~AssetsListWindow() {}

void AssetsListWindow::render() {
    SDL_SetRenderDrawColor(p_renderer, p_color.r, p_color.g, p_color.b, p_color.a);
    SDL_RenderFillRect(p_renderer, &rect); // Draw background

    int yPos = m_assetStartYPos - m_scrollOffset;
    int i = 0;
    for (const Asset& asset : *m_assetsList->getAllAssets()) {
        // Use alternative background color rect for every second asset
        if (i++ % 2 == 1) {
            SDL_Rect altBG = { rect.x, yPos - 1, rect.w, m_assetImageHeight + 2 };
            SDL_SetRenderDrawColor(p_renderer, m_altColor.r, m_altColor.g, m_altColor.b, m_altColor.a);
            SDL_RenderFillRect(p_renderer, &altBG); // Draw background
        }

        // Get the width and height of the original video texture
        int videoFrameWidth, videoFrameHeight;
        SDL_QueryTexture(asset.assetFrameTexture, nullptr, nullptr, &videoFrameWidth, &videoFrameHeight);

        SDL_Rect thumbnailRect = { m_assetXPos, yPos, m_assetImageWidth, m_assetImageHeight };
        // If the video frame is too thin, make a black BG and put it in the middle
        if (videoFrameWidth * m_assetImageHeight < videoFrameHeight * m_assetImageWidth) {
            SDL_SetRenderDrawColor(p_renderer, 0, 0, 0, 255); // black
            SDL_RenderFillRect(p_renderer, &thumbnailRect);

            thumbnailRect.w = (videoFrameWidth * m_assetImageHeight) / videoFrameHeight;
            thumbnailRect.x += (m_assetImageWidth - thumbnailRect.w) / 2; // Center horizontally
        }
        SDL_RenderCopy(p_renderer, asset.assetFrameTexture, nullptr, &thumbnailRect);

        renderText(p_renderer,
            m_assetXPos + m_assetImageWidth + 6, // X position
            yPos + 4,                            // Y position
            getFont(),
            asset.assetName.c_str());

        yPos += 2 + m_assetImageHeight;
    }

    // If scrolling is possible, draw the scrollbar
    int assetListLength;
    assetListLength = m_assetsList->getAssetCount() * m_assetHeight;
    if (assetListLength + m_assetStartYPos > rect.h) {
        // Draw the scrollbar border
        SDL_Rect scrollbarBorder = { rect.w - m_scrollBarXPos - m_scrollBarWidth, m_assetStartYPos, m_scrollBarWidth, rect.h - 2 * m_assetStartYPos };
        SDL_SetRenderDrawColor(p_renderer, m_scrollBarBorderColor.r, m_scrollBarBorderColor.g, m_scrollBarBorderColor.b, m_scrollBarBorderColor.a);
        SDL_RenderFillRect(p_renderer, &scrollbarBorder);

        // Draw the scrollbar background
        SDL_Rect scrollbarBackground = { scrollbarBorder.x + 1, scrollbarBorder.y + 1, scrollbarBorder.w - 2, scrollbarBorder.h - 2 };
        SDL_SetRenderDrawColor(p_renderer, m_scrollBarBGColor.r, m_scrollBarBGColor.g, m_scrollBarBGColor.b, m_scrollBarBGColor.a);
        SDL_RenderFillRect(p_renderer, &scrollbarBackground);

        // Draw the scrollbar handle
        int scrollbarYpos = scrollbarBorder.y + (m_scrollOffset * scrollbarBorder.h / assetListLength);
        int scrollbarHeight = scrollbarBorder.h * scrollbarBorder.h / assetListLength;
        SDL_Rect scrollbarHandle = { scrollbarBorder.x, scrollbarYpos, scrollbarBorder.w, scrollbarHeight };
        SDL_SetRenderDrawColor(p_renderer, m_scrollBarColor.r, m_scrollBarColor.g, m_scrollBarColor.b, m_scrollBarColor.a);
        SDL_RenderFillRect(p_renderer, &scrollbarHandle);
    }
}

void AssetsListWindow::update(int x, int y, int w, int h) {
    rect = { x, y, w, h };

    // If the list is longer than can be displayed, update the furthest yPos the scroll position can be in. (So when increasing the window size, it will scroll up if possible)
    int assetListLength;
    assetListLength = m_assetsList->getAssetCount() * m_assetHeight + m_assetStartYPos;
    if (assetListLength > rect.h) {
        if (m_scrollOffset > assetListLength + m_assetStartYPos - rect.h) m_scrollOffset = assetListLength + m_assetStartYPos - rect.h;
    }
}

void AssetsListWindow::handleEvent(SDL_Event& event) {
    static bool mouseInThisWindow = false;

    switch (event.type) {
    case SDL_MOUSEMOTION: {
        SDL_Point mousePoint = { event.motion.x, event.motion.y };
        mouseInThisWindow = SDL_PointInRect(&mousePoint, &rect) ? true : false;
        break;
    }
    case SDL_MOUSEWHEEL: {
        if (!mouseInThisWindow) break;

        // Check if scrolling neccesary
        int assetListLength = m_assetsList->getAssetCount() * m_assetHeight + m_assetStartYPos;
        if (assetListLength <= rect.h) break;

        // Scroll
        m_scrollOffset -= event.wheel.y * m_scrollSpeed;

        // Clamp the scroll position
        if (m_scrollOffset < 0) m_scrollOffset = 0;
        if (m_scrollOffset > assetListLength + m_assetStartYPos - rect.h) m_scrollOffset = assetListLength + m_assetStartYPos - rect.h;
        break;
    }
    case SDL_DROPFILE: {
        if (!mouseInThisWindow) break;

        const char* droppedFile = event.drop.file;
        loadFile(droppedFile);
        SDL_free(event.drop.file);
        break;
    }
    }
}

Window* AssetsListWindow::findTypeImpl(const std::type_info& type) {
    if (type == typeid(AssetsListWindow)) {
        return this;
    }
    return nullptr;
}

AssetData* AssetsListWindow::getAssetFromAssetList(int mouseX, int mouseY) {
    // If no assets loaded in, return null
    if (m_assetsList->IsEmpty()) return nullptr;

    // Otherwise, loop through all assets
    int i = 0;
    for (const Asset& asset : *m_assetsList->getAllAssets()) {
        if (mouseY + m_scrollOffset > m_assetStartYPos + i * m_assetHeight && mouseY + m_scrollOffset < m_assetStartYPos + (i+1) * m_assetHeight) {
            return new AssetData(asset.videoData, asset.audioData); // If clicked inside the y-range of the this asset, return it
        }
    }
    return nullptr;
}

bool AssetsListWindow::loadFile(const char* filepath) {
    return m_assetsList->loadFile(filepath);
}
