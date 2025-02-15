// src/traffic/traffic_manager.h
#ifndef TRAFFIC_MANAGER_H
#define TRAFFIC_MANAGER_H

#include "lane.h"
#include "queue_processor.h"
#include "vehicle.h"
#include <map>
#include <memory>
#include <vector>

class TrafficManager {
public:
  TrafficManager(SDL_Renderer *renderer);
  void update(float deltaTime);
  void render() const;
  void addVehicle(Vehicle *vehicle);
  void processLanes(float deltaTime);
  void checkPriorityConditions();

private:
  SDL_Renderer *m_renderer;
  std::map<LaneId, std::unique_ptr<Lane>> m_lanes;
  std::unique_ptr<QueueProcessor> m_queueProcessor;
  bool m_priorityMode;

  // Add these missing method declarations
  void processPriorityLanes(float deltaTime);
  std::vector<Vehicle *> readNewVehicles();


  int calculateVehiclesToProcess() const;

};

#endif // TRAFFIC_MANAGER_H
