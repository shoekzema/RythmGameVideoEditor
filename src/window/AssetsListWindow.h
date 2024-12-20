#pragma once
#include <SDL.h>
#include <iostream>
#include <functional>
#include "Window.h"
#include "EventManager.h"
#include "VideoData.h"

struct Asset {
    std::string assetName = "";
    SDL_Texture* assetFrameTexture = nullptr; // The Video frame (or Audio album cover) to show as an image
    VideoData* videoData = new VideoData(); // Holds all VideoData that ffmpeg needs for processing video
    AudioData* audioData = new AudioData(); // Holds all AudioData that ffmpeg needs for processing audio
};

/**
 * @class AssetsListWindow
 * @brief Window segment containing the list of assets loaded in the current project.
 */
class AssetsListWindow : public Window {
public:
    AssetsListWindow(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Window* parent = nullptr, SDL_Color color = { 27, 30, 32, 255 });
    ~AssetsListWindow();

    void render() override;
    void update(int x, int y, int w, int h) override;
    void handleEvent(SDL_Event& event) override;
    Window* findTypeImpl(const std::type_info& type) override;

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
     * @brief Return a texture for a video thumbail.
     * @param videoData The videoData needed to get the frame to create a texture for.
     * @return The texture with the video's thumbnail.
     */
    SDL_Texture* getThumbnail(VideoData* videoData);

#ifdef _WIN32
    /**
     * @brief Return the texture of the same video thumbail as windows shows.
     * @param wfilepath The path to the video file in windows format.
     * @return The texture with the video's thumbnail.
     */
    SDL_Texture* getWindowsThumbnail(const wchar_t* wfilepath);
#endif // _WIN32
private:
    std::vector<Asset> m_assets; // List of all video/audio assets
    SDL_Color m_altColor; // Alternative color for alterating assets BG
    bool m_useWindowsThumbnail = false; // Whether or not to use the same frame as windows for the video image (if on windows)
    int m_scrollOffset = 0;
    int m_scrollSpeed = 20;

    int m_assetXPos = 10; // Whitespace amount left of an asset image
    int m_assetStartYPos = 8; // y-pos where the first asset starts
    int m_assetImageWidth = 96; // w:h ratio = 16:9
    int m_assetImageHeight = 54;
    int m_assetHeight = m_assetImageHeight + 2; // An additional two pixels between images
    int m_scrollBarWidth = 6;
    int m_scrollBarXPos = 6; // x-pos from the right of the window
    SDL_Color m_scrollBarBGColor = { 27, 30, 32, 255 };
    SDL_Color m_scrollBarColor = { 48, 91, 115, 255 };
    SDL_Color m_scrollBarBorderColor = { 80, 84, 87, 255 };
};