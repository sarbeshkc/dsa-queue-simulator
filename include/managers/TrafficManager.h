#ifndef TRAFFIC_MANAGER_H
#define TRAFFIC_MANAGER_H

#include <vector>
#include <map>
#include <atomic>
#include <memory>
#include <string>
#include <SDL3/SDL.h>

#include "core/Lane.h"
#include "core/TrafficLight.h"
#include "managers/FileHandler.h"

class TrafficManager {
public:
    TrafficManager();
    ~TrafficManager();

    // Initialize the manager
    bool initialize();

    // Start the manager
    void start();

    // Stop the manager
    void stop();

    // Update the traffic state - now does file checking and priority updates
    void update(uint32_t delta);

    // Get the lanes for rendering
    const std::vector<Lane*>& getLanes() const;

    // Get the traffic light
    TrafficLight* getTrafficLight() const;

    // Check if a lane is being prioritized
    bool isLanePrioritized(char laneId, int laneNumber) const;

    // Get the priority lane
    Lane* getPriorityLane() const;

    // Get statistics for display
    std::string getStatistics() const;

    // Find lane by ID and number - making this public for rendering
    Lane* findLane(char laneId, int laneNumber) const;

private:
    // Lanes for each road
    std::vector<Lane*> lanes;

    // Traffic light
    TrafficLight* trafficLight;

    // File handler for reading vehicle data
    FileHandler* fileHandler;

    // Flag to indicate if the manager is running
    std::atomic<bool> running;

    // Time tracking for periodic operations
    uint32_t lastFileCheckTime;
    uint32_t lastPriorityUpdateTime;

    // Read vehicles from files - now called directly from update()
    void readVehicles();

    // Update lane priorities - now called directly from update()
    void updatePriorities();

    // Add a vehicle to the appropriate lane
    void addVehicle(Vehicle* vehicle);

    // Process vehicles in lanes
    void processVehicles(uint32_t delta);

    // Check for vehicles leaving the simulation
    void checkVehicleBoundaries();
};

#endif // TRAFFIC_MANAGER_H
