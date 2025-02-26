#include "core/Lane.h"
#include "utils/DebugLogger.h"
#include <algorithm>
#include <sstream>

Lane::Lane(char laneId, int laneNumber)
    : laneId(laneId),
      laneNumber(laneNumber),
      isPriority(laneId == 'A' && laneNumber == 2), // AL2 is the priority lane
      priority(0) {

    std::stringstream ss;
    ss << "Created lane " << laneId << laneNumber << " (Priority: " << (isPriority ? "Yes" : "No") << ")";
    DebugLogger::log(ss.str());
}

Lane::~Lane() {
    // Lock the mutex to ensure thread safety when cleaning up
    std::lock_guard<std::mutex> lock(mutex);
    for (auto vehicle : vehicles) {
        delete vehicle;
    }
    vehicles.clear();
}

void Lane::enqueue(Vehicle* vehicle) {
    // Lock the mutex to ensure thread safety
    std::lock_guard<std::mutex> lock(mutex);

    // Set vehicle's lane properties
    vehicle->setLane(laneId);
    vehicle->setLaneNumber(laneNumber);

    // Initialize animation position based on lane
    if (laneId == 'A') {
        vehicle->setAnimationPos(0.0f); // Top of screen
    } else if (laneId == 'B') {
        vehicle->setAnimationPos(800.0f); // Bottom of screen (assuming 800 height)
    } else if (laneId == 'C') {
        vehicle->setAnimationPos(800.0f); // Right of screen (assuming 800 width)
    } else if (laneId == 'D') {
        vehicle->setAnimationPos(0.0f); // Left of screen
    }

    vehicles.push_back(vehicle);

    std::stringstream ss;
    ss << "Enqueued vehicle " << vehicle->getId() << " to lane " << laneId << laneNumber
       << " (Queue size: " << vehicles.size() << ")";
    DebugLogger::log(ss.str());

    // Update priority if this is a priority lane
    if (isPriority) {
        updatePriority();
    }
}

Vehicle* Lane::dequeue() {
    // Lock the mutex to ensure thread safety
    std::lock_guard<std::mutex> lock(mutex);

    if (vehicles.empty()) {
        return nullptr;
    }

    Vehicle* vehicle = vehicles.front();
    vehicles.erase(vehicles.begin());

    std::stringstream ss;
    ss << "Dequeued vehicle " << vehicle->getId() << " from lane " << laneId << laneNumber
       << " (Queue size: " << vehicles.size() << ")";
    DebugLogger::log(ss.str());

    // Update priority if this is a priority lane
    if (isPriority) {
        updatePriority();
    }

    return vehicle;
}

Vehicle* Lane::peek() const {
    // Lock the mutex to ensure thread safety
    std::lock_guard<std::mutex> lock(mutex);

    if (vehicles.empty()) {
        return nullptr;
    }

    return vehicles.front();
}

bool Lane::isEmpty() const {
    // Lock the mutex to ensure thread safety
    std::lock_guard<std::mutex> lock(mutex);
    return vehicles.empty();
}

int Lane::getVehicleCount() const {
    // Lock the mutex to ensure thread safety
    std::lock_guard<std::mutex> lock(mutex);
    return vehicles.size();
}

int Lane::getPriority() const {
    // Priority doesn't need a lock as it's an atomic read
    return priority;
}

void Lane::updatePriority() {
    // Use a separate lock for priority update to avoid deadlocks
    std::lock_guard<std::mutex> lock(mutex);

    // For AL2 (priority lane), priority is based on the number of vehicles
    if (isPriority) {
        int count = vehicles.size();  // Already locked by mutex

        // If more than 10 vehicles, give high priority
        if (count > 10) {
            priority = 100 + count; // High priority value

            std::stringstream ss;
            ss << "Lane " << laneId << laneNumber << " priority increased to " << priority
               << " (Vehicle count: " << count << ")";
            DebugLogger::log(ss.str());
        }
        // If less than 5 vehicles, reset to normal priority
        else if (count < 5) {
            priority = 0;

            std::stringstream ss;
            ss << "Lane " << laneId << laneNumber << " priority reset to normal"
               << " (Vehicle count: " << count << ")";
            DebugLogger::log(ss.str());
        }
        // Between 5-10, maintain current state (hysteresis)
    }
    // For non-priority lanes, priority remains 0
    else {
        priority = 0;
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
    std::stringstream ss;
    ss << laneId << laneNumber;
    return ss.str();
}

const std::vector<Vehicle*>& Lane::getVehicles() const {
    // Note: This method returns a reference to the vector.
    // Callers should be careful not to modify it directly.
    // The mutex is not locked here to avoid deadlocks when iterating.
    return vehicles;
}
