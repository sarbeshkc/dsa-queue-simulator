// src/core/traffic_queue.cpp
#include "traffic_queue.h"

TrafficQueue::TrafficQueue(bool isPriorityLane)
    : m_isPriorityLane(isPriorityLane)
    , m_processingTime(0.0f) {}

void TrafficQueue::enqueue(Vehicle* vehicle) {
    if (!vehicle) return;
    m_vehicles.emplace_back(vehicle);
    updateVehiclePositions();
}

Vehicle* TrafficQueue::dequeue() {
    if (m_vehicles.empty()) return nullptr;

    Vehicle* vehicle = m_vehicles.front().vehicle;
    m_vehicles.erase(m_vehicles.begin());
    updateVehiclePositions();
    return vehicle;
}

Vehicle* TrafficQueue::peek() const {
    return m_vehicles.empty() ? nullptr : m_vehicles.front().vehicle;
}

void TrafficQueue::update(float deltaTime) {
    if (!m_vehicles.empty()) {
        m_processingTime += deltaTime;
    }
}

float TrafficQueue::getWaitTime() const {
    if (m_vehicles.empty()) return 0.0f;

    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>
                   (now - m_vehicles.front().entryTime).count();
    return static_cast<float>(duration);
}

bool TrafficQueue::needsPriorityProcessing() const {
    return m_isPriorityLane && size() >= PRIORITY_THRESHOLD;
}

bool TrafficQueue::canExitPriorityMode() const {
    return size() <= PRIORITY_RESET_THRESHOLD;
}

float TrafficQueue::calculateServiceTime() const {
    // |V| = 1/n Σ|Li| formula implementation
    size_t totalVehicles = size();
    return totalVehicles * PROCESSING_TIME;
}

void TrafficQueue::updateVehiclePositions() {
    const float VEHICLE_SPACING = 40.0f;  // Space between vehicles in queue

    for (size_t i = 0; i < m_vehicles.size(); ++i) {
        if (m_vehicles[i].vehicle) {
            // Calculate position based on queue index
            // This is a simplified version - you'll need to adjust based on lane orientation
            float offset = static_cast<float>(i) * VEHICLE_SPACING;
            // Set vehicle position - implementation depends on your coordinate system
        }
    }
}

