#ifndef LANE_H
#define LANE_H

#include <queue>
#include <mutex>
#include <string>
#include "core/Vehicle.h"

class Lane {
public:
    Lane(char laneId, int laneNumber);
    ~Lane();

    // Queue operations
    void enqueue(Vehicle* vehicle);
    Vehicle* dequeue();
    Vehicle* peek() const;
    bool isEmpty() const;
    int getVehicleCount() const;

    // Priority related operations
    int getPriority() const;
    void updatePriority();
    bool isPriorityLane() const;

    // Lane identification
    char getLaneId() const;
    int getLaneNumber() const;
    std::string getName() const;

    // Iterate through vehicles for rendering
    const std::vector<Vehicle*>& getVehicles() const;

private:
    char laneId;               // A, B, C, or D
    int laneNumber;            // 1, 2, or 3
    bool isPriority;           // Is this a priority lane (AL2)
    int priority;              // Current priority (higher means served first)
    std::vector<Vehicle*> vehicles; // Storage for vehicles in the lane
    mutable std::mutex mutex;  // For thread safety

    // Update vehicle positions based on traffic light state
    void updateVehiclePositions();
};

#endif // LANE_H
