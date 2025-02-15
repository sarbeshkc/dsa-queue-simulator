// src/visualization/statistics_display.h
#ifndef STATISTICS_DISPLAY_H
#define STATISTICS_DISPLAY_H

#include <SDL3/SDL.h>
#include "../traffic/traffic_manager.h"
#include "../core/queue.h"
#include <string>
#include <sstream>

// This class handles the visual representation of our queue statistics
// and traffic flow information, helping users monitor the simulation
class StatisticsDisplay {
public:
    StatisticsDisplay(SDL_Renderer* renderer);

    // Core display functions
    void render(const TrafficManager& trafficManager);
    void update(float deltaTime);

private:
    SDL_Renderer* m_renderer;
    float m_updateTimer;
    static constexpr float UPDATE_INTERVAL = 0.5f;  // Update every half second

    // Statistics tracking
    struct QueueStats {
        int queueLength;
        float waitTime;
        bool isPriorityActive;
        float processingProgress;
    };
    std::map<LaneId, QueueStats> m_laneStats;

    // Helper rendering functions
    void renderQueueLengths(float startX, float startY);
    void renderWaitTimes(float startX, float startY);
    void renderPriorityStatus(float startX, float startY);
    void renderVehicleProcessingFormula();

    // Text rendering helpers
    void renderText(const std::string& text, float x, float y, SDL_Color color);
    SDL_Texture* createTextTexture(const std::string& text, SDL_Color color);
};

#endif // STATISTICS_DISPLAY_H
