// src/traffic/intersection_controller.cpp
#include "intersection_controller.h"
#include <algorithm>
#include <cmath>

IntersectionController::IntersectionController(SDL_Renderer* renderer)
    : m_renderer(renderer) {
    initializeTrafficLights();
}

void IntersectionController::initializeTrafficLights() {
    // Create traffic lights at each approach to the intersection
    const float centerX = 640.0f;
    const float centerY = 360.0f;
    const float offset = 90.0f;  // Offset from intersection center

    // Create and position traffic lights
    m_trafficLights[NORTH_LIGHT] = std::make_unique<TrafficLight>(
        m_renderer, centerX - offset, centerY - offset, true);
    m_trafficLights[EAST_LIGHT] = std::make_unique<TrafficLight>(
        m_renderer, centerX + offset, centerY - offset, false);
    m_trafficLights[SOUTH_LIGHT] = std::make_unique<TrafficLight>(
        m_renderer, centerX + offset, centerY + offset, true);
    m_trafficLights[WEST_LIGHT] = std::make_unique<TrafficLight>(
        m_renderer, centerX - offset, centerY + offset, false);

    // Set initial states
    synchronizeTrafficLights();
}

void IntersectionController::update(float deltaTime) {
    // Update all traffic lights
    for (auto& light : m_trafficLights) {
        light->update(deltaTime);
    }

    // Maintain traffic light synchronization
    synchronizeTrafficLights();

    // Clean up vehicles that have left the intersection
    m_vehiclesInIntersection.erase(
        std::remove_if(m_vehiclesInIntersection.begin(),
                      m_vehiclesInIntersection.end(),
                      [this](Vehicle* vehicle) {
                          return !isVehicleInIntersectionBounds(vehicle);
                      }),
        m_vehiclesInIntersection.end());
}

void IntersectionController::synchronizeTrafficLights() {
    // Ensure opposing traffic lights are synchronized
    if (m_trafficLights[NORTH_LIGHT]->getState() !=
        m_trafficLights[SOUTH_LIGHT]->getState()) {
        m_trafficLights[SOUTH_LIGHT]->setState(
            m_trafficLights[NORTH_LIGHT]->getState());
    }

    if (m_trafficLights[EAST_LIGHT]->getState() !=
        m_trafficLights[WEST_LIGHT]->getState()) {
        m_trafficLights[WEST_LIGHT]->setState(
            m_trafficLights[EAST_LIGHT]->getState());
    }

    // Ensure perpendicular lights are opposite
    if (m_trafficLights[NORTH_LIGHT]->isGreen()) {
        m_trafficLights[EAST_LIGHT]->setState(LightState::RED);
        m_trafficLights[WEST_LIGHT]->setState(LightState::RED);
    } else if (m_trafficLights[EAST_LIGHT]->isGreen()) {
        m_trafficLights[NORTH_LIGHT]->setState(LightState::RED);
        m_trafficLights[SOUTH_LIGHT]->setState(LightState::RED);
    }
}

bool IntersectionController::canVehicleEnterIntersection(
    const Vehicle* vehicle) const {
    if (!vehicle) return false;

    // Check if intersection is at capacity
    if (m_vehiclesInIntersection.size() >= MAX_VEHICLES) {
        return false;
    }

    // Check traffic light state for vehicle's direction
    if (!isLaneActive(vehicle->getCurrentLaneId())) {
        return false;
    }

    // Check for potential collisions
    if (checkCollisionRisk(vehicle)) {
        return false;
    }

    // Check if the intended path is clear
    return isPathClear(vehicle);
}

bool IntersectionController::checkCollisionRisk(const Vehicle* vehicle) const {
    if (m_vehiclesInIntersection.empty()) return false;

    Direction vehicleDir = vehicle->getFacingDirection();
    Vector2D vehiclePos = vehicle->getPosition();

    for (const Vehicle* other : m_vehiclesInIntersection) {
        // Skip if same vehicle
        if (vehicle == other) continue;

        Direction otherDir = other->getFacingDirection();
        Vector2D otherPos = other->getPosition();

        // Calculate distance between vehicles
        float distance = std::sqrt(
            std::pow(vehiclePos.x - otherPos.x, 2) +
            std::pow(vehiclePos.y - otherPos.y, 2));

        // Check if vehicles are too close
        if (distance < SAFE_DISTANCE) {
            return true;
        }

        // Check if directions conflict
        if (areDirectionsConflicting(vehicleDir, otherDir)) {
            return true;
        }
    }

    return false;
}

bool IntersectionController::isPathClear(const Vehicle* vehicle) const {
    if (!vehicle) return false;

    // Get vehicle's intended path through intersection
    Vector2D startPos = vehicle->getPosition();
    Vector2D endPos;

    // Calculate end position based on vehicle's turn intention
    switch (vehicle->getFacingDirection()) {
        case Direction::NORTH:
            endPos = {startPos.x, startPos.y - INTERSECTION_SIZE};
            break;
        case Direction::SOUTH:
            endPos = {startPos.x, startPos.y + INTERSECTION_SIZE};
            break;
        case Direction::EAST:
            endPos = {startPos.x + INTERSECTION_SIZE, startPos.y};
            break;
        case Direction::WEST:
            endPos = {startPos.x - INTERSECTION_SIZE, startPos.y};
            break;
    }

    // Check for vehicles along the path
    for (const Vehicle* other : m_vehiclesInIntersection) {
        Vector2D otherPos = other->getPosition();

        // Calculate distance to path
        float distance = pointToLineDistance(otherPos, startPos, endPos);

        if (distance < SAFE_DISTANCE) {
            return false;
        }
    }

    return true;
}

void IntersectionController::render() const {
    // Render traffic lights
    for (const auto& light : m_trafficLights) {
        light->render();
    }

    // Debug visualization of intersection boundaries
    SDL_FRect intersectionBounds = {
        640.0f - INTERSECTION_SIZE/2,
        360.0f - INTERSECTION_SIZE/2,
        INTERSECTION_SIZE,
        INTERSECTION_SIZE
    };

    SDL_SetRenderDrawColor(m_renderer, 100, 100, 100, 255);
    SDL_RenderDrawRect(m_renderer, &intersectionBounds);
}

bool IntersectionController::areDirectionsConflicting(
    Direction dir1, Direction dir2) const {
    // Opposite directions never conflict
    if (dir1 == getOppositeDirection(dir2)) {
        return false;
    }

    // Adjacent directions might conflict depending on turns
    return true;
}

Direction IntersectionController::getOppositeDirection(Direction dir) const {
    switch (dir) {
        case Direction::NORTH: return Direction::SOUTH;
        case Direction::SOUTH: return Direction::NORTH;
        case Direction::EAST:  return Direction::WEST;
        case Direction::WEST:  return Direction::EAST;
        default: return dir;
    }
}

// Helper function to calculate distance from a point to a line segment
float IntersectionController::pointToLineDistance(
    const Vector2D& point,
    const Vector2D& lineStart,
    const Vector2D& lineEnd) const {

    float numerator = std::abs(
        (lineEnd.y - lineStart.y) * point.x -
        (lineEnd.x - lineStart.x) * point.y +
        lineEnd.x * lineStart.y -
        lineEnd.y * lineStart.x);

    float denominator = std::sqrt(
        std::pow(lineEnd.y - lineStart.y, 2) +
        std::pow(lineEnd.x - lineStart.x, 2));

    return numerator / denominator;
}
