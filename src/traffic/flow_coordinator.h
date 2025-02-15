// src/traffic/flow_coordinator.h
#ifndef FLOW_COORDINATOR_H
#define FLOW_COORDINATOR_H

#include "lane_manager.h"
#include "light_controller.h"
#include "lane_change_handler.h"
#include <memory>
#include <queue>

class FlowCoordinator {
public:
    FlowCoordinator(SDL_Renderer* renderer);
    ~FlowCoordinator() = default;

    // Core coordination functions
    void update(float deltaTime);
    void render() const;

    // Vehicle management
    void addVehicle(Vehicle* vehicle);
    void removeVehicle(Vehicle* vehicle);

    // Traffic state queries
    bool isPriorityModeActive() const { return m_priorityModeActive; }
    int getTotalVehiclesProcessed() const { return m_totalVehiclesProcessed; }
    float getAverageWaitTime() const;
    float getMaxWaitTime() const { return m_maxWaitTime; }

private:
    // Core components
    std::unique_ptr<LaneManager> m_laneManager;
    std::unique_ptr<LightController> m_lightController;
    std::unique_ptr<LaneChangeHandler> m_laneChangeHandler;
    SDL_Renderer* m_renderer;

    // Statistics tracking
    int m_totalVehiclesProcessed;
    float m_maxWaitTime;
    bool m_priorityModeActive;
    float m_stateTimer;

    struct VehicleNode {
        Vehicle* vehicle;
        float entryTime;

        VehicleNode(Vehicle* v)
            : vehicle(v)
            , entryTime(SDL_GetTicks() / 1000.0f) {}
    };
    std::vector<VehicleNode> m_activeVehicles;

    // Helper methods
    void updateVehicleStates(float deltaTime);
    void checkIntersectionCrossings();
    void updateStatistics();
    void managePriorityFlow();
    bool canVehicleProceed(const Vehicle* vehicle) const;
    void cleanupVehicles();
};

#endif

