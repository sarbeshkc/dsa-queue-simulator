// include/managers/FileHandler.hpp
#pragma once
#include "core/Vehicle.h"
#include <memory>
#include <vector>
#include <string>
#include <map>
#include <filesystem>
#include <fstream>
#include <iostream>

class FileHandler {
private:



    static constexpr int WINDOW_WIDTH = 1024;
    static constexpr int WINDOW_HEIGHT = 768;
    static constexpr int ROAD_WIDTH = 270;     // Width for 3 lanes (90 * 3)
    static constexpr int LANE_WIDTH = 90;      // Individual lane width
    static constexpr int CENTER_X = WINDOW_WIDTH / 2;
    static constexpr int CENTER_Y = WINDOW_HEIGHT / 2;
    static constexpr float QUEUE_SPACING = 40.0f;
    static constexpr float QUEUE_START_OFFSET = 250.0f;



    static const std::string BASE_PATH;


void ensureDataDirectoryExists() {
    const auto dataPath = std::filesystem::current_path() / "data" / "lanes";
    std::filesystem::create_directories(dataPath);
    std::cout << "Data directory ensured at: " << dataPath << std::endl;
}

// Add this to FileHandler class header
void debugPrintFileContents(const std::filesystem::path& filepath) {
    std::ifstream file(filepath);
    if (!file) {
        std::cerr << "Could not open file for debug: " << filepath << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::cout << "File content: " << line << std::endl;
    }
}
    std::map<LaneId, std::filesystem::path> laneFiles;
    std::map<std::filesystem::path, int64_t> lastReadPositions;
    std::filesystem::path dataDir;

public:
    FileHandler();  // Declaration only, definition will be in cpp file

    std::vector<std::pair<LaneId, std::shared_ptr<Vehicle>>> readNewVehicles();
    void clearLaneFiles();

private:
    std::vector<std::shared_ptr<Vehicle>> parseVehicleData(const std::string& data, LaneId laneId);
};
