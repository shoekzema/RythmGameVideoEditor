#include "Segment.h"

VideoPlayer::VideoPlayer(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Segment* parent, SDL_Color color)
    : Segment(x, y, w, h, renderer, eventManager, parent, color), videoData(nullptr), videoTexture(nullptr), timeline(nullptr), audioBuffer(nullptr), audioBufferSize(0)
{
    // Subscribe to the VideoSelected event
    eventManager->subscribe(EventType::VideoSelected, [this](VideoData* videoData) {
        loadAndPlayVideo(videoData);
    });

    // Initialize SDL audio device and resampler (SwrContext)
    SDL_zero(audioSpec);
    audioSpec.freq = 44100;
    audioSpec.format = AUDIO_S16SYS; // 16-bit signed audio format
    audioSpec.channels = 2;
    audioSpec.samples = 1024;
    audioSpec.callback = nullptr; // Use SDL_QueueAudio instead

    // Open audio device
    audioDevice = SDL_OpenAudioDevice(nullptr, 0, &audioSpec, nullptr, 0);
    if (audioDevice == 0) {
        std::cerr << "Error opening audio device: " << SDL_GetError() << std::endl;
        return;
    }

    // Allocate buffer for converted audio
    audioBufferSize = 192000; // example size, adjust as necessary
    audioBuffer = static_cast<uint8_t*>(av_malloc(audioBufferSize));
}

VideoPlayer::~VideoPlayer() {
    SDL_CloseAudioDevice(audioDevice);
    av_free(audioBuffer);
}

void VideoPlayer::loadAndPlayVideo(VideoData* videoData) {
    if (videoData) {
        VideoPlayer::videoData = videoData;

        // Set frame timing based on video FPS
        frameDurationMs = (1.0 / videoData->codecContext->framerate.num) * 1000.0; // ms per frame
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
        else {
            SDL_PauseAudioDevice(audioDevice, 1);

            // Pause or stop any other playback actions as needed
            lastVideoSegment = nullptr;
            lastAudioSegment = nullptr;
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
        if (packet.stream_index == videoData->streamIndex) {
            // Send the packet to the codec for decoding
            avcodec_send_packet(videoData->codecContext, &packet);

            // Receive the decoded frame from the codec
            int ret = avcodec_receive_frame(videoData->codecContext, videoData->frame);
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
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, videoData->codecContext->width, videoData->codecContext->height, 1);
    uint8_t* buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));

    av_image_fill_arrays(videoData->rgbFrame->data, videoData->rgbFrame->linesize, buffer, AV_PIX_FMT_RGB24, videoData->codecContext->width, videoData->codecContext->height, 1);
    sws_scale(videoData->swsContext, frame->data, frame->linesize, 0, videoData->codecContext->height, videoData->rgbFrame->data, videoData->rgbFrame->linesize);

    // Update the SDL texture with the new frame
    if (!videoTexture) {
        videoTexture = SDL_CreateTexture(renderer,
            SDL_PIXELFORMAT_RGB24,
            SDL_TEXTUREACCESS_STREAMING,
            videoData->codecContext->width,
            videoData->codecContext->height);
    }

    // Lock the texture to update pixel data
    void* pixels;
    int pitch;
    SDL_LockTexture(videoTexture, NULL, &pixels, &pitch);

    for (int y = 0; y < videoData->codecContext->height; y++) {
        memcpy((uint8_t*)pixels + y * pitch, videoData->rgbFrame->data[0] + y * videoData->rgbFrame->linesize[0], videoData->codecContext->width * 3);
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

void VideoPlayer::playTimeline(Timeline* timeline) {
    // Get the current video segment from the timeline
    VideoSegment* currentVideoSegment = timeline->getCurrentVideoSegment();
    if (currentVideoSegment) {
        // Get and decode the video frame at the corresponding time in the segment
        if (!getVideoFrame(currentVideoSegment)) {
            std::cerr << "Failed to retrieve video frame." << std::endl;
            return;
        }

        // Display the video frame using SDL
        if (currentVideoSegment->videoData->rgbFrame) {
            // Create an SDL texture if not already created
            if (!videoTexture) {
                videoTexture = SDL_CreateTexture(
                    renderer,
                    SDL_PIXELFORMAT_RGB24,
                    SDL_TEXTUREACCESS_STREAMING,
                    currentVideoSegment->videoData->codecContext->width,
                    currentVideoSegment->videoData->codecContext->height
                );
                if (!videoTexture) {
                    std::cerr << "Failed to create SDL texture: " << SDL_GetError() << std::endl;
                    return;
                }
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

    // Get the current audio segment (if applicable)
    AudioSegment* currentAudioSegment = timeline->getCurrentAudioSegment();
    if (!currentAudioSegment) {
        //std::cout << "No audio segment found at the current timeline position." << std::endl;
        SDL_PauseAudioDevice(audioDevice, 1); // Pause audio if no audio segments found at the current timeline position.
    }
    else {
        playAudioSegment(currentAudioSegment);
    }
}

bool VideoPlayer::getVideoFrame(VideoSegment* videoSegment) {
    if (!videoSegment) {
        std::cerr << "Invalid video segment" << std::endl;
        return false;
    }

    // Seek to the beginning of the segment if this is a new segment
    if (lastVideoSegment != videoSegment) {
        // Get the timestamp in the stream's time base
        AVRational timeBase = videoSegment->videoData->formatContext->streams[videoSegment->videoData->streamIndex]->time_base;
        int64_t targetTimestamp = (int64_t)(videoSegment->sourceStartTime / av_q2d(timeBase));

        if (av_seek_frame(videoSegment->videoData->formatContext, videoSegment->videoData->streamIndex, targetTimestamp, AVSEEK_FLAG_BACKWARD) < 0) {
            std::cerr << "Error seeking video to timestamp: " << videoSegment->sourceStartTime << " seconds." << std::endl;
            return false;
        }

        // Flush the codec context buffers to clear any data from previous frames.
        avcodec_flush_buffers(videoSegment->videoData->codecContext);
    }

    // Update last segment pointer
    lastVideoSegment = videoSegment;

    AVPacket packet;
    while (av_read_frame(videoSegment->videoData->formatContext, &packet) >= 0) {
        if (packet.stream_index == videoSegment->videoData->streamIndex) {
            // Send packet to the decoder
            if (avcodec_send_packet(videoSegment->videoData->codecContext, &packet) != 0) {
                av_packet_unref(&packet);
                continue;
            }

            // Receive the frame from the decoder
            if (avcodec_receive_frame(videoSegment->videoData->codecContext, videoSegment->videoData->frame) == 0) {
                // Calculate the frame's presentation timestamp in seconds
                double framePTS = videoSegment->videoData->frame->pts *
                    av_q2d(videoSegment->videoData->formatContext->streams[videoSegment->videoData->streamIndex]->time_base);

                // Check if the frame is too late
                double currentPlaybackTime = timeline->getCurrentTime() - videoSegment->timelinePosition;
                if (framePTS < currentPlaybackTime - frameDropThreshold) {
                    av_packet_unref(&packet);
                    continue; // Skip this frame
                }

                // Allocate buffer for rgbFrame if not already done
                if (!videoSegment->videoData->rgbFrame->data[0]) {
                    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24,
                        videoSegment->videoData->codecContext->width,
                        videoSegment->videoData->codecContext->height, 1);
                    uint8_t* buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
                    av_image_fill_arrays(videoSegment->videoData->rgbFrame->data,
                        videoSegment->videoData->rgbFrame->linesize,
                        buffer, AV_PIX_FMT_RGB24,
                        videoSegment->videoData->codecContext->width,
                        videoSegment->videoData->codecContext->height, 1);
                }

                // Convert the frame from YUV to RGB
                sws_scale(videoSegment->videoData->swsContext,
                    videoSegment->videoData->frame->data,
                    videoSegment->videoData->frame->linesize,
                    0,
                    videoSegment->videoData->codecContext->height,
                    videoSegment->videoData->rgbFrame->data,
                    videoSegment->videoData->rgbFrame->linesize);

                av_packet_unref(&packet);  // Free the packet
                return true; // Successfully got the frame
            }
        }
        av_packet_unref(&packet);  // Free the packet
    }
    return false; // No frame found or error occurred
}

void VideoPlayer::playAudioSegment(AudioSegment* audioSegment) {
    if (!audioSegment) {
        std::cerr << "Invalid audio segment" << std::endl;
        return;
    }

    double startTime = audioSegment->sourceStartTime;
    double endTime = startTime + audioSegment->duration;

    // Seek to the beginning of the segment if this is a new segment
    if (lastAudioSegment != audioSegment) {
        // Get the timestamp in the stream's time base
        AVRational timeBase = audioSegment->audioData->formatContext->streams[audioSegment->audioData->streamIndex]->time_base;
        int64_t targetTimestamp = (int64_t)(startTime / av_q2d(timeBase));

        if (av_seek_frame(audioSegment->audioData->formatContext, audioSegment->audioData->streamIndex, targetTimestamp, AVSEEK_FLAG_BACKWARD) < 0) {
            std::cerr << "Error seeking audio to timestamp: " << startTime << " seconds." << std::endl;
            return;
        }

        // Flush the codec context buffers to clear any data from previous audio frames.
        avcodec_flush_buffers(audioSegment->audioData->codecContext);

        // Clear the audio queue
        SDL_ClearQueuedAudio(audioDevice);
    }

    // Update last segment pointer
    lastAudioSegment = audioSegment;

    // Decode audio frames and play them until reaching the end of the segment duration
    AVPacket packet;
    while (av_read_frame(audioSegment->audioData->formatContext, &packet) >= 0) {
        if (packet.stream_index == audioSegment->audioData->streamIndex) {
            // Send packet to the decoder
            int ret = avcodec_send_packet(audioSegment->audioData->codecContext, &packet);
            if (ret < 0) break;
            while (ret >= 0) {
                // Receive the audio frame from the decoder
                ret = avcodec_receive_frame(audioSegment->audioData->codecContext, audioSegment->audioData->frame);

                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
                if (ret < 0) return;

                double currentTime = audioSegment->audioData->frame->pts * av_q2d(audioSegment->audioData->formatContext->streams[audioSegment->audioData->streamIndex]->time_base);
                if (currentTime > endTime) break;

                // Check if the audio frame is valid
                if (!audioSegment->audioData->frame || !audioSegment->audioData->frame->data[0]) {
                    std::cerr << "Invalid audio frame" << std::endl;
                    return;
                }

                // Resample and convert audio to SDL format
                int numSamples = swr_convert(audioSegment->audioData->swrContext,
                    &audioBuffer,                                          // output buffer
                    audioSegment->audioData->frame->nb_samples * 2,        // number of samples to output (2 for stereo)
                    (const uint8_t**)audioSegment->audioData->frame->data, // input buffer
                    audioSegment->audioData->frame->nb_samples);           // number of input samples

                if (numSamples < 0) {
                    std::cerr << "Error in resampling audio" << std::endl;
                    return;
                }

                int bufferSize = numSamples * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16) * 2; // 2 channels (assuming stereo)

                // Ensure the buffer is valid before queueing
                if (bufferSize <= 0) {
                    std::cerr << "Buffer size is 0, not queuing audio." << std::endl;
                }

                // Queue audio data to SDL
                if (SDL_QueueAudio(audioDevice, audioBuffer, bufferSize) < 0) {
                    std::cerr << "Error queueing audio: " << SDL_GetError() << std::endl;
                }

                // Check if the audio segment duration is reached
                if (currentTime >= endTime) {
                    SDL_PauseAudioDevice(audioDevice, 1); // Pause audio if end of segment is reached
                    break;
                }
                else {
                    SDL_PauseAudioDevice(audioDevice, 0); // Unpause audio
                }
            }
        }
        av_packet_unref(&packet);
    }
    return;
}

Segment* VideoPlayer::findTypeImpl(const std::type_info& type) {
    if (type == typeid(VideoPlayer)) {
        return this;
    }
    return nullptr;
}
