#pragma once
#include <SDL.h>
#include <iostream>
#include "Segment.h"
#include "EventManager.h"
#include "VideoData.h"

struct Asset {
    std::string assetName = "";
    SDL_Texture* videoFrameTexture = nullptr;
    VideoData* videoData = new VideoData(); // Holds all VideoData that ffmpeg needs for processing video
    AudioData* audioData = new AudioData(); // Holds all AudioData that ffmpeg needs for processing audio
};

/**
 * @class AssetsList
 * @brief Window segment containing the list of assets loaded in the current project.
 */
class AssetsList : public Segment {
public:
    AssetsList(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Segment* parent = nullptr, SDL_Color color = { 27, 30, 32, 255 });
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
    AVFrame* getFrame(VideoData* videoData, int frameIndex);

    /**
     * @brief Return a texture for a video thumbail.
     * @param filepath The path to the video file.
     * @return The texture with the video's thumbnail.
     */
    SDL_Texture* getFrameTexture(VideoData* videoData);

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
    bool m_useWindowsThumbnail = false;
    int m_scrollOffset = 0;
    int m_scrollSpeed = 20;

    int m_assetXPos = 10;
    int m_assetStartYPos = 8;
    int m_assetImageWidth = 96; // w:h ratio = 16:9
    int m_assetImageHeight = 54;
    int m_assetHeight = m_assetImageHeight + 2;
    int m_scrollBarWidth = 6;
    int m_scrollBarXPos = 6; // x-pos from the right of the segment
    SDL_Color m_scrollBarBGColor = { 27, 30, 32, 255 };
    SDL_Color m_scrollBarColor = { 48, 91, 115, 255 };
    SDL_Color m_scrollBarBorderColor = { 80, 84, 87, 255 };
};