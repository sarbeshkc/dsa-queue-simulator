#ifndef TRAFFIC_QUEUE_H
#define TRAFFIC_QUEUE_H

#include <vector>
#include <chrono>
#include "../traffic/vehicle.h"

class TrafficQueue {
public:
    TrafficQueue(bool isPriorityLane = false);

    // Core queue operations
    void enqueue(Vehicle* vehicle);
    Vehicle* dequeue();
    Vehicle* peek() const;

    // Queue state
    bool empty() const { return m_vehicles.empty(); }
    size_t size() const { return m_vehicles.size(); }
    bool isPriorityLane() const { return m_isPriorityLane; }

    // Time management
    void update(float deltaTime);
    float getWaitTime() const;
    bool isReadyToProcess() const { return m_processingTime >= PROCESSING_TIME; }
    void resetProcessingTime() { m_processingTime = 0.0f; }

    // Priority management
    bool needsPriorityProcessing() const;
    bool canExitPriorityMode() const;

private:
    struct QueuedVehicle {
        Vehicle* vehicle;
        std::chrono::steady_clock::time_point entryTime;

        QueuedVehicle(Vehicle* v)
            : vehicle(v)
            , entryTime(std::chrono::steady_clock::now()) {}
    };

    std::vector<QueuedVehicle> m_vehicles;
    bool m_isPriorityLane;
    float m_processingTime;

    // Constants
    static constexpr float PROCESSING_TIME = 2.0f;  // 2 seconds per vehicle
    static constexpr int PRIORITY_THRESHOLD = 10;
    static constexpr int PRIORITY_RESET_THRESHOLD = 5;

    // Helper methods
    float calculateServiceTime() const;
    void updateVehiclePositions();
};
