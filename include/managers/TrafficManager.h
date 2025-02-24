// include/managers/TrafficManager.h
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

class TrafficManager {
public:
    TrafficManager();
    ~TrafficManager() = default;

    // Core update methods
    void update(float deltaTime);
    void addVehicleToLane(LaneId laneId, std::shared_ptr<Vehicle> vehicle);
    size_t getLaneSize(LaneId laneId) const;

    // State queries
    bool isInPriorityMode() const { return inPriorityMode; }
    const std::vector<std::unique_ptr<Lane>>& getLanes() const { return lanes; }
    const std::map<LaneId, TrafficLight>& getTrafficLights() const { return trafficLights; }
    const std::map<uint32_t, std::shared_ptr<Vehicle>>& getActiveVehicles() const { return activeVehicles; }

    // Stats queries
    float getAverageWaitingTime() const;
    size_t getTotalVehiclesProcessed() const { return totalVehiclesProcessed; }

private:
    // Core components
    std::vector<std::unique_ptr<Lane>> lanes;
    std::map<LaneId, TrafficLight> trafficLights;
    std::map<uint32_t, std::shared_ptr<Vehicle>> activeVehicles;
    FileHandler fileHandler;
    PriorityQueue<LaneId> laneQueue;

    // State tracking
    bool inPriorityMode;
    float stateTimer;
    float lastUpdateTime;
    float processingTimer;
    size_t totalVehiclesProcessed;
    float averageWaitTime;

    // Traffic flow methods
    void processNewVehicles();
    void processPriorityLane();
    void processNormalLanes(size_t vehicleCount);
    void processFreeLanes();
    void checkWaitTimes();
    size_t calculateVehiclesToProcess() const;
    void updateVehiclePositions(float deltaTime);
    void updateLaneQueue();
    bool checkCollision(const std::shared_ptr<Vehicle>& vehicle, float newX, float newY) const;

    // Traffic light methods
    void updateTrafficLights(float deltaTime);
    void synchronizeTrafficLights();
    void handleStateTransition(float deltaTime);
    bool checkPriorityConditions() const;
    bool canVehicleMove(const std::shared_ptr<Vehicle>& vehicle) const;
    LightState getLightStateForLane(LaneId laneId) const;

    // Vehicle processing
    void updateVehicleTurns(float deltaTime);
    LaneId determineOptimalLane(Direction direction, LaneId sourceLane) const;
    bool isValidSpawnLane(LaneId laneId, Direction direction) const;
    bool isFreeLane(LaneId laneId) const;
    Lane* getPriorityLane() const;
    void removeVehicle(uint32_t vehicleId);
    void setupVehicleTurn(std::shared_ptr<Vehicle> vehicle);

    // Utility methods
    void updateTimers(float deltaTime);
    void updateStatistics(float deltaTime);
    void cleanupFinishedVehicles();
};
