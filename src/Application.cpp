#include <iostream>
#include "Application.h"
#include "util.h"
#include "segment/SegmentIncludes.h"
#include "PopupMenu.h"

Application::Application(int width, int height) : m_screenWidth(width), m_screenHeight(height) {
    if (init()) {
        SegmentHSplit* root = new SegmentHSplit(0, 0, m_screenWidth, m_screenHeight, m_renderer, &m_eventManager);
            SegmentVSplit* top = new SegmentVSplit(0, 0, m_screenWidth, m_screenHeight, m_renderer, &m_eventManager, root);
            root->setTopSegment(top);
                Segment* assetList   = new AssetsList(   0, 0, m_screenWidth, m_screenHeight, m_renderer, &m_eventManager, top);
                Segment* videoPlayer = new VideoPlayer(0, 0, m_screenWidth, m_screenHeight, m_renderer, &m_eventManager, top);
                top->setLeftSegment(assetList);
                top->setRightSegment(videoPlayer);
            Segment* timeLine = new Timeline(0, 0, m_screenWidth, m_screenHeight, m_renderer, &m_eventManager, root);
            root->setBottomSegment(timeLine);

        m_rootSegment = root;
        m_running = true;
    }
}

bool Application::init() {
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return false;
    }
    m_window = SDL_CreateWindow("RythmGameVideoEditor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, m_screenWidth, m_screenHeight, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!m_window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        return false;
    }
    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!m_renderer) {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
        return false;
    }

    TTF_Init();

    // Load a font
    m_font = TTF_OpenFont(getFullPath("assets/fonts/SegoeUIVF.ttf").c_str(), 13);
    if (!m_font) {
        printf("Failed to load font: %s\n", TTF_GetError());
        return false;
    }
    setFont(m_font);

    m_font = TTF_OpenFont(getFullPath("assets/fonts/SegoeUIVF.ttf").c_str(), 16);
    setFontBig(m_font);

    m_font = TTF_OpenFont(getFullPath("assets/fonts/SegoeUIVF.ttf").c_str(), 10);
    setFontSmall(m_font);

    return true;
}

Application::~Application() {
    if (m_rootSegment) delete m_rootSegment;
    if (m_renderer) SDL_DestroyRenderer(m_renderer);
    if (m_window) SDL_DestroyWindow(m_window);
    if (m_font) TTF_CloseFont(m_font);
    TTF_Quit();
    SDL_Quit();
}

void Application::handleEvents() {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        // Pass event to the root segment (which will pass it to all its child segments)
        if (m_rootSegment) {
            m_rootSegment->handleEvent(event);
        }
        // Pass event to popup menu
        PopupMenu::handleEvent(event);

        switch (event.type) {
        case SDL_QUIT: {
            m_running = false;
            break;
        }
        case SDL_WINDOWEVENT: {
            if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                int newWidth = event.window.data1;
                int newHeight = event.window.data2;

                appWindowSizeX = newWidth;
                appWindowSizeY = newHeight;

                // Handle the window resize
                m_rootSegment->update(0, 0, newWidth, newHeight);
            }
            break;
        }
        case SDL_MOUSEBUTTONDOWN: {
            if (event.button.button == SDL_BUTTON_LEFT) {
                SDL_Point mouseButton = { event.button.x, event.button.y };

                // Check if pressed inside an AssetList segment
                AssetsList* assetList = m_rootSegment->findType<AssetsList>();
                if (SDL_PointInRect(&mouseButton, &assetList->rect)) {
                    m_draggedAsset = assetList->getAssetFromAssetList(mouseButton.x, mouseButton.y);
                    m_isDragging = true;
                }
            }
            break;
        }
        case SDL_MOUSEMOTION: {
            if (m_isDragging && m_draggedAsset) {
                SDL_Point mouseButton = { event.button.x, event.button.y };

                // Check if released inside a Timeline segment
                Timeline* timeline = m_rootSegment->findType<Timeline>();
                if (SDL_PointInRect(&mouseButton, &timeline->rect)) {
                    // Add the new segments to the timeline
                    if (timeline->addAssetSegments(m_draggedAsset, mouseButton.x, mouseButton.y)) {
                        // Reset the dragging state
                        m_isDragging = false;
                        m_draggedAsset = nullptr;
                        break;
                    }
                }
            }
            break;
        }
        case SDL_MOUSEBUTTONUP: {
            // Reset the dragging state
            m_isDragging = false;
            m_draggedAsset = nullptr;
            break;
        }
        }
    }
}

void Application::render() {
    SDL_SetRenderDrawColor(m_renderer, 255, 0, 0, 255); // red (easy to find problems)
    SDL_RenderClear(m_renderer);

    // Render segments
    if (m_rootSegment) {
        m_rootSegment->render();
    }

    PopupMenu::render(m_renderer);

    SDL_RenderPresent(m_renderer);
}

void Application::run() {
    const int targetFrameTime = 1000 / 60;  // 16.67 ms per frame for 60 FPS
    Uint32 frameStart, frameEnd, frameDuration;

    while (m_running) {
        // Record the time at the start of the frame
        frameStart = SDL_GetTicks();

        handleEvents(); // Process input events
        render();       // Render everything

        // Record the time at the end of the frame
        frameEnd = SDL_GetTicks();

        // Calculate how long the frame took
        frameDuration = frameEnd - frameStart;

        // If the frame took less time than the target frame duration, delay the difference
        if (frameDuration < targetFrameTime) {
            SDL_Delay(targetFrameTime - frameDuration);
        }
        // If the frame took longer, we don't delay and the game continues running
    }
}