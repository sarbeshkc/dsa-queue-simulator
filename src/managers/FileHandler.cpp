#include "managers/FileHandler.h"
#include "utils/DebugLogger.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <ctime>

namespace fs = std::filesystem;

FileHandler::FileHandler(const std::string& dataPath)
    : dataPath(dataPath) {

    // Create directories if they don't exist
    try {
        if (!fs::exists(dataPath)) {
            fs::create_directories(dataPath);
            DebugLogger::log("Created directory: " + dataPath);
        }
    }
    catch (const std::exception& e) {
        DebugLogger::log("Error creating directory: " + std::string(e.what()), DebugLogger::LogLevel::ERROR);
    }
}

FileHandler::~FileHandler() {}

std::vector<Vehicle*> FileHandler::readVehiclesFromFiles() {
    std::vector<Vehicle*> vehicles;
    // Lock the mutex to prevent multiple threads from reading/writing files simultaneously
    std::lock_guard<std::mutex> lock(mutex);

    // Process each lane file
    for (char laneId : {'A', 'B', 'C', 'D'}) {
        // Create file path for this lane
        std::string filePath = dataPath + "/lane" + laneId + ".txt";

        try {
            std::ifstream file(filePath);
            if (!file.is_open()) {
                continue; // Skip if file can't be opened
            }

            // Read all lines first
            std::vector<std::string> lines;
            std::string line;
            while (std::getline(file, line)) {
                if (!line.empty()) {
                    lines.push_back(line);
                }
            }
            file.close();

            // Clear the file atomically
            std::ofstream clearFile(filePath, std::ios::trunc);
            clearFile.close();

            // Process the lines
            for (const auto& line : lines) {
                size_t pos = line.find(":");
                if (pos == std::string::npos) continue;

                std::string vehicleId = line.substr(0, pos);
                char lane = line[pos + 1];

                // Determine lane number from ID
                int laneNumber = 2; // Default
                if (vehicleId.find("L1") != std::string::npos) {
                    laneNumber = 1;
                } else if (vehicleId.find("L3") != std::string::npos) {
                    laneNumber = 3;
                }

                // Create vehicle (without emergency flag)
                try {
                    bool isEmergency = false; // Always false since we don't use emergency vehicles
                    vehicles.push_back(new Vehicle(vehicleId, lane, laneNumber, isEmergency));
                } catch (const std::exception& e) {
                    DebugLogger::log("Error creating vehicle from: " + line + " - " + e.what(), DebugLogger::LogLevel::ERROR);
                }
            }
        }
        catch (const std::exception& e) {
            DebugLogger::log("Error reading file " + filePath + ": " + std::string(e.what()), DebugLogger::LogLevel::ERROR);
        }
    }

    return vehicles;
}

void FileHandler::writeLaneStatus(char laneId, int laneNumber, int vehicleCount, bool isPriority) {
    // Lock the mutex to prevent multiple threads from writing simultaneously
    std::lock_guard<std::mutex> lock(mutex);

    std::string statusPath = dataPath + "/lane_status.txt";

    try {
        // Simple timestamp
        time_t now = std::time(nullptr);
        struct tm timeinfo;

    #ifdef _WIN32
        localtime_s(&timeinfo, &now);
    #else
        localtime_r(&now, &timeinfo);
    #endif

        char timeBuffer[80];
        strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &timeinfo);

        // Create status line
        std::stringstream ss;
        ss << timeBuffer << " | Lane " << laneId << laneNumber
           << " | Vehicles: " << vehicleCount
           << " | Priority: " << (isPriority ? "Yes" : "No");

        // Write to file
        std::ofstream file(statusPath, std::ios::app);
        if (file.is_open()) {
            file << ss.str() << std::endl;
            file.close();
        }
        else {
            DebugLogger::log("Failed to open status file: " + statusPath, DebugLogger::LogLevel::ERROR);
        }
    }
    catch (const std::exception& e) {
        DebugLogger::log("Error writing lane status: " + std::string(e.what()), DebugLogger::LogLevel::ERROR);
    }
}

bool FileHandler::checkFilesExist() {
    try {
        if (!fs::exists(dataPath)) {
            return false;
        }

        for (char laneId : {'A', 'B', 'C', 'D'}) {
            std::string filePath = dataPath + "/lane" + laneId + ".txt";
            if (!fs::exists(filePath)) {
                return false;
            }
        }

        return true;
    }
    catch (const std::exception& e) {
        DebugLogger::log("Error checking files: " + std::string(e.what()), DebugLogger::LogLevel::ERROR);
        return false;
    }
}

bool FileHandler::initializeFiles() {
    // Lock the mutex to prevent multiple threads from initializing simultaneously
    std::lock_guard<std::mutex> lock(mutex);

    try {
        // Create data directory
        if (!fs::exists(dataPath)) {
            fs::create_directories(dataPath);
            DebugLogger::log("Created directory: " + dataPath);
        }

        // Create lane files
        for (char laneId : {'A', 'B', 'C', 'D'}) {
            std::string filePath = dataPath + "/lane" + laneId + ".txt";
            if (!fs::exists(filePath)) {
                std::ofstream file(filePath);
                if (!file) {
                    DebugLogger::log("Failed to create file: " + filePath, DebugLogger::LogLevel::ERROR);
                    return false;
                }
                file.close();
                DebugLogger::log("Created file: " + filePath);
            }
        }

        // Create status file
        std::string statusPath = dataPath + "/lane_status.txt";
        std::ofstream file(statusPath, std::ios::trunc);
        if (!file) {
            DebugLogger::log("Failed to create status file: " + statusPath, DebugLogger::LogLevel::ERROR);
            return false;
        }
        file << "Timestamp | Lane | Vehicles | Priority" << std::endl;
        file << "----------------------------------------" << std::endl;
        file.close();
        DebugLogger::log("Created status file: " + statusPath);

        return true;
    } catch (const std::exception& e) {
        DebugLogger::log("Error initializing files: " + std::string(e.what()), DebugLogger::LogLevel::ERROR);
        return false;
    }
}

std::string FileHandler::getLaneFilePath(char laneId) const {
    return dataPath + "/lane" + laneId + ".txt";
}

std::string FileHandler::getLaneStatusFilePath() const {
    return dataPath + "/lane_status.txt";
}
