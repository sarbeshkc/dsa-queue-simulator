// DebugOverlay.h
#pragma once
#include <SDL3/SDL.h>
#include <cmath>
#include "managers/TrafficManager.h"
#include <map>

class DebugOverlay {
private:
    struct LaneStatistics {
        int vehicleCount;
        float avgWaitTime;
        int processedCount;
    };

    std::map<LaneId, LaneStatistics> stats;
    void updateStatistics(const TrafficManager& trafficManager);
    void renderQueueStats(SDL_Renderer* renderer, int x, int y);
    void renderLaneLoadIndicator(SDL_Renderer* renderer, int x, int y);
    void renderSystemStatus(SDL_Renderer* renderer, int x, int y);

public:
    DebugOverlay() = default;
    void render(SDL_Renderer* renderer, const TrafficManager& trafficManager);
};
