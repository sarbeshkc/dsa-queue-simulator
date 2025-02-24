// Lane.h
#pragma once
#include "core/Constants.h"
#include "core/Vehicle.h"
#include <queue>
#include <memory>

class Lane {
private:
    LaneId id;
    bool isPriority;
    std::queue<std::shared_ptr<Vehicle>> vehicles;

public:
    Lane(LaneId laneId, bool priority = false);

    void addVehicle(std::shared_ptr<Vehicle> vehicle);
    std::shared_ptr<Vehicle> peekFront() const;
    std::shared_ptr<Vehicle> dequeue();
    std::shared_ptr<Vehicle> removeVehicle();
    LaneId getLaneId() const;
    LaneId getId() const;
    bool isPriorityLane() const;
    size_t getQueueSize() const;
};
