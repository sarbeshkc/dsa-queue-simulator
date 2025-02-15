// src/core/main.h
#ifndef MAIN_H
#define MAIN_H

#include "../traffic/road_system.h"
#include "../traffic/intersection_controller.h"
#include "../generator/traffic_generator.h"
#include "../communication/file_communicator.h"
#include "../visualization/visualization_manager.h"
#include "window.h"
#include <memory>
#include <random>

class SimulationApp {
public:
    SimulationApp();
    ~SimulationApp();

    void run();

private:
    // Core simulation settings
    static constexpr int WINDOW_WIDTH = 1280;
    static constexpr int WINDOW_HEIGHT = 720;
    static constexpr float SPAWN_INTERVAL = 1000.0f;  // Milliseconds between vehicle spawns
    static constexpr float TIME_STEP = 1.0f / 60.0f;  // 60 FPS fixed timestep

    // Core components
    std::unique_ptr<Window> m_window;
    std::unique_ptr<RoadSystem> m_roadSystem;
    std::unique_ptr<IntersectionController> m_intersectionController;
    std::unique_ptr<TrafficGenerator> m_generator;
    std::unique_ptr<FileCommunicator> m_communicator;
    std::unique_ptr<VisualizationManager> m_visualizer;

    // Simulation state
    bool m_running;
    float m_elapsedTime;
    float m_lastSpawnTime;
    std::mt19937 m_rng;

    // Statistics tracking
    struct SimulationStats {
        int totalVehiclesProcessed;
        float averageWaitTime;
        int maxQueueLength;
        float simulationTime;

        SimulationStats()
            : totalVehiclesProcessed(0)
            , averageWaitTime(0.0f)
            , maxQueueLength(0)
            , simulationTime(0.0f) {}
    } m_stats;

    // Core loop methods
    void processEvents();
    void update(float deltaTime);
    void render();

    // Helper methods
    void initializeComponents();
    void updateStatistics(float deltaTime);
    void handleVehicleSpawning(float deltaTime);
    void cleanupVehicles();
};

#endif // MAIN_H
