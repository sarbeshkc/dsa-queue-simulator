// src/traffic/lane_proce// src/traffic/lane_processor.h
#ifndef LANE_PROCESSOR_H
#define LANE_PROCESSOR_H

#include "vehicle.h" // For Vehicle class
#include <memory>    // For smart pointers
#include <queue>     // For std::queue

class LaneProcessor {
public:
  // Initialize with no processing time
  LaneProcessor() : m_processingTimer(0.0f) {}

  // Core processing methods
  void update(float deltaTime) {
    if (!m_vehicleQueue.empty()) {
      m_processingTimer += deltaTime;
    }
  }

  bool canProcessVehicle() const { return m_processingTimer >= PROCESS_TIME; }

  size_t getQueueLength() const { return m_vehicleQueue.size(); }

  void resetProcessingTimer() { m_processingTimer = 0.0f; }

private:
  std::queue<Vehicle *>
      m_vehicleQueue;      // Queue of vehicles waiting to be processed
  float m_processingTimer; // Time spent processing current vehicle
  static constexpr float PROCESS_TIME = 2.0f;
};

#endif // LANE_PROCESSOR_H
