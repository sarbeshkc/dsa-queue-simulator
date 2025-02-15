// src/traffic/traffic_manager.cpp
#include "traffic_manager.h"
#include <algorithm> // For std::find_if and other algorithms
#include <cmath>     // For mathematical operations like ceil()

// Constructor initializes our traffic management system
TrafficManager::TrafficManager(SDL_Renderer *renderer)
    : m_renderer(renderer),
      m_queueProcessor(std::make_unique<QueueProcessor>()),
      m_priorityMode(false) { // Start in normal mode

  // Initialize lanes for each road
  for (int i = 0; i < 12; ++i) {
    LaneId laneId = static_cast<LaneId>(i);
    m_lanes[laneId] = std::make_unique<Lane>(m_renderer, laneId);
  }
}



int TrafficManager::calculateVehiclesToProcess() const {
  // Implement the formula |V| = 1/n Σ|Li|
  int normalLaneCount = 0;
  int totalQueueLength = 0;

  // Count only normal (non-priority) lanes
  for (const auto &[id, lane] : m_lanes) {
    if (!lane->isPriorityLane()) {
      normalLaneCount++;
      totalQueueLength += lane->getQueueLength();
    }
  }

  // Calculate average and round up as per assignment requirements
  if (normalLaneCount == 0)
    return 0;
  return static_cast<int>(
      std::ceil(static_cast<float>(totalQueueLength) / normalLaneCount));
}

// Update function processes all traffic management logic
void TrafficManager::update(float deltaTime) {
  // Check if any priority conditions need handling
  checkPriorityConditions();

  // Process all lanes according to traffic rules
  processLanes(deltaTime);

  // Read any new vehicles from the communication system
  auto newVehicles = readNewVehicles();
  for (auto *vehicle : newVehicles) {
    addVehicle(vehicle);
  }
}

// Render displays the current state of all traffic elements
void TrafficManager::render() const {
  // Render all lanes and their vehicles
  for (const auto &[id, lane] : m_lanes) {
    lane->render();
  }
}

// Add a new vehicle to the appropriate lane
void TrafficManager::addVehicle(Vehicle *vehicle) {
  if (!vehicle)
    return;

  auto laneId = vehicle->getCurrentLaneId();
  if (auto it = m_lanes.find(laneId); it != m_lanes.end()) {
    it->second->addVehicle(vehicle);
  }
}

// Process all lanes according to traffic rules
void TrafficManager::processLanes(float deltaTime) {
  // In priority mode, handle priority lanes first
  if (m_priorityMode) {
    processPriorityLanes(deltaTime);
  }

  // Process normal lanes using the |V| = 1/n Σ|Li| formula
  int vehiclesToProcess = calculateVehiclesToProcess();

  for (auto &[id, lane] : m_lanes) {
    if (!lane->isPriorityLane()) {
      lane->update(deltaTime);
    }
  }
}



// Check and update priority conditions
void TrafficManager::checkPriorityConditions() {
  // Check if any priority lane has more than 10 vehicles
  for (const auto &[id, lane] : m_lanes) {
    if (lane->isPriorityLane() && lane->getQueueLength() >= 10) {
      m_priorityMode = true;
      return;
    }
  }

  // Check if we can exit priority mode (all priority lanes < 5 vehicles)
  bool canExitPriority = true;
  for (const auto &[id, lane] : m_lanes) {
    if (lane->isPriorityLane() && lane->getQueueLength() >= 5) {
      canExitPriority = false;
      break;
    }
  }

  if (canExitPriority) {
    m_priorityMode = false;
  }
}

// Process priority lanes when in priority mode
void TrafficManager::processPriorityLanes(float deltaTime) {
  for (auto &[id, lane] : m_lanes) {
    if (lane->isPriorityLane()) {
      lane->update(deltaTime);
    }
  }
}

// Helper function to read new vehicles from communication system
std::vector<Vehicle *> TrafficManager::readNewVehicles() {
  // Implementation depends on your communication system
  return std::vector<Vehicle *>();
}
