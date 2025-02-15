
// src/traffic/lane_change_handler.cpp
#include "lane_change_handler.h"

LaneChangeHandler::LaneChangeHandler(LaneManager& laneManager)
    : m_laneManager(laneManager) {}

bool LaneChangeHandler::canChangeLane(const Vehicle* vehicle, LaneId targetLane) const {
    if (!vehicle) return false;

    // Verify basic conditions
    if (!isValidLaneChange(vehicle->getCurrentLaneId(), targetLane)) {
        return false;
    }

    // Check distance from intersection
    float distanceToIntersection = calculateLaneChangeDistance(vehicle);
    if (distanceToIntersection < LaneChangeConstants::LANE_CHANGE_THRESHOLD) {
        return false;
    }

    // Verify space in target lane
    if (!checkLaneChangeSpace(vehicle, targetLane)) {
        return false;
    }

    // Check vehicle state
    if (vehicle->getStatus() != VehicleStatus::MOVING) {
        return false;
    }

    return true;
}

void LaneChangeHandler::performLaneChange(Vehicle* vehicle, LaneId targetLane) {
    if (!vehicle || !canChangeLane(vehicle, targetLane)) return;

    // Store original lane for reference
    LaneId originalLane = vehicle->getCurrentLaneId();

    // Begin lane change process
    vehicle->setStatus(VehicleStatus::CHANGING_LANE);
    vehicle->setLaneChangeProgress(0.0f);
    vehicle->setTargetLane(targetLane);

    // Calculate intermediate positions
    Vector2D startPos = vehicle->getPosition();
    Vector2D endPos = m_laneManager.getLanePosition(targetLane, vehicle->getLanePosition());
    vehicle->setLaneChangePoints(startPos, endPos);

    // Update vehicle's current lane
    vehicle->setCurrentLane(targetLane);
}

void LaneChangeHandler::updateLaneChange(Vehicle* vehicle, float deltaTime) {
    if (!vehicle || vehicle->getStatus() != VehicleStatus::CHANGING_LANE) return;

    // Update lane change progress
    float progress = vehicle->getLaneChangeProgress();
    progress += deltaTime / LaneChangeConstants::CHANGE_DURATION;

    if (progress >= 1.0f) {
        // Complete lane change
        vehicle->setStatus(VehicleStatus::MOVING);
        vehicle->setLaneChangeProgress(0.0f);
        return;
    }

    // Calculate and set intermediate position
    Vector2D newPos = calculateIntermediatePosition(vehicle, progress);

    // Check for obstacles
    if (!hasObstaclesInPath(vehicle, newPos)) {
        vehicle->setPosition(newPos);
        vehicle->setLaneChangeProgress(progress);
    } else {
        // Abort lane change if obstacle detected
        vehicle->setStatus(VehicleStatus::MOVING);
        vehicle->setLaneChangeProgress(0.0f);
    }
}

bool LaneChangeHandler::checkLaneChangeSpace(const Vehicle* vehicle, LaneId targetLane) const {
    if (!vehicle) return false;

    // Get vehicles in target lane
    const auto& targetVehicles = m_laneManager.getVehiclesInLane(targetLane);
    Vector2D vehiclePos = vehicle->getPosition();

    // Check distance to each vehicle in target lane
    for (const auto* targetVehicle : targetVehicles) {
        if (!targetVehicle) continue;

        // Calculate distance between vehicles
        Vector2D targetPos = targetVehicle->getPosition();
        float distance = std::sqrt(
            std::pow(vehiclePos.x - targetPos.x, 2) +
            std::pow(vehiclePos.y - targetPos.y, 2)
        );

        // Check if distance is less than safe threshold
        if (distance < LaneChangeConstants::SAFE_DISTANCE) {
            return false;
        }
    }

    return true;
}

bool LaneChangeHandler::isValidLaneChange(LaneId currentLane, LaneId targetLane) const {
    // Check if lanes are adjacent
    if (!checkAdjacentLanes(currentLane, targetLane)) {
        return false;
    }

    // Prevent changes from/to free turn lanes after certain point
    if (isFreeTurnLane(currentLane) || isFreeTurnLane(targetLane)) {
        return false;
    }

    // Validate direction of change
    return isValidChangeDirection(currentLane, targetLane);
}

float LaneChangeHandler::calculateLaneChangeDistance(const Vehicle* vehicle) const {
    if (!vehicle) return 0.0f;

    // Get intersection center
    const float CENTER_X = 640.0f;
    const float CENTER_Y = 360.0f;

    // Calculate distance to intersection center
    Vector2D pos = vehicle->getPosition();
    return std::sqrt(
        std::pow(pos.x - CENTER_X, 2) +
        std::pow(pos.y - CENTER_Y, 2)
    );
}

bool LaneChangeHandler::checkAdjacentLanes(LaneId current, LaneId target) const {
    // Map of valid adjacent lanes
    static const std::map<LaneId, std::vector<LaneId>> adjacentLanes = {
        {LaneId::AL1_INCOMING, {LaneId::AL2_PRIORITY}},
        {LaneId::AL2_PRIORITY, {LaneId::AL1_INCOMING, LaneId::AL3_FREELANE}},
        {LaneId::AL3_FREELANE, {LaneId::AL2_PRIORITY}},
        // Add mappings for other lanes...
    };

    auto it = adjacentLanes.find(current);
    if (it == adjacentLanes.end()) return false;

    return std::find(it->second.begin(), it->second.end(), target) != it->second.end();
}

Vector2D LaneChangeHandler::calculateIntermediatePosition(const Vehicle* vehicle, float progress) const {
    if (!vehicle) return Vector2D();

    // Get start and end points
    Vector2D start = vehicle->getLaneChangeStartPoint();
    Vector2D end = vehicle->getLaneChangeEndPoint();

    // Use smooth interpolation for natural movement
    float smoothProgress = progress * progress * (3 - 2 * progress);  // Smoothstep function

    return Vector2D(
        start.x + (end.x - start.x) * smoothProgress,
        start.y + (end.y - start.y) * smoothProgress
    );
}

bool LaneChangeHandler::hasObstaclesInPath(const Vehicle* vehicle, const Vector2D& targetPos) const {
    if (!vehicle) return true;

    // Get all nearby vehicles
    const auto& nearbyVehicles = m_laneManager.getNearbyVehicles(vehicle->getPosition());

    // Check for potential collisions along path
    for (const auto* other : nearbyVehicles) {
        if (other == vehicle) continue;

        // Calculate minimum distance between vehicles
        float distance = calculateMinimumDistance(
            vehicle->getPosition(), targetPos,
            other->getPosition()
        );

        if (distance < LaneChangeConstants::SAFE_DISTANCE) {
            return true;
        }
    }

    return false;
}

