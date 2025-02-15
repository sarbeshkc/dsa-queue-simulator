// src/traffic/road_system.h
#ifndef ROAD_SYSTEM_H
#define ROAD_SYSTEM_H

#include "lane.h"
#include "traffic_light.h"
#include <map>
#include <memory>
#include "../utils/math_utils.h"

// The RoadSystem class manages the physical layout of roads and their interactions
class RoadSystem {
public:
    RoadSystem(SDL_Renderer* renderer);

    void update(float deltaTime);
    void render() const;

    // Lane management
    Lane* getLane(LaneId id);
    void addVehicle(Vehicle* vehicle, LaneId lane);

    // Road dimensions
    static constexpr float ROAD_WIDTH = 180.0f;   // Total width for 3 lanes
    static constexpr float LANE_WIDTH = 60.0f;    // Width of each lane

private:
    SDL_Renderer* m_renderer;
    std::map<LaneId, std::unique_ptr<Lane>> m_lanes;
    std::vector<std::unique_ptr<TrafficLight>> m_trafficLights;

    // Helper methods for road layout
    void initializeLanes();
    void initializeTrafficLights();

    // Calculate positions for lanes
    Vector2D calculateLaneStart(LaneId id) const;
    Vector2D calculateLaneEnd(LaneId id) const;
};

#endif // ROAD_SYSTEM_H
