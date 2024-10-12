#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include "VideoDecoder.h"
#include "VideoRenderer.h"

/**
 * @class VideoPlayer
 * 
 * @brief Coordinates the decoding of frames using VideoDecoder and rendering them using VideoRenderer.
 * 
 * @note It also sets up the scaling context using FFmpeg’s sws_getContext() for converting the frames to RGB format.
 */
class VideoPlayer {
public:
    VideoPlayer(const char* filename);
    ~VideoPlayer();
    void play();

private:
    VideoDecoder* decoder;
    VideoRenderer* renderer;
    SwsContext* sws_ctx;
};

#endif // VIDEOPLAYER_H
