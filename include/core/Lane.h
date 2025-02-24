// include/core/Lane.h
#pragma once
#include "Vehicle.h"
#include "utils/Queue.h"
#include <memory>
#include <string>

class Lane {
private:
    LaneId id;
    Queue<std::shared_ptr<Vehicle>> vehicleQueue;
    bool isPriority;
    std::string dataFile;
    float waitTime;  // Track the average wait time for this lane
    bool isFreeLane;  // Flag for 3rd lanes that can freely turn left

public:
    Lane(LaneId id, bool isPriority);

    void addVehicle(std::shared_ptr<Vehicle> vehicle);
    std::shared_ptr<Vehicle> removeVehicle();
    Direction getVehicleDirection(size_t index) const;
    size_t getQueueSize() const;
    bool isPriorityLane() const;
    LaneId getId() const;
    const std::string& getDataFile() const;
    void update(float deltaTime);
    bool isFreeLaneType() const;
    float getAverageWaitTime() const;
    void updateVehicleWaitTimes(float deltaTime);
    bool exceedsPriorityThreshold() const;  // Check if lane exceeds priority threshold
    bool isBelowNormalThreshold() const;    // Check if lane is below normal threshold
};
;
