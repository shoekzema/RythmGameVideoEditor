#pragma once
#include <SDL.h>
#include "Segment.h"
#include "EventManager.h"
#include "VideoData.h"

/**
 * @class AssetsList
 * @brief Window segment containing the list of assets loaded in the current project.
 */
class AssetsList : public Segment {
public:
    AssetsList(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Segment* parent = nullptr, SDL_Color color = { 0, 0, 0, 255 });
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
    AVFrame* getFrame(int frameIndex);

    /**
     * @brief Return a texture for a video thumbail.
     * @param filepath The path to the video file.
     * @return The texture with the video's thumbnail.
     */
    SDL_Texture* getFrameTexture(const char* filepath);

#ifdef _WIN32
    /**
     * @brief Return the texture of the same video thumbail as windows shows.
     * @param wfilepath The path to the video file in windows format.
     * @return The texture with the video's thumbnail.
     */
    SDL_Texture* getWindowsThumbnail(const wchar_t* wfilepath);
#endif // _WIN32
private:
    SDL_Texture* m_videoFrameTexture = nullptr; // The texture to render
    VideoData* m_videoData = new VideoData(); // Holds all VideoData that ffmpeg needs for processing video
    AudioData* m_audioData = new AudioData(); // Holds all AudioData that ffmpeg needs for processing audio
};