#include "managers/TrafficManager.h"
#include "utils/DebugLogger.h"
#include <sstream>
#include <algorithm>

TrafficManager::TrafficManager()
    : trafficLight(nullptr),
      fileHandler(nullptr),
      lastFileCheckTime(0),
      lastPriorityUpdateTime(0),
      running(false) {

    DebugLogger::log("TrafficManager created");
}

TrafficManager::~TrafficManager() {
    // Clean up resources
    for (auto* lane : lanes) {
        delete lane;
    }
    lanes.clear();

    if (trafficLight) {
        delete trafficLight;
        trafficLight = nullptr;
    }

    if (fileHandler) {
        delete fileHandler;
        fileHandler = nullptr;
    }

    DebugLogger::log("TrafficManager destroyed");
}

bool TrafficManager::initialize() {
    // Create file handler
    fileHandler = new FileHandler();
    if (!fileHandler->initializeFiles()) {
        DebugLogger::log("Failed to initialize lane files");
        return false;
    }

    // Create lanes for each road and lane number
    for (char road : {'A', 'B', 'C', 'D'}) {
        for (int laneNum = 1; laneNum <= 3; laneNum++) {
            lanes.push_back(new Lane(road, laneNum));
        }
    }

    // Create traffic light
    trafficLight = new TrafficLight();

    std::ostringstream oss;
    oss << "TrafficManager initialized with " << lanes.size() << " lanes";
    DebugLogger::log(oss.str());

    return true;
}

void TrafficManager::start() {
    running = true;
    DebugLogger::log("TrafficManager started");
}

void TrafficManager::stop() {
    running = false;
    DebugLogger::log("TrafficManager stopped");
}

void TrafficManager::update(uint32_t delta) {
    if (!running) return;

    uint32_t currentTime = SDL_GetTicks();

    // Check for new vehicles periodically
    if (currentTime - lastFileCheckTime >= 1000) { // Every second
        readVehicles();
        lastFileCheckTime = currentTime;
    }

    // Update lane priorities periodically
    if (currentTime - lastPriorityUpdateTime >= 500) { // Every half second
        updatePriorities();
        lastPriorityUpdateTime = currentTime;
    }

    // Process vehicles based on traffic light state
    processVehicles(delta);

    // Check for vehicles leaving the simulation
    checkVehicleBoundaries();

    // Update traffic light
    if (trafficLight) {
        trafficLight->update(lanes);
    }
}

void TrafficManager::readVehicles() {
    if (!fileHandler) return;

    // Read new vehicles from files
    std::vector<Vehicle*> newVehicles = fileHandler->readVehiclesFromFiles();

    // Add vehicles to appropriate lanes
    for (auto* vehicle : newVehicles) {
        addVehicle(vehicle);
    }
}

void TrafficManager::addVehicle(Vehicle* vehicle) {
    if (!vehicle) return;

    Lane* targetLane = findLane(vehicle->getLane(), vehicle->getLaneNumber());
    if (targetLane) {
        targetLane->enqueue(vehicle);

        // Log the action
        std::ostringstream oss;
        oss << "Added vehicle " << vehicle->getId() << " to lane "
            << vehicle->getLane() << vehicle->getLaneNumber();
        DebugLogger::log(oss.str());

        // Write lane status to file for monitoring
        if (fileHandler) {
            fileHandler->writeLaneStatus(
                targetLane->getLaneId(),
                targetLane->getLaneNumber(),
                targetLane->getVehicleCount(),
                targetLane->isPriorityLane() && targetLane->getPriority() > 0
            );
        }
    } else {
        // Clean up if lane not found
        delete vehicle;
        DebugLogger::log("Error: No matching lane found for vehicle");
    }
}

// In TrafficManager.cpp - fix for updatePriorities method
void TrafficManager::updatePriorities() {
    // Update all lane priorities
    for (auto* lane : lanes) {
        lane->updatePriority();
    }

    // Instead of using PriorityQueue, use a simple sorting approach
    // Create a temporary vector for sorting
    std::vector<Lane*> prioritizedLanes = lanes;

    // Sort lanes by priority (highest first)
    std::sort(prioritizedLanes.begin(), prioritizedLanes.end(),
              [](Lane* a, Lane* b) {
                  // AL2 lane always gets highest priority if priority > 0
                  if (a->getLaneId() == 'A' && a->getLaneNumber() == 2 && a->getPriority() > 0)
                      return true;
                  if (b->getLaneId() == 'A' && b->getLaneNumber() == 2 && b->getPriority() > 0)
                      return false;

                  // Lane 3 (free lanes) get medium priority
                  if (a->getLaneNumber() == 3 && b->getLaneNumber() != 3)
                      return true;
                  if (b->getLaneNumber() == 3 && a->getLaneNumber() != 3)
                      return false;

                  // Compare regular priorities
                  return a->getPriority() > b->getPriority();
              });

    // Get highest priority lane for debugging/monitoring
    if (!prioritizedLanes.empty()) {
        Lane* highPriorityLane = prioritizedLanes[0];

        // Log which lane has highest priority
        if (highPriorityLane->getPriority() > 0 || highPriorityLane->getLaneNumber() == 3) {
            std::ostringstream oss;
            oss << "Highest priority lane: " << highPriorityLane->getLaneId()
                << highPriorityLane->getLaneNumber();
            if (highPriorityLane->getPriority() > 0) {
                oss << " (priority: " << highPriorityLane->getPriority() << ")";
            } else if (highPriorityLane->getLaneNumber() == 3) {
                oss << " (free lane)";
            }
            DebugLogger::log(oss.str());
        }
    }
}

void TrafficManager::processVehicles(uint32_t delta) {
    for (auto* lane : lanes) {
        bool isGreenLight = trafficLight ? trafficLight->isGreen(lane->getLaneId()) : false;

        // Precomputed positioning data for queue visualization
        float targetPos = 0.0f;
        int queuePosition = 0;

        // Process each vehicle in the lane
        for (auto* vehicle : lane->getVehicles()) {
            // Update vehicle position
            vehicle->update(delta, isGreenLight, targetPos);

            // Update queue position for next vehicle
            queuePosition++;
            targetPos += 30.0f; // Space between vehicles
        }
    }
}

void TrafficManager::checkVehicleBoundaries() {
    for (auto* lane : lanes) {
        auto& vehicles = lane->getVehicles();

        // Create a copy of vehicles for processing
        std::vector<Vehicle*> vehiclesToKeep;

        for (auto* vehicle : vehicles) {
            if (!vehicle->hasExited()) {
                vehiclesToKeep.push_back(vehicle);
            } else {
                // Log and delete vehicle that has exited
                std::ostringstream oss;
                oss << "Vehicle " << vehicle->getId() << " left the simulation";
                DebugLogger::log(oss.str());
                delete vehicle;
            }
        }

        // Replace with vehicles to keep
        vehicles = vehiclesToKeep;
    }
}

Lane* TrafficManager::findLane(char laneId, int laneNumber) const {
    for (auto* lane : lanes) {
        if (lane->getLaneId() == laneId && lane->getLaneNumber() == laneNumber) {
            return lane;
        }
    }
    return nullptr;
}

const std::vector<Lane*>& TrafficManager::getLanes() const {
    return lanes;
}

TrafficLight* TrafficManager::getTrafficLight() const {
    return trafficLight;
}

bool TrafficManager::isLanePrioritized(char laneId, int laneNumber) const {
    if (laneId == 'A' && laneNumber == 2) {
        Lane* lane = findLane('A', 2);
        return lane && lane->getPriority() > 0;
    }
    return false;
}

Lane* TrafficManager::getPriorityLane() const {
    return findLane('A', 2);
}

std::string TrafficManager::getStatistics() const {
    std::ostringstream stats;
    stats << "Lane Statistics:\n";
    int totalVehicles = 0;

    for (auto* lane : lanes) {
        int count = lane->getVehicleCount();
        totalVehicles += count;

        stats << lane->getName() << ": " << count << " vehicles";
        if (lane->isPriorityLane() && lane->getPriority() > 0) {
            stats << " (PRIORITY)";
        }
        stats << "\n";
    }

    stats << "Total Vehicles: " << totalVehicles << "\n";

    return stats.str();
}
