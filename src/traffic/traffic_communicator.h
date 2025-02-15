// src/communication/traffic_communicator.h
#ifndef TRAFFIC_COMMUNICATOR_H
#define TRAFFIC_COMMUNICATOR_H

#include <string>
#include <filesystem>
#include <fstream>
#include "../traffic/vehicle.h"

class TrafficCommunicator {
public:
    TrafficCommunicator(const std::string& dataPath)
        : m_dataPath(dataPath) {
        initializeDirectory();
    }

    // Write vehicle data to appropriate lane file
    void writeVehicleState(const Vehicle& vehicle) {
        std::string filename = getLaneFileName(vehicle.getLane());
        std::ofstream file(m_dataPath / filename, std::ios::app);

        if (file.is_open()) {
            // Write vehicle data in simple format: id,x,y,priority
            file << vehicle.getId() << ","
                 << vehicle.getX() << ","
                 << vehicle.getY() << ","
                 << (vehicle.isPriority() ? "1" : "0")
                 << std::endl;
        }
    }

    // Read vehicle data from lane files
    std::vector<VehicleData> readVehicleStates() {
        std::vector<VehicleData> vehicles;

        // Read from each lane file
        for (const auto& file : std::filesystem::directory_iterator(m_dataPath)) {
            if (file.path().extension() == ".txt") {
                std::ifstream inFile(file.path());
                std::string line;

                while (std::getline(inFile, line)) {
                    vehicles.push_back(parseVehicleData(line));
                }

                // Clear file after reading
                std::ofstream clearFile(file.path(), std::ios::trunc);
            }
        }

        return vehicles;
    }

private:
    std::filesystem::path m_dataPath;

    void initializeDirectory() {
        if (!std::filesystem::exists(m_dataPath)) {
            std::filesystem::create_directories(m_dataPath);
        }
    }

    std::string getLaneFileName(LaneId lane) const {
        switch (lane) {
            case LaneId::AL1_INCOMING: return "lane_a1.txt";
            case LaneId::AL2_PRIORITY: return "lane_a2.txt";
            case LaneId::AL3_FREELANE: return "lane_a3.txt";
            // Add other cases...
            default: return "unknown_lane.txt";
        }
    }

    VehicleData parseVehicleData(const std::string& line) {
        std::stringstream ss(line);
        std::string item;
        std::vector<std::string> data;

        while (std::getline(ss, item, ',')) {
            data.push_back(item);
        }

        VehicleData vehicle;
        if (data.size() >= 4) {
            vehicle.id = std::stoi(data[0]);
            vehicle.x = std::stof(data[1]);
            vehicle.y = std::stof(data[2]);
            vehicle.isPriority = (data[3] == "1");
        }
        return vehicle;
    }
};

#endif // TRAFFIC_COMMUNICATOR_H
