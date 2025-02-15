// src/traffic/lane_change_handler.h
#ifndef LANE_CHANGE_HANDLER_H
#define LANE_CHANGE_HANDLER_H

#include "vehicle.h"
#include "lane_manager.h"
#include <cmath>

// Constants for lane change behavior
struct LaneChangeConstants {
    static constexpr float SAFE_DISTANCE = 50.0f;           // Minimum safe distance between vehicles
    static constexpr float LANE_CHANGE_THRESHOLD = 100.0f;  // Minimum distance from intersection
    static constexpr float MAX_CHANGE_ANGLE = 45.0f;        // Maximum angle for lane change
    static constexpr float CHANGE_DURATION = 2.0f;          // Time to complete lane change
};

class LaneChangeHandler {
public:
    LaneChangeHandler(LaneManager& laneManager);

    // Core lane change operations
    bool canChangeLane(const Vehicle* vehicle, LaneId targetLane) const;
    void performLaneChange(Vehicle* vehicle, LaneId targetLane);
    void updateLaneChange(Vehicle* vehicle, float deltaTime);

    // Lane change status queries
    bool isChangingLane(const Vehicle* vehicle) const;
    float getLaneChangeProgress(const Vehicle* vehicle) const;

private:
    LaneManager& m_laneManager;

    // Helper methods for lane change validation
    bool checkLaneChangeSpace(const Vehicle* vehicle, LaneId targetLane) const;
    bool isValidLaneChange(LaneId currentLane, LaneId targetLane) const;
    float calculateLaneChangeDistance(const Vehicle* vehicle) const;
    bool checkAdjacentLanes(LaneId current, LaneId target) const;

    // Lane change path calculations
    Vector2D calculateIntermediatePosition(const Vehicle* vehicle, float progress) const;
    bool hasObstaclesInPath(const Vehicle* vehicle, const Vector2D& targetPos) const;
};

#endif

