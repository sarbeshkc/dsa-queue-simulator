#include "managers/TrafficManager.h"
#include "utils/DebugLogger.h"
#include <sstream>
#include <algorithm>
#include <chrono>

TrafficManager::TrafficManager()
    : trafficLight(nullptr),
      fileHandler(nullptr),
      running(false),
      lastFileCheckTime(0),
      lastPriorityUpdateTime(0) {
}

TrafficManager::~TrafficManager() {
    stop();

    // Clean up resources
    for (auto lane : lanes) {
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
}

bool TrafficManager::initialize() {
    try {
        // Create file handler
        fileHandler = new FileHandler("data/lanes");
        if (!fileHandler->initializeFiles()) {
            DebugLogger::log("Failed to initialize lane files", DebugLogger::LogLevel::ERROR);
            return false;
        }

        // Create lanes for each road (A, B, C, D) with 3 lanes each
        lanes.push_back(new Lane('A', 1)); // AL1 - left lane
        lanes.push_back(new Lane('A', 2)); // AL2 - middle lane (priority)
        lanes.push_back(new Lane('A', 3)); // AL3 - right lane
        lanes.push_back(new Lane('B', 1));
        lanes.push_back(new Lane('B', 2));
        lanes.push_back(new Lane('B', 3));
        lanes.push_back(new Lane('C', 1));
        lanes.push_back(new Lane('C', 2));
        lanes.push_back(new Lane('C', 3));
        lanes.push_back(new Lane('D', 1));
        lanes.push_back(new Lane('D', 2));
        lanes.push_back(new Lane('D', 3));

        // Create traffic light
        trafficLight = new TrafficLight();

        // Set initial timestamps
        lastFileCheckTime = SDL_GetTicks();
        lastPriorityUpdateTime = SDL_GetTicks();

        DebugLogger::log("TrafficManager initialized with 12 lanes");
        return true;
    }
    catch (const std::exception& e) {
        DebugLogger::log("Error initializing TrafficManager: " + std::string(e.what()), DebugLogger::LogLevel::ERROR);
        return false;
    }
}

void TrafficManager::start() {
    if (running) {
        return;
    }

    running = true;
    DebugLogger::log("TrafficManager started");
}

void TrafficManager::stop() {
    if (!running) {
        return;
    }

    running = false;
    DebugLogger::log("TrafficManager stopped");
}

void TrafficManager::update(uint32_t delta) {
    if (!running) return;

    try {
        // Get current time
        uint32_t currentTime = SDL_GetTicks();

        // Check for new vehicles periodically (every 1 second)
        if (currentTime - lastFileCheckTime >= 1000) {
            readVehicles();
            lastFileCheckTime = currentTime;
        }

        // Update priorities periodically (every 500ms)
        if (currentTime - lastPriorityUpdateTime >= 500) {
            updatePriorities();
            lastPriorityUpdateTime = currentTime;
        }

        // Update traffic light
        if (trafficLight) {
            trafficLight->update(lanes);
        }

        // Process vehicles in lanes
        processVehicles(delta);

        // Check for vehicles leaving the simulation
        checkVehicleBoundaries();
    }
    catch (const std::exception& e) {
        DebugLogger::log("Error in TrafficManager::update: " + std::string(e.what()), DebugLogger::LogLevel::ERROR);
    }
}

const std::vector<Lane*>& TrafficManager::getLanes() const {
    return lanes;
}

TrafficLight* TrafficManager::getTrafficLight() const {
    return trafficLight;
}

bool TrafficManager::isLanePrioritized(char laneId, int laneNumber) const {
    // AL2 is the prioritized lane when it has more than 10 vehicles
    if (laneId == 'A' && laneNumber == 2) {
        Lane* lane = findLane('A', 2);
        return lane && lane->getPriority() > 0;
    }

    return false;
}

Lane* TrafficManager::getPriorityLane() const {
    // Find AL2 lane
    return findLane('A', 2);
}

std::string TrafficManager::getStatistics() const {
    std::stringstream ss;

    // Count total vehicles in each lane
    int totalVehicles = 0;

    ss << "Lane Statistics:\n";

    for (const auto& lane : lanes) {
        int count = lane->getVehicleCount();
        totalVehicles += count;

        ss << lane->getName() << ": " << count << " vehicles";

        if (isLanePrioritized(lane->getLaneId(), lane->getLaneNumber())) {
            ss << " (PRIORITY)";
        }

        ss << "\n";
    }

    ss << "Total Vehicles: " << totalVehicles << "\n";

    // Traffic light state
    if (trafficLight) {
        ss << "Traffic Light: ";
        switch (trafficLight->getCurrentState()) {
            case TrafficLight::State::ALL_RED:
                ss << "All Red";
                break;
            case TrafficLight::State::A_GREEN:
                ss << "A Green";
                break;
            case TrafficLight::State::B_GREEN:
                ss << "B Green";
                break;
            case TrafficLight::State::C_GREEN:
                ss << "C Green";
                break;
            case TrafficLight::State::D_GREEN:
                ss << "D Green";
                break;
        }
    }

    return ss.str();
}

void TrafficManager::readVehicles() {
    if (!fileHandler) return;

    try {
        // Read vehicles from files
        std::vector<Vehicle*> vehicles = fileHandler->readVehiclesFromFiles();

        // Add vehicles to appropriate lanes
        for (auto vehicle : vehicles) {
            addVehicle(vehicle);
        }
    }
    catch (const std::exception& e) {
        DebugLogger::log("Error reading vehicles: " + std::string(e.what()), DebugLogger::LogLevel::ERROR);
    }
}

void TrafficManager::updatePriorities() {
    try {
        // Update lane priorities
        for (auto lane : lanes) {
            if (lane) {
                lane->updatePriority();

                // Log priority lane status
                if (lane->isPriorityLane() && fileHandler) {
                    fileHandler->writeLaneStatus(
                        lane->getLaneId(),
                        lane->getLaneNumber(),
                        lane->getVehicleCount(),
                        lane->getPriority() > 0
                    );
                }
            }
        }
    }
    catch (const std::exception& e) {
        DebugLogger::log("Error updating priorities: " + std::string(e.what()), DebugLogger::LogLevel::ERROR);
    }
}

void TrafficManager::addVehicle(Vehicle* vehicle) {
    if (!vehicle) {
        return;
    }

    try {
        // Find the appropriate lane
        Lane* targetLane = findLane(vehicle->getLane(), vehicle->getLaneNumber());

        if (targetLane) {
            targetLane->enqueue(vehicle);

            std::stringstream ss;
            ss << "Added vehicle " << vehicle->getId() << " to lane "
               << vehicle->getLane() << vehicle->getLaneNumber();
            DebugLogger::log(ss.str());
        } else {
            // If lane not found, delete the vehicle to prevent memory leak
            delete vehicle;

            std::stringstream ss;
            ss << "Could not find lane " << vehicle->getLane() << vehicle->getLaneNumber()
               << " for vehicle " << vehicle->getId();
            DebugLogger::log(ss.str());
        }
    }
    catch (const std::exception& e) {
        delete vehicle; // Clean up to prevent memory leak
        DebugLogger::log("Error adding vehicle: " + std::string(e.what()), DebugLogger::LogLevel::ERROR);
    }
}

void TrafficManager::processVehicles(uint32_t delta) {
    try {
        // Get the current green light lane (if any)
        char greenLightLane = '\0';

        if (trafficLight) {
            if (trafficLight->getCurrentState() == TrafficLight::State::A_GREEN) {
                greenLightLane = 'A';
            } else if (trafficLight->getCurrentState() == TrafficLight::State::B_GREEN) {
                greenLightLane = 'B';
            } else if (trafficLight->getCurrentState() == TrafficLight::State::C_GREEN) {
                greenLightLane = 'C';
            } else if (trafficLight->getCurrentState() == TrafficLight::State::D_GREEN) {
                greenLightLane = 'D';
            }
        }

        // Process vehicles in each lane - we keep this non-locking access because
        // all operations are now in main thread
        for (auto lane : lanes) {
            if (!lane) continue;

            const std::vector<Vehicle*>& vehicles = lane->getVehicles();

            for (size_t i = 0; i < vehicles.size(); ++i) {
                Vehicle* vehicle = vehicles[i];

                if (!vehicle) {
                    continue;
                }

                // Check if this vehicle's lane has a green light
                bool isGreenLight = (vehicle->getLane() == greenLightLane);

                // The 3rd lane (right lane) can always turn left (free lane)
                if (vehicle->getLaneNumber() == 3) {
                    isGreenLight = true;
                }

                // Calculate target position based on lane
                float targetPos = 0.0f;

                if (vehicle->getLane() == 'A') {
                    targetPos = 800.0f; // Bottom of screen
                } else if (vehicle->getLane() == 'B') {
                    targetPos = 0.0f; // Top of screen
                } else if (vehicle->getLane() == 'C') {
                    targetPos = 0.0f; // Left of screen
                } else if (vehicle->getLane() == 'D') {
                    targetPos = 800.0f; // Right of screen
                }

                // Update vehicle position
                vehicle->update(delta, isGreenLight, targetPos);

                // Check if vehicle needs to turn
                handleVehicleTurning(vehicle);
            }
        }
    }
    catch (const std::exception& e) {
        DebugLogger::log("Error processing vehicles: " + std::string(e.what()), DebugLogger::LogLevel::ERROR);
    }
}

void TrafficManager::checkVehicleBoundaries() {
    try {
        // Check each lane for vehicles that have left the simulation area
        for (auto lane : lanes) {
            if (!lane || lane->isEmpty()) {
                continue;
            }

            Vehicle* frontVehicle = lane->peek();

            if (!frontVehicle) {
                continue;
            }

            bool shouldRemove = false;

            // Check if vehicle has left the simulation area
            if (frontVehicle->getLane() == 'A' && frontVehicle->getAnimationPos() > 800.0f) {
                shouldRemove = true;
            } else if (frontVehicle->getLane() == 'B' && frontVehicle->getAnimationPos() < 0.0f) {
                shouldRemove = true;
            } else if (frontVehicle->getLane() == 'C' && frontVehicle->getAnimationPos() < 0.0f) {
                shouldRemove = true;
            } else if (frontVehicle->getLane() == 'D' && frontVehicle->getAnimationPos() > 800.0f) {
                shouldRemove = true;
            }

            if (shouldRemove) {
                Vehicle* vehicle = lane->dequeue();

                if (vehicle) {
                    std::stringstream ss;
                    ss << "Vehicle " << vehicle->getId() << " left the simulation";
                    DebugLogger::log(ss.str());

                    delete vehicle;
                }
            }
        }
    }
    catch (const std::exception& e) {
        DebugLogger::log("Error checking vehicle boundaries: " + std::string(e.what()), DebugLogger::LogLevel::ERROR);
    }
}

void TrafficManager::handleVehicleTurning(Vehicle* vehicle) {
    if (!vehicle || vehicle->isTurning()) {
        return;
    }

    try {
        // Lane A3 (right lane of road A) turns right to lane C1 (left lane of road C)
        if (vehicle->getLane() == 'A' && vehicle->getLaneNumber() == 3) {
            float turnThreshold = 350.0f; // Position where turn starts

            if (vehicle->getAnimationPos() >= turnThreshold) {
                // Start turning
                vehicle->setTurning(true);
                vehicle->setTurnProgress(0.0f);

                // Set initial turning position
                vehicle->setTurnPosX(400.0f + 50.0f); // Right of center
                vehicle->setTurnPosY(vehicle->getAnimationPos());

                // Calculate turn path
                float startX = 400.0f + 50.0f;
                float startY = vehicle->getAnimationPos();
                float controlX = startX + 50.0f;
                float controlY = 400.0f;
                float endX = 520.0f;
                float endY = 400.0f - 50.0f;

                vehicle->calculateTurnPath(startX, startY, controlX, controlY, endX, endY, 0.0f);

                DebugLogger::log("Vehicle " + vehicle->getId() + " started turning from A3 to C1");
            }
        }

        // Add handling for other turning scenarios (B3->D1, C3->B1, D3->A1)
        // as needed
    }
    catch (const std::exception& e) {
        DebugLogger::log("Error handling vehicle turning: " + std::string(e.what()), DebugLogger::LogLevel::ERROR);
    }
}

Lane* TrafficManager::findLane(char laneId, int laneNumber) const {
    for (auto lane : lanes) {
        if (lane && lane->getLaneId() == laneId && lane->getLaneNumber() == laneNumber) {
            return lane;
        }
    }

    return nullptr;
}
