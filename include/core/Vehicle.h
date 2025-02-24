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
    LaneId targetLane;
    float waitTime = 0.0f;
    bool isProcessing = false;
    float turnProgress = 0.0f;
    bool hasStartedTurn = false;
    float speed = 0.0f;
    float position = 0.0f;
    std::chrono::steady_clock::time_point entryTime;

    // Animation and movement state
    bool isTurning = false;
    bool isMoving = false;
    float turnAngle = 0.0f;
    float turnRadius = 0.0f;
    float turnCenterX = 0.0f;
    float turnCenterY = 0.0f;

    // Turning control points
    float turnStartX = 0.0f;
    float turnStartY = 0.0f;
    float turnControlX = 0.0f;
    float turnControlY = 0.0f;
    float turnEndX = 0.0f;
    float turnEndY = 0.0f;
    int lane_number = 2;  // 1 for left, 2 for middle, 3 for right

    struct Position {
        float x = 0.0f;
        float y = 0.0f;
        float angle = 0.0f;
        float targetX = 0.0f;
        float targetY = 0.0f;
        float targetAngle = 0.0f;
    } pos;

public:
    static constexpr float TURN_SPEED = 0.8f;  // Units per second for turning animation
    static constexpr float BASE_SPEED = 50.0f;
    static constexpr float TURN_SPEED_FACTOR = 0.75f;

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
    bool getIsTurning() const { return isTurning; }
    bool getIsMoving() const { return isMoving; }
    float getTurnAngle() const { return turnAngle; }
    float getTurnRadius() const { return turnRadius; }
    float getTurnCenterX() const { return turnCenterX; }
    float getTurnCenterY() const { return turnCenterY; }

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
    void setTargetLane(LaneId lane);
    void setCurrentLane(LaneId lane);
    void setIsTurning(bool turning);
    void setIsMoving(bool moving);
    void setTurnAngle(float angle);
    void setTurnRadius(float radius);
    void setTurnCenter(float x, float y);
    void setTurnProgress(float progress);

    // Position setters
    void setX(float x);
    void setY(float y);
    void setAngle(float angle);
    void setTargetPosition(float x, float y, float angle);

    // Movement control
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

    // Turning animation constants
    static constexpr float TURN_DURATION = 1.5f;  // 1.5 seconds for a turn
    static constexpr float BEZIER_CONTROL_OFFSET = 80.0f;

    // Turning animation helpers
    void calculateTurnCurve(float t, float& outX, float& outY) const;
    float easeInOutQuad(float t) const;
};
