#include "core/Lane.h"
#include "utils/DebugLogger.h"
#include <sstream>

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
    std::lock_guard<std::mutex> lock(mutex);
    // Clean up vehicles
    for (auto* vehicle : vehicles) {
        delete vehicle;
    }
    vehicles.clear();
}

void Lane::enqueue(Vehicle* vehicle) {
    int currentCount;
    {
        std::lock_guard<std::mutex> lock(mutex);
        // Add the vehicle to the queue
        vehicles.push_back(vehicle);
        currentCount = vehicles.size();
    }

    // Log the action
    std::ostringstream oss;
    oss << "Vehicle " << vehicle->getId() << " added to lane " << laneId << laneNumber;
    DebugLogger::log(oss.str());

    // Update priority if this is the priority lane
    if (isPriority) {
        if (currentCount > 10) {
            priority = 100; // High priority
            std::ostringstream oss;
            oss << "Lane " << laneId << laneNumber
                << " priority increased (vehicles: " << currentCount << ")";
            DebugLogger::log(oss.str());
        }
        else if (currentCount < 5) {
            priority = 0; // Normal priority
            std::ostringstream oss;
            oss << "Lane " << laneId << laneNumber
                << " priority reset to normal (vehicles: " << currentCount << ")";
            DebugLogger::log(oss.str());
        }
    }
}

Vehicle* Lane::dequeue() {
    Vehicle* vehicle = nullptr;
    int currentCount = 0;

    {
        std::lock_guard<std::mutex> lock(mutex);

        if (vehicles.empty()) {
            return nullptr;
        }

        // Remove the first vehicle (FIFO queue behavior)
        vehicle = vehicles.front();
        vehicles.erase(vehicles.begin());
        currentCount = vehicles.size();
    }

    // Log the action
    std::ostringstream oss;
    oss << "Vehicle " << vehicle->getId() << " removed from lane " << laneId << laneNumber;
    DebugLogger::log(oss.str());

    // Update priority if this is the priority lane
    if (isPriority) {
        if (currentCount > 10) {
            priority = 100; // High priority
        }
        else if (currentCount < 5) {
            priority = 0; // Normal priority
            std::ostringstream oss;
            oss << "Lane " << laneId << laneNumber
                << " priority reset to normal (vehicles: " << currentCount << ")";
            DebugLogger::log(oss.str());
        }
    }

    return vehicle;
}

Vehicle* Lane::peek() const {
    std::lock_guard<std::mutex> lock(mutex);

    if (vehicles.empty()) {
        return nullptr;
    }

    return vehicles.front();
}

bool Lane::isEmpty() const {
    std::lock_guard<std::mutex> lock(mutex);
    return vehicles.empty();
}

int Lane::getVehicleCount() const {
    std::lock_guard<std::mutex> lock(mutex);
    return vehicles.size();
}

int Lane::getPriority() const {
    return priority;
}

void Lane::updatePriority() {
    int count;
    {
        std::lock_guard<std::mutex> lock(mutex);
        count = vehicles.size();
    }

    // Update priority based on vehicle count for AL2 lane
    if (isPriority) {
        if (count > 10) { // PRIORITY_THRESHOLD_HIGH (more than 10 vehicles)
            if (priority != 100) { // Only log if status changed
                priority = 100; // High priority
                std::ostringstream oss;
                oss << "Lane " << laneId << laneNumber
                    << " priority increased (vehicles: " << count << " > 10)";
                DebugLogger::log(oss.str());
            }
        }
        else if (count < 5) { // PRIORITY_THRESHOLD_LOW (less than 5 vehicles)
            if (priority != 0) { // Only log if status changed
                priority = 0; // Normal priority
                std::ostringstream oss;
                oss << "Lane " << laneId << laneNumber
                    << " priority reset to normal (vehicles: " << count << " < 5)";
                DebugLogger::log(oss.str());
            }
        }
        // Between 5-10 vehicles, maintain current priority state (hysteresis)
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
