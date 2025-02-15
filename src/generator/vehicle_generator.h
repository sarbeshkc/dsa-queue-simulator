// src/generator/vehicle_generator.h
#ifndef VEHICLE_GENERATOR_H
#define VEHICLE_GENERATOR_H

#include <SDL3/SDL.h>
#include <random>
#include <queue>
#include <memory>
#include "../traffic/vehicle.h"
#include "../traffic/traffic_queue.h"
#include "../utils/math_utils.h"

// Structure to define vehicle spawn points
struct SpawnPoint {
    Vector2D position;     // Where vehicles appear
    Direction facing;      // Initial direction
    LaneId lane;          // Which lane they spawn in

    SpawnPoint(Vector2D pos, Direction dir, LaneId l)
        : position(pos), facing(dir), lane(l) {}
};

class VehicleGenerator {
public:
    explicit VehicleGenerator(SDL_Renderer* renderer);

    // Core generation functions
    void update(float deltaTime);
    Vehicle* generateVehicle();

    // Generation settings
    void setGenerationRate(float vehiclesPerSecond);
    void setPriorityLaneRatio(float ratio);
    
    // Queue status queries
    int getQueueLength(LaneId lane) const;
    bool isPriorityLaneActive() const;

private:
    SDL_Renderer* m_renderer;
    std::mt19937 m_rng;              // Random number generator
    float m_generationTimer;         // Tracks time since last generation
    float m_generationInterval;      // Time between vehicle spawns
    int m_vehicleCount;              // Total vehicles generated
    float m_priorityRatio;           // Ratio of vehicles going to priority lanes
    std::vector<SpawnPoint> m_spawnPoints;

    // Constants
    static constexpr float DEFAULT_GENERATION_INTERVAL = 1.0f;  // 1 vehicle per second
    static constexpr float DEFAULT_PRIORITY_RATIO = 0.3f;       // 30% to priority lanes
    static constexpr float SPAWN_MARGIN = 50.0f;               // Distance from edge
    
    // Helper methods
    void initializeSpawnPoints();
    SpawnPoint selectSpawnPoint();
    LaneId determineTargetLane(const SpawnPoint& spawn);
    bool shouldGenerateVehicle() const;
    
    // Lane selection logic
    bool isValidLaneChange(LaneId from, LaneId to) const;
    bool isPriorityLane(LaneId lane) const;
    Direction getLaneDirection(LaneId lane) const;
    
    // Vehicle initialization
    void setupVehicleProperties(Vehicle* vehicle, const SpawnPoint& spawn);
    Vector2D calculateSpawnPosition(const SpawnPoint& spawn) const;
};

#endif // VEHICLE_GENERATOR_H