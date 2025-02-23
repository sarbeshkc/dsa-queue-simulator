// TrafficManager.cpp
#include "managers/TrafficManager.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include<core/Constants.h>

TrafficManager::TrafficManager()
    : inPriorityMode(false)
    , stateTimer(0.0f)
    , lastUpdateTime(0.0f)
    , processingTimer(0.0f)
    , totalVehiclesProcessed(0)
    , averageWaitTime(0.0f)
{
    // Initialize lanes
    lanes.push_back(std::make_unique<Lane>(LaneId::AL1_INCOMING, false));
    lanes.push_back(std::make_unique<Lane>(LaneId::AL2_PRIORITY, true));
    lanes.push_back(std::make_unique<Lane>(LaneId::AL3_FREELANE, false));
    lanes.push_back(std::make_unique<Lane>(LaneId::BL1_INCOMING, false));
    lanes.push_back(std::make_unique<Lane>(LaneId::BL2_NORMAL, false));
    lanes.push_back(std::make_unique<Lane>(LaneId::BL3_FREELANE, false));
    lanes.push_back(std::make_unique<Lane>(LaneId::CL1_INCOMING, false));
    lanes.push_back(std::make_unique<Lane>(LaneId::CL2_NORMAL, false));
    lanes.push_back(std::make_unique<Lane>(LaneId::CL3_FREELANE, false));
    lanes.push_back(std::make_unique<Lane>(LaneId::DL1_INCOMING, false));
    lanes.push_back(std::make_unique<Lane>(LaneId::DL2_NORMAL, false));
    lanes.push_back(std::make_unique<Lane>(LaneId::DL3_FREELANE, false));

    // Initialize traffic lights
    trafficLights[LaneId::AL2_PRIORITY] = TrafficLight();
    trafficLights[LaneId::BL2_NORMAL] = TrafficLight();
    trafficLights[LaneId::CL2_NORMAL] = TrafficLight();
    trafficLights[LaneId::DL2_NORMAL] = TrafficLight();

    synchronizeTrafficLights();
}

void TrafficManager::update(float deltaTime) {
    updateTimers(deltaTime);
    processNewVehicles();
    handleStateTransition(deltaTime);
    updateVehiclePositions(deltaTime);
    updateTrafficLights(deltaTime);
    updateStatistics(deltaTime);

    // Process vehicles based on mode
    if (processingTimer >= VEHICLE_PROCESS_TIME) {
        if (inPriorityMode) {
            processPriorityLane();
        } else {
            processNormalLanes(calculateVehiclesToProcess());
        }
        processFreeLanes();
        processingTimer = 0.0f;
    }

    checkWaitTimes();
    cleanupRemovedVehicles();
}

void TrafficManager::addVehicleToLane(LaneId laneId, std::shared_ptr<Vehicle> vehicle) {
    auto it = std::find_if(lanes.begin(), lanes.end(),
        [laneId](const auto& lane) { return lane->getId() == laneId; });

    if (it != lanes.end()) {
        (*it)->addVehicle(vehicle);
    }
}

size_t TrafficManager::getLaneSize(LaneId laneId) const {
    auto it = std::find_if(lanes.begin(), lanes.end(),
        [laneId](const auto& lane) { return lane->getId() == laneId; });
    return it != lanes.end() ? (*it)->getQueueSize() : 0;
}

bool TrafficManager::isFreeLane(LaneId laneId) const {
    return laneId == LaneId::AL3_FREELANE ||
           laneId == LaneId::BL3_FREELANE ||
           laneId == LaneId::CL3_FREELANE ||
           laneId == LaneId::DL3_FREELANE;
}

Lane* TrafficManager::getPriorityLane() const {
    auto it = std::find_if(lanes.begin(), lanes.end(),
        [](const auto& lane) { return lane->isPriorityLane(); });
    return it != lanes.end() ? it->get() : nullptr;
}

bool TrafficManager::isNearIntersection(const VehicleState& state) const {
    using namespace SimConstants;
    float dx = state.pos.x - CENTER_X;
    float dy = state.pos.y - CENTER_Y;
    float distance = std::sqrt(dx * dx + dy * dy);

    // Consider vehicle near intersection if within ROAD_WIDTH of center
    float threshold = ROAD_WIDTH + VEHICLE_WIDTH;
    return distance < threshold;
}


bool TrafficManager::isInIntersection(const Position& pos) const {
    using namespace SimConstants;
    float dx = pos.x - CENTER_X;
    float dy = pos.y - CENTER_Y;
    return std::sqrt(dx * dx + dy * dy) < ROAD_WIDTH/2.0f;
}


// TrafficManager.cpp continuation


void TrafficManager::updateVehiclePositions(float deltaTime) {
    for (auto& [_, state] : activeVehicles) {
        if (!state.isMoving && canVehicleMove(state)) {
            state.isMoving = true;
            if (state.direction != Direction::STRAIGHT) {
                calculateTurnPath(state);
            }
        }

        if (state.isMoving) {
            float dx = state.targetPos.x - state.pos.x;
            float dy = state.targetPos.y - state.pos.y;
            float distance = std::sqrt(dx * dx + dy * dy);

            if (distance > 0.1f) {
                // Calculate speed and movement
                float speedFactor = 1.0f;
                if (isInIntersection(state.pos)) {
                    speedFactor = 1.2f; // Faster through intersection
                } else {
                    float distToIntersection = std::abs(distance - SimConstants::ROAD_WIDTH);
                    if (distToIntersection < SimConstants::ROAD_WIDTH) {
                        speedFactor = 0.7f + (distToIntersection / SimConstants::ROAD_WIDTH) * 0.3f;
                    }
                }

                float currentSpeed = state.speed * speedFactor * deltaTime;
                float moveX = state.pos.x + (dx / distance) * currentSpeed;
                float moveY = state.pos.y + (dy / distance) * currentSpeed;

                // Check for collision before moving
                if (!checkCollision(state, moveX, moveY)) {
                    state.pos.x = moveX;
                    state.pos.y = moveY;
                    state.turnAngle = std::atan2(dy, dx);
                } else {
                    state.speed *= 0.5f; // Slow down when near collision
                }
            }
        }
    }

    // Clean up vehicles that have reached their destination
    cleanupRemovedVehicles();
}

void TrafficManager::updateStraightMovement(VehicleState& state, float deltaTime) {
    float dx = state.targetPos.x - state.pos.x;
    float dy = state.targetPos.y - state.pos.y;
    float distance = std::sqrt(dx * dx + dy * dy);

    if (distance > 0.1f) {
        // Adjust speed based on distance to intersection and other vehicles
        float speedFactor = 1.0f;
        if (isInIntersection(state.pos)) {
            state.inIntersection = true;
            speedFactor = 1.2f; // Faster through intersection
            state.isPassing = true;
        } else if (!state.inIntersection && !state.isPassing) {
            // Slow down approaching intersection
            float distToIntersection = std::abs(distance - SimConstants::ROAD_WIDTH);
            if (distToIntersection < SimConstants::ROAD_WIDTH) {
                speedFactor = 0.7f + (distToIntersection / SimConstants::ROAD_WIDTH) * 0.3f;
            }
        }

        float currentSpeed = state.speed * speedFactor;
        float moveRatio = std::min(1.0f, currentSpeed * deltaTime / distance);

        float newX = state.pos.x + dx * moveRatio;
        float newY = state.pos.y + dy * moveRatio;

        if (!checkCollision(state, newX, newY)) {
            state.pos.x = newX;
            state.pos.y = newY;
            state.turnAngle = std::atan2(dy, dx);
        } else {
            state.speed *= 0.5f; // Slow down when near collision
        }
    }
}

void TrafficManager::updateTurningMovement(VehicleState& state, float deltaTime) {
    if (!state.hasStartedTurn && isNearIntersection(state)) {
        state.hasStartedTurn = true;
        state.turnProgress = 0.0f;
        state.speed = SimConstants::VEHICLE_TURN_SPEED;
    }

    if (state.hasStartedTurn) {
        // Smooth turn progression
        float progressDelta = deltaTime * (1.0f - state.turnProgress * 0.5f);
        state.turnProgress = std::min(1.0f, state.turnProgress + progressDelta);

        // Use cubic easing for smoother turns
        float easedProgress = state.turnProgress * state.turnProgress * (3.0f - 2.0f * state.turnProgress);
        float currentAngle = state.startAngle + (state.endAngle - state.startAngle) * easedProgress;

        // Calculate new position along turn arc
        float newX = state.turnCenter.x + state.turnRadius * std::cos(currentAngle);
        float newY = state.turnCenter.y + state.turnRadius * std::sin(currentAngle);

        // Look ahead for collisions
        bool willCollide = false;
        const int LOOK_AHEAD_STEPS = 5;
        for (int i = 1; i <= LOOK_AHEAD_STEPS; i++) {
            float futureProgress = std::min(1.0f, state.turnProgress + progressDelta * i);
            float futureAngle = state.startAngle + (state.endAngle - state.startAngle) * futureProgress;
            float futureX = state.turnCenter.x + state.turnRadius * std::cos(futureAngle);
            float futureY = state.turnCenter.y + state.turnRadius * std::sin(futureAngle);

            if (checkCollision(state, futureX, futureY)) {
                willCollide = true;
                break;
            }
        }

        if (!willCollide && !checkCollision(state, newX, newY)) {
            state.pos.x = newX;
            state.pos.y = newY;
            state.turnAngle = currentAngle;
            state.inIntersection = true;
        } else {
            state.speed *= 0.8f; // Slow down if collision likely
        }
    } else {
        updateStraightMovement(state, deltaTime);
    }
}


bool TrafficManager::checkCollision(const VehicleState& state, float newX, float newY) const {
    const float MIN_DISTANCE = SimConstants::VEHICLE_WIDTH * 2.0f;
    const float INTERSECTION_MARGIN = SimConstants::VEHICLE_WIDTH * 2.5f;

    for (const auto& [otherId, otherState] : activeVehicles) {
        if (otherId != state.vehicle->getId()) {
            float dx = newX - otherState.pos.x;
            float dy = newY - otherState.pos.y;
            float distance = std::sqrt(dx * dx + dy * dy);

            float requiredDistance = isInIntersection(Position(newX, newY)) ?
                                   INTERSECTION_MARGIN : MIN_DISTANCE;

            // Increase margin if both vehicles are moving
            if (state.isMoving && otherState.isMoving) {
                requiredDistance *= 1.2f;
            }

            // Check for collision
            if (distance < requiredDistance) {
                return true;
            }
        }
    }
    return false;
}

void TrafficManager::addNewVehicleToState(std::shared_ptr<Vehicle> vehicle, LaneId laneId) {
    using namespace SimConstants;

    VehicleState state;
    state.vehicle = vehicle;
    state.speed = VEHICLE_BASE_SPEED;
    state.isMoving = false;
    state.direction = vehicle->getDirection();
    state.hasStartedTurn = false;
    state.turnProgress = 0.0f;
    state.waitTime = 0.0f;
    state.processingTime = 0.0f;
    state.inIntersection = false;
    state.isPassing = false;

    // Get current queue size for this lane
    size_t queuePosition = getLaneSize(laneId);
    float backOffset = QUEUE_START_OFFSET + (queuePosition * QUEUE_SPACING);
    float laneOffset = static_cast<float>((static_cast<int>(laneId) % 3)) * LANE_WIDTH;

    // Set initial spawn position
    switch (laneId) {
        case LaneId::AL1_INCOMING:
        case LaneId::AL2_PRIORITY:
        case LaneId::AL3_FREELANE: {
            // Spawn from left side
            state.pos.x = -backOffset;
            state.pos.y = CENTER_Y - ROAD_WIDTH/2.0f + LANE_WIDTH/2.0f + laneOffset;
            state.turnAngle = 0.0f;
            break;
        }
        case LaneId::BL1_INCOMING:
        case LaneId::BL2_NORMAL:
        case LaneId::BL3_FREELANE: {
            // Spawn from top
            state.pos.x = CENTER_X - ROAD_WIDTH/2.0f + LANE_WIDTH/2.0f + laneOffset;
            state.pos.y = -backOffset;
            state.turnAngle = static_cast<float>(M_PI) / 2.0f;
            break;
        }
        case LaneId::CL1_INCOMING:
        case LaneId::CL2_NORMAL:
        case LaneId::CL3_FREELANE: {
            // Spawn from right side
            state.pos.x = WINDOW_WIDTH + backOffset;
            state.pos.y = CENTER_Y - ROAD_WIDTH/2.0f + LANE_WIDTH/2.0f + laneOffset;
            state.turnAngle = static_cast<float>(M_PI);
            break;
        }
        case LaneId::DL1_INCOMING:
        case LaneId::DL2_NORMAL:
        case LaneId::DL3_FREELANE: {
            // Spawn from bottom
            state.pos.x = CENTER_X - ROAD_WIDTH/2.0f + LANE_WIDTH/2.0f + laneOffset;
            state.pos.y = WINDOW_HEIGHT + backOffset;
            state.turnAngle = -static_cast<float>(M_PI) / 2.0f;
            break;
        }
    }

    // Set queue position and calculate target
    state.queuePosition = queuePosition;
    calculateTargetPosition(state, laneId);

    // Add to active vehicles
    activeVehicles[vehicle->getId()] = state;

    std::cout << "Vehicle " << vehicle->getId()
              << " spawned in lane " << static_cast<int>(laneId)
              << " at position (" << state.pos.x << ", " << state.pos.y << ")"
              << " with direction " << static_cast<int>(state.direction)
              << std::endl;
}

// TrafficManager.cpp continuation

void TrafficManager::updateTrafficLights(float deltaTime) {
    // Update each light's state
    for (auto& [_, light] : trafficLights) {
        light.update(deltaTime);
    }

    if (inPriorityMode) {
        // Priority mode: AL2 gets green, others red
        trafficLights[LaneId::AL2_PRIORITY].setState(LightState::GREEN);
        trafficLights[LaneId::BL2_NORMAL].setState(LightState::RED);
        trafficLights[LaneId::CL2_NORMAL].setState(LightState::RED);
        trafficLights[LaneId::DL2_NORMAL].setState(LightState::RED);
    } else {
        // Normal mode: Alternate between N-S and E-W traffic
        float cycleTime = 10.0f;
        bool northSouthGreen = std::fmod(stateTimer, cycleTime * 2) < cycleTime;

        trafficLights[LaneId::BL2_NORMAL].setState(northSouthGreen ? LightState::GREEN : LightState::RED);
        trafficLights[LaneId::DL2_NORMAL].setState(northSouthGreen ? LightState::GREEN : LightState::RED);
        trafficLights[LaneId::AL2_PRIORITY].setState(northSouthGreen ? LightState::RED : LightState::GREEN);
        trafficLights[LaneId::CL2_NORMAL].setState(northSouthGreen ? LightState::RED : LightState::GREEN);
    }
}

void TrafficManager::synchronizeTrafficLights() {
    if (inPriorityMode) {
        trafficLights[LaneId::AL2_PRIORITY].setState(LightState::GREEN);
        trafficLights[LaneId::BL2_NORMAL].setState(LightState::RED);
        trafficLights[LaneId::CL2_NORMAL].setState(LightState::RED);
        trafficLights[LaneId::DL2_NORMAL].setState(LightState::RED);
    } else {
        // Start with North-South flow
        trafficLights[LaneId::AL2_PRIORITY].setState(LightState::RED);
        trafficLights[LaneId::BL2_NORMAL].setState(LightState::GREEN);
        trafficLights[LaneId::CL2_NORMAL].setState(LightState::RED);
        trafficLights[LaneId::DL2_NORMAL].setState(LightState::GREEN);
    }
}

void TrafficManager::handleStateTransition(float deltaTime) {
    bool shouldBePriority = checkPriorityConditions();

    if (shouldBePriority && !inPriorityMode) {
        if (stateTimer >= MIN_STATE_TIME) {
            inPriorityMode = true;
            stateTimer = 0.0f;
            synchronizeTrafficLights();
        }
    } else if (!shouldBePriority && inPriorityMode) {
        auto* priorityLane = getPriorityLane();
        if (priorityLane && priorityLane->getQueueSize() <= PRIORITY_RELEASE_THRESHOLD &&
            stateTimer >= MIN_STATE_TIME) {
            inPriorityMode = false;
            stateTimer = 0.0f;
            synchronizeTrafficLights();
        }
    }

    // Force state change if stuck too long
    if (stateTimer >= MAX_STATE_TIME) {
        inPriorityMode = !inPriorityMode;
        stateTimer = 0.0f;
        synchronizeTrafficLights();
    }
}

bool TrafficManager::canVehicleMove(const VehicleState& state) const {
    LaneId laneId = state.vehicle->getCurrentLane();

    // Free lanes always move
    if (isFreeLane(laneId)) {
        return true;
    }

    // Check if vehicle is first in queue
    size_t queuePos = 0;
    for (const auto& [otherId, otherState] : activeVehicles) {
        if (otherState.vehicle->getCurrentLane() == laneId && otherId != state.vehicle->getId()) {
            if ((laneId <= LaneId::AL3_FREELANE && otherState.pos.x < state.pos.x) ||
                (laneId <= LaneId::BL3_FREELANE && otherState.pos.y < state.pos.y) ||
                (laneId <= LaneId::CL3_FREELANE && otherState.pos.x > state.pos.x) ||
                (laneId <= LaneId::DL3_FREELANE && otherState.pos.y > state.pos.y)) {
                queuePos++;
            }
        }
    }

    if (queuePos > 0) {
        return false; // Not first in queue
    }

    // Check traffic light
    auto lightIt = trafficLights.find(laneId);
    if (lightIt != trafficLights.end()) {
        return lightIt->second.getState() == LightState::GREEN;
    }

    // Check parent light for incoming lanes
    switch (laneId) {
        case LaneId::AL1_INCOMING:
            return trafficLights.at(LaneId::AL2_PRIORITY).getState() == LightState::GREEN;
        case LaneId::BL1_INCOMING:
            return trafficLights.at(LaneId::BL2_NORMAL).getState() == LightState::GREEN;
        case LaneId::CL1_INCOMING:
            return trafficLights.at(LaneId::CL2_NORMAL).getState() == LightState::GREEN;
        case LaneId::DL1_INCOMING:
            return trafficLights.at(LaneId::DL2_NORMAL).getState() == LightState::GREEN;
        default:
            return false;
    }
}

void TrafficManager::processNewVehicles() {
    auto newVehicles = fileHandler.readNewVehicles();

    for (const auto& [laneId, vehicle] : newVehicles) {
        std::cout << "Processing new vehicle " << vehicle->getId()
                 << " for lane " << static_cast<int>(laneId) << std::endl;

        LaneId optimalLane = determineOptimalLane(vehicle->getDirection(), laneId);

        if (isValidSpawnLane(optimalLane, vehicle->getDirection())) {
            // First add to lane queue
            addVehicleToLane(optimalLane, vehicle);

            // Then initialize vehicle state
            addNewVehicleToState(vehicle, optimalLane);

            std::cout << "Vehicle " << vehicle->getId()
                     << " added to lane " << static_cast<int>(optimalLane) << std::endl;
        } else {
            std::cout << "Invalid spawn configuration for vehicle "
                     << vehicle->getId() << std::endl;
        }
    }
}

LaneId TrafficManager::determineOptimalLane(Direction direction, LaneId sourceLane) const {
    int roadGroup = static_cast<int>(sourceLane) / 3;

    switch (direction) {
        case Direction::LEFT:
            return static_cast<LaneId>(roadGroup * 3 + 2); // Third lane for left turns

        case Direction::RIGHT:
            return static_cast<LaneId>(roadGroup * 3); // First lane for right turns

        case Direction::STRAIGHT: {
            // Use less congested lane between first and second
            LaneId lane1 = static_cast<LaneId>(roadGroup * 3);
            LaneId lane2 = static_cast<LaneId>(roadGroup * 3 + 1);
            return (getLaneSize(lane1) <= getLaneSize(lane2)) ? lane1 : lane2;
        }

        default:
            return sourceLane;
    }
}

bool TrafficManager::isValidSpawnLane(LaneId laneId, Direction direction) const {
    int laneInRoad = static_cast<int>(laneId) % 3;

    switch (direction) {
        case Direction::LEFT:
            return laneInRoad == 2; // Third lane only
        case Direction::RIGHT:
            return laneInRoad == 0; // First lane only
        case Direction::STRAIGHT:
            return laneInRoad == 0 || laneInRoad == 1; // First or second lane
        default:
            return false;
    }
}

void TrafficManager::processPriorityLane() {
    auto* priorityLane = getPriorityLane();
    if (!priorityLane) return;

    while (priorityLane->getQueueSize() > PRIORITY_RELEASE_THRESHOLD) {
        auto vehicle = priorityLane->removeVehicle();
        if (vehicle) {
            auto it = activeVehicles.find(vehicle->getId());
            if (it != activeVehicles.end()) {
                it->second.isMoving = true;
            }
        }
    }
}


// TrafficManager.cpp final part

void TrafficManager::processNormalLanes(size_t vehicleCount) {
    if (vehicleCount == 0) return;

    for (auto& lane : lanes) {
        if (!lane->isPriorityLane() && !isFreeLane(lane->getId())) {
            for (size_t i = 0; i < vehicleCount && lane->getQueueSize() > 0; ++i) {
                auto vehicle = lane->removeVehicle();
                if (vehicle) {
                    auto it = activeVehicles.find(vehicle->getId());
                    if (it != activeVehicles.end()) {
                        it->second.isMoving = true;
                    }
                }
            }
        }
    }
}

void TrafficManager::processFreeLanes() {
    for (auto& lane : lanes) {
        if (isFreeLane(lane->getId())) {
            while (lane->getQueueSize() > 0) {
                auto vehicle = lane->removeVehicle();
                if (vehicle) {
                    auto it = activeVehicles.find(vehicle->getId());
                    if (it != activeVehicles.end()) {
                        it->second.isMoving = true;
                    }
                }
            }
        }
    }
}

size_t TrafficManager::calculateVehiclesToProcess() const {
    size_t totalVehicles = 0;
    size_t normalLaneCount = 0;

    for (const auto& lane : lanes) {
        if (!lane->isPriorityLane() && !isFreeLane(lane->getId())) {
            totalVehicles += lane->getQueueSize();
            normalLaneCount++;
        }
    }

    return normalLaneCount > 0 ?
        static_cast<size_t>(std::ceil(static_cast<float>(totalVehicles) / normalLaneCount)) : 0;
}

void TrafficManager::checkWaitTimes() {
    using namespace SimConstants;

    for (auto& lane : lanes) {
        if (lane->getQueueSize() > 0 && !isFreeLane(lane->getId())) {
            bool needsProcessing = false;

            // Get the first vehicle's wait time
            auto firstVehicleIt = std::find_if(activeVehicles.begin(), activeVehicles.end(),
                [&lane](const auto& pair) {
                    return pair.second.vehicle->getCurrentLane() == lane->getId() &&
                           !pair.second.isMoving;
                });

            if (firstVehicleIt != activeVehicles.end()) {
                if (firstVehicleIt->second.waitTime > MAX_WAIT_TIME) {
                    needsProcessing = true;
                }
            }

            if (needsProcessing || lane->getQueueSize() >= 8) {
                auto vehicle = lane->removeVehicle();
                if (vehicle) {
                    auto it = activeVehicles.find(vehicle->getId());
                    if (it != activeVehicles.end()) {
                        it->second.isMoving = true;
                    }
                }
            }
        }
    }
}

void TrafficManager::updateTimers(float deltaTime) {
    stateTimer += deltaTime;
    processingTimer += deltaTime;
    lastUpdateTime += deltaTime;

    // Update wait times for vehicles
    for (auto& [_, state] : activeVehicles) {
        if (!state.isMoving) {
            state.waitTime += deltaTime;
        }
    }
}

void TrafficManager::updateStatistics(float deltaTime) {
    // Update average wait time
    float totalWaitTime = 0.0f;
    size_t waitingVehicles = 0;

    for (const auto& [_, state] : activeVehicles) {
        if (!state.isMoving) {
            totalWaitTime += state.waitTime;
            waitingVehicles++;
        }
    }

    if (waitingVehicles > 0) {
        averageWaitTime = totalWaitTime / static_cast<float>(waitingVehicles);
    }
}

float TrafficManager::calculateAverageWaitTime() const {
    float totalWaitTime = 0.0f;
    size_t vehicleCount = 0;

    for (const auto& [_, state] : activeVehicles) {
        if (!state.isMoving) {
            totalWaitTime += state.waitTime;
            vehicleCount++;
        }
    }

    return vehicleCount > 0 ? totalWaitTime / static_cast<float>(vehicleCount) : 0.0f;
}

size_t TrafficManager::getQueuedVehicleCount() const {
    size_t count = 0;
    for (const auto& lane : lanes) {
        count += lane->getQueueSize();
    }
    return count;
}

void TrafficManager::cleanupRemovedVehicles() {
    auto it = activeVehicles.begin();
    while (it != activeVehicles.end()) {
        if (hasReachedDestination(it->second)) {
            it = activeVehicles.erase(it);
        } else {
            ++it;
        }
    }
}

bool TrafficManager::checkPriorityConditions() const {
    auto* priorityLane = getPriorityLane();
    if (!priorityLane) return false;

    return priorityLane->getQueueSize() > PRIORITY_THRESHOLD;
}

float TrafficManager::calculateTurningRadius(Direction dir) const {
    using namespace SimConstants;
    switch (dir) {
        case Direction::LEFT:
            return TURN_GUIDE_RADIUS * 1.2f;  // Wider radius for left turns
        case Direction::RIGHT:
            return TURN_GUIDE_RADIUS * 0.8f;  // Tighter radius for right turns
        default:
            return TURN_GUIDE_RADIUS;
    }
}

Position TrafficManager::calculateLaneEndpoint(LaneId laneId) const {
    using namespace SimConstants;
    const float EXIT_DISTANCE = QUEUE_START_OFFSET * 1.5f;

    float laneOffset = static_cast<float>((static_cast<int>(laneId) % 3)) * LANE_WIDTH;
    float baseY = CENTER_Y - ROAD_WIDTH/2.0f + LANE_WIDTH/2.0f + laneOffset;

    switch (laneId) {
        case LaneId::AL1_INCOMING:
        case LaneId::AL2_PRIORITY:
        case LaneId::AL3_FREELANE:
            return Position(CENTER_X + EXIT_DISTANCE, baseY);

        case LaneId::BL1_INCOMING:
        case LaneId::BL2_NORMAL:
        case LaneId::BL3_FREELANE:
            return Position(CENTER_X - ROAD_WIDTH/2.0f + LANE_WIDTH/2.0f + laneOffset,
                          CENTER_Y + EXIT_DISTANCE);

        case LaneId::CL1_INCOMING:
        case LaneId::CL2_NORMAL:
        case LaneId::CL3_FREELANE:
            return Position(CENTER_X - EXIT_DISTANCE, baseY);

        case LaneId::DL1_INCOMING:
        case LaneId::DL2_NORMAL:
        case LaneId::DL3_FREELANE:
            return Position(CENTER_X - ROAD_WIDTH/2.0f + LANE_WIDTH/2.0f + laneOffset,
                          CENTER_Y - EXIT_DISTANCE);

        default:
            return Position(CENTER_X, CENTER_Y);
    }
}

bool TrafficManager::hasReachedDestination(const VehicleState& state) const {
    float dx = state.pos.x - state.targetPos.x;
    float dy = state.pos.y - state.targetPos.y;
    return std::sqrt(dx * dx + dy * dy) < 1.0f;
}



void TrafficManager::calculateTurnPath(VehicleState& state) {
    using namespace SimConstants;
    float turnOffset = ROAD_WIDTH * 0.25f;
    state.turnRadius = calculateTurningRadius(state.direction);

    LaneId laneId = state.vehicle->getCurrentLane();
    bool isLeftTurn = state.direction == Direction::LEFT;

    // Set turn center based on lane and turn direction
    switch (laneId) {
        case LaneId::AL1_INCOMING:
        case LaneId::AL2_PRIORITY:
        case LaneId::AL3_FREELANE:
            state.turnCenter.x = CENTER_X - ROAD_WIDTH/2.0f + turnOffset;
            state.turnCenter.y = isLeftTurn ?
                CENTER_Y - ROAD_WIDTH/2.0f - turnOffset :
                CENTER_Y + ROAD_WIDTH/2.0f + turnOffset;
            state.startAngle = 0.0f;
            state.endAngle = isLeftTurn ? -M_PI/2.0f : M_PI/2.0f;
            break;

        case LaneId::BL1_INCOMING:
        case LaneId::BL2_NORMAL:
        case LaneId::BL3_FREELANE:
            state.turnCenter.x = isLeftTurn ?
                CENTER_X + ROAD_WIDTH/2.0f + turnOffset :
                CENTER_X - ROAD_WIDTH/2.0f - turnOffset;
            state.turnCenter.y = CENTER_Y - ROAD_WIDTH/2.0f + turnOffset;
            state.startAngle = M_PI/2.0f;
            state.endAngle = isLeftTurn ? 0.0f : M_PI;
            break;

        case LaneId::CL1_INCOMING:
        case LaneId::CL2_NORMAL:
        case LaneId::CL3_FREELANE:
            state.turnCenter.x = CENTER_X + ROAD_WIDTH/2.0f - turnOffset;
            state.turnCenter.y = isLeftTurn ?
                CENTER_Y + ROAD_WIDTH/2.0f + turnOffset :
                CENTER_Y - ROAD_WIDTH/2.0f - turnOffset;
            state.startAngle = M_PI;
            state.endAngle = isLeftTurn ? M_PI/2.0f : -M_PI/2.0f;
            break;

        case LaneId::DL1_INCOMING:
        case LaneId::DL2_NORMAL:
        case LaneId::DL3_FREELANE:
            state.turnCenter.x = isLeftTurn ?
                CENTER_X - ROAD_WIDTH/2.0f - turnOffset :
                CENTER_X + ROAD_WIDTH/2.0f + turnOffset;
            state.turnCenter.y = CENTER_Y + ROAD_WIDTH/2.0f - turnOffset;
            state.startAngle = -M_PI/2.0f;
            state.endAngle = isLeftTurn ? M_PI : 0.0f;
            break;
    }
}

// Add these implementations to TrafficManager.cpp

void TrafficManager::updateVehicleQueuePosition(VehicleState& state, LaneId laneId, size_t queuePosition) {
    using namespace SimConstants;

    float laneOffset = static_cast<float>((static_cast<int>(laneId) % 3)) * LANE_WIDTH;
    float queueOffset = QUEUE_START_OFFSET + (queuePosition * QUEUE_SPACING);

    // Calculate target position based on lane
    switch (laneId) {
        case LaneId::AL1_INCOMING:
        case LaneId::AL2_PRIORITY:
        case LaneId::AL3_FREELANE: {
            state.pos.x = CENTER_X - queueOffset;
            state.pos.y = CENTER_Y - ROAD_WIDTH/2.0f + LANE_WIDTH/2.0f + laneOffset;
            state.turnAngle = 0.0f;
            break;
        }
        case LaneId::BL1_INCOMING:
        case LaneId::BL2_NORMAL:
        case LaneId::BL3_FREELANE: {
            state.pos.x = CENTER_X - ROAD_WIDTH/2.0f + LANE_WIDTH/2.0f + laneOffset;
            state.pos.y = CENTER_Y - queueOffset;
            state.turnAngle = static_cast<float>(M_PI) / 2.0f;
            break;
        }
        case LaneId::CL1_INCOMING:
        case LaneId::CL2_NORMAL:
        case LaneId::CL3_FREELANE: {
            state.pos.x = CENTER_X + queueOffset;
            state.pos.y = CENTER_Y - ROAD_WIDTH/2.0f + LANE_WIDTH/2.0f + laneOffset;
            state.turnAngle = static_cast<float>(M_PI);
            break;
        }
        case LaneId::DL1_INCOMING:
        case LaneId::DL2_NORMAL:
        case LaneId::DL3_FREELANE: {
            state.pos.x = CENTER_X - ROAD_WIDTH/2.0f + LANE_WIDTH/2.0f + laneOffset;
            state.pos.y = CENTER_Y + queueOffset;
            state.turnAngle = -static_cast<float>(M_PI) / 2.0f;
            break;
        }
    }
}

void TrafficManager::calculateTargetPosition(VehicleState& state, LaneId laneId) {
    using namespace SimConstants;
    float laneOffset = static_cast<float>((static_cast<int>(laneId) % 3)) * LANE_WIDTH;

    // For each lane, set appropriate target based on lane and direction
    switch (laneId) {
        case LaneId::AL1_INCOMING:
        case LaneId::AL2_PRIORITY:
        case LaneId::AL3_FREELANE: {
            // Coming from left side
            if (state.direction == Direction::STRAIGHT) {
                // Go straight to right
                state.targetPos = Position(
                    WINDOW_WIDTH + 100.0f,
                    CENTER_Y - ROAD_WIDTH/2.0f + LANE_WIDTH/2.0f + laneOffset
                );
            } else if (state.direction == Direction::LEFT) {
                // Turn left (towards up)
                state.targetPos = Position(
                    CENTER_X - ROAD_WIDTH/2.0f + LANE_WIDTH/2.0f + laneOffset,
                    -100.0f
                );
            } else { // RIGHT
                // Turn right (towards down)
                state.targetPos = Position(
                    CENTER_X - ROAD_WIDTH/2.0f + LANE_WIDTH/2.0f + laneOffset,
                    WINDOW_HEIGHT + 100.0f
                );
            }
            break;
        }

        case LaneId::BL1_INCOMING:
        case LaneId::BL2_NORMAL:
        case LaneId::BL3_FREELANE: {
            // Coming from top
            if (state.direction == Direction::STRAIGHT) {
                // Go straight down
                state.targetPos = Position(
                    CENTER_X - ROAD_WIDTH/2.0f + LANE_WIDTH/2.0f + laneOffset,
                    WINDOW_HEIGHT + 100.0f
                );
            } else if (state.direction == Direction::LEFT) {
                // Turn left (towards right)
                state.targetPos = Position(
                    WINDOW_WIDTH + 100.0f,
                    CENTER_Y - ROAD_WIDTH/2.0f + LANE_WIDTH/2.0f + laneOffset
                );
            } else { // RIGHT
                // Turn right (towards left)
                state.targetPos = Position(
                    -100.0f,
                    CENTER_Y - ROAD_WIDTH/2.0f + LANE_WIDTH/2.0f + laneOffset
                );
            }
            break;
        }

        case LaneId::CL1_INCOMING:
        case LaneId::CL2_NORMAL:
        case LaneId::CL3_FREELANE: {
            // Coming from right side
            if (state.direction == Direction::STRAIGHT) {
                // Go straight to left
                state.targetPos = Position(
                    -100.0f,
                    CENTER_Y - ROAD_WIDTH/2.0f + LANE_WIDTH/2.0f + laneOffset
                );
            } else if (state.direction == Direction::LEFT) {
                // Turn left (towards down)
                state.targetPos = Position(
                    CENTER_X - ROAD_WIDTH/2.0f + LANE_WIDTH/2.0f + laneOffset,
                    WINDOW_HEIGHT + 100.0f
                );
            } else { // RIGHT
                // Turn right (towards up)
                state.targetPos = Position(
                    CENTER_X - ROAD_WIDTH/2.0f + LANE_WIDTH/2.0f + laneOffset,
                    -100.0f
                );
            }
            break;
        }

        case LaneId::DL1_INCOMING:
        case LaneId::DL2_NORMAL:
        case LaneId::DL3_FREELANE: {
            // Coming from bottom
            if (state.direction == Direction::STRAIGHT) {
                // Go straight up
                state.targetPos = Position(
                    CENTER_X - ROAD_WIDTH/2.0f + LANE_WIDTH/2.0f + laneOffset,
                    -100.0f
                );
            } else if (state.direction == Direction::LEFT) {
                // Turn left (towards left)
                state.targetPos = Position(
                    -100.0f,
                    CENTER_Y - ROAD_WIDTH/2.0f + LANE_WIDTH/2.0f + laneOffset
                );
            } else { // RIGHT
                // Turn right (towards right)
                state.targetPos = Position(
                    WINDOW_WIDTH + 100.0f,
                    CENTER_Y - ROAD_WIDTH/2.0f + LANE_WIDTH/2.0f + laneOffset
                );
            }
            break;
        }
    }

    // Calculate turn path if needed
    if (state.direction != Direction::STRAIGHT) {
        calculateTurnPath(state);
    }

}

