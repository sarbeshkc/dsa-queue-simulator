// src/generator/traffic_generator.h
#ifndef TRAFFIC_GENERATOR_H
#define TRAFFIC_GENERATOR_H

#include <SDL3/SDL.h>
#include "../traffic/vehicle.h"
#include "../utils/math_utils.h"
#include "../communication/file_handler.h"
#include <random>

class TrafficGenerator {
public:
    TrafficGenerator(SDL_Renderer* renderer);

    // Core generation functions
    void update(float deltaTime);
    Vehicle* generateVehicle();

    // Generation settings
    void setGenerationRate(float vehiclesPerSecond);
    void setPriorityRatio(float ratio);  // Ratio of vehicles going to priority lanes

private:
    struct SpawnPoint {
        Vector2D position;
        Direction facing;
        LaneId lane;

        SpawnPoint(const Vector2D& pos, Direction dir, LaneId l)
            : position(pos), facing(dir), lane(l) {}
    };

    SDL_Renderer* m_renderer;
    std::mt19937 m_rng;              // Random number generator
    float m_generationTimer;         // Time since last generation
    float m_generationInterval;      // Time between generations
    int m_vehicleCount;              // Total vehicles generated
    float m_priorityRatio;           // Ratio of priority vehicles
    std::vector<SpawnPoint> m_spawnPoints;
    std::unique_ptr<FileHandler> m_fileHandler;

    static constexpr float DEFAULT_GENERATION_RATE = 0.5f;  // One vehicle every 2 seconds
    static constexpr float DEFAULT_PRIORITY_RATIO = 0.3f;   // 30% priority vehicles

    void initializeSpawnPoints();
    SpawnPoint selectSpawnPoint();
    LaneId determineTargetLane(const SpawnPoint& spawn);
};

#endif // TRAFFIC_GENERATOR_H
