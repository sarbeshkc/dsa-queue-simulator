// src/visualization/debug_overlay.h
#ifndef DEBUG_OVERLAY_H
#define DEBUG_OVERLAY_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <memory>
#include "../traffic/traffic_manager.h"
#include "../core/text.h"

// Forward declarations
class Vehicle;
std::string getLaneString(LaneId lane);

class DebugOverlay {
public:
    /**
     * Constructor for the debug overlay.
     * @param renderer The SDL renderer to use for drawing
     * @param font The TTF font to use for text rendering
     */
    DebugOverlay(SDL_Renderer* renderer, TTF_Font* font);

    /**
     * Renders the debug overlay if visible.
     * @param manager Reference to the traffic manager for queue information
     */
    void render(const TrafficManager& manager);

    /**
     * Toggles the visibility of the debug overlay.
     */
    void toggleDisplay() { m_isVisible = !m_isVisible; }

    /**
     * Checks if the debug overlay is currently visible.
     * @return true if visible, false otherwise
     */
    bool isVisible() const { return m_isVisible; }

private:
    SDL_Renderer* m_renderer;        // Renderer reference for drawing
    std::unique_ptr<Text> m_text;    // Text rendering utility
    bool m_isVisible;                // Visibility toggle

    // Helper rendering methods
    void renderGrid() const;
    void renderDebugInfo(const TrafficManager& manager) const;
    void renderLaneInfo(const TrafficManager& manager) const;
    void renderIntersectionDebug() const;
    void renderVehicleDebug(const Vehicle* vehicle) const;
};

#endif // DEBUG_OVERLAY_H