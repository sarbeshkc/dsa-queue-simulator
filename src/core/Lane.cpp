// src/core/Lane.cpp
#include "core/Lane.h"

Lane::Lane(LaneId laneId, bool priority)
    : id(laneId)
    , isPriority(priority) {
}

void Lane::addVehicle(std::shared_ptr<Vehicle> vehicle) {
    vehicles.push(vehicle);
}

std::shared_ptr<Vehicle> Lane::peekFront() const {
    return vehicles.empty() ? nullptr : vehicles.front();
}

std::shared_ptr<Vehicle> Lane::dequeue() {
    if (vehicles.empty()) {
        return nullptr;
    }
    auto vehicle = vehicles.front();
    vehicles.pop();
    return vehicle;
}

std::shared_ptr<Vehicle> Lane::removeVehicle() {
    return dequeue();
}

LaneId Lane::getLaneId() const {
    return id;
}

LaneId Lane::getId() const {
    return getLaneId();
}

bool Lane::isPriorityLane() const {
    return isPriority;
}

size_t Lane::getQueueSize() const {
    return vehicles.size();
}
