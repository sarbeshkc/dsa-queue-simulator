
// src/traffic/flow_coordinator.cpp
#include "flow_coordinator.h"
#include <algorithm>
#include <cmath>

FlowCoordinator::FlowCoordinator(SDL_Renderer* renderer)
    : m_renderer(renderer)
    , m_totalVehiclesProcessed(0)
    , m_maxWaitTime(0.0f)
    , m_priorityModeActive(false)
    , m_stateTimer(0.0f) {

    // Initialize core components
    m_laneManager = std::make_unique<LaneManager>(renderer);
    m_lightController = std::make_unique<LightController>(renderer);
    m_laneChangeHandler = std::make_unique<LaneChangeHandler>(*m_laneManager);
}

void FlowCoordinator::update(float deltaTime) {
    m_stateTimer += deltaTime;

    // Update core components
    m_laneManager->update(deltaTime);
    m_lightController->update(deltaTime);

    // Process traffic flow
    updateVehicleStates(deltaTime);
    checkIntersectionCrossings();
    managePriorityFlow();
    updateStatistics();
    cleanupVehicles();
}

void FlowCoordinator::updateVehicleStates(float deltaTime) {
    for (auto& node : m_activeVehicles) {
        Vehicle* vehicle = node.vehicle;
        if (!vehicle) continue;

        // Update wait times
        if (vehicle->getStatus() == VehicleStatus::WAITING) {
            float waitTime = SDL_GetTicks() / 1000.0f - node.entryTime;
            vehicle->setWaitTime(waitTime);
            m_maxWaitTime = std::max(m_maxWaitTime, waitTime);
        }

        // Handle lane changes
        if (vehicle->getStatus() == VehicleStatus::CHANGING_LANE) {
            m_laneChangeHandler->updateLaneChange(vehicle, deltaTime);
            continue;
        }

        // Check if vehicle can proceed
        if (vehicle->getStatus() == VehicleStatus::WAITING && canVehicleProceed(vehicle)) {
            vehicle->setStatus(VehicleStatus::MOVING);
        }

        // Process vehicle movement
        if (vehicle->getStatus() == VehicleStatus::MOVING) {
            // Calculate new position based on vehicle's direction and speed
            Vector2D newPos = vehicle->getPosition();
            float speed = vehicle->getSpeed() * deltaTime;

            switch (vehicle->getFacingDirection()) {
                case Direction::NORTH:
                    newPos.y -= speed;
                    break;
                case Direction::SOUTH:
                    newPos.y += speed;
                    break;
                case Direction::EAST:
                    newPos.x += speed;
                    break;
                case Direction::WEST:
                    newPos.x -= speed;
                    break;
            }

            // Check for collisions before moving
            if (!m_laneManager->checkCollision(vehicle, newPos)) {
                vehicle->setPosition(newPos);
            } else {
                vehicle->setStatus(VehicleStatus::WAITING);
            }
        }
    }
}

void FlowCoordinator::checkIntersectionCrossings() {
    const float INTERSECTION_THRESHOLD = 30.0f;
    const float CENTER_X = 640.0f;
    const float CENTER_Y = 360.0f;

    for (auto& node : m_activeVehicles) {
        Vehicle* vehicle = node.vehicle;
        if (!vehicle || vehicle->getStatus() != VehicleStatus::MOVING) continue;

        Vector2D pos = vehicle->getPosition();
        float distToIntersection = std::sqrt(
            std::pow(pos.x - CENTER_X, 2) +
            std::pow(pos.y - CENTER_Y, 2)
        );

        if (distToIntersection < INTERSECTION_THRESHOLD) {
            // Check if vehicle can enter intersection
            if (m_lightController->canProceedThroughIntersection(vehicle->getCurrentLaneId())) {
                // Handle turn behavior for free lanes
                if (isFreeTurnLane(vehicle->getCurrentLaneId())) {
                    vehicle->setTurnBehaviour(TurnBehaviour::TURNING_LEFT);
                }
            } else {
                vehicle->setStatus(VehicleStatus::WAITING);
            }
        }
    }
}

void FlowCoordinator::managePriorityFlow() {
    // Check priority conditions
    bool needsPriority = false;
    for (const auto& laneId : {LaneId::AL2_PRIORITY, LaneId::BL2_PRIORITY,
                              LaneId::CL2_PRIORITY, LaneId::DL2_PRIORITY}) {
        if (m_laneManager->getVehicleCount(laneId) >= 10) {
            needsPriority = true;
            break;
        }
    }

    // Update priority mode
    if (needsPriority != m_priorityModeActive) {
        m_priorityModeActive = needsPriority;
        m_lightController->setPriorityMode(needsPriority);
    }
}

bool FlowCoordinator::canVehicleProceed(const Vehicle* vehicle) const {
    if (!vehicle) return false;

    // Check traffic light state
    if (!m_lightController->canProceedThroughIntersection(vehicle->getCurrentLaneId())) {
        return false;
    }

    // Check for vehicles ahead
    Vector2D pos = vehicle->getPosition();
    Vector2D ahead = pos;
    const float CHECK_DISTANCE = 50.0f;

    switch (vehicle->getFacingDirection()) {
        case Direction::NORTH: ahead.y -= CHECK_DISTANCE; break;
        case Direction::SOUTH: ahead.y += CHECK_DISTANCE; break;
        case Direction::EAST:  ahead.x += CHECK_DISTANCE; break;
        case Direction::WEST:  ahead.x -= CHECK_DISTANCE; break;
    }

    return !m_laneManager->checkCollision(vehicle, ahead);
}

void FlowCoordinator::cleanupVehicles() {
    // Remove vehicles that have left the screen
    const float BOUNDARY_MARGIN = 50.0f;

    m_activeVehicles.erase(
        std::remove_if(m_activeVehicles.begin(), m_activeVehicles.end(),
            [this, BOUNDARY_MARGIN](const VehicleNode& node) {
                if (!node.vehicle) return true;

                Vector2D pos = node.vehicle->getPosition();
                bool outOfBounds = pos.x < -BOUNDARY_MARGIN || pos.x > 1280 + BOUNDARY_MARGIN ||
                                 pos.y < -BOUNDARY_MARGIN || pos.y > 720 + BOUNDARY_MARGIN;

                if (outOfBounds) {
                    delete node.vehicle;
                    m_totalVehiclesProcessed++;
                    return true;
                }
                return false;
            }
        ),
        m_activeVehicles.end()
    );
}

void FlowCoordinator::render() const {
    // Render core components
    m_laneManager->render();
    m_lightController->render();

    // Render all active vehicles
    for (const auto& node : m_activeVehicles) {
        if (node.vehicle) {
            node.vehicle->render();
        }
    }
}

void FlowCoordinator::addVehicle(Vehicle* vehicle) {
    if (!vehicle) return;
    m_activeVehicles.emplace_back(vehicle);
    m_laneManager->addVehicle(vehicle, vehicle->getCurrentLaneId());
}

void FlowCoordinator::removeVehicle(Vehicle* vehicle) {
    if (!vehicle) return;

    m_activeVehicles.erase(
        std::remove_if(m_activeVehicles.begin(), m_activeVehicles.end(),
            [vehicle](const VehicleNode& node) { return node.vehicle == vehicle; }
        ),
        m_activeVehicles.end()
    );

    m_laneManager->removeVehicle(vehicle);
}

float FlowCoordinator::getAverageWaitTime() const {
    if (m_activeVehicles.empty()) return 0.0f;

    float totalWaitTime = 0.0f;
    int waitingCount = 0;

    for (const auto& node : m_activeVehicles) {
        if (node.vehicle && node.vehicle->getStatus() == VehicleStatus::WAITING) {
            totalWaitTime += SDL_GetTicks() / 1000.0f - node.entryTime;
            waitingCount++;
        }
    }

    return waitingCount > 0 ? totalWaitTime / waitingCount : 0.0f;
}

