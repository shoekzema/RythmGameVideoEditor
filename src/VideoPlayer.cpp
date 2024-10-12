#include "VideoPlayer.h"

/**
 * @brief Constructs a VideoPlayer for the specified video file.
 *
 * This constructor initializes a VideoDecoder to decode the video file
 * specified by the given filename. It also initializes a VideoRenderer
 * for rendering the video frames. If the decoder or renderer fails to
 * initialize, an error message is printed to the standard error output.
 *
 * @param filename The path to the video file to be played.
 */
VideoPlayer::VideoPlayer(const char* filename) {
    // Initialize the video decoder with the specified filename
    decoder = new VideoDecoder(filename);
    if (!decoder->open()) {
        throw std::runtime_error("Failed to open video file");
    }

    // Initialize the video renderer with the dimensions of the video
    renderer = new VideoRenderer(decoder->getWidth(), decoder->getHeight());
    if (!renderer->init()) {
        throw std::runtime_error("Failed to initialize video renderer");
    }

    // Set up the scaling context for converting frames to RGB format
    sws_ctx = sws_getContext(
        decoder->getWidth(), decoder->getHeight(), 
        decoder->getPixelFormat(), // Source pixel format
        decoder->getWidth(), decoder->getHeight(), 
        AV_PIX_FMT_RGB24, // Destination pixel format
        SWS_BILINEAR, NULL, NULL, NULL);
}

/**
 * @brief Destroys the VideoPlayer and frees associated resources.
 */
VideoPlayer::~VideoPlayer() {
    delete decoder;    // Free memory allocated for the video decoder
    delete renderer;   // Free memory allocated for the video renderer
    sws_freeContext(sws_ctx); // Free the scaling context
}

/**
 * @brief Plays the video by decoding frames and rendering them.
 *
 * This method continuously retrieves video frames from the decoder,
 * converts them to RGB format, and renders them using the renderer.
 * It also handles SDL events to allow the user to close the window.
 * The playback continues until the video ends or the user requests to quit.
 */
void VideoPlayer::play() {
    // Allocate an AVFrame for decoding
    AVFrame* frame = av_frame_alloc();

    // Create a Frame object to hold the RGB data (3 bytes per pixel for RGB24)
    Frame rgb_frame(decoder->getWidth(), decoder->getHeight(), decoder->getWidth() * 3);

    bool quit = false;
    // Loop until the user requests to quit or no more frames are available
    while (!quit && decoder->getNextFrame(frame)) {
        // Convert the decoded frame to RGB format
        uint8_t* rgb_data[1] = { rgb_frame.data.data() };  // Create a pointer array from the vector's data
        sws_scale(sws_ctx, (uint8_t const* const*)frame->data, frame->linesize, 0, decoder->getHeight(), rgb_data, &rgb_frame.linesize);

        // Render the converted RGB frame
        renderer->renderFrame(rgb_frame);

        // Handle SDL events (e.g., check if the user closed the window)
        renderer->handleEvents(quit);
    }

    av_frame_free(&frame); // Free the allocated AVFrame
}
