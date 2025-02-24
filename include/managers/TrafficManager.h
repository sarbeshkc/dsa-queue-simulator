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
    bool isChangingLanes;       // Whether changing lanes
    bool hasStoppedAtLight;     // Whether stopped at light
    std::vector<Position> intermediateTargets;  // Waypoints for lane changes
    size_t currentTargetIndex;   // Current waypoint index
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

    // Vehicle movement methods
    void updateVehiclePositions(float deltaTime);
    void updateVehicleMovement(VehicleState& state, float deltaTime);
    void updateStraightMovement(VehicleState& state, float deltaTime);
    void updateTurningMovement(VehicleState& state, float deltaTime);
    bool checkCollision(const VehicleState& state, float newX, float newY) const;
    void calculateTurnParameters(VehicleState& state);
    void calculateTurnPath(VehicleState& state);
    void calculateLeftTurnPath(VehicleState& state);
    void calculateRightTurnPath(VehicleState& state);
    void calculateTargetPosition(VehicleState& state, LaneId laneId);
    void calculateInitialPosition(VehicleState& state, LaneId laneId);
    bool hasReachedDestination(const VehicleState& state) const;
    void completeTurn(VehicleState& state);
    void checkTrafficLightAndProceed(VehicleState& state);

    // Vehicle state management
    void addNewVehicleToState(std::shared_ptr<Vehicle> vehicle, LaneId laneId);
    void updateVehicleQueuePosition(VehicleState& state, LaneId laneId, size_t queuePosition);
    bool isNearIntersection(const VehicleState& state) const;
    bool isInIntersection(const Position& pos) const;
    float getDistanceToIntersection(const VehicleState& state) const;
    bool hasVehicleAhead(const VehicleState& state) const;
    bool isVehicleAhead(const VehicleState& first, const VehicleState& second) const;
    float calculateTurningRadius(Direction dir) const;

    // Traffic flow methods
    void processNewVehicles();
    void processPriorityLane();
    void processNormalLanes(size_t vehicleCount);
    void processFreeLanes();
    void checkWaitTimes();
    size_t calculateVehiclesToProcess() const;

    // Lane management methods
    LaneId determineOptimalLane(Direction direction, LaneId sourceLane) const;
    LaneId determineTargetLane(LaneId currentLane, Direction direction) const;
    bool isValidSpawnLane(LaneId laneId, Direction direction) const;
    bool isFreeLane(LaneId laneId) const;
    Lane* getPriorityLane() const;
    Position calculateLaneEndpoint(LaneId laneId) const;
    void changeLaneToFree(VehicleState& state);
    void changeLaneToFirst(VehicleState& state);

    // Traffic light management
    void updateTrafficLights();
    void synchronizeTrafficLights();
    void handleStateTransition(float deltaTime);
    bool checkPriorityConditions() const;
    bool canVehicleMove(const VehicleState& state) const;
    LightState getLightStateForLane(LaneId laneId) const;

    // Utility methods
    void updateTimers(float deltaTime);
    void cleanupRemovedVehicles();
    void removeVehicle(uint32_t vehicleId);
    void updateStatistics(float deltaTime);
    float calculateAverageWaitTime() const;
    size_t getQueuedVehicleCount() const;

    void checkTurningConditions(Vehicle& vehicle);
    void processQueuedVehicles();
    bool hasCompletedJourney(const Vehicle& vehicle) const;
};
