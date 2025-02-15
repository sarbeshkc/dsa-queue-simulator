// src/visualization/statistics_display.cpp
#include "statistics_display.h"

StatisticsDisplay::StatisticsDisplay(SDL_Renderer* renderer)
    : m_renderer(renderer)
    , m_updateTimer(0.0f) {
    // Initialize our statistics tracking map for each lane
    for (int i = 0; i < 12; ++i) {
        LaneId lane = static_cast<LaneId>(i);
        m_laneStats[lane] = QueueStats{0, 0.0f, false, 0.0f};
    }
}

void StatisticsDisplay::render(const TrafficManager& trafficManager) {
    // Create the statistics panel on the right side of the screen
    SDL_FRect panel = {980.0f, 0.0f, 300.0f, 720.0f};
    SDL_SetRenderDrawColor(m_renderer, 40, 40, 40, 200);
    SDL_RenderFillRect(m_renderer, &panel);

    // Render each section of statistics
    renderQueueLengths(1000.0f, 20.0f);
    renderWaitTimes(1000.0f, 250.0f);
    renderPriorityStatus(1000.0f, 480.0f);
    renderVehicleProcessingFormula();
}

void StatisticsDisplay::renderQueueLengths(float startX, float startY) {
    SDL_Color titleColor = {255, 255, 255, 255};
    renderText("Queue Lengths", startX, startY, titleColor);

    float y = startY + 30.0f;
    const float LINE_HEIGHT = 20.0f;

    for (const auto& [lane, stats] : m_laneStats) {
        std::stringstream ss;
        ss << "Lane " << getLaneString(lane) << ": " << stats.queueLength;

        // Color code based on queue length
        SDL_Color color = {255, 255, 255, 255};  // Default white
        if (stats.queueLength >= 10) {
            color = {255, 0, 0, 255};  // Red for critical
        } else if (stats.queueLength >= 5) {
            color = {255, 255, 0, 255};  // Yellow for warning
        }

        renderText(ss.str(), startX, y, color);
        y += LINE_HEIGHT;
    }
}

void StatisticsDisplay::renderWaitTimes(float startX, float startY) {
    SDL_Color titleColor = {255, 255, 255, 255};
    renderText("Wait Times", startX, startY, titleColor);

    float y = startY + 30.0f;
    const float LINE_HEIGHT = 20.0f;

    for (const auto& [lane, stats] : m_laneStats) {
        std::stringstream ss;
        ss << "Lane " << getLaneString(lane) << ": "
           << std::fixed << std::setprecision(1) << stats.waitTime << "s";

        // Color code based on wait time
        SDL_Color color = {255, 255, 255, 255};
        if (stats.waitTime > 30.0f) {
            color = {255, 0, 0, 255};  // Red for long waits
        } else if (stats.waitTime > 15.0f) {
            color = {255, 255, 0, 255};  // Yellow for medium waits
        }

        renderText(ss.str(), startX, y, color);
        y += LINE_HEIGHT;
    }
}

void StatisticsDisplay::renderPriorityStatus(float startX, float startY) {
    SDL_Color titleColor = {255, 255, 255, 255};
    renderText("Priority Status", startX, startY, titleColor);

    float y = startY + 30.0f;
    const float LINE_HEIGHT = 20.0f;

    // Render priority mode indicator
    bool anyPriorityActive = std::any_of(
        m_laneStats.begin(),
        m_laneStats.end(),
        [](const auto& pair) { return pair.second.isPriorityActive; }
    );

    SDL_Color statusColor = anyPriorityActive ?
        SDL_Color{255, 0, 0, 255} : SDL_Color{0, 255, 0, 255};

    std::string statusText = "Priority Mode: " +
        std::string(anyPriorityActive ? "ACTIVE" : "INACTIVE");
    renderText(statusText, startX, y, statusColor);
}

void StatisticsDisplay::renderVehicleProcessingFormula() {
    // Display the vehicle processing formula and current values
    std::stringstream ss;
    ss << "Vehicle Processing Formula:\n"
       << "|V| = 1/n Σ|Li|\n"
       << "where:\n"
       << "n = number of normal lanes\n"
       << "|Li| = length of lane i";

    SDL_Color formulaColor = {200, 200, 255, 255};
    renderText(ss.str(), 1000.0f, 600.0f, formulaColor);
}

void StatisticsDisplay::update(float deltaTime) {
    m_updateTimer += deltaTime;

    // Update statistics periodically to avoid performance impact
    if (m_updateTimer >= UPDATE_INTERVAL) {
        // Update will be handled by TrafficManager
        m_updateTimer = 0.0f;
    }
}
