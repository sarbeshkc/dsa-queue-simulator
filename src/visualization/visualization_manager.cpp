// src/visualization/visualization_manager.cpp
#include "visualization_manager.h"
#include <sstream>
#include <iomanip>

VisualizationManager::VisualizationManager(SDL_Renderer* renderer)
    : m_renderer(renderer) {

    // Initialize TTF
    if (TTF_Init() < 0) {
        throw std::runtime_error("TTF initialization failed");
    }

    // Load font
    m_font = TTF_OpenFont("assets/fonts/Arial.ttf", 16);
    if (!m_font) {
        throw std::runtime_error("Font loading failed");
    }

    m_text = std::make_unique<Text>(renderer, m_font);
}

VisualizationManager::~VisualizationManager() {
    TTF_CloseFont(m_font);
    TTF_Quit();
}

void VisualizationManager::render(const TrafficManager& trafficManager) {
    renderBackground();
    renderQueueLengths(trafficManager);
    renderWaitTimes(trafficManager);
    renderStatistics();
    renderLegend();
}

void VisualizationManager::renderBackground() const {
    // Draw background panel for statistics
    SDL_FRect statsPanel = {0, 0, 200, 720};
    SDL_SetRenderDrawColor(m_renderer, 40, 40, 40, 255);
    SDL_RenderFillRect(m_renderer, &statsPanel);
}

void VisualizationManager::renderQueueLengths(const TrafficManager& manager) const {
    float y = 10.0f;
    const float LINE_HEIGHT = 20.0f;
    const float MARGIN = 10.0f;

    // Render queue lengths for each lane
    auto queueLengths = manager.getQueueLengths();
    for (const auto& [lane, length] : queueLengths) {
        std::stringstream ss;
        ss << "Lane " << getLaneString(lane) << ": " << length;

        // Color coding based on queue length
        SDL_Color color = {255, 255, 255, 255};  // Default white
        if (length > 10) {
            color = {255, 0, 0, 255};  // Red for critical
        } else if (length > 5) {
            color = {255, 255, 0, 255};  // Yellow for warning
        }

        m_text->setText(ss.str(), color);
        m_text->render(MARGIN, y);
        y += LINE_HEIGHT;
    }
}

void VisualizationManager::renderWaitTimes(const TrafficManager& manager) const {
    float y = 250.0f;
    const float LINE_HEIGHT = 20.0f;
    const float MARGIN = 10.0f;

    // Render wait times for each lane
    auto waitTimes = manager.getWaitTimes();
    for (const auto& [lane, time] : waitTimes) {
        std::stringstream ss;
        ss << "Wait " << getLaneString(lane) << ": "
           << std::fixed << std::setprecision(1) << time << "s";

        // Color coding based on wait time
        SDL_Color color = {255, 255, 255, 255};  // Default white
        if (time > 30.0f) {
            color = {255, 0, 0, 255};  // Red for long wait
        } else if (time > 15.0f) {
            color = {255, 255, 0, 255};  // Yellow for medium wait
        }

        m_text->setText(ss.str(), color);
        m_text->render(MARGIN, y);
        y += LINE_HEIGHT;
    }
}

void VisualizationManager::renderStatistics() const {
    float y = 500.0f;
    const float LINE_HEIGHT = 25.0f;
    const float MARGIN = 10.0f;

    // Render overall statistics
    std::vector<std::string> stats = {
        "Total Vehicles: " + std::to_string(m_stats.totalVehiclesProcessed),
        "Avg Wait: " + std::to_string(static_cast<int>(m_stats.averageWaitTime)) + "s",
        "Max Wait: " + std::to_string(static_cast<int>(m_stats.maxWaitTime)) + "s",
        "Max Queue: " + std::to_string(m_stats.maxQueueLength),
        "Priority Mode: " + std::string(m_stats.priorityModeActive ? "ON" : "OFF")
    };

    for (const auto& stat : stats) {
        m_text->setText(stat);
        m_text->render(MARGIN, y);
        y += LINE_HEIGHT;
    }
}

void VisualizationManager::renderLegend() const {
    float y = 650.0f;
    const float LINE_HEIGHT = 20.0f;
    const float MARGIN = 10.0f;

    // Draw color legend
    m_text->setText("Legend:", {255, 255, 255, 255});
    m_text->render(MARGIN, y);
    y += LINE_HEIGHT;

    std::vector<std::pair<std::string, SDL_Color>> legend = {
        {"Normal", {255, 255, 255, 255}},
        {"Warning", {255, 255, 0, 255}},
        {"Critical", {255, 0, 0, 255}}
    };

    for (const auto& [text, color] : legend) {
        m_text->setText(text, color);
        m_text->render(MARGIN, y);
        y += LINE_HEIGHT;
    }
}

void VisualizationManager::updateStatistics(const TrafficManager& trafficManager) {
    // Update statistics from traffic manager
    m_stats.totalVehiclesProcessed = trafficManager.getTotalVehiclesProcessed();
    m_stats.averageWaitTime = trafficManager.getAverageWaitTime();
    m_stats.maxWaitTime = trafficManager.getMaxWaitTime();
    m_stats.maxQueueLength = trafficManager.getMaxQueueLength();
    m_stats.priorityModeActive = trafficManager.isInPriorityMode();
}

// Helper function to convert LaneId to string
std::string getLaneString(LaneId lane) {
    switch (lane) {
        case LaneId::AL1_INCOMING: return "A1";
        case LaneId::AL2_PRIORITY: return "A2";
        case LaneId::AL3_FREELANE: return "A3";
        case LaneId::BL1_INCOMING: return "B1";
        case LaneId::BL2_PRIORITY: return "B2";
        case LaneId::BL3_FREELANE: return "B3";
        case LaneId::CL1_INCOMING: return "C1";
        case LaneId::CL2_PRIORITY: return "C2";
        case LaneId::CL3_FREELANE: return "C3";
        case LaneId::DL1_INCOMING: return "D1";
        case LaneId::DL2_PRIORITY: return "D2";
        case LaneId::DL3_FREELANE: return "D3";
        default: return "Unknown";
    }
}

// src/visualization/debug_overlay.h
#ifndef DEBUG_OVERLAY_H
#define DEBUG_OVERLAY_H

#include <SDL3/SDL.h>
#include "../traffic/traffic_manager.h"
#include "text.h"

class DebugOverlay {
public:
    DebugOverlay(SDL_Renderer* renderer, TTF_Font* font);
    void render(const TrafficManager& manager);
    void toggleDisplay() { m_isVisible = !m_isVisible; }

private:
    SDL_Renderer* m_renderer;
    std::unique_ptr<Text> m_text;
    bool m_isVisible;

    void renderDebugInfo(const TrafficManager& manager) const;
    void renderGrid() const;
    void renderVehicleDebug(const Vehicle* vehicle) const;
};

// src/visualization/debug_overlay.cpp
#include "debug_overlay.h"

DebugOverlay::DebugOverlay(SDL_Renderer* renderer, TTF_Font* font)
    : m_renderer(renderer)
    , m_text(std::make_unique<Text>(renderer, font))
    , m_isVisible(false) {}

void DebugOverlay::render(const TrafficManager& manager) {
    if (!m_isVisible) return;

    renderGrid();
    renderDebugInfo(manager);
}

void DebugOverlay::renderGrid() const {
    // Draw grid lines for visualization
    SDL_SetRenderDrawColor(m_renderer, 50, 50, 50, 255);

    // Vertical lines
    for (int x = 0; x < 1280; x += 50) {
        SDL_RenderDrawLine(m_renderer, x, 0, x, 720);
    }

    // Horizontal lines
    for (int y = 0; y < 720; y += 50) {
        SDL_RenderDrawLine(m_renderer, 0, y, 1280, y);
    }
}

void DebugOverlay::renderDebugInfo(const TrafficManager& manager) const {
    float y = 10.0f;
    const float LINE_HEIGHT = 20.0f;

    // Render simulation timing info
    std::stringstream ss;
    ss << "Frame Time: " << std::fixed << std::setprecision(2)
       << SDL_GetTicks() / 1000.0f << "s";
    m_text->setText(ss.str());
    m_text->render(1000, y);
    y += LINE_HEIGHT;

    // Render memory usage
    ss.str("");
    ss << "Active Vehicles: " << manager.getActiveVehicleCount();
    m_text->setText(ss.str());
    m_text->render(1000, y);
}

void DebugOverlay::renderVehicleDebug(const Vehicle* vehicle) const {
    if (!vehicle) return;

    // Draw vehicle bounding box
    SDL_FRect bounds = {
        vehicle->getPosition().x - 20,
        vehicle->getPosition().y - 20,
        40, 40
    };
    SDL_SetRenderDrawColor(m_renderer, 255, 0, 0, 255);
    SDL_RenderDrawRect(m_renderer, &bounds);

    // Draw vehicle info
    std::stringstream ss;
    ss << "ID: " << vehicle->getId() << "\n"
       << "Lane: " << getLaneString(vehicle->getCurrentLaneId()) << "\n"
       << "Wait: " << std::fixed << std::setprecision(1)
       << vehicle->getWaitTime() << "s";

    m_text->setText(ss.str());
    m_text->render(
        static_cast<int>(vehicle->getPosition().x) + 25,
        static_cast<int>(vehicle->getPosition().y) - 25
    );
}
