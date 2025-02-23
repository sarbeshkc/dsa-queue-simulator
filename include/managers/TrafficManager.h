// T// TrafficManager.h
#pragma once
#include "core/Constants.h"
#include "core/Lane.h"
#include "core/TrafficLight.h"
#include "core/Vehicle.h"
#include "managers/FileHandler.h"
#include "utils/PriorityQueue.h"
#include <vector>
#include <memory>
#include <map>
#include <chrono>

struct Position {
    float x;
    float y;
    Position(float x_ = 0.0f, float y_ = 0.0f) : x(x_), y(y_) {}
};

struct VehicleState {
    std::shared_ptr<Vehicle> vehicle;
    Position pos;                  // Current position
    Position targetPos;           // Target position
    float speed;                  // Current speed
    bool isMoving;               // Is vehicle in motion
    Direction direction;          // Turn direction
    bool hasStartedTurn;         // Has turn begun
    float turnProgress;          // Turn progress (0-1)
    float waitTime;              // Time in queue
    float turnAngle;             // Current angle
    float targetAngle;           // Target angle
    Position turnCenter;         // Center of turn
    float turnRadius;            // Turn radius
    float startAngle;           // Start angle for turn
    float endAngle;             // End angle for turn
    float processingTime;        // Time being processed
    size_t queuePosition;        // Position in lane queue
    bool inIntersection;        // Whether in intersection
    bool isPassing;             // Whether passing through intersection
};

class TrafficManager {
public:
    TrafficManager();

    // Core update methods
    void update(float deltaTime);
    void addVehicleToLane(LaneId laneId, std::shared_ptr<Vehicle> vehicle);
    size_t getLaneSize(LaneId laneId) const;

    // State queries
    bool isInPriorityMode() const { return inPriorityMode; }
    const std::vector<std::unique_ptr<Lane>>& getLanes() const { return lanes; }
    const std::map<LaneId, TrafficLight>& getTrafficLights() const { return trafficLights; }
    const std::map<uint32_t, VehicleState>& getActiveVehicles() const { return activeVehicles; }

private:
    // Core components
    std::vector<std::unique_ptr<Lane>> lanes;
    std::map<LaneId, TrafficLight> trafficLights;
    std::map<uint32_t, VehicleState> activeVehicles;
    FileHandler fileHandler;

    // State tracking
    bool inPriorityMode;
    float stateTimer;
    float lastUpdateTime;
    float processingTimer;
    size_t totalVehiclesProcessed;
    float averageWaitTime;

    // Constants
    static constexpr size_t PRIORITY_THRESHOLD = 10;
    static constexpr size_t PRIORITY_RELEASE_THRESHOLD = 5;
    static constexpr float MIN_STATE_TIME = 5.0f;
    static constexpr float MAX_STATE_TIME = 30.0f;
    static constexpr float VEHICLE_PROCESS_TIME = 2.0f;

    // Vehicle management methods
    void addNewVehicleToState(std::shared_ptr<Vehicle> vehicle, LaneId laneId);
    void updateVehiclePositions(float deltaTime);
    void updateStraightMovement(VehicleState& state, float deltaTime);
    void updateTurningMovement(VehicleState& state, float deltaTime);
    bool checkCollision(const VehicleState& state, float newX, float newY) const;
    float calculateTurningRadius(Direction dir) const;
    void calculateTurnPath(VehicleState& state);
    bool hasReachedDestination(const VehicleState& state) const;
    void updateVehicleQueuePosition(VehicleState& state, LaneId laneId, size_t queuePosition);
    void calculateTargetPosition(VehicleState& state, LaneId laneId);

    // Lane management methods
    LaneId determineOptimalLane(Direction direction, LaneId sourceLane) const;
    bool isValidSpawnLane(LaneId laneId, Direction direction) const;
    bool isFreeLane(LaneId laneId) const;
    Lane* getPriorityLane() const;
    void processNewVehicles();
    Position calculateLaneEndpoint(LaneId laneId) const;
    bool isInIntersection(const Position& pos) const;

    bool isNearIntersection(const VehicleState& state) const;

    // Traffic light management
    void updateTrafficLights(float deltaTime);
    void synchronizeTrafficLights();
    void handleStateTransition(float deltaTime);
    bool checkPriorityConditions() const;
    bool canVehicleMove(const VehicleState& state) const;

    // Queue processing
    void processPriorityLane();
    void processNormalLanes(size_t vehicleCount);
    void processFreeLanes();
    size_t calculateVehiclesToProcess() const;
    void checkWaitTimes();
    void updateTimers(float deltaTime);

    // Statistics and metrics
    void updateStatistics(float deltaTime);
    float calculateAverageWaitTime() const;
    size_t getQueuedVehicleCount() const;
    void cleanupRemovedVehicles();
};


