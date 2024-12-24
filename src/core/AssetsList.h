#pragma once
#include <SDL.h>
#include <iostream>
#include <vector>
#include "VideoData.h"

struct Asset {
    std::string assetName = "";
    SDL_Texture* assetFrameTexture = nullptr; // The Video frame (or Audio album cover) to show as an image
    VideoData* videoData = new VideoData(); // Holds all VideoData that ffmpeg needs for processing video
    AudioData* audioData = new AudioData(); // Holds all AudioData that ffmpeg needs for processing audio
};

/**
 * @class AssetsList
 * @brief Container class with a list of all assets loaded in the current project.
 */
class AssetsList {
public:
    AssetsList(SDL_Renderer* renderer);
    ~AssetsList();

    bool IsEmpty();
    int getAssetCount();
    const std::vector<Asset>* getAllAssets();

    /**
     * @brief Opens the video file, finds the stream with video data and set up the codec context to decode video.
     * @param filepath The path to the video or audio file.
     * @return True if successful, otherwise false.
     */
    bool loadFile(const char* filepath);
private:
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
    SDL_Renderer* m_renderer;
    std::vector<Asset> m_assets; // List of all video/audio assets
    bool m_useWindowsThumbnail = false; // Whether or not to use the same frame as windows for the video image (if on windows)
};