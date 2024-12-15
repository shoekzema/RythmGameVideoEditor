#include <iostream>
#include "VideoPlayer.h"

VideoPlayer::VideoPlayer(int x, int y, int w, int h, SDL_Renderer* renderer, EventManager* eventManager, Segment* parent, SDL_Color color)
    : Segment(x, y, w, h, renderer, eventManager, parent, color)
{
    setVideoRect(&rect);

    // Initialize SDL audio device and resampler (SwrContext)
    SDL_zero(m_audioSpec);
    m_audioSpec.freq = 44100;
    m_audioSpec.format = AUDIO_S16SYS; // 16-bit signed audio format
    m_audioSpec.channels = 2;
    m_audioSpec.samples = 1024;
    m_audioSpec.callback = nullptr; // Use SDL_QueueAudio instead

    // Open audio device
    m_audioDevice = SDL_OpenAudioDevice(nullptr, 0, &m_audioSpec, nullptr, 0);
    if (m_audioDevice == 0) {
        std::cerr << "Error opening audio device: " << SDL_GetError() << std::endl;
    }

    // Allocate buffer for converted audio
    // Use formula: bufferSize = sampleRate * bytesPerSample * channels * desiredLatencyInSeconds
    // Example based on wanted audioSpec: 44100 (Hz) * 2 (16-bit) * 2 (stereo) * 1/60 (60 fps) = 2940 bytes
    // Double the buffer size just in case to prevent problems with lag
    m_audioBufferSize = (44100 / 60 * 2 * 2) * 2;
    m_audioBuffer = static_cast<uint8_t*>(av_malloc(m_audioBufferSize));
}

VideoPlayer::~VideoPlayer() {
    SDL_CloseAudioDevice(m_audioDevice);
    av_free(m_audioBuffer);
}

void VideoPlayer::render() {
    SDL_SetRenderDrawColor(p_renderer, p_color.r, p_color.g, p_color.b, p_color.a);
    SDL_RenderFillRect(p_renderer, &rect); // Draw background

    if (m_timeline) {
        SDL_SetRenderDrawColor(p_renderer, 0, 0, 0, 255); // black
        SDL_RenderFillRect(p_renderer, &m_videoRect); // Draw empty frame

        renderTimeline();
    }
    else {
        // Find the root segment
        Segment* rootSegment = this;
        while (rootSegment->parent) {
            rootSegment = rootSegment->parent;
        }
        // Find the timeline segment and save it
        m_timeline = rootSegment->findType<Timeline>();
    }
}

void VideoPlayer::handleEvent(SDL_Event& event) { }

void VideoPlayer::update(int x, int y, int w, int h) {
    rect = { x, y, w, h };
    setVideoRect(&rect);
}

void VideoPlayer::setVideoRect(SDL_Rect* rect) {
    // Set the video display Rect. Keeps the video resolution, regardless of segment proportions
    m_videoRect = *rect;

    // If the segment's height to width ratio is longer than the video, reshape the height and ypos
    if (rect->w * m_WtoH_ratioH < rect->h * m_WtoH_ratioW) { // for 16:9 ratio
        m_videoRect.h = rect->w * m_WtoH_ratioH / m_WtoH_ratioW;
        m_videoRect.y = rect->y + (rect->h - m_videoRect.h) / 2;
    }
    // If the segment's width to height ratio is wider than the video, reshape the width and xpos
    if (rect->w * m_WtoH_ratioH > rect->h * m_WtoH_ratioW) { // for 16:9 ratio
        m_videoRect.w = rect->h * m_WtoH_ratioW / m_WtoH_ratioH;
        m_videoRect.x = rect->x + (rect->w - m_videoRect.w) / 2;
    }
}

void VideoPlayer::renderTimeline() {
    // Get the current video segment from the timeline
    VideoSegment* currentVideoSegment = m_timeline->getCurrentVideoSegment();

    if (currentVideoSegment) {
        renderFrame(currentVideoSegment);
    }

    if (m_timeline->isPlaying()) {
        playAudio();
    }
    else {
        pauseAudioPlayback();
    }
}

void VideoPlayer::playAudio() {
    // Get the current audio segment (if applicable)
    AudioSegment* currentAudioSegment = m_timeline->getCurrentAudioSegment();
    if (!currentAudioSegment) {
        // Pause audio if no audio segments found at the current timeline position.
        SDL_PauseAudioDevice(m_audioDevice, 1);
        return;
    }

    playAudioSegment(currentAudioSegment);
}

void VideoPlayer::pauseAudioPlayback() {
    SDL_PauseAudioDevice(m_audioDevice, 1);

    // Pause or stop any other playback actions as needed
    m_lastVideoSegment = nullptr;
    m_lastAudioSegment = nullptr;
}

void VideoPlayer::renderFrame(VideoSegment* videoSegment) {
    Uint32 currentVideoSegmentFrame = m_timeline->getCurrentTime() - videoSegment->timelinePosition;

    if (currentVideoSegmentFrame != m_lastVideoSegmentFrame) {
        // Get and decode the video frame at the corresponding time in the segment
        if (!getVideoFrame(videoSegment)) {
            std::cerr << "Failed to retrieve video frame during playback." << std::endl;
            return;
        }
        m_lastVideoSegmentFrame = currentVideoSegmentFrame;
    }

    renderFrameToScreen(videoSegment);
}

void VideoPlayer::renderFrameToScreen(VideoSegment* videoSegment) {
    if (!videoSegment->videoData->rgbFrame) return;

    static int s_lastFrameWidth = 0, s_lastFrameHeight = 0;

    int frameWidth = videoSegment->videoData->codecContext->width;
    int frameHeight = videoSegment->videoData->codecContext->height;

    // Create an SDL texture if not already created or if size has changed
    if (!m_videoTexture || frameWidth != s_lastFrameWidth || frameHeight != s_lastFrameHeight) {
        if (m_videoTexture) SDL_DestroyTexture(m_videoTexture);  // Free existing texture
        m_videoTexture = SDL_CreateTexture(
            p_renderer,
            SDL_PIXELFORMAT_RGB24,
            SDL_TEXTUREACCESS_STREAMING,
            frameWidth,
            frameHeight
        );
        if (!m_videoTexture) {
            std::cerr << "Failed to create SDL texture: " << SDL_GetError() << std::endl;
            return;
        }
        s_lastFrameWidth = frameWidth;
        s_lastFrameHeight = frameHeight;
    }

    // Copy frame data to the texture
    SDL_UpdateTexture(
        m_videoTexture,
        nullptr,
        videoSegment->videoData->rgbFrame->data[0],
        videoSegment->videoData->rgbFrame->linesize[0]
    );

    SDL_Rect destRect = m_videoRect;

    // Scale to fit within m_videoRect by maintaining aspect ratio
    if (frameWidth * m_videoRect.h > frameHeight * m_videoRect.w) {
        // Fit to width
        destRect.w = m_videoRect.w;
        destRect.h = (frameHeight * m_videoRect.w) / frameWidth;
        destRect.y = m_videoRect.y + (m_videoRect.h - destRect.h) / 2; // Center vertically
    }
    else {
        // Fit to height
        destRect.h = m_videoRect.h;
        destRect.w = (frameWidth * m_videoRect.h) / frameHeight;
        destRect.x = m_videoRect.x + (m_videoRect.w - destRect.w) / 2; // Center horizontally
    }

    // Render the texture
    SDL_RenderCopy(p_renderer, m_videoTexture, nullptr, &destRect);
}

bool VideoPlayer::getVideoFrame(VideoSegment* videoSegment) {
    if (!videoSegment) {
        std::cerr << "Invalid video segment" << std::endl;
        return false;
    }

    Uint32 currentFrame = m_timeline->getCurrentTime() - videoSegment->timelinePosition;

    // Check if we need to seek
    bool isNewSegment = m_lastVideoSegment != videoSegment;
    bool isPausedAndFrameChanged = !m_timeline->isPlaying() && currentFrame != m_lastVideoSegmentFrame;
    bool isPlayingAndFrameBehind =  m_timeline->isPlaying() && currentFrame < m_lastVideoSegmentFrame;
    bool isPlayingAndFrameAhead  =  m_timeline->isPlaying() && currentFrame > m_lastVideoSegmentFrame + m_framebehindSeekThreshold;

    if (isNewSegment || isPausedAndFrameChanged || isPlayingAndFrameBehind || isPlayingAndFrameAhead) {
        // Get the timestamp in the stream's time base
        AVRational timeBase = videoSegment->videoData->formatContext->streams[videoSegment->videoData->streamIndex]->time_base;
        
        Uint32 targetFrame = videoSegment->sourceStartTime;
        // Convert the desired frame number to a timestamp
        int64_t targetTimestamp  = av_rescale_q(targetFrame,  videoSegment->fps,          timeBase);
        int64_t currentTimestamp = av_rescale_q(currentFrame, { 1, m_timeline->getFPS()}, timeBase);

        if (currentTimestamp > targetTimestamp) {
            targetTimestamp = currentTimestamp;
        }

        if (av_seek_frame(videoSegment->videoData->formatContext, videoSegment->videoData->streamIndex, targetTimestamp, AVSEEK_FLAG_BACKWARD) < 0) {
            std::cerr << "Error seeking video to timestamp: " << targetTimestamp << std::endl;
            return false;
        }

        // Flush the codec context buffers to clear any data from previous frames.
        avcodec_flush_buffers(videoSegment->videoData->codecContext);

        m_lastVideoSegment = videoSegment;
    }

    // Decode and display the frame
    return decodeAndProcessFrame(videoSegment, currentFrame);
}

bool VideoPlayer::decodeAndProcessFrame(VideoSegment* videoSegment, Uint32 currentFrame) {
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
                if (processFrame(videoSegment, currentFrame)) {
                    av_packet_unref(&packet); // Free the packet
                    return true; // Successfully got the frame
                }
            }
        }
        av_packet_unref(&packet); // Free the packet
    }
    return false; // No frame found or error occurred
}

bool VideoPlayer::processFrame(VideoSegment* videoSegment, Uint32 currentFrame) {
    auto stream = videoSegment->videoData->formatContext->streams[videoSegment->videoData->streamIndex];

    // Calculate the frame's presentation timestamp in frames (frame index)
    double framePTS = videoSegment->videoData->frame->pts * av_q2d(stream->time_base);

    // Convert framePTS to frames
    framePTS *= stream->r_frame_rate.num / stream->r_frame_rate.den;

    // Adjust the frame's timing based on the target frame rate (newPTS = PTS * targetFPS / videoFPS)
    double adjustedFramePTS = framePTS * m_timeline->getFPS() / av_q2d(videoSegment->fps);

    Uint32 currentPlaybackFrame = m_timeline->getCurrentTime() - videoSegment->timelinePosition;

    // Check if the frame is too late
    if (adjustedFramePTS + m_frameDropThreshold < currentPlaybackFrame) {
        return false; // Skip this frame
    }

    // Check if the frame is too early
    if (adjustedFramePTS > currentPlaybackFrame) {
        double frameTimeDifference = adjustedFramePTS - currentPlaybackFrame;
        double targetDelayTime = frameTimeDifference / m_timeline->getFPS(); // Time delay in seconds
        SDL_Delay(static_cast<Uint32>(targetDelayTime * 1000));  // Delay in milliseconds
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

    return true;
}

void VideoPlayer::playAudioSegment(AudioSegment* audioSegment) {
    if (!audioSegment) {
        std::cerr << "Invalid audio segment" << std::endl;
        return;
    }

    Uint32 startFrameIndex = audioSegment->sourceStartTime;
    Uint32 endFrameIndex = startFrameIndex + audioSegment->duration;

    // Seek to the beginning of the segment if this is a new segment
    if (m_lastAudioSegment != audioSegment) {
        // Get the timestamp in the stream's time base
        AVRational timeBase = audioSegment->audioData->formatContext->streams[audioSegment->audioData->streamIndex]->time_base;

        Uint32 currentFrame = m_timeline->getCurrentTime() - audioSegment->timelinePosition;
        // Convert the desired frame number to a timestamp
        int64_t startTimestamp   = av_rescale_q(startFrameIndex, { 1, m_timeline->getFPS() }, timeBase);
        int64_t currentTimestamp = av_rescale_q(currentFrame,    { 1, m_timeline->getFPS() }, timeBase);

        if (currentTimestamp > startTimestamp) {
            startTimestamp = currentTimestamp;
        }

        if (av_seek_frame(audioSegment->audioData->formatContext, audioSegment->audioData->streamIndex, startTimestamp, AVSEEK_FLAG_BACKWARD) < 0) {
            std::cerr << "Error seeking audio to timestamp: " << startTimestamp << std::endl;
            return;
        }

        // Flush the codec context buffers to clear any data from previous audio frames.
        avcodec_flush_buffers(audioSegment->audioData->codecContext);

        // Clear the audio queue
        SDL_ClearQueuedAudio(m_audioDevice);
    }

    // Update last segment pointer
    m_lastAudioSegment = audioSegment;

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

                // Calculate the frame's presentation timestamp (in frame index)
                int64_t currentFrameIndex = audioSegment->audioData->frame->pts;

                if (currentFrameIndex > endFrameIndex) break;

                // Check if the audio frame is valid
                if (!audioSegment->audioData->frame || !audioSegment->audioData->frame->data[0]) {
                    std::cerr << "Invalid audio frame" << std::endl;
                    return;
                }

                // Resample and convert audio to SDL format
                int numSamples = swr_convert(audioSegment->audioData->swrContext,
                    &m_audioBuffer,                                          // output buffer
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
                if (SDL_QueueAudio(m_audioDevice, m_audioBuffer, bufferSize) < 0) {
                    std::cerr << "Error queueing audio: " << SDL_GetError() << std::endl;
                }

                // Check if the audio segment duration is reached
                if (currentFrameIndex >= endFrameIndex) {
                    SDL_PauseAudioDevice(m_audioDevice, 1); // Pause audio if end of segment is reached
                    break;
                }
                else {
                    SDL_PauseAudioDevice(m_audioDevice, 0); // Unpause audio
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
