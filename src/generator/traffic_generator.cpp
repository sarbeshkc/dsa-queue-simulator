// src/generator/traffic_generator.cpp
#include "traffic_generator.h"

TrafficGenerator::TrafficGenerator(SDL_Renderer *renderer)
    : m_renderer(renderer), m_rng(std::random_device{}()),
      m_generationTimer(0.0f),
      m_generationInterval(1.0f / DEFAULT_GENERATION_RATE), m_vehicleCount(0),
      m_priorityRatio(DEFAULT_PRIORITY_RATIO),
      m_fileHandler(std::make_unique<FileHandler>("./traffic_data")) {

  initializeSpawnPoints();
}

void TrafficGenerator::initializeSpawnPoints() {
  const float CENTER_X = 640.0f;
  const float CENTER_Y = 360.0f;
  const float ROAD_WIDTH = 180.0f;
  const float LANE_WIDTH = ROAD_WIDTH / 3.0f;

  // Create spawn points for each incoming lane
  m_spawnPoints.emplace_back(
      Vector2D(CENTER_X - ROAD_WIDTH / 2 + LANE_WIDTH / 2, 0), Direction::SOUTH,
      LaneId::AL1_INCOMING);

  m_spawnPoints.emplace_back(
      Vector2D(1280, CENTER_Y - ROAD_WIDTH / 2 + LANE_WIDTH / 2),
      Direction::WEST, LaneId::BL1_INCOMING);

  // Add spawn points for other lanes...
}

void TrafficGenerator::update(float deltaTime) {
  m_generationTimer += deltaTime;

  // Check if it's time to generate a new vehicle
  if (m_generationTimer >= m_generationInterval) {
    if (Vehicle *vehicle = generateVehicle()) {
      // Write vehicle data to file for simulator
      m_fileHandler->writeVehicleData(*vehicle);
      delete vehicle; // Clean up as data is now in file
    }
    m_generationTimer = 0.0f;
  }
}

// src/generator/traffic_generator.cpp

// Add this implementation to your existing traffic_generator.cpp
TrafficGenerator::SpawnPoint TrafficGenerator::selectSpawnPoint() {
  // This method should randomly select a spawn point for new vehicles

  // Use our random number generator to pick a random spawn point
  std::uniform_int_distribution<> dist(0, m_spawnPoints.size() - 1);
  size_t index = dist(m_rng);

  // Return the selected spawn point
  return m_spawnPoints[index];
}

Vehicle *TrafficGenerator::generateVehicle() {
  // Select random spawn point
  SpawnPoint spawn = selectSpawnPoint();

  // Create new vehicle
  Vehicle *vehicle = new Vehicle(m_renderer, m_vehicleCount++, spawn.lane);
  vehicle->setPosition(spawn.position.x, spawn.position.y);

  // Determine if this should be a priority vehicle
  std::uniform_real_distribution<float> dist(0.0f, 1.0f);
  if (dist(m_rng) < m_priorityRatio) {
    // Convert to priority lane
    switch (spawn.lane) {
    case LaneId::AL1_INCOMING:
      vehicle->setTargetLane(LaneId::AL2_PRIORITY);
      break;
      // Handle other lanes...
    }
  }

  return vehicle;
}
