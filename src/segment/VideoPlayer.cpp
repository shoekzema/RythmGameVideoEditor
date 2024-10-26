#include "Segment.h"

VideoPlayer::VideoPlayer(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Segment* parent, SDL_Color color)
    : Segment(x, y, w, h, renderer, eventManager, parent, color), videoData(nullptr), videoTexture(nullptr), timeline(nullptr)
{
    // Subscribe to the VideoSelected event
    eventManager->subscribe(EventType::VideoSelected, [this](VideoData* videoData) {
        loadAndPlayVideo(videoData);
    });
}

VideoPlayer::~VideoPlayer() {
}

void VideoPlayer::loadAndPlayVideo(VideoData* videoData) {
    if (videoData) {
        VideoPlayer::videoData = videoData;

        // Set frame timing based on video FPS
        frameDurationMs = (1.0 / videoData->videoCodecContext->framerate.num) * 1000.0; // ms per frame
        lastFrameTime = 0; //SDL_GetTicks(); // Record the start time
        playing = true;

        playVideo();
    }
}

void VideoPlayer::playVideo() {
    playing = true;
}

void VideoPlayer::render() {
    if (timeline) {
        if (timeline->playing) {
            playTimeline(timeline);
        }
    }
    else {
        // Find the root segment
        Segment* rootSegment = this;
        while (rootSegment->parent) {
            rootSegment = rootSegment->parent;
        }
        // Find the timeline segment and save it
        timeline = rootSegment->findType<Timeline>();
    }
}

AVFrame* VideoPlayer::getNextFrame() {
    AVPacket packet;
    int frameFinished = 0;

    while (av_read_frame(videoData->formatContext, &packet) >= 0) {
        if (packet.stream_index == videoData->videoStreamIndex) {
            // Send the packet to the codec for decoding
            avcodec_send_packet(videoData->videoCodecContext, &packet);

            // Receive the decoded frame from the codec
            int ret = avcodec_receive_frame(videoData->videoCodecContext, videoData->frame);
            if (ret >= 0) {
                av_packet_unref(&packet);
                return videoData->frame;  // Return the decoded frame
            }
        }
        av_packet_unref(&packet);
    }

    return nullptr; // No more frames available
}

void VideoPlayer::updateTextureFromFrame(AVFrame* frame) {
    // Convert the frame to RGB (similar to your original method)
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, videoData->videoCodecContext->width, videoData->videoCodecContext->height, 1);
    uint8_t* buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));

    av_image_fill_arrays(videoData->rgbFrame->data, videoData->rgbFrame->linesize, buffer, AV_PIX_FMT_RGB24, videoData->videoCodecContext->width, videoData->videoCodecContext->height, 1);
    sws_scale(videoData->swsContext, frame->data, frame->linesize, 0, videoData->videoCodecContext->height, videoData->rgbFrame->data, videoData->rgbFrame->linesize);

    // Update the SDL texture with the new frame
    if (!videoTexture) {
        videoTexture = SDL_CreateTexture(renderer,
            SDL_PIXELFORMAT_RGB24,
            SDL_TEXTUREACCESS_STREAMING,
            videoData->videoCodecContext->width,
            videoData->videoCodecContext->height);
    }

    // Lock the texture to update pixel data
    void* pixels;
    int pitch;
    SDL_LockTexture(videoTexture, NULL, &pixels, &pitch);

    for (int y = 0; y < videoData->videoCodecContext->height; y++) {
        memcpy((uint8_t*)pixels + y * pitch, videoData->rgbFrame->data[0] + y * videoData->rgbFrame->linesize[0], videoData->videoCodecContext->width * 3);
    }

    SDL_UnlockTexture(videoTexture);
}

void VideoPlayer::handleEvent(SDL_Event& event) {
    switch (event.type) {
    case SDL_MOUSEBUTTONDOWN:
        if (event.button.button == SDL_BUTTON_LEFT) {
            SDL_Point mouseButton = { event.button.x, event.button.y };

            // If not in this Segment, we ignore it 
            if (SDL_PointInRect(&mouseButton, &rect)) {
                if (playing) playing = false;
                else playing = true;
            }
        }
        break;
    }
}

void VideoPlayer::update(int x, int y, int w, int h) {
    rect = { x, y, w, h };
}

bool VideoPlayer::getVideoFrameAtTime(double timeInSeconds, VideoSegment* videoSegment) {
    // Get the timestamp in the stream's time base
    AVRational timeBase = videoSegment->videoData->formatContext->streams[videoSegment->videoData->videoStreamIndex]->time_base;
    int64_t targetTimestamp = timeInSeconds / av_q2d(timeBase);

    if (av_seek_frame(videoSegment->videoData->formatContext, videoSegment->videoData->videoStreamIndex, targetTimestamp, AVSEEK_FLAG_BACKWARD) < 0) {
        std::cerr << "Error seeking to timestamp: " << timeInSeconds << " seconds." << std::endl;
        return false;
    }

    // Flush the codec context buffers to clear any data from previous frames.
    avcodec_flush_buffers(videoSegment->videoData->videoCodecContext);

    AVPacket packet;
    while (av_read_frame(videoSegment->videoData->formatContext, &packet) >= 0) {
        if (packet.stream_index == videoSegment->videoData->videoStreamIndex) {
            // Send packet to the decoder
            if (avcodec_send_packet(videoSegment->videoData->videoCodecContext, &packet) != 0) {
                av_packet_unref(&packet);
                continue;
            }

            // Receive the frame from the decoder
            if (avcodec_receive_frame(videoSegment->videoData->videoCodecContext, videoSegment->videoData->frame) == 0) {
                // Allocate buffer for rgbFrame if not already done
                if (!videoSegment->videoData->rgbFrame->data[0]) {
                    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24,
                        videoSegment->videoData->videoCodecContext->width,
                        videoSegment->videoData->videoCodecContext->height, 1);
                    uint8_t* buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
                    av_image_fill_arrays(videoSegment->videoData->rgbFrame->data,
                        videoSegment->videoData->rgbFrame->linesize,
                        buffer, AV_PIX_FMT_RGB24,
                        videoSegment->videoData->videoCodecContext->width,
                        videoSegment->videoData->videoCodecContext->height, 1);
                }

                // Convert the frame from YUV to RGB
                sws_scale(videoSegment->videoData->swsContext,
                    videoSegment->videoData->frame->data,
                    videoSegment->videoData->frame->linesize,
                    0, 
                    videoSegment->videoData->videoCodecContext->height,
                    videoSegment->videoData->rgbFrame->data,
                    videoSegment->videoData->rgbFrame->linesize);

                // Now, videoData->rgbFrame contains the RGB frame at the desired time
                // Process or render this frame as needed...

                av_packet_unref(&packet);  // Free the packet
                return true; // Successfully got the frame
            }
        }
        av_packet_unref(&packet);  // Free the packet
    }
    return false; // No frame found or error occurred
}

int VideoPlayer::getAudioFrameAtTime(double timeInSeconds, AudioSegment* audioSegment) {
    int64_t targetTimestamp = timeInSeconds * AV_TIME_BASE;
    if (av_seek_frame(audioSegment->audioData->formatContext, audioSegment->audioData->audioStreamIndex, targetTimestamp, AVSEEK_FLAG_BACKWARD) < 0) {
        std::cerr << "Error seeking to timestamp: " << timeInSeconds << " seconds." << std::endl;
        return -1;
    }

    // Flush the codec context buffers to clear any data from previous frames.
    avcodec_flush_buffers(audioSegment->audioData->audioCodecContext);

    AVPacket packet;
    while (av_read_frame(audioSegment->audioData->formatContext, &packet) >= 0) {
        if (packet.stream_index == audioSegment->audioData->audioStreamIndex) {
            // Send packet to the decoder
            if (avcodec_send_packet(audioSegment->audioData->audioCodecContext, &packet) != 0) {
                av_packet_unref(&packet);
                continue;
            }

            // Receive the audio frame from the decoder
            while (avcodec_receive_frame(audioSegment->audioData->audioCodecContext, audioSegment->audioData->audioFrame) == 0) {
                // The decoded audio is in videoData->audioFrame->data
                // You can process or send this audio data to SDL2 for playback
                av_packet_unref(&packet);
                return 0; // Successfully got the audio frame
            }
        }
        av_packet_unref(&packet);
    }
    return -1; // No audio frame found or error occurred
}

void VideoPlayer::playTimeline(Timeline* timeline) {
    // Get the current video segment from the timeline
    VideoSegment* currentVideoSegment = timeline->getCurrentVideoSegment();
    if (!currentVideoSegment) {
        std::cerr << "No video segment found at the current timeline position." << std::endl;
        return;
    }

    // Get the current audio segment (if applicable)
    AudioSegment* currentAudioSegment = timeline->getCurrentAudioSegment();

    // Calculate the current time within the segment
    double currentTimeInSegment = currentVideoSegment->sourceStartTime + timeline->getCurrentTime() - currentVideoSegment->timelinePosition;

    // Get and decode the video frame at the corresponding time in the segment
    if (!getVideoFrameAtTime(currentTimeInSegment, currentVideoSegment)) {
        std::cerr << "Failed to retrieve video frame." << std::endl;
        return;
    }

    // Get and decode the audio frame (if applicable)
    if (currentAudioSegment && getAudioFrameAtTime(currentAudioSegment->sourceStartTime + currentTimeInSegment, currentAudioSegment) < 0) {
        std::cerr << "Failed to retrieve audio frame." << std::endl;
        return;
    }

    // Queue audio data for playback (if available)
    if (currentAudioSegment) {
        playAudioFrame(currentAudioSegment->audioData->audioFrame);
    }

    // Display the video frame using SDL
    if (currentVideoSegment->videoData->rgbFrame) {
        // Create an SDL texture if not already created
        if (!videoTexture) {
            videoTexture = SDL_CreateTexture(
                renderer,
                SDL_PIXELFORMAT_RGB24,
                SDL_TEXTUREACCESS_STREAMING,
                currentVideoSegment->videoData->videoCodecContext->width,
                currentVideoSegment->videoData->videoCodecContext->height
            );
        }

        // Copy frame data to the texture
        SDL_UpdateTexture(
            videoTexture,
            nullptr,
            currentVideoSegment->videoData->rgbFrame->data[0],
            currentVideoSegment->videoData->rgbFrame->linesize[0]
        );

        // Draw the texture
        SDL_RenderCopy(renderer, videoTexture, nullptr, &rect);
    }
}

Segment* VideoPlayer::findTypeImpl(const std::type_info& type) {
    if (type == typeid(VideoPlayer)) {
        return this;
    }
    return nullptr;
}

void VideoPlayer::playAudioFrame(AVFrame* audioFrame) {
    // Audio conversion and playback with SDL
    // For example, you can use SDL_QueueAudio() to queue decoded audio for playback
}

