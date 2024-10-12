#ifndef VIDEODECODER_H
#define VIDEODECODER_H

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}
#include <iostream>

/**
 * @class VideoDecoder
 *
 * @brief Handles all FFmpeg-related video decoding. It abstracts the process of opening the file, finding the video stream, and decoding each frame.
 */
class VideoDecoder {
public:
    VideoDecoder(const char* filename);
    ~VideoDecoder();

    bool open();
    bool getNextFrame(AVFrame* frame);
    int getWidth() const;
    int getHeight() const;
    AVPixelFormat getPixelFormat() const;

private:
    const char* filename;
    AVFormatContext* format_ctx;
    AVCodecContext* codec_ctx;
    AVCodecParameters* codec_params;
    AVPacket* packet;
    int video_stream_index;
    const AVCodec* codec;
};

#endif // VIDEODECODER_H
