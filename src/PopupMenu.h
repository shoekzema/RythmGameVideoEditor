#include <SDL.h>
#include <vector>
#include <string>
#include <functional>
#include "util.h"

class PopupMenu {
public:
    struct MenuItem {
        std::string label;
        std::function<void()> action; // Function to execute on selection
    };
public:
    // Static method to get the singleton instance
    static PopupMenu& getInstance(SDL_Renderer* renderer = nullptr) {
        static PopupMenu instance(renderer);
        if (renderer) {
            instance.m_renderer = renderer; // Set renderer if provided (once)
        }
        return instance;
    }

    static void show(int x, int y, const std::vector<MenuItem>& items) {
        auto& instance = getInstance();
        instance.m_mousePoint.x = x;
        instance.m_mousePoint.y = y;
        instance.m_items = items;
        instance.m_isVisible = true;
    }

    static void hide() {
        getInstance().m_isVisible = false;
    }

    static void render(SDL_Renderer* renderer) {
        auto& instance = getInstance(renderer);
        if (!instance.m_isVisible) return;

        SDL_Rect outlineRect = {
            instance.m_mousePoint.x,
            instance.m_mousePoint.y,
            instance.m_windowWidth,
            instance.m_optionHeight * static_cast<int>(instance.m_items.size())
        };
        SDL_SetRenderDrawColor(instance.m_renderer, instance.m_outlineColor.r, instance.m_outlineColor.g, instance.m_outlineColor.b, instance.m_outlineColor.a);
        SDL_RenderFillRect(instance.m_renderer, &outlineRect);

        SDL_Rect backgroundRect = { 
            instance.m_mousePoint.x + 1, 
            instance.m_mousePoint.y + 1, 
            instance.m_windowWidth - 2, 
            instance.m_optionHeight * static_cast<int>(instance.m_items.size() - 2)
        };
        SDL_SetRenderDrawColor(instance.m_renderer, instance.m_backgroundColor.r, instance.m_backgroundColor.g, instance.m_backgroundColor.b, instance.m_backgroundColor.a);
        SDL_RenderFillRect(instance.m_renderer, &backgroundRect);

        int textY = instance.m_mousePoint.y;
        for (const auto& item : instance.m_items) {
            renderText(instance.m_renderer, instance.m_mousePoint.x + instance.m_textXPos, textY, getFont(), item.label.c_str());
            textY += instance.m_optionHeight;
        }
    }

    static void handleEvent(SDL_Event& event) {
        auto& instance = getInstance();
        if (!instance.m_isVisible) return;

        if (event.type == SDL_MOUSEBUTTONDOWN) {
            int mouseX = event.button.x;
            int mouseY = event.button.y;
            if (event.button.button == SDL_BUTTON_LEFT) {
                for (int i = 0; i < instance.m_items.size(); ++i) {
                    SDL_Rect itemRect = { 
                        instance.m_mousePoint.x, 
                        instance.m_mousePoint.y + i * instance.m_optionHeight,
                        instance.m_windowWidth, 
                        instance.m_optionHeight 
                    };
                    if (mouseX >= itemRect.x && mouseX <= itemRect.x + itemRect.w &&
                        mouseY >= itemRect.y && mouseY <= itemRect.y + itemRect.h) {
                        instance.m_items[i].action();
                        hide();
                        return;
                    }
                }
                hide();
            }
        }
    }
private:
    PopupMenu(SDL_Renderer* renderer) : m_renderer(renderer) {}
private:
    SDL_Renderer* m_renderer;
    SDL_Point m_mousePoint = { 0, 0 };
    std::vector<MenuItem> m_items;
    bool m_isVisible = false;
    int m_windowWidth = 200;
    int m_optionHeight = 24;
    int m_textXPos = 10;

    SDL_Color m_outlineColor    = { 95, 98, 101, 255 };
    SDL_Color m_backgroundColor = { 38, 41,  45, 255 };
};
