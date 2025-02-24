# pragma once
#include "core/Constants.h"
#include <cstdint>
#include <chrono>
#include <memory>
#include <string>

class Vehicle {
private:
    uint32_t id;
    Direction direction;
    LaneId currentLane;
    LaneId targetLane;  // Added for lane transitions
    float waitTime;
    bool isProcessing;
    float turnProgress;
    bool hasStartedTurn;
    float speed;
    float position;
    std::chrono::steady_clock::time_point entryTime;

    struct Position {
        float x;
        float y;
        float angle;
        float targetX;
        float targetY;
        float targetAngle;
    } pos;

public:
    Vehicle(uint32_t vehicleId, Direction dir, LaneId lane);

    // Core accessors
    uint32_t getId() const { return id; }
    Direction getDirection() const { return direction; }
    LaneId getCurrentLane() const { return currentLane; }
    LaneId getTargetLane() const { return targetLane; }
    bool isInProcess() const { return isProcessing; }
    float getWaitTime() const { return waitTime; }
    float getTurnProgress() const { return turnProgress; }
    bool hasTurnStarted() const { return hasStartedTurn; }
    float getSpeed() const { return speed; }
    float getPosition() const { return position; }

    // Position getters
    float getX() const { return pos.x; }
    float getY() const { return pos.y; }
    float getAngle() const { return pos.angle; }
    float getTargetX() const { return pos.targetX; }
    float getTargetY() const { return pos.targetY; }
    float getTargetAngle() const { return pos.targetAngle; }

    // State modifiers
    void setProcessing(bool processing);
    void updateWaitTime(float delta);
    void updateTurnProgress(float delta);
    void startTurn();
    void setSpeed(float newSpeed);
    void setPosition(float pos);
    void setTargetLane(LaneId lane) { targetLane = lane; }

    // Movement control
    void setTargetPosition(float x, float y, float angle);
    void updateMovement(float deltaTime);
    bool hasReachedTarget() const;
    float calculateTurnRadius() const;

    // Static helpers
    static float calculateLanePosition(LaneId lane, size_t queuePosition);
    static float calculateTurnAngle(Direction dir, LaneId fromLane, LaneId toLane);

    // Queue metrics
    std::chrono::steady_clock::time_point getEntryTime() const { return entryTime; }
    float getTimeInSystem() const;

    // Debug helper
    std::string toString() const;
};
