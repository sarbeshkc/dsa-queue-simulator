// FILE: src/managers/TrafficManager.cpp
#include "managers/TrafficManager.h"
#include "utils/DebugLogger.h"
#include <sstream>
#include <algorithm>
#include "core/Constants.h"

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
    // Create file handler with consistent path
    fileHandler = new FileHandler(Constants::DATA_PATH);
    if (!fileHandler->initializeFiles()) {
        DebugLogger::log("Failed to initialize lane files", DebugLogger::LogLevel::ERROR);
        return false;
    }

    // Create lanes for each road and lane number
    for (char road : {'A', 'B', 'C', 'D'}) {
        for (int laneNum = 1; laneNum <= 3; laneNum++) {
            Lane* lane = new Lane(road, laneNum);
            lanes.push_back(lane);

            // Add to priority queue with initial priority
            lanePriorityQueue.enqueue(lane, lane->getPriority());
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

    // Check for new vehicles more frequently (every 200ms)
    if (currentTime - lastFileCheckTime >= 200) { // 5 times per second
        readVehicles();
        lastFileCheckTime = currentTime;
    }

    // Update lane priorities regularly
    if (currentTime - lastPriorityUpdateTime >= 300) { // Every 300ms
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
    if (!fileHandler) {
        DebugLogger::log("FileHandler not initialized", DebugLogger::LogLevel::ERROR);
        return;
    }

    // Ensure directories exist before reading
    if (!fileHandler->checkFilesExist()) {
        if (!fileHandler->initializeFiles()) {
            DebugLogger::log("Failed to initialize files", DebugLogger::LogLevel::ERROR);
            return;
        }
    }

    // Read new vehicles from files
    std::vector<Vehicle*> newVehicles = fileHandler->readVehiclesFromFiles();

    if (!newVehicles.empty()) {
        std::ostringstream oss;
        oss << "Read " << newVehicles.size() << " new vehicles from files";
        DebugLogger::log(oss.str());

        // Add vehicles to appropriate lanes
        for (auto* vehicle : newVehicles) {
            addVehicle(vehicle);
        }
    }

    // Write status to file periodically for monitoring
    static uint32_t lastStatusTime = 0;
    uint32_t currentTime = SDL_GetTicks();

    if (currentTime - lastStatusTime >= 5000) { // Every 5 seconds
        for (auto* lane : lanes) {
            if (fileHandler && lane->getVehicleCount() > 0) {
                fileHandler->writeLaneStatus(
                    lane->getLaneId(),
                    lane->getLaneNumber(),
                    lane->getVehicleCount(),
                    lane->isPriorityLane() && lane->getPriority() > 0
                );
            }
        }
        lastStatusTime = currentTime;
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
    } else {
        // Clean up if lane not found
        delete vehicle;
        DebugLogger::log("Error: No matching lane found for vehicle", DebugLogger::LogLevel::ERROR);
    }
}

void TrafficManager::updatePriorities() {
    // Use lambda for comparing lanes
    auto compareLanes = [](const Lane* a, const Lane* b) -> bool {
        return a == b;
    };

    for (auto* lane : lanes) {
        // Store old priority
        int oldPriority = lane->getPriority();

        // Update priority
        lane->updatePriority();

        // If priority changed, update in the priority queue
        if (oldPriority != lane->getPriority()) {
            lanePriorityQueue.updatePriority(lane, lane->getPriority(), compareLanes);

            // Log the priority change
            std::ostringstream oss;
            oss << "Updated priority for lane " << lane->getName()
                << " from " << oldPriority << " to " << lane->getPriority();
            DebugLogger::log(oss.str());

            // For AL2 lane, log special messages when crossing thresholds
            if (lane->getLaneId() == 'A' && lane->getLaneNumber() == 2) {
                if (oldPriority == 0 && lane->getPriority() > 0) {
                    DebugLogger::log("PRIORITY MODE ACTIVATED for lane A2", DebugLogger::LogLevel::INFO);
                } else if (oldPriority > 0 && lane->getPriority() == 0) {
                    DebugLogger::log("PRIORITY MODE DEACTIVATED for lane A2", DebugLogger::LogLevel::INFO);
                }
            }
        }
    }
}

void TrafficManager::processVehicles(uint32_t delta) {
    // Determine active lanes based on traffic light state
    char activeLaneId = ' ';
    if (trafficLight) {
        if (trafficLight->getCurrentState() == TrafficLight::State::A_GREEN) activeLaneId = 'A';
        else if (trafficLight->getCurrentState() == TrafficLight::State::B_GREEN) activeLaneId = 'B';
        else if (trafficLight->getCurrentState() == TrafficLight::State::C_GREEN) activeLaneId = 'C';
        else if (trafficLight->getCurrentState() == TrafficLight::State::D_GREEN) activeLaneId = 'D';
    }

    // Process vehicles in all lanes
    for (auto* lane : lanes) {
        bool isGreenLight = (lane->getLaneId() == activeLaneId) ||
                           (lane->getLaneNumber() == 3); // L3 is free lane

        // Get all vehicles in this lane
        const auto& vehicles = lane->getVehicles();
        int queuePos = 0;

        // Update each vehicle with proper queue position for spacing
        for (auto* vehicle : vehicles) {
            if (vehicle) {
                // Update vehicle position based on traffic light
                vehicle->update(delta, isGreenLight, 0.0f); // targetPos not used with new queue positioning

                // Update queue position for visualization
                queuePos++;
            }
        }
    }
}

void TrafficManager::checkVehicleBoundaries() {
    for (auto* lane : lanes) {
        // Check each vehicle
        while (!lane->isEmpty()) {
            Vehicle* vehicle = lane->peek();

            if (vehicle && vehicle->hasExited()) {
                // Remove the vehicle from the queue
                Vehicle* removedVehicle = lane->dequeue();

                // Log vehicle exit with lane info
                std::ostringstream oss;
                oss << "Vehicle " << removedVehicle->getId() << " exited the simulation from lane "
                    << removedVehicle->getLane() << removedVehicle->getLaneNumber();
                DebugLogger::log(oss.str());

                // Delete the vehicle
                delete removedVehicle;
            } else {
                // If the first vehicle hasn't exited, the rest haven't either
                break;
            }
        }
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
    Lane* lane = findLane(laneId, laneNumber);
    return lane && lane->isPriorityLane() && lane->getPriority() > 0;
}

Lane* TrafficManager::getPriorityLane() const {
    return findLane('A', 2); // AL2 is the priority lane
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

    // Add traffic light status
    if (trafficLight) {
        stats << "Traffic Light: ";
        switch (trafficLight->getCurrentState()) {
            case TrafficLight::State::ALL_RED: stats << "ALL RED"; break;
            case TrafficLight::State::A_GREEN: stats << "A GREEN"; break;
            case TrafficLight::State::B_GREEN: stats << "B GREEN"; break;
            case TrafficLight::State::C_GREEN: stats << "C GREEN"; break;
            case TrafficLight::State::D_GREEN: stats << "D GREEN"; break;
        }
        stats << "\n";
    }

    return stats.str();
}
