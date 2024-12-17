#include <SDL.h>
#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include "util.h"

class ContextMenu {
public:
    struct MenuItem {
        std::string label;
        std::function<void()> action; // Function to execute on selection
        std::vector<MenuItem> submenu; // Submenu items
    };
public:
    // Static method to get the singleton instance
    static ContextMenu& getInstance(SDL_Renderer* renderer = nullptr) {
        static ContextMenu instance(renderer);
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

        // Render the main menu
        instance.renderMenu(instance.m_renderer, instance.m_mousePoint.x, instance.m_mousePoint.y, instance.m_items, instance.m_hoveredIndex);

        // Render any active submenu
        if (instance.m_activeSubmenu) {
            instance.renderMenu(instance.m_renderer, instance.m_submenuPoint.x, instance.m_submenuPoint.y, *instance.m_activeSubmenu, instance.m_submenuHoveredIndex);
        }
    }

    static void handleEvent(SDL_Event& event) {
        auto& instance = getInstance();
        if (!instance.m_isVisible) return;

        if (event.type == SDL_MOUSEMOTION) {
            int mouseX = event.motion.x;
            int mouseY = event.motion.y;
            instance.handleHover(instance.m_mousePoint.x, instance.m_mousePoint.y, mouseX, mouseY, instance.m_items);
        }
        else if (event.type == SDL_MOUSEBUTTONDOWN) {
            int mouseX = event.button.x;
            int mouseY = event.button.y;
            if (event.button.button == SDL_BUTTON_LEFT) {
                // Handle click on main menu or submenu
                if (!instance.handleClick(instance.m_mousePoint.x, instance.m_mousePoint.y, mouseX, mouseY, instance.m_items)) {
                    instance.hide();
                }
            }
        }
    }
private:
    ContextMenu(SDL_Renderer* renderer) : m_renderer(renderer) {}

    void renderMenu(SDL_Renderer* renderer, int x, int y, const std::vector<MenuItem>& items, int hoveredIndex) {
        SDL_Rect outlineRect = {
            x, y,
            m_windowWidth,
            m_optionHeight * static_cast<int>(items.size())
        };
        SDL_SetRenderDrawColor(renderer, m_outlineColor.r, m_outlineColor.g, m_outlineColor.b, m_outlineColor.a);
        SDL_RenderFillRect(renderer, &outlineRect);

        SDL_Rect backgroundRect = {
            x + 1, y + 1,
            m_windowWidth - 2,
            m_optionHeight * static_cast<int>(items.size()) - 2
        };
        SDL_SetRenderDrawColor(renderer, m_backgroundColor.r, m_backgroundColor.g, m_backgroundColor.b, m_backgroundColor.a);
        SDL_RenderFillRect(renderer, &backgroundRect);

        int textY = y;
        for (int i = 0; i < items.size(); i++) {
            // Highlight the hovered item
            if (i == hoveredIndex) {
                SDL_SetRenderDrawColor(renderer, m_selectedColor.r, m_selectedColor.g, m_selectedColor.b, m_selectedColor.a);
                SDL_Rect highlightRect = { 
                    x + 1, textY + 1, 
                    m_windowWidth - 2, 
                    m_optionHeight - 2 
                };
                SDL_RenderFillRect(renderer, &highlightRect);
            }

            renderText(renderer, x + m_textXPos, textY, getFont(), items[i].label.c_str());

            // Render an indicator for submenus
            if (!items[i].submenu.empty()) {
                renderText(renderer, x + m_windowWidth - 20, textY, getFont(), ">"); // ">" indicates a submenu
            }

            textY += m_optionHeight;
        }
    }

    void handleHover(int menuX, int menuY, int mouseX, int mouseY, const std::vector<MenuItem>& items) {
        int textY = menuY;

        for (int i = 0; i < items.size(); ++i) {
            SDL_Rect itemRect = {
                menuX, textY,
                m_windowWidth,
                m_optionHeight
            };

            if (mouseX >= itemRect.x && mouseX <= itemRect.x + itemRect.w &&
                mouseY >= itemRect.y && mouseY <= itemRect.y + itemRect.h) 
            {
                m_hoveredIndex = i;

                // If the item has a submenu, show it
                if (!items[i].submenu.empty()) {
                    m_activeSubmenu = &items[i].submenu;
                    m_submenuPoint = { menuX + m_windowWidth, textY };
                }
                else {
                    m_activeSubmenu = nullptr; // Hide submenu if no submenu for this item
                    m_submenuHoveredIndex = -1;
                }
                return;
            }

            textY += m_optionHeight;
        }

        // Check if the mouse is over the active submenu
        if (m_activeSubmenu) {
            SDL_Rect submenuRect = {
                m_submenuPoint.x, m_submenuPoint.y,
                m_windowWidth,
                static_cast<int>(m_activeSubmenu->size()) * m_optionHeight
            };

            if (mouseX >= submenuRect.x && mouseX <= submenuRect.x + submenuRect.w &&
                mouseY >= submenuRect.y && mouseY <= submenuRect.y + submenuRect.h) 
            {
                // Determine which submenu item is hovered
                textY = m_submenuPoint.y;
                for (int i = 0; i < m_activeSubmenu->size(); ++i) {
                    SDL_Rect submenuItemRect = {
                        m_submenuPoint.x, textY,
                        m_windowWidth,
                        m_optionHeight
                    };

                    if (mouseX >= submenuItemRect.x && mouseX <= submenuItemRect.x + submenuItemRect.w &&
                        mouseY >= submenuItemRect.y && mouseY <= submenuItemRect.y + submenuItemRect.h) {
                        m_submenuHoveredIndex = i;
                        return;
                    }

                    textY += m_optionHeight;
                }
            }
        }

        // If the mouse is not over any item or submenu, hide the submenu
        m_activeSubmenu = nullptr;
        m_submenuHoveredIndex = -1;
        m_hoveredIndex = -1;
    }

    bool handleClick(int menuX, int menuY, int mouseX, int mouseY, const std::vector<MenuItem>& items) {
        int textY = menuY;

        for (const auto& item : items) {
            SDL_Rect itemRect = {
                menuX, textY,
                m_windowWidth,
                m_optionHeight
            };

            if (mouseX >= itemRect.x && mouseX <= itemRect.x + itemRect.w &&
                mouseY >= itemRect.y && mouseY <= itemRect.y + itemRect.h) {
                if (!item.submenu.empty()) {
                    // Do not close menu, let the user interact with the submenu
                    return true;
                }
                else {
                    if (item.action) item.action();
                    return false;
                }
            }

            textY += m_optionHeight;
        }

        // Check clicks in the submenu
        if (m_activeSubmenu) {
            return handleClick(m_submenuPoint.x, m_submenuPoint.y, mouseX, mouseY, *m_activeSubmenu);
        }

        return false;
    }
private:
    SDL_Renderer* m_renderer;
    SDL_Point m_mousePoint = { 0, 0 };
    SDL_Point m_submenuPoint = { 0, 0 }; // Position of the active submenu
    std::vector<MenuItem> m_items;
    const std::vector<MenuItem>* m_activeSubmenu = nullptr; // Current active submenu
    bool m_isVisible = false;
    int m_windowWidth = 200;
    int m_optionHeight = 24;
    int m_textXPos = 10;
    int m_highlightBoxXPos = 3;
    int m_highlightBoxYPos = 3;
    int m_hoveredIndex = -1;        // Index of the hovered item in the main menu
    int m_submenuHoveredIndex = -1; // Index of the hovered item in a submenu

    SDL_Color m_outlineColor    = { 95, 98, 101, 255 };
    SDL_Color m_backgroundColor = { 38, 41,  45, 255 };
    SDL_Color m_selectedColor   = { 45, 81, 101, 255 };
};
