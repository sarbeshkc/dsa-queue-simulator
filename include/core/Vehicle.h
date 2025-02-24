// include/core/Vehicle.h (modified)
#pragma once
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
    bool isEmergency;
    bool turning;

    // Animation state variables
    struct AnimationState {
        float x;             // Current X position
        float y;             // Current Y position
        float targetX;       // Target X position
        float targetY;       // Target Y position
        float angle;         // Current angle/rotation
        float targetAngle;   // Target angle
        float turnPosX;      // Position during turn
        float turnPosY;      // Position during turn
        float turnCenter_x;  // Center of turn circle X
        float turnCenter_y;  // Center of turn circle Y
        float startAngle;    // Start angle for turn
        float endAngle;      // End angle for turn
        float turnRadius;    // Radius of turn
    };

    AnimationState pos;  // Animation position state

    // Add the missing methods
    void initializePosition();
    void calculateNewTargetPosition();

public:
    Vehicle(uint32_t vehicleId, Direction dir, LaneId lane, bool emergency = false);

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
    bool isEmergencyVehicle() const { return isEmergency; }
    bool isTurning() const { return turning; }

    // Animation state getters
    float getX() const { return pos.x; }
    float getY() const { return pos.y; }
    float getAngle() const { return pos.angle; }
    float getTargetX() const { return pos.targetX; }
    float getTargetY() const { return pos.targetY; }
    float getTargetAngle() const { return pos.targetAngle; }
    float getTurnPosX() const { return pos.turnPosX; }
    float getTurnPosY() const { return pos.turnPosY; }

    // State modifiers
    void setProcessing(bool processing);
    void updateWaitTime(float delta);
    void updateTurnProgress(float delta);
    void startTurn();
    void setSpeed(float newSpeed);
    void setPosition(float pos);
    void setTargetLane(LaneId lane) { targetLane = lane; }
    void setTurning(bool isTurning) { turning = isTurning; }

    // Movement control
    void setTargetPosition(float x, float y, float angle);
    void updateMovement(float deltaTime);
    bool hasReachedTarget() const;

    // Turn animation
    void setupTurn(float centerX, float centerY, float radius, float startAng, float endAng);
    void updateTurn(float deltaTime);
    void setTurnPosition(float x, float y);

    // Calculate turn parameters based on lane/direction
    void calculateTurnParameters(float roadWidth, float laneWidth, float centerX, float centerY);
    float calculateTurnRadius() const;

    // Static helpers
    static float calculateLanePosition(LaneId lane, size_t queuePosition);
    static float calculateTurnAngle(Direction dir, LaneId fromLane, LaneId toLane);

    // Queue metrics
    std::chrono::steady_clock::time_point getEntryTime() const { return entryTime; }
    float getTimeInSystem() const;

    // Debug helper
    std::string toString() const;
};;
