// IntersectionController.h
#pragma once

#include "core/Lane.h"
#include "core/Vehicle.h"
#include "utils/PriorityQueue.h"
#include <vector>
#include <memory>
#include <cmath>

class IntersectionController {
public:
    // Public types for status reporting
    struct LaneStatus {
        LaneId id;
        size_t queueSize;
        int priority;
        bool isActive;
        float waitTime;
    };

    struct ProcessingStats {
        float avgWaitTime;
        size_t totalVehiclesProcessed;
        size_t currentQueueSizes[12]; // One for each lane
        bool isPriorityMode;
    };

    // Constructor
    explicit IntersectionController(std::vector<std::unique_ptr<Lane>>& lanes);

    // Core update method
    void update(float deltaTime);

    // State query methods
    bool isInPriorityMode() const { return isPriorityMode; }
    std::vector<LaneStatus> getLaneStatuses() const;
    ProcessingStats getStats() const;

private:
    // Core components
    std::vector<std::unique_ptr<Lane>>& lanes;
    PriorityQueue<LaneId> laneQueue;

    // State tracking
    bool isPriorityMode;
    float stateTimer;
    float elapsedTime;
    float processingTimer;
    size_t vehiclesProcessedInState;
    size_t totalVehiclesProcessed;

    // Configuration constants
    static constexpr size_t PRIORITY_THRESHOLD = 10;
    static constexpr size_t PRIORITY_RELEASE_THRESHOLD = 5;
    static constexpr float BASE_VEHICLE_PROCESS_TIME = 2.0f;
    static constexpr float MIN_STATE_TIME = 5.0f;
    static constexpr float MAX_STATE_TIME = 30.0f;
    static constexpr float MAX_WAIT_TIME = 45.0f;

    // Queue management methods
    void updateLaneQueue();
    void processPriorityLane();
    void processNormalLanes();
    void processFreeLanes();
    size_t calculateVehiclesToProcess() const;

    // State management methods
    void handleStateTransition();
    void checkWaitTimes();
    void updateTimers(float deltaTime);

    // Utility methods
    float calculateAverageWaitingVehicles() const;
    float calculateProcessingTime() const;
    bool shouldSwitchToNormalMode() const;
    bool shouldSwitchToPriorityMode() const;
    void resetStateTimers();
    Lane* getPriorityLane() const;
    bool isFreeLane(LaneId id) const;
    int calculateLanePriority(const Lane& lane) const;
};
