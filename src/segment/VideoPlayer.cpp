#include "Segment.h"

VideoPlayer::VideoPlayer(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Segment* parent, SDL_Color color)
    : Segment(x, y, w, h, renderer, eventManager, parent, color), videoData(nullptr), videoTexture(nullptr), timeline(nullptr), audioBuffer(nullptr), audioBufferSize(0)
{
    // Subscribe to the VideoSelected event
    eventManager->subscribe(EventType::VideoSelected, [this](VideoData* videoData) {
        loadAndPlayVideo(videoData);
    });

    // Initialize SDL audio device and resampler (SwrContext)
    SDL_AudioSpec audioSpec;
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
    SDL_PauseAudioDevice(audioDevice, 0);

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
    int64_t targetTimestamp = (int64_t)(timeInSeconds / av_q2d(timeBase));

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
    // Get the timestamp in the stream's time base
    AVRational timeBase = audioSegment->audioData->formatContext->streams[audioSegment->audioData->audioStreamIndex]->time_base;
    int64_t targetTimestamp = (int64_t)(timeInSeconds / av_q2d(timeBase));

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

            //SDL_ClearQueuedAudio(audioDevice);

            // Receive the audio frame from the decoder
            while (avcodec_receive_frame(audioSegment->audioData->audioCodecContext, audioSegment->audioData->audioFrame) == 0) {
                // The decoded audio is in audioData->audioFrame->data
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
    }
    else {
        // Calculate the current time within the segment
        double currentTimeInSegment = currentVideoSegment->sourceStartTime + timeline->getCurrentTime() - currentVideoSegment->timelinePosition;

        // Get and decode the video frame at the corresponding time in the segment
        if (!getVideoFrameAtTime(currentTimeInSegment, currentVideoSegment)) {
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
                    currentVideoSegment->videoData->videoCodecContext->width,
                    currentVideoSegment->videoData->videoCodecContext->height
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
        std::cerr << "No audio segment found at the current timeline position." << std::endl;
    }
    else {
        // Calculate the current time within the segment
        double currentTimeInSegment = currentAudioSegment->sourceStartTime + timeline->getCurrentTime() - currentAudioSegment->timelinePosition;

        // Get and decode the audio frame (if applicable)
        if (getAudioFrameAtTime(currentAudioSegment->sourceStartTime + currentTimeInSegment, currentAudioSegment) < 0) {
            std::cerr << "Failed to retrieve audio frame." << std::endl;
            return;
        }
        // Queue audio data for playback
        playAudioFrame(currentAudioSegment->audioData);
    }
}

Segment* VideoPlayer::findTypeImpl(const std::type_info& type) {
    if (type == typeid(VideoPlayer)) {
        return this;
    }
    return nullptr;
}

void VideoPlayer::playAudioFrame(VideoData* audioData) {
    // Check if the audio frame is valid
    if (!audioData || !audioData->audioFrame || !audioData->audioFrame->data[0]) {
        std::cerr << "Invalid audio frame" << std::endl;
        return;
    }

    // Resample and convert audio to SDL format
    int numSamples = swr_convert(audioData->swrCtx,
        &audioBuffer,                 // output buffer
        audioData->audioFrame->nb_samples * 2,       // number of samples to output (2 for stereo)
        (const uint8_t**)audioData->audioFrame->data, // input buffer
        audioData->audioFrame->nb_samples);      // number of input samples

    if (numSamples < 0) {
        std::cerr << "Error in resampling audio" << std::endl;
        return;
    }

    int bufferSize = numSamples * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16) * 2; // 2 channels (assuming stereo)

    // Ensure the buffer is valid before queueing
    if (bufferSize <= 0) {
        std::cerr << "Buffer size is 0, not queuing audio." << std::endl;
    }

    //SDL_ClearQueuedAudio(audioDevice);

    // Queue audio data to SDL
    if (SDL_QueueAudio(audioDevice, audioBuffer, bufferSize) < 0) {
        std::cerr << "Error queueing audio: " << SDL_GetError() << std::endl;
    }
}

