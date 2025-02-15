// src/visualization/visualization_manager.h
#ifndef VISUALIZATION_MANAGER_H
#define VISUALIZATION_MANAGER_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "../traffic/traffic_manager.h"
#include "../core/text.h"
#include <memory>

class VisualizationManager {
public:
    VisualizationManager(SDL_Renderer* renderer);
    ~VisualizationManager();

    void render(const TrafficManager& trafficManager);
    void updateStatistics(const TrafficManager& trafficManager);

private:
    SDL_Renderer* m_renderer;
    TTF_Font* m_font;
    std::unique_ptr<Text> m_text;

    // Statistics tracking
    struct Statistics {
        int totalVehiclesProcessed;
        float averageWaitTime;
        float maxWaitTime;
        int maxQueueLength;
        bool priorityModeActive;

        Statistics()
            : totalVehiclesProcessed(0)
            , averageWaitTime(0.0f)
            , maxWaitTime(0.0f)
            , maxQueueLength(0)
            , priorityModeActive(false) {}
    } m_stats;

    void renderBackground() const;
    void renderQueueLengths(const TrafficManager& manager) const;
    void renderWaitTimes(const TrafficManager& manager) const;
    void renderStatistics() const;
    void renderLegend() const;
};

#endif // VISUALIZATION_MANAGER_H