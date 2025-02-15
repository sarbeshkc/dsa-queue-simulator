// src/traffic/intersection_controller.h
#ifndef INTERSECTION_CONTROLLER_H
#define INTERSECTION_CONTROLLER_H

#include "traffic_light.h"
#include "vehicle.h"
#include <array>
#include <memory>
#include <vector>

// The IntersectionController class coordinates all activities at the intersection,
// including traffic light synchronization, vehicle movement permissions, and
// conflict resolution.
class IntersectionController {
public:
    IntersectionController(SDL_Renderer* renderer);
    ~IntersectionController() = default;

    // Core control functions
    void update(float deltaTime);
    void render() const;

    // Vehicle management
    bool canVehicleEnterIntersection(const Vehicle* vehicle) const;
    bool canVehicleTurn(const Vehicle* vehicle) const;
    void registerVehicleInIntersection(Vehicle* vehicle);
    void unregisterVehicleFromIntersection(Vehicle* vehicle);

    // Traffic light control
    bool isLaneActive(LaneId lane) const;
    void synchronizeTrafficLights();

    // Intersection state queries
    bool isIntersectionClear() const;
    int getVehicleCount() const;

private:
    // Constants for intersection management
    static constexpr float INTERSECTION_SIZE = 180.0f;  // Size of intersection box
    static constexpr float SAFE_DISTANCE = 30.0f;       // Minimum distance between vehicles
    static constexpr int MAX_VEHICLES = 4;              // Maximum vehicles in intersection

    // Core components
    SDL_Renderer* m_renderer;
    std::array<std::unique_ptr<TrafficLight>, 4> m_trafficLights;
    std::vector<Vehicle*> m_vehiclesInIntersection;

    // Traffic light indices for easy reference
    static constexpr int NORTH_LIGHT = 0;
    static constexpr int EAST_LIGHT = 1;
    static constexpr int SOUTH_LIGHT = 2;
    static constexpr int WEST_LIGHT = 3;

    // Helper methods
    void initializeTrafficLights();
    bool checkCollisionRisk(const Vehicle* vehicle) const;
    bool isPathClear(const Vehicle* vehicle) const;
    Direction getOppositeDirection(Direction dir) const;
    bool areDirectionsConflicting(Direction dir1, Direction dir2) const;
};

#endif // INTERSECTION_CONTROLLER_H
