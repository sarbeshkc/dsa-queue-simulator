// src/visualization/debug_overlay.cpp
#include "debug_overlay.h"
#include <sstream>
#include <iomanip>
#include "../traffic/vehicle.h"
#include "../utils/math_utils.h"

DebugOverlay::DebugOverlay(SDL_Renderer* renderer, TTF_Font* font)
    : m_renderer(renderer)
    , m_text(std::make_unique<Text>(renderer, font))
    , m_isVisible(false) {}

void DebugOverlay::render(const TrafficManager& manager) {
    if (!m_isVisible) return;

    renderGrid();
    renderDebugInfo(manager);
    renderLaneInfo(manager);
    renderIntersectionDebug();
}

void DebugOverlay::renderGrid() const {
    // Draw coordinate grid for debugging
    SDL_SetRenderDrawColor(m_renderer, 30, 30, 30, 128);

    // Vertical lines every 100 pixels
    for (int x = 0; x < 1280; x += 100) {
        // In SDL3, we use SDL_RenderLine instead of SDL_RenderDrawLine
        SDL_RenderLine(m_renderer, 
            static_cast<float>(x), 0.0f,
            static_cast<float>(x), 720.0f);
        
        // Draw coordinate text
        std::stringstream ss;
        ss << x;
        m_text->setText(ss.str(), {128, 128, 128, 255});
        m_text->render(x + 5, 5);
    }

    // Horizontal lines every 100 pixels
    for (int y = 0; y < 720; y += 100) {
        // Using SDL3's floating-point coordinates
        SDL_RenderLine(m_renderer,
            0.0f, static_cast<float>(y),
            1280.0f, static_cast<float>(y));
        
        std::stringstream ss;
        ss << y;
        m_text->setText(ss.str(), {128, 128, 128, 255});
        m_text->render(5, y + 5);
    }
}

void DebugOverlay::renderDebugInfo(const TrafficManager& manager) const {
    float y = 10.0f;
    const float LINE_HEIGHT = 20.0f;
    const float RIGHT_MARGIN = 200.0f;

    // Render FPS and timing info
    std::stringstream ss;
    ss << "Frame Time: " << std::fixed << std::setprecision(2)
       << SDL_GetTicks() / 1000.0f << "s";
    m_text->setText(ss.str(), {255, 255, 255, 255});
    m_text->render(1280 - RIGHT_MARGIN, y);
    y += LINE_HEIGHT;

    // Priority mode status
    ss.str("");
    ss << "Priority Mode: " << (manager.isInPriorityMode() ? "ACTIVE" : "INACTIVE");
    SDL_Color priorityColor = manager.isInPriorityMode() ? 
        SDL_Color{255, 255, 0, 255} : SDL_Color{255, 255, 255, 255};
    m_text->setText(ss.str(), priorityColor);
    m_text->render(1280 - RIGHT_MARGIN, y);
}

void DebugOverlay::renderLaneInfo(const TrafficManager& manager) const {
    const float LINE_HEIGHT = 20.0f;
    float y = 100.0f;

    // Show detailed info for each lane
    for (int i = 0; i < 12; ++i) {
        LaneId lane = static_cast<LaneId>(i);
        int queueLength = manager.getQueueLength(lane);
        
        std::stringstream ss;
        ss << "Lane " << getLaneString(lane) << ": "
           << queueLength << " vehicles";

        // Color code based on queue length
        SDL_Color color = {255, 255, 255, 255};
        if (queueLength >= 10) {
            color = {255, 0, 0, 255};  // Red for critical
        } else if (queueLength >= 5) {
            color = {255, 255, 0, 255};  // Yellow for warning
        }

        m_text->setText(ss.str(), color);
        m_text->render(1280 - 200.0f, y);
        y += LINE_HEIGHT;
    }
}

void DebugOverlay::renderIntersectionDebug() const {
    // Draw intersection boundaries
    const float CENTER_X = 640.0f;
    const float CENTER_Y = 360.0f;
    const float INTERSECTION_SIZE = 180.0f;

    // Create intersection rectangle using SDL3's FRect
    SDL_FRect intersection = {
        CENTER_X - INTERSECTION_SIZE/2,
        CENTER_Y - INTERSECTION_SIZE/2,
        INTERSECTION_SIZE,
        INTERSECTION_SIZE
    };

    SDL_SetRenderDrawColor(m_renderer, 255, 0, 0, 64);
    // In SDL3, we use SDL_RenderRect instead of SDL_RenderDrawRect
    SDL_RenderRect(m_renderer, &intersection);

    // Draw intersection center marker
    const float MARKER_SIZE = 10.0f;
    SDL_RenderLine(m_renderer,
        CENTER_X - MARKER_SIZE, CENTER_Y,
        CENTER_X + MARKER_SIZE, CENTER_Y);
    SDL_RenderLine(m_renderer,
        CENTER_X, CENTER_Y - MARKER_SIZE,
        CENTER_X, CENTER_Y + MARKER_SIZE);
}

void DebugOverlay::renderVehicleDebug(const Vehicle* vehicle) const {
    if (!vehicle) return;

    // Draw vehicle bounding box
    SDL_FRect bounds = {
        vehicle->getPosition().x - Vehicle::VEHICLE_WIDTH/2,
        vehicle->getPosition().y - Vehicle::VEHICLE_LENGTH/2,
        Vehicle::VEHICLE_WIDTH,
        Vehicle::VEHICLE_LENGTH
    };
    SDL_SetRenderDrawColor(m_renderer, 255, 0, 0, 255);
    SDL_RenderRect(m_renderer, &bounds);

    // Show vehicle details
    std::stringstream ss;
    ss << "Lane: " << getLaneString(vehicle->getCurrentLaneId()) << "\n"
       << "Wait: " << std::fixed << std::setprecision(1)
       << vehicle->getWaitTime() << "s";

    m_text->setText(ss.str(), {255, 255, 255, 255});
    m_text->render(
        static_cast<int>(vehicle->getPosition().x) + 25,
        static_cast<int>(vehicle->getPosition().y) - 25
    );
}