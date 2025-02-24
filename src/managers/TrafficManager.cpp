// TrafficManager.cpp
#include "managers/TrafficManager.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <core/Constants.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

std::vector<uint32_t> vehiclesToRemove;

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
    stateTimer += deltaTime;
    processingTimer += deltaTime;

    // Update traffic light states
    if (stateTimer >= SimConstants::TRAFFIC_UPDATE_INTERVAL / 1000.0f) {
        updateTrafficLights();
        stateTimer = 0.0f;
    }

    // Process new vehicles from files
    processNewVehicles();

    // Update vehicle positions and states
    for (auto& [id, state] : activeVehicles) {
        // Update wait time for non-processing vehicles
        if (!state.vehicle->isInProcess()) {
            state.vehicle->updateWaitTime(deltaTime);
            state.waitTime += deltaTime;
        }

        // Update vehicle movement
        updateVehicleMovement(state, deltaTime);

        // Check if vehicle has completed its journey
        if (hasCompletedJourney(*state.vehicle)) {
            vehiclesToRemove.push_back(id);
            totalVehiclesProcessed++;
            averageWaitTime = (averageWaitTime * (totalVehiclesProcessed - 1) + state.vehicle->getWaitTime()) / totalVehiclesProcessed;
        }
    }

    // Remove completed vehicles
    for (auto id : vehiclesToRemove) {
        activeVehicles.erase(id);
    }
    vehiclesToRemove.clear();

    // Process queued vehicles
    processQueuedVehicles();
}

void TrafficManager::checkTurningConditions(Vehicle& vehicle) {
    LaneId lane = vehicle.getCurrentLane();
    float turnThreshold = 0.0f;
    bool shouldTurn = false;
    Position pos{vehicle.getX(), vehicle.getY()};

    // Calculate turn threshold based on lane
    switch (lane) {
        case LaneId::AL2_PRIORITY:
        case LaneId::AL3_FREELANE:
            turnThreshold = SimConstants::CENTER_Y - SimConstants::ROAD_WIDTH/2.0f - 20.0f;
            shouldTurn = pos.y >= turnThreshold;
            break;
        case LaneId::BL2_NORMAL:
        case LaneId::BL3_FREELANE:
            turnThreshold = SimConstants::CENTER_Y + SimConstants::ROAD_WIDTH/2.0f + 20.0f;
            shouldTurn = pos.y <= turnThreshold;
            break;
        case LaneId::CL2_NORMAL:
        case LaneId::CL3_FREELANE:
            turnThreshold = SimConstants::CENTER_X + SimConstants::ROAD_WIDTH/2.0f + 20.0f;
            shouldTurn = pos.x <= turnThreshold;
            break;
        case LaneId::DL2_NORMAL:
        case LaneId::DL3_FREELANE:
            turnThreshold = SimConstants::CENTER_X - SimConstants::ROAD_WIDTH/2.0f - 20.0f;
            shouldTurn = pos.x >= turnThreshold;
            break;
        default:
            break;
    }

    // Start turn if conditions are met
    if (shouldTurn) {
        // Lane 3 vehicles can always turn
        if (lane == LaneId::AL3_FREELANE || lane == LaneId::BL3_FREELANE ||
            lane == LaneId::CL3_FREELANE || lane == LaneId::DL3_FREELANE) {
            vehicle.startTurn();
        }
        // Lane 2 vehicles can only turn when their light is green
        else if ((lane == LaneId::AL2_PRIORITY && trafficLights[lane].getState() == LightState::GREEN) ||
                 (lane == LaneId::BL2_NORMAL && trafficLights[lane].getState() == LightState::GREEN) ||
                 (lane == LaneId::CL2_NORMAL && trafficLights[lane].getState() == LightState::GREEN) ||
                 (lane == LaneId::DL2_NORMAL && trafficLights[lane].getState() == LightState::GREEN)) {
            vehicle.startTurn();
        }
    }
}

void TrafficManager::updateTrafficLights() {
    static int currentState = 0;
    static float stateTime = 0.0f;

    stateTime += SimConstants::TRAFFIC_UPDATE_INTERVAL / 1000.0f;

    // State transition timing
    if (currentState == 0 && stateTime >= 2.0f) {  // All red state
        currentState = 1;
        stateTime = 0.0f;
    }
    else if (currentState > 0 && stateTime >= 3.0f) {  // Individual green states
        currentState = (currentState % 4) + 1;
        stateTime = 0.0f;
    }

    // Update traffic light states
    for (auto& [lane, light] : trafficLights) {
        switch (currentState) {
            case 1:  // A green
                light.setState(lane == LaneId::AL2_PRIORITY ? LightState::GREEN : LightState::RED);
                break;
            case 2:  // B green
                light.setState(lane == LaneId::BL2_NORMAL ? LightState::GREEN : LightState::RED);
                break;
            case 3:  // C green
                light.setState(lane == LaneId::CL2_NORMAL ? LightState::GREEN : LightState::RED);
                break;
            case 4:  // D green
                light.setState(lane == LaneId::DL2_NORMAL ? LightState::GREEN : LightState::RED);
                break;
            default:  // All red
                light.setState(LightState::RED);
                break;
        }
    }
}

void TrafficManager::updateVehicleMovement(VehicleState& state, float deltaTime) {
    if (!state.isMoving) return;

    // Calculate distance to target
    float dx = state.targetPos.x - state.pos.x;
    float dy = state.targetPos.y - state.pos.y;
    float distance = std::sqrt(dx * dx + dy * dy);

    // Update position if not at target
    if (distance > 0.1f) {
        float moveSpeed = state.speed * deltaTime;
        float moveRatio = std::min(1.0f, moveSpeed / distance);

        // Update position
        state.pos.x += dx * moveRatio;
        state.pos.y += dy * moveRatio;

        // Update vehicle's position
        state.vehicle->setX(state.pos.x);
        state.vehicle->setY(state.pos.y);
        state.vehicle->setAngle(state.turnAngle);

        // Check if vehicle has reached intersection for turning
        if (!state.hasStartedTurn && state.direction != Direction::STRAIGHT) {
            float distToIntersection = getDistanceToIntersection(state);
            if (distToIntersection < SimConstants::LANE_WIDTH) {
                state.hasStartedTurn = true;
                calculateTurnPath(state);
            }
        }
    } else {
        // Reached current target
        if (state.hasStartedTurn && !state.inIntersection) {
            // Start intersection traversal
            state.inIntersection = true;
            calculateTargetPosition(state, state.vehicle->getTargetLane());
        } else if (state.inIntersection && !state.isPassing) {
            // Complete intersection traversal
            state.isPassing = true;
            state.isMoving = false;  // Stop for traffic light check
            checkTrafficLightAndProceed(state);
        }
    }
}

void TrafficManager::checkTrafficLightAndProceed(VehicleState& state) {
    LaneId currentLane = state.vehicle->getCurrentLane();
    auto lightIt = trafficLights.find(currentLane);
    
    if (lightIt != trafficLights.end()) {
        if (lightIt->second.getState() == LightState::GREEN) {
            state.isMoving = true;
        } else {
            state.hasStoppedAtLight = true;
        }
    } else {
        // No traffic light for this lane (e.g., free lanes)
        state.isMoving = true;
    }
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
    float dx = state.pos.x - SimConstants::CENTER_X;
    float dy = state.pos.y - SimConstants::CENTER_Y;
    float distance = std::sqrt(dx * dx + dy * dy);
    return distance < SimConstants::ROAD_WIDTH * 0.75f;
}

bool TrafficManager::isInIntersection(const Position& pos) const {
    using namespace SimConstants;
    float dx = pos.x - CENTER_X;
    float dy = pos.y - CENTER_Y;
    return std::sqrt(dx * dx + dy * dy) < ROAD_WIDTH/2.0f;
}

void TrafficManager::updateVehiclePositions(float deltaTime) {
    for (auto& [_, state] : activeVehicles) {
        if (state.isMoving) {
            // Check if vehicle should start turning
            if (!state.hasStartedTurn && state.direction != Direction::STRAIGHT) {
                float distToIntersection = getDistanceToIntersection(state);
                if (distToIntersection < SimConstants::TURN_ENTRY_DISTANCE) {
                    state.hasStartedTurn = true;
                    state.turnProgress = 0.0f;
                    state.speed = SimConstants::VEHICLE_TURN_SPEED;
                    calculateTurnParameters(state);
                }
            }

            if (state.hasStartedTurn) {
                // Execute turn
                updateTurningMovement(state, deltaTime);
            } else {
                // Normal straight movement
                updateStraightMovement(state, deltaTime);
            }

            // Check for intermediate targets (lane changes)
            if (!state.intermediateTargets.empty()) {
                auto targetPos = state.intermediateTargets.front();
                float dx = targetPos.x - state.pos.x;
                float dy = targetPos.y - state.pos.y;
                float distanceToTarget = std::sqrt(dx * dx + dy * dy);

                if (distanceToTarget < 1.0f) {
                    state.intermediateTargets.erase(state.intermediateTargets.begin());
                } else {
                    float moveSpeed = state.speed * deltaTime;
                    float moveX = state.pos.x + (dx / distanceToTarget) * moveSpeed;
                    float moveY = state.pos.y + (dy / distanceToTarget) * moveSpeed;

                    if (!checkCollision(state, moveX, moveY)) {
                        state.pos.x = moveX;
                        state.pos.y = moveY;
                    }
                }
            }
        }
    }

    // Clean up vehicles that have reached their destination
    cleanupRemovedVehicles();
}

void TrafficManager::updateStraightMovement(VehicleState& state, float deltaTime) {
    if (!state.isMoving) return;

    float dx = state.targetPos.x - state.pos.x;
    float dy = state.targetPos.y - state.pos.y;
    float distance = std::sqrt(dx * dx + dy * dy);

    if (distance > 0.1f) {
        float moveSpeed = state.speed * deltaTime;
        float moveRatio = std::min(1.0f, moveSpeed / distance);

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
    using namespace SimConstants;

    // Update turn progress
    state.turnProgress += deltaTime * Vehicle::TURN_SPEED;
    if (state.turnProgress > 1.0f) {
        state.turnProgress = 1.0f;
    }

    // Calculate bezier curve points based on current lane and direction
    float startX = state.pos.x;
    float startY = state.pos.y;
    float controlX, controlY, endX, endY;

    // Determine turn parameters based on source lane
    LaneId currentLane = state.vehicle->getCurrentLane();
    Direction turnDir = state.direction;
    
    // Calculate control points based on lane and turn direction
    if (currentLane == LaneId::AL2_PRIORITY) {
        // AL2 to BL1 turn (West to North)
        controlX = CENTER_X - Vehicle::BEZIER_CONTROL_OFFSET;
        controlY = CENTER_Y - Vehicle::BEZIER_CONTROL_OFFSET;
        endX = CENTER_X - LANE_WIDTH;
        endY = CENTER_Y - ROAD_WIDTH/2.0f - 20.0f;
    } else if (currentLane == LaneId::BL2_NORMAL) {
        // BL2 to CL1 turn (North to East)
        controlX = CENTER_X + Vehicle::BEZIER_CONTROL_OFFSET;
        controlY = CENTER_Y - Vehicle::BEZIER_CONTROL_OFFSET;
        endX = CENTER_X + ROAD_WIDTH/2.0f + 20.0f;
        endY = CENTER_Y - LANE_WIDTH;
    } else if (currentLane == LaneId::CL2_NORMAL) {
        // CL2 to DL1 turn (East to South)
        controlX = CENTER_X + Vehicle::BEZIER_CONTROL_OFFSET;
        controlY = CENTER_Y + Vehicle::BEZIER_CONTROL_OFFSET;
        endX = CENTER_X + LANE_WIDTH;
        endY = CENTER_Y + ROAD_WIDTH/2.0f + 20.0f;
    } else if (currentLane == LaneId::DL2_NORMAL) {
        // DL2 to AL1 turn (South to West)
        controlX = CENTER_X - Vehicle::BEZIER_CONTROL_OFFSET;
        controlY = CENTER_Y + Vehicle::BEZIER_CONTROL_OFFSET;
        endX = CENTER_X - ROAD_WIDTH/2.0f - 20.0f;
        endY = CENTER_Y + LANE_WIDTH;
    }

    // Calculate position using quadratic bezier curve
    float t = state.turnProgress;
    float oneMinusT = 1.0f - t;
    state.pos.x = oneMinusT * oneMinusT * startX + 2 * oneMinusT * t * controlX + t * t * endX;
    state.pos.y = oneMinusT * oneMinusT * startY + 2 * oneMinusT * t * controlY + t * t * endY;

    // Calculate angle for vehicle rotation
    float dx = 2 * (1-t) * (controlX - startX) + 2 * t * (endX - controlX);
    float dy = 2 * (1-t) * (controlY - startY) + 2 * t * (endY - controlY);
    state.turnAngle = std::atan2f(dy, dx);

    // Update vehicle state
    state.vehicle->setX(state.pos.x);
    state.vehicle->setY(state.pos.y);
    state.vehicle->setAngle(state.turnAngle);
    state.vehicle->setTurnProgress(t);

    // Check if turn is complete
    if (state.turnProgress >= 1.0f) {
        completeTurn(state);
    }
}

void TrafficManager::completeTurn(VehicleState& state) {
    // Reset turn state
    state.hasStartedTurn = false;
    state.turnProgress = 0.0f;
    state.vehicle->setIsTurning(false);
    
    // Update lane based on turn direction
    LaneId currentLane = state.vehicle->getCurrentLane();
    LaneId newLane;
    
    if (currentLane == LaneId::AL2_PRIORITY) {
        newLane = LaneId::BL1_INCOMING;
    } else if (currentLane == LaneId::BL2_NORMAL) {
        newLane = LaneId::CL1_INCOMING;
    } else if (currentLane == LaneId::CL2_NORMAL) {
        newLane = LaneId::DL1_INCOMING;
    } else if (currentLane == LaneId::DL2_NORMAL) {
        newLane = LaneId::AL1_INCOMING;
    }
    
    state.vehicle->setCurrentLane(newLane);
    calculateTargetPosition(state, newLane);
    
    // Reset movement state
    state.isMoving = true;
    state.speed = SimConstants::VEHICLE_BASE_SPEED;
}

bool TrafficManager::checkCollision(const VehicleState& state, float newX, float newY) const {
    using namespace SimConstants;

    const float MIN_DISTANCE = VEHICLE_WIDTH * 1.5f;

    for (const auto& [otherId, otherState] : activeVehicles) {
        if (otherId != state.vehicle->getId()) {
            float dx = newX - otherState.pos.x;
            float dy = newY - otherState.pos.y;
            float distance = std::sqrt(dx * dx + dy * dy);

            if (distance < MIN_DISTANCE) {
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

    // Calculate lane offset from center (important for preventing collisions)
    float laneOffset = 0.0f;
    int lanePosition = static_cast<int>(laneId) % 3;

    // Proper lane spacing to prevent collisions
    if (lanePosition == 0) {  // Left lane
        laneOffset = -LANE_WIDTH;
    } else if (lanePosition == 1) {  // Middle lane
        laneOffset = 0.0f;
    } else {  // Right lane
        laneOffset = LANE_WIDTH;
    }

    // Calculate initial positions based on road
    int roadNum = static_cast<int>(laneId) / 3;
    switch(roadNum) {
        case 0: // West approach (A lanes)
            state.pos.x = -VEHICLE_WIDTH * 2;
            state.pos.y = CENTER_Y + laneOffset;
            state.turnAngle = 0.0f;
            break;

        case 1: // North approach (B lanes)
            state.pos.x = CENTER_X + laneOffset;
            state.pos.y = -VEHICLE_HEIGHT * 2;
            state.turnAngle = M_PI/2;
            break;

        case 2: // East approach (C lanes)
            state.pos.x = WINDOW_WIDTH + VEHICLE_WIDTH * 2;
            state.pos.y = CENTER_Y - laneOffset; // Note negative offset
            state.turnAngle = M_PI;
            break;

        case 3: // South approach (D lanes)
            state.pos.x = CENTER_X - laneOffset; // Note negative offset
            state.pos.y = WINDOW_HEIGHT + VEHICLE_HEIGHT * 2;
            state.turnAngle = -M_PI/2;
            break;
    }

    // Calculate target position based on direction
    calculateTargetPosition(state, laneId);

    // Add queue offset for multiple vehicles
    size_t queuePosition = getLaneSize(laneId);
    float queueOffset = queuePosition * (VEHICLE_LENGTH + MIN_VEHICLE_SPACING);

    // Apply queue offset based on approach direction
    switch(roadNum) {
        case 0: state.pos.x -= queueOffset; break;
        case 1: state.pos.y -= queueOffset; break;
        case 2: state.pos.x += queueOffset; break;
        case 3: state.pos.y += queueOffset; break;
    }

    activeVehicles[vehicle->getId()] = state;
}

void TrafficManager::calculateTurnParameters(VehicleState& state) {
    using namespace SimConstants;

    // Get the quadrant (0-3 for W,N,E,S)
    int quadrant = static_cast<int>(state.vehicle->getCurrentLane()) / 3;

    // Calculate turn center and angles
    switch (quadrant) {
        case 0: // From West
            state.turnCenter.x = CENTER_X - ROAD_WIDTH/4;
            state.turnCenter.y = CENTER_Y;
            state.startAngle = 0.0f;
            state.endAngle = -M_PI/2;  // For right turns
            break;
        case 1: // From North
            state.turnCenter.x = CENTER_X;
            state.turnCenter.y = CENTER_Y - ROAD_WIDTH/4;
            state.startAngle = M_PI/2;
            state.endAngle = 0.0f;  // For right turns
            break;
        case 2: // From East
            state.turnCenter.x = CENTER_X + ROAD_WIDTH/4;
            state.turnCenter.y = CENTER_Y;
            state.startAngle = M_PI;
            state.endAngle = M_PI/2;  // For right turns
            break;
        case 3: // From South
            state.turnCenter.x = CENTER_X;
            state.turnCenter.y = CENTER_Y + ROAD_WIDTH/4;
            state.startAngle = -M_PI/2;
            state.endAngle = M_PI;  // For right turns
            break;
    }
}

void TrafficManager::processQueuedVehicles() {
    // Process vehicles in each lane
    for (auto& lane : lanes) {
        // Skip if lane is empty
        if (lane->getQueueSize() == 0) continue;

        // Get the front vehicle
        auto vehicle = lane->peekFront();
        if (!vehicle) continue;

        // Check if vehicle can be processed based on traffic light state
        bool canProcess = false;
        LaneId laneId = lane->getLaneId();

        // Lane 3 vehicles can always proceed (free right turn)
        if (laneId == LaneId::AL3_FREELANE || laneId == LaneId::BL3_FREELANE ||
            laneId == LaneId::CL3_FREELANE || laneId == LaneId::DL3_FREELANE) {
            canProcess = true;
        }
        // Lane 2 vehicles need green light
        else if (laneId == LaneId::AL2_PRIORITY || laneId == LaneId::BL2_NORMAL ||
                 laneId == LaneId::CL2_NORMAL || laneId == LaneId::DL2_NORMAL) {
            auto it = trafficLights.find(laneId);
            if (it != trafficLights.end()) {
                canProcess = it->second.getState() == LightState::GREEN;
            }
        }
        // Lane 1 vehicles can proceed if no turning vehicle is blocking
        else if (laneId == LaneId::AL1_INCOMING || laneId == LaneId::BL1_INCOMING ||
                 laneId == LaneId::CL1_INCOMING || laneId == LaneId::DL1_INCOMING) {
            canProcess = true;
            // Check for turning vehicles in the intersection
            for (const auto& [id, state] : activeVehicles) {
                if (state.vehicle->getIsTurning()) {
                    canProcess = false;
                    break;
                }
            }
        }

        if (canProcess) {
            // Remove from queue and add to active vehicles
            auto dequeuedVehicle = lane->dequeue();
            if (dequeuedVehicle) {
                dequeuedVehicle->setProcessing(true);
                VehicleState newState;
                newState.vehicle = dequeuedVehicle;
                newState.isMoving = true;
                newState.speed = SimConstants::VEHICLE_BASE_SPEED;
                newState.waitTime = 0.0f;
                newState.turnProgress = 0.0f;
                newState.hasStartedTurn = false;
                newState.inIntersection = false;
                newState.isPassing = false;
                activeVehicles[dequeuedVehicle->getId()] = newState;
            }
        }
    }
}

void TrafficManager::processNewVehicles() {
    auto newVehicles = fileHandler.readNewVehicles();

    for (const auto& [laneId, vehicle] : newVehicles) {
        std::cout << "Processing new vehicle " << vehicle->getId()
                  << " for lane " << static_cast<int>(laneId) << std::endl;

        // Add to lane queue first
        addVehicleToLane(laneId, vehicle);
            
        // Create vehicle state
        VehicleState newState;
        newState.vehicle = vehicle;
        newState.isMoving = true;  // Start moving immediately
        newState.speed = SimConstants::VEHICLE_BASE_SPEED;
        newState.direction = vehicle->getDirection();
        newState.hasStartedTurn = false;
        newState.turnProgress = 0.0f;
        newState.waitTime = 0.0f;
        newState.processingTime = 0.0f;
        newState.inIntersection = false;
        newState.isPassing = false;
        newState.isChangingLanes = false;
        newState.hasStoppedAtLight = false;
        newState.currentTargetIndex = 0;

        // Set initial position based on lane
        calculateInitialPosition(newState, laneId);
        
        // Update vehicle's position
        vehicle->setX(newState.pos.x);
        vehicle->setY(newState.pos.y);
        vehicle->setAngle(newState.turnAngle);

        // Add to active vehicles
        activeVehicles[vehicle->getId()] = newState;

        std::cout << "Vehicle " << vehicle->getId() << " initialized at position ("
                  << newState.pos.x << ", " << newState.pos.y << ") with angle "
                  << newState.turnAngle << std::endl;
    }
}

void TrafficManager::calculateInitialPosition(VehicleState& state, LaneId laneId) {
    // Set initial position based on lane
    switch (laneId) {
        case LaneId::AL1_INCOMING:
        case LaneId::AL2_PRIORITY:
        case LaneId::AL3_FREELANE:
            state.pos.x = -SimConstants::ROAD_LENGTH;
            state.pos.y = SimConstants::LANE_WIDTH * (static_cast<int>(laneId) % 3);
            state.turnAngle = 0.0f; // Facing right
            break;
            
        case LaneId::BL1_INCOMING:
        case LaneId::BL2_NORMAL:
        case LaneId::BL3_FREELANE:
            state.pos.x = SimConstants::LANE_WIDTH * (static_cast<int>(laneId) % 3);
            state.pos.y = SimConstants::ROAD_LENGTH;
            state.turnAngle = 270.0f; // Facing up
            break;
            
        case LaneId::CL1_INCOMING:
        case LaneId::CL2_NORMAL:
        case LaneId::CL3_FREELANE:
            state.pos.x = SimConstants::ROAD_LENGTH;
            state.pos.y = -SimConstants::LANE_WIDTH * (static_cast<int>(laneId) % 3);
            state.turnAngle = 180.0f; // Facing left
            break;
            
        case LaneId::DL1_INCOMING:
        case LaneId::DL2_NORMAL:
        case LaneId::DL3_FREELANE:
            state.pos.x = -SimConstants::LANE_WIDTH * (static_cast<int>(laneId) % 3);
            state.pos.y = -SimConstants::ROAD_LENGTH;
            state.turnAngle = 90.0f; // Facing down
            break;
    }

    // Set target position as the intersection entry point
    state.targetPos.x = state.pos.x;
    state.targetPos.y = state.pos.y;
    if (state.direction == Direction::STRAIGHT) {
        switch (laneId) {
            case LaneId::AL1_INCOMING:
            case LaneId::AL2_PRIORITY:
            case LaneId::AL3_FREELANE:
                state.targetPos.x = SimConstants::ROAD_LENGTH;
                break;
            case LaneId::BL1_INCOMING:
            case LaneId::BL2_NORMAL:
            case LaneId::BL3_FREELANE:
                state.targetPos.y = -SimConstants::ROAD_LENGTH;
                break;
            case LaneId::CL1_INCOMING:
            case LaneId::CL2_NORMAL:
            case LaneId::CL3_FREELANE:
                state.targetPos.x = -SimConstants::ROAD_LENGTH;
                break;
            case LaneId::DL1_INCOMING:
            case LaneId::DL2_NORMAL:
            case LaneId::DL3_FREELANE:
                state.targetPos.y = SimConstants::ROAD_LENGTH;
                break;
        }
    }
}

LaneId TrafficManager::determineOptimalLane(Direction direction, LaneId sourceLane) const {
    int roadGroup = static_cast<int>(sourceLane) / 3;

    switch (direction) {
        case Direction::LEFT:
            return static_cast<LaneId>(roadGroup * 3 + 2);
        case Direction::RIGHT:
            return static_cast<LaneId>(roadGroup * 3);
        default: {
            LaneId lane1 = static_cast<LaneId>(roadGroup * 3);
            LaneId lane2 = static_cast<LaneId>(roadGroup * 3 + 1);
            size_t lane1Size = getLaneSize(lane1);
            size_t lane2Size = getLaneSize(lane2);

            if (lane1Size <= lane2Size) {
                return lane1;
            } else if (lane2Size <= 2) {
                return lane2;
            } else {
                return lane1;
            }
        }
    }
}

bool TrafficManager::isValidSpawnLane(LaneId laneId, Direction direction) const {
    int laneInRoad = static_cast<int>(laneId) % 3;

    switch (direction) {
        case Direction::LEFT:  return laneInRoad == 2;
        case Direction::RIGHT: return laneInRoad == 0;
        default:               return laneInRoad == 0 || laneInRoad == 1;
    }
}

void TrafficManager::processPriorityLane() {
    auto* priorityLane = getPriorityLane();
    if (!priorityLane) return;

    while (priorityLane->getQueueSize() > PRIORITY_RELEASE_THRESHOLD) {
        auto vehicle = priorityLane->dequeue();
        if (vehicle) {
            auto it = activeVehicles.find(vehicle->getId());
            if (it != activeVehicles.end()) {
                it->second.isMoving = true;
            }
        }
    }
}

void TrafficManager::processNormalLanes(size_t vehicleCount) {
    if (vehicleCount == 0) return;

    for (auto& lane : lanes) {
        if (!lane->isPriorityLane() && !isFreeLane(lane->getLaneId())) {
            for (size_t i = 0; i < vehicleCount && lane->getQueueSize() > 0; ++i) {
                auto vehicle = lane->dequeue();
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
        if (isFreeLane(lane->getLaneId())) {
            while (lane->getQueueSize() > 0) {
                auto vehicle = lane->dequeue();
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
        if (!lane->isPriorityLane() && !isFreeLane(lane->getLaneId())) {
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
        if (lane->getQueueSize() > 0 && !isFreeLane(lane->getLaneId())) {
            bool needsProcessing = false;

            // Get the first vehicle's wait time
            auto firstVehicleIt = std::find_if(activeVehicles.begin(), activeVehicles.end(),
                [&lane](const auto& pair) {
                    return pair.second.vehicle->getCurrentLane() == lane->getLaneId() &&
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
                    auto vehicleIt = activeVehicles.find(vehicle->getId());
                    if (vehicleIt != activeVehicles.end()) {
                        vehicleIt->second.isMoving = true;
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

    // Get road number (0-3) and lane position (0-2)
    int roadNum = static_cast<int>(laneId) / 3;
    int lanePosition = static_cast<int>(laneId) % 3;

    if (state.direction == Direction::STRAIGHT) {
        switch(roadNum) {
            case 0: // West to East
                state.targetPos.x = WINDOW_WIDTH + VEHICLE_WIDTH;
                state.targetPos.y = CENTER_Y + (lanePosition - 1) * LANE_WIDTH;
                break;
            case 1: // North to South
                state.targetPos.x = CENTER_X + (lanePosition - 1) * LANE_WIDTH;
                state.targetPos.y = WINDOW_HEIGHT + VEHICLE_HEIGHT;
                break;
            case 2: // East to West
                state.targetPos.x = -VEHICLE_WIDTH;
                state.targetPos.y = CENTER_Y + (1 - lanePosition) * LANE_WIDTH;
                break;
            case 3: // South to North
                state.targetPos.x = CENTER_X + (1 - lanePosition) * LANE_WIDTH;
                state.targetPos.y = -VEHICLE_HEIGHT;
                break;
        }
    }
    else if (state.direction == Direction::RIGHT) {
        switch(roadNum) {
            case 0: // West to North
                state.targetPos.x = CENTER_X + LANE_WIDTH;
                state.targetPos.y = -VEHICLE_HEIGHT;
                break;
            case 1: // North to East
                state.targetPos.x = WINDOW_WIDTH + VEHICLE_WIDTH;
                state.targetPos.y = CENTER_Y - LANE_WIDTH;
                break;
            case 2: // East to South
                state.targetPos.x = CENTER_X - LANE_WIDTH;
                state.targetPos.y = WINDOW_HEIGHT + VEHICLE_HEIGHT;
                break;
            case 3: // South to West
                state.targetPos.x = -VEHICLE_WIDTH;
                state.targetPos.y = CENTER_Y + LANE_WIDTH;
                break;
        }
        state.turnRadius = ROAD_WIDTH * 0.75f;
    }
}

LaneId TrafficManager::determineTargetLane(LaneId currentLane, Direction direction) const {
    // Return appropriate target lane based on current lane and intended direction
    switch(direction) {
        case Direction::LEFT:
            // Third lane only for left turns
            switch(currentLane) {
                case LaneId::AL3_FREELANE: return LaneId::BL3_FREELANE;
                case LaneId::BL3_FREELANE: return LaneId::CL3_FREELANE;
                case LaneId::CL3_FREELANE: return LaneId::DL3_FREELANE;
                case LaneId::DL3_FREELANE: return LaneId::AL3_FREELANE;
                default: return currentLane; // Should not happen
            }

        case Direction::RIGHT:
            // First lane only for right turns
            switch(currentLane) {
                case LaneId::AL1_INCOMING: return LaneId::DL1_INCOMING;
                case LaneId::BL1_INCOMING: return LaneId::AL1_INCOMING;
                case LaneId::CL1_INCOMING: return LaneId::BL1_INCOMING;
                case LaneId::DL1_INCOMING: return LaneId::CL1_INCOMING;
                default: return currentLane;
            }

        default: // STRAIGHT
            // Can use first or second lane
            return currentLane;
    }
}

void TrafficManager::changeLaneToFree(VehicleState& state) {
    float targetLaneY = state.pos.y;
    if (state.pos.x < SimConstants::CENTER_X) targetLaneY += SimConstants::LANE_WIDTH;
    else targetLaneY -= SimConstants::LANE_WIDTH;

    state.intermediateTargets.push_back({state.pos.x, targetLaneY});
}

void TrafficManager::changeLaneToFirst(VehicleState& state) {
    float targetLaneY = state.pos.y;
    if (state.pos.x < SimConstants::CENTER_X) targetLaneY -= SimConstants::LANE_WIDTH;
    else targetLaneY += SimConstants::LANE_WIDTH;

    state.intermediateTargets.push_back({state.pos.x, targetLaneY});
}

bool TrafficManager::canVehicleMove(const VehicleState& state) const {
    using namespace SimConstants;  // Add this for constant access

    // Check traffic light state
    auto lightIt = trafficLights.find(state.vehicle->getCurrentLane());
    if (lightIt != trafficLights.end() && lightIt->second.getState() == LightState::RED
        && !state.inIntersection) {
        // Handle red light stop
        float distToIntersection = getDistanceToIntersection(state);
        if (distToIntersection < STOP_LINE_OFFSET) {
            std::cout << "Vehicle " << state.vehicle->getId() << " stopped at red light" << std::endl;
            return false;
        }
    }

    return !hasVehicleAhead(state);
}

LightState TrafficManager::getLightStateForLane(LaneId laneId) const {
    auto it = trafficLights.find(laneId);
    return it != trafficLights.end() ? it->second.getState() : LightState::RED;
}

float TrafficManager::getDistanceToIntersection(const VehicleState& state) const {
    using namespace SimConstants;

    float dx = CENTER_X - state.pos.x;
    float dy = CENTER_Y - state.pos.y;
    return std::sqrt(dx * dx + dy * dy) - INTERSECTION_RADIUS;
}

bool TrafficManager::hasVehicleAhead(const VehicleState& state) const {
    for (const auto& [_, otherState] : activeVehicles) {
        if (state.vehicle->getId() != otherState.vehicle->getId() &&
            state.vehicle->getCurrentLane() == otherState.vehicle->getCurrentLane()) {

            float dx = otherState.pos.x - state.pos.x;
            float dy = otherState.pos.y - state.pos.y;
            float distance = std::sqrt(dx * dx + dy * dy);

            if (distance < SimConstants::VEHICLE_MIN_SPACING &&
                isVehicleAhead(state, otherState)) {
                return true;
            }
        }
    }
    return false;
}

bool TrafficManager::isVehicleAhead(const VehicleState& first,
                                  const VehicleState& second) const {
    // Determine if second vehicle is ahead of first based on road direction
    LaneId laneId = first.vehicle->getCurrentLane();

    if (laneId <= LaneId::AL3_FREELANE) {
        return second.pos.x > first.pos.x;
    } else if (laneId <= LaneId::BL3_FREELANE) {
        return second.pos.y > first.pos.y;
    } else if (laneId <= LaneId::CL3_FREELANE) {
        return second.pos.x < first.pos.x;
    } else {
        return second.pos.y < first.pos.y;
    }
}

void TrafficManager::calculateLeftTurnPath(VehicleState& state) {
    using namespace SimConstants;

    // Calculate larger radius for left turns
    state.turnRadius = TURN_GUIDE_RADIUS * 1.2f;

    // Calculate turn center and angles based on approach direction
    LaneId laneId = state.vehicle->getCurrentLane();

    if (laneId <= LaneId::AL3_FREELANE) {  // Coming from West
        // Turn center will be north-west of intersection
        state.turnCenter.x = CENTER_X - ROAD_WIDTH/2.0f;
        state.turnCenter.y = CENTER_Y - ROAD_WIDTH/2.0f;
        state.startAngle = 0.0f;  // Start facing east
        state.endAngle = -M_PI/2.0f;  // End facing north
        state.targetPos.x = CENTER_X;
        state.targetPos.y = -VEHICLE_HEIGHT;  // Exit north
    }
    else if (laneId <= LaneId::BL3_FREELANE) {  // Coming from North
        // Turn center will be north-east of intersection
        state.turnCenter.x = CENTER_X + ROAD_WIDTH/2.0f;
        state.turnCenter.y = CENTER_Y - ROAD_WIDTH/2.0f;
        state.startAngle = M_PI/2.0f;  // Start facing south
        state.endAngle = 0.0f;  // End facing east
        state.targetPos.x = WINDOW_WIDTH + VEHICLE_WIDTH;  // Exit east
        state.targetPos.y = CENTER_Y;
    }
    else if (laneId <= LaneId::CL3_FREELANE) {  // Coming from East
        // Turn center will be south-east of intersection
        state.turnCenter.x = CENTER_X + ROAD_WIDTH/2.0f;
        state.turnCenter.y = CENTER_Y + ROAD_WIDTH/2.0f;
        state.startAngle = M_PI;  // Start facing west
        state.endAngle = M_PI/2.0f;  // End facing south
        state.targetPos.x = CENTER_X;
        state.targetPos.y = WINDOW_HEIGHT + VEHICLE_HEIGHT;  // Exit south
    }
    else {  // Coming from South
        // Turn center will be south-west of intersection
        state.turnCenter.x = CENTER_X - ROAD_WIDTH/2.0f;
        state.turnCenter.y = CENTER_Y + ROAD_WIDTH/2.0f;
        state.startAngle = -M_PI/2.0f;  // Start facing north
        state.endAngle = M_PI;  // End facing west
        state.targetPos.x = -VEHICLE_WIDTH;  // Exit west
        state.targetPos.y = CENTER_Y;
    }

    // Adjust speed for turning
    state.speed = VEHICLE_TURN_SPEED;
    state.turnProgress = 0.0f;
    state.hasStartedTurn = true;
}

void TrafficManager::calculateRightTurnPath(VehicleState& state) {
    using namespace SimConstants;

    // Calculate smaller radius for right turns
    state.turnRadius = TURN_GUIDE_RADIUS * 0.8f;

    // Calculate turn center and angles based on approach direction
    LaneId laneId = state.vehicle->getCurrentLane();

    if (laneId <= LaneId::AL3_FREELANE) {  // Coming from West
        // Turn center will be south-west of intersection
        state.turnCenter.x = CENTER_X - ROAD_WIDTH/2.0f;
        state.turnCenter.y = CENTER_Y + ROAD_WIDTH/2.0f;
        state.startAngle = 0.0f;  // Start facing east
        state.endAngle = M_PI/2.0f;  // End facing south
        state.targetPos.x = CENTER_X;
        state.targetPos.y = WINDOW_HEIGHT + VEHICLE_HEIGHT;  // Exit south
    }
    else if (laneId <= LaneId::BL3_FREELANE) {  // Coming from North
        // Turn center will be north-west of intersection
        state.turnCenter.x = CENTER_X - ROAD_WIDTH/2.0f;
        state.turnCenter.y = CENTER_Y - ROAD_WIDTH/2.0f;
        state.startAngle = M_PI/2.0f;  // Start facing south
        state.endAngle = M_PI;  // End facing west
        state.targetPos.x = -VEHICLE_WIDTH;  // Exit west
        state.targetPos.y = CENTER_Y;
    }
    else if (laneId <= LaneId::CL3_FREELANE) {  // Coming from East
        // Turn center will be north-east of intersection
        state.turnCenter.x = CENTER_X + ROAD_WIDTH/2.0f;
        state.turnCenter.y = CENTER_Y - ROAD_WIDTH/2.0f;
        state.startAngle = M_PI;  // Start facing west
        state.endAngle = -M_PI/2.0f;  // End facing north
        state.targetPos.x = CENTER_X;
        state.targetPos.y = -VEHICLE_HEIGHT;  // Exit north
    }
    else {  // Coming from South
        // Turn center will be south-east of intersection
        state.turnCenter.x = CENTER_X + ROAD_WIDTH/2.0f;
        state.turnCenter.y = CENTER_Y + ROAD_WIDTH/2.0f;
        state.startAngle = -M_PI/2.0f;  // Start facing north
        state.endAngle = 0.0f;  // End facing east
        state.targetPos.x = WINDOW_WIDTH + VEHICLE_WIDTH;  // Exit east
        state.targetPos.y = CENTER_Y;
    }

    // Adjust speed for turning
    state.speed = VEHICLE_TURN_SPEED;
    state.turnProgress = 0.0f;
    state.hasStartedTurn = true;
}

bool TrafficManager::hasCompletedJourney(const Vehicle& vehicle) const {
    const Position pos{vehicle.getX(), vehicle.getY()};
    
    // Check if vehicle has reached the edge of the screen based on its lane
    switch (vehicle.getCurrentLane()) {
        case LaneId::AL1_INCOMING:
        case LaneId::AL2_PRIORITY:
        case LaneId::AL3_FREELANE:
            return pos.y >= SimConstants::WINDOW_HEIGHT;
            
        case LaneId::BL1_INCOMING:
        case LaneId::BL2_NORMAL:
        case LaneId::BL3_FREELANE:
            return pos.y <= 0;
            
        case LaneId::CL1_INCOMING:
        case LaneId::CL2_NORMAL:
        case LaneId::CL3_FREELANE:
            return pos.x <= 0;
            
        case LaneId::DL1_INCOMING:
        case LaneId::DL2_NORMAL:
        case LaneId::DL3_FREELANE:
            return pos.x >= SimConstants::WINDOW_WIDTH;
            
        default:
            return false;
    }
}

void TrafficManager::synchronizeTrafficLights() {
    // Set initial state for all traffic lights to red
    for (auto& [_, light] : trafficLights) {
        light.setState(LightState::RED);
    }
    
    // Start with priority lane (AL2) green
    auto it = trafficLights.find(LaneId::AL2_PRIORITY);
    if (it != trafficLights.end()) {
        it->second.setState(LightState::GREEN);
    }
}
