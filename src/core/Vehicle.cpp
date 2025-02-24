// src/core/Vehicle.cpp
#include "core/Vehicle.h"
#include <sstream>
#include <iomanip>
#include <cmath>

Vehicle::Vehicle(uint32_t vehicleId, Direction dir, LaneId lane)
    : id(vehicleId)
    , direction(dir)
    , currentLane(lane)
    , targetLane(lane)
    , entryTime(std::chrono::steady_clock::now())
{
    // All other members are zero-initialized in the header
}

void Vehicle::setProcessing(bool processing) {
    isProcessing = processing;
    if (processing) {
        speed = SimConstants::VEHICLE_BASE_SPEED;
    }
}

void Vehicle::updateWaitTime(float delta) {
    if (!isProcessing) {
        waitTime += delta;
    }
}

void Vehicle::updateTurnProgress(float delta) {
    if (hasStartedTurn && turnProgress < 1.0f) {
        turnProgress = std::min(1.0f, turnProgress + delta);
    }
}

void Vehicle::startTurn() {
    hasStartedTurn = true;
    isTurning = true;
    turnProgress = 0.0f;
    speed = SimConstants::VEHICLE_TURN_SPEED;
    
    // Save current position as turn start
    turnStartX = pos.x;
    turnStartY = pos.y;
    
    // Calculate control and end points based on current lane and target lane
    float centerX = static_cast<float>(SimConstants::CENTER_X);
    float centerY = static_cast<float>(SimConstants::CENTER_Y);
    float roadWidth = static_cast<float>(SimConstants::ROAD_WIDTH);
    float laneWidth = roadWidth / 3.0f;
    
    switch (currentLane) {
        case LaneId::AL2_PRIORITY: // Turning left from A to B
            turnControlX = centerX + 50.0f;
            turnControlY = pos.y + (centerY - pos.y) / 2.0f;
            turnEndX = centerX;
            turnEndY = centerY + roadWidth/2.0f + 20.0f;
            break;
            
        case LaneId::BL2_NORMAL: // Turning left from B to A
            turnControlX = centerX - 50.0f;
            turnControlY = pos.y - (pos.y - centerY) / 2.0f;
            turnEndX = centerX;
            turnEndY = centerY - roadWidth/2.0f - 20.0f;
            break;
            
        case LaneId::CL2_NORMAL: // Turning left from C to D
            turnControlX = pos.x - (pos.x - centerX) / 2.0f;
            turnControlY = centerY + 50.0f;
            turnEndX = centerX - roadWidth/2.0f - 20.0f;
            turnEndY = centerY;
            break;
            
        case LaneId::DL2_NORMAL: // Turning left from D to C
            turnControlX = pos.x + (centerX - pos.x) / 2.0f;
            turnControlY = centerY - 50.0f;
            turnEndX = centerX + roadWidth/2.0f + 20.0f;
            turnEndY = centerY;
            break;
            
        default:
            break;
    }
}

void Vehicle::setSpeed(float newSpeed) {
    speed = newSpeed;
}

void Vehicle::setPosition(float newPos) {
    position = newPos;
}

void Vehicle::setTargetPosition(float x, float y, float angle) {
    pos.targetX = x;
    pos.targetY = y;
    pos.targetAngle = angle;
}

void Vehicle::updateMovement(float deltaTime) {
    if (!isProcessing) return;

    // Handle turning animation
    if (isTurning) {
        turnProgress += deltaTime * TURN_SPEED * TURN_SPEED_FACTOR;
        if (turnProgress > 1.0f) turnProgress = 1.0f;

        // Calculate position using quadratic Bezier curve
        float t = easeInOutQuad(turnProgress);
        calculateTurnCurve(t, pos.x, pos.y);

        // Update angle for smooth rotation
        float dx = pos.x - turnStartX;
        float dy = pos.y - turnStartY;
        pos.angle = std::atan2f(dy, dx);

        if (turnProgress >= 1.0f) {
            isTurning = false;
            currentLane = targetLane;
            turnProgress = 0.0f;
            pos.x = turnEndX;
            pos.y = turnEndY;
            speed = BASE_SPEED;
        }
        return;
    }

    // Normal forward movement
    float dx = pos.targetX - pos.x;
    float dy = pos.targetY - pos.y;
    float distance = std::sqrt(dx * dx + dy * dy);

    if (distance > 0.1f) {
        float moveSpeed = speed * deltaTime;
        float moveRatio = std::min(1.0f, moveSpeed / distance);

        pos.x += dx * moveRatio;
        pos.y += dy * moveRatio;

        // Update angle smoothly
        float targetAngle = std::atan2f(dy, dx);
        float angleDiff = targetAngle - pos.angle;

        // Normalize angle to [-π, π]
        while (angleDiff > static_cast<float>(M_PI)) {
            angleDiff -= 2.0f * static_cast<float>(M_PI);
        }
        while (angleDiff < -static_cast<float>(M_PI)) {
            angleDiff += 2.0f * static_cast<float>(M_PI);
        }

        pos.angle += angleDiff * moveRatio;
    }
}

bool Vehicle::hasReachedTarget() const {
    float dx = pos.targetX - pos.x;
    float dy = pos.targetY - pos.y;
    return std::sqrt(dx * dx + dy * dy) < 0.1f;
}

float Vehicle::calculateTurnRadius() const {
    switch (direction) {
        case Direction::LEFT:
            return SimConstants::TURN_GUIDE_RADIUS * 1.2f;
        case Direction::RIGHT:
            return SimConstants::TURN_GUIDE_RADIUS * 0.8f;
        default:
            return SimConstants::TURN_GUIDE_RADIUS;
    }
}

float Vehicle::calculateLanePosition(LaneId lane, size_t queuePosition) {
    using namespace SimConstants;
    float baseOffset = QUEUE_START_OFFSET + queuePosition * QUEUE_SPACING;

    switch (lane) {
        case LaneId::AL1_INCOMING:
        case LaneId::AL2_PRIORITY:
        case LaneId::AL3_FREELANE:
            return CENTER_X - baseOffset;

        case LaneId::BL1_INCOMING:
        case LaneId::BL2_NORMAL:
        case LaneId::BL3_FREELANE:
            return CENTER_Y - baseOffset;

        case LaneId::CL1_INCOMING:
        case LaneId::CL2_NORMAL:
        case LaneId::CL3_FREELANE:
            return CENTER_X + baseOffset;

        case LaneId::DL1_INCOMING:
        case LaneId::DL2_NORMAL:
        case LaneId::DL3_FREELANE:
            return CENTER_Y + baseOffset;

        default:
            return 0.0f;
    }
}

float Vehicle::calculateTurnAngle(Direction dir, LaneId fromLane, LaneId /*toLane*/) {
    const float WEST_ANGLE = 0.0f;
    const float NORTH_ANGLE = static_cast<float>(M_PI) / 2.0f;
    const float EAST_ANGLE = static_cast<float>(M_PI);
    const float SOUTH_ANGLE = -static_cast<float>(M_PI) / 2.0f;

    // Get base angle from source lane
    float baseAngle;
    if (fromLane <= LaneId::AL3_FREELANE) baseAngle = WEST_ANGLE;
    else if (fromLane <= LaneId::BL3_FREELANE) baseAngle = NORTH_ANGLE;
    else if (fromLane <= LaneId::CL3_FREELANE) baseAngle = EAST_ANGLE;
    else baseAngle = SOUTH_ANGLE;

    // Adjust for turn direction
    switch (dir) {
        case Direction::LEFT:
            return baseAngle - static_cast<float>(M_PI) / 2.0f;
        case Direction::RIGHT:
            return baseAngle + static_cast<float>(M_PI) / 2.0f;
        default:
            return baseAngle;
    }
}

float Vehicle::getTimeInSystem() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<float>(now - entryTime).count();
}

std::string Vehicle::toString() const {
    std::stringstream ss;
    ss << "Vehicle[ID:" << id
       << ", Lane:" << static_cast<int>(currentLane)
       << ", Dir:" << static_cast<int>(direction)
       << ", Pos:(" << std::fixed << std::setprecision(1)
       << pos.x << "," << pos.y << ")"
       << ", Wait:" << std::setprecision(1) << waitTime << "s"
       << ", Turn:" << (hasStartedTurn ? "Yes" : "No")
       << ", Progress:" << std::setprecision(2) << turnProgress * 100 << "%]";
    return ss.str();
}

void Vehicle::setTargetLane(LaneId lane) {
    targetLane = lane;
}

void Vehicle::setCurrentLane(LaneId lane) {
    currentLane = lane;
}

void Vehicle::setIsTurning(bool turning) {
    isTurning = turning;
}

void Vehicle::setIsMoving(bool moving) {
    isMoving = moving;
}

void Vehicle::setTurnAngle(float angle) {
    turnAngle = angle;
}

void Vehicle::setTurnRadius(float radius) {
    turnRadius = radius;
}

void Vehicle::setTurnCenter(float x, float y) {
    turnCenterX = x;
    turnCenterY = y;
}

void Vehicle::setTurnProgress(float progress) {
    turnProgress = progress;
}

void Vehicle::setX(float x) {
    pos.x = x;
}

void Vehicle::setY(float y) {
    pos.y = y;
}

void Vehicle::setAngle(float angle) {
    pos.angle = angle;
}

float Vehicle::easeInOutQuad(float t) const {
    return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
}

void Vehicle::calculateTurnCurve(float t, float& outX, float& outY) const {
    outX = (1-t)*(1-t)*turnStartX + 2*(1-t)*t*turnControlX + t*t*turnEndX;
    outY = (1-t)*(1-t)*turnStartY + 2*(1-t)*t*turnControlY + t*t*turnEndY;
}
