// FILE: src/core/Lane.cpp
#include "core/Lane.h"
#include "utils/DebugLogger.h"
#include <sstream>
#include "core/Constants.h"

Lane::Lane(char laneId, int laneNumber)
    : laneId(laneId),
      laneNumber(laneNumber),
      isPriority(laneId == 'A' && laneNumber == 2), // AL2 is the priority lane
      priority(0) {

    std::ostringstream oss;
    oss << "Created lane " << laneId << laneNumber;
    DebugLogger::log(oss.str());
}

Lane::~Lane() {
    // Clean up vehicles
    while (!vehicleQueue.isEmpty()) {
        Vehicle* vehicle = vehicleQueue.dequeue();
        delete vehicle;
    }
}

void Lane::enqueue(Vehicle* vehicle) {
    if (!vehicle) {
        DebugLogger::log("Attempted to enqueue null vehicle", DebugLogger::LogLevel::ERROR);
        return;
    }

    vehicleQueue.enqueue(vehicle);
    int currentCount = vehicleQueue.size();

    // Log the action
    std::ostringstream oss;
    oss << "Vehicle " << vehicle->getId() << " added to lane " << laneId << laneNumber;
    DebugLogger::log(oss.str());

    // Update priority if this is the priority lane
    if (isPriority) {
        if (currentCount > Constants::PRIORITY_THRESHOLD_HIGH) {
            priority = 100; // High priority
            std::ostringstream priorityOss;
            priorityOss << "Lane " << laneId << laneNumber
                << " priority increased (vehicles: " << currentCount << ")";
            DebugLogger::log(priorityOss.str());
        }
        else if (currentCount < Constants::PRIORITY_THRESHOLD_LOW) {
            priority = 0; // Normal priority
            std::ostringstream priorityOss;
            priorityOss << "Lane " << laneId << laneNumber
                << " priority reset to normal (vehicles: " << currentCount << ")";
            DebugLogger::log(priorityOss.str());
        }
    }
}

Vehicle* Lane::dequeue() {
    if (vehicleQueue.isEmpty()) {
        return nullptr;
    }

    Vehicle* vehicle = vehicleQueue.dequeue();
    int currentCount = vehicleQueue.size();

    // Log the action
    std::ostringstream oss;
    oss << "Vehicle " << vehicle->getId() << " removed from lane " << laneId << laneNumber;
    DebugLogger::log(oss.str());

    // Update priority if this is the priority lane
    if (isPriority) {
        if (currentCount < Constants::PRIORITY_THRESHOLD_LOW && priority > 0) {
            priority = 0; // Normal priority
            std::ostringstream priorityOss;
            priorityOss << "Lane " << laneId << laneNumber
                << " priority reset to normal (vehicles: " << currentCount << ")";
            DebugLogger::log(priorityOss.str());
        }
    }

    return vehicle;
}

Vehicle* Lane::peek() const {
    if (vehicleQueue.isEmpty()) {
        return nullptr;
    }

    return vehicleQueue.peek();
}

bool Lane::isEmpty() const {
    return vehicleQueue.isEmpty();
}

int Lane::getVehicleCount() const {
    return vehicleQueue.size();
}

const std::vector<Vehicle*>& Lane::getVehicles() const {
    // Get all elements from the queue for rendering
    return vehicleQueue.getAllElements();
}

int Lane::getPriority() const {
    return priority;
}

void Lane::updatePriority() {
    int count = vehicleQueue.size();

    // Update priority based on vehicle count for AL2 lane
    if (isPriority) {
        if (count > Constants::PRIORITY_THRESHOLD_HIGH && priority == 0) {
            priority = 100; // High priority
            std::ostringstream oss;
            oss << "Lane " << laneId << laneNumber
                << " priority increased (vehicles: " << count << ")";
            DebugLogger::log(oss.str());
        }
        else if (count < Constants::PRIORITY_THRESHOLD_LOW && priority > 0) {
            priority = 0; // Normal priority
            std::ostringstream oss;
            oss << "Lane " << laneId << laneNumber
                << " priority reset to normal (vehicles: " << count << ")";
            DebugLogger::log(oss.str());
        }
    }
}

bool Lane::isPriorityLane() const {
    return isPriority;
}

char Lane::getLaneId() const {
    return laneId;
}

int Lane::getLaneNumber() const {
    return laneNumber;
}

std::string Lane::getName() const {
    std::string name;
    name += laneId;
    name += std::to_string(laneNumber);
    return name;
}
