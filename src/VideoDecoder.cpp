#include "VideoDecoder.h"
#include <iostream>

/**
 * @brief Constructs a VideoDecoder for the specified video file.
 *
 * This constructor initializes the VideoDecoder with the filename of
 * the video file to be decoded. It sets up various member variables
 * to prepare for video decoding.
 *
 * @param filename The path to the video file to be decoded.
 */
VideoDecoder::VideoDecoder(const char* filename)
    : filename(filename), format_ctx(nullptr), codec_ctx(nullptr), codec_params(nullptr), packet(nullptr), video_stream_index(-1), codec(nullptr) {}

/**
 * @brief Destroys the VideoDecoder and frees associated resources.
 */
VideoDecoder::~VideoDecoder() {
    avcodec_free_context(&codec_ctx); // Free the codec context
    avformat_close_input(&format_ctx); // Close the input format context
    av_packet_free(&packet); // Free the packet
}

/**
 * @brief Opens the video file for decoding.
 *
 * This method initializes the format context and retrieves information
 * about the video stream. It finds the appropriate codec to decode the
 * video and prepares the codec context for decoding frames.
 *
 * @return true if the video file is successfully opened and prepared for decoding, false otherwise.
 */
bool VideoDecoder::open() {
    // Open the video file and initialize the format context
    if (avformat_open_input(&format_ctx, filename, NULL, NULL) != 0) {
        std::cerr << "Error: Cannot open video file\n";
        return false;
    }

    // Retrieve information about the video streams in the file
    if (avformat_find_stream_info(format_ctx, NULL) < 0) {
        std::cerr << "Error: Cannot find stream information\n";
        return false;
    }

    // Find the index of the video stream
    for (unsigned int i = 0; i < format_ctx->nb_streams; i++) {
        if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }

    if (video_stream_index == -1) {
        std::cerr << "Error: No video stream found\n";
        return false;
    }

    // Retrieve codec parameters for the video stream
    codec_params = format_ctx->streams[video_stream_index]->codecpar;
    codec = avcodec_find_decoder(codec_params->codec_id); // Find the appropriate codec for the video stream
    codec_ctx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codec_ctx, codec_params);
    avcodec_open2(codec_ctx, codec, NULL);

    // Allocate a packet for reading the video frames
    packet = av_packet_alloc();
    return true;
}

/**
 * @brief Retrieves the next frame from the video stream.
 *
 * This method reads packets from the video stream and decodes them into
 * frames. If a frame is successfully decoded, it is stored in the
 * provided AVFrame pointer.
 *
 * @param frame A pointer to the AVFrame where the decoded frame will be stored.
 * @return true if a frame is successfully retrieved and decoded, false if no more frames are available.
 */
bool VideoDecoder::getNextFrame(AVFrame* frame) {
    // Loop to read packets from the video stream
    while (av_read_frame(format_ctx, packet) >= 0) {
        // Check if the packet is from the video stream
        if (packet->stream_index == video_stream_index) {
            // Send the packet to the codec context for decoding
            avcodec_send_packet(codec_ctx, packet);

            // Try to receive the decoded frame
            if (avcodec_receive_frame(codec_ctx, frame) == 0) {
                av_packet_unref(packet);
                return true;
            }
        }
        av_packet_unref(packet);
    }
    // Return false if no more frames are available
    return false;
}

/**
 * @brief Gets the width of the video frames.
 *
 * @return The width of the video frames in pixels.
 */
int VideoDecoder::getWidth() const {
    return codec_ctx->width;
}

/**
 * @brief Gets the height of the video frames.
 *
 * @return The height of the video frames in pixels.
 */
int VideoDecoder::getHeight() const {
    return codec_ctx->height;
}

/**
 * @brief Gets the pixel format of the video frames.
 *
 * @return The pixel format used for the video frames.
 */
AVPixelFormat VideoDecoder::getPixelFormat() const {
    return codec_ctx->pix_fmt;
}
