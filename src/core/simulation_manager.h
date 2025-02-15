// src/core/simulation_manager.h
#ifndef SIMULATION_MANAGER_H
#define SIMULATION_MANAGER_H

#include "../traffic/traffic_coordinator.h"
#include "../traffic/traffic_communicator.h"
#include <memory>

class SimulationManager {
public:
    SimulationManager()
        : m_coordinator(std::make_unique<TrafficCoordinator>(m_window.getRenderer()))
        , m_communicator(std::make_unique<TrafficCommunicator>("./traffic_data"))
        , m_accumulatedTime(0.0f) {
    }

    void update(float deltaTime) {
        m_accumulatedTime += deltaTime;

        // Process vehicles every 2 seconds as per assignment requirement
        if (m_accumulatedTime >= 2.0f) {
            // Read new vehicle data from files
            auto vehicleData = m_communicator->readVehicleStates();
            for (const auto& data : vehicleData) {
                addVehicleToSimulation(data);
            }

            // Update traffic coordination
            m_coordinator->update(deltaTime);

            // Write current vehicle states back to files
            for (const auto& vehicle : m_coordinator->getActiveVehicles()) {
                m_communicator->writeVehicleState(*vehicle);
            }

            m_accumulatedTime = 0.0f;
        }
    }

private:
    std::unique_ptr<TrafficCoordinator> m_coordinator;
    std::unique_ptr<TrafficCommunicator> m_communicator;
    float m_accumulatedTime;

    void addVehicleToSimulation(const VehicleData& data) {
        // Create new vehicle from data and add to coordinator
        Vehicle* vehicle = new Vehicle(data);
        m_coordinator->addVehicle(vehicle);
    }
};

#endif // SIMULATION_MANAGER_H
