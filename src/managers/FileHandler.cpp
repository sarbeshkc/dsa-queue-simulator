// In FileHandler.cpp - improve file communication for better reliability

#include "managers/FileHandler.h"
#include "utils/DebugLogger.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;

FileHandler::FileHandler(const std::string& dataPath)
    : dataPath(dataPath) {
    DebugLogger::log("FileHandler created with path: " + dataPath);
}

FileHandler::~FileHandler() {
    DebugLogger::log("FileHandler destroyed");
}

std::vector<Vehicle*> FileHandler::readVehiclesFromFiles() {
    std::lock_guard<std::mutex> lock(mutex);
    std::vector<Vehicle*> vehicles;

    // Read from each lane file (A, B, C, D)
    for (char laneId : {'A', 'B', 'C', 'D'}) {
        auto laneVehicles = readVehiclesFromFile(laneId);
        vehicles.insert(vehicles.end(), laneVehicles.begin(), laneVehicles.end());
    }

    return vehicles;
}

std::vector<Vehicle*> FileHandler::readVehiclesFromFile(char laneId) {
    std::vector<Vehicle*> vehicles;
    std::string filePath = getLaneFilePath(laneId);

    // Try multiple times in case of file access issues
    const int maxRetries = 3;
    for (int retry = 0; retry < maxRetries; retry++) {
        try {
            // Create a temporary file path
            std::string tempFilePath = filePath + ".tmp";

            // Rename the original file to temp file (atomic operation)
            // This prevents losing data if the program crashes during reading
            if (fs::exists(filePath)) {
                if (fs::exists(tempFilePath)) {
                    fs::remove(tempFilePath);
                }
                fs::rename(filePath, tempFilePath);

                // Now read from the temp file
                std::ifstream file(tempFilePath);
                if (!file.is_open()) {
                    DebugLogger::log("Warning: Could not open temp file " + tempFilePath, DebugLogger::LogLevel::WARNING);
                    // Try to restore the original file
                    fs::rename(tempFilePath, filePath);
                    continue; // Try again
                }

                std::string line;
                while (std::getline(file, line)) {
                    if (!line.empty()) {
                        Vehicle* vehicle = parseVehicleLine(line);
                        if (vehicle) {
                            vehicles.push_back(vehicle);
                        }
                    }
                }
                file.close();

                // Delete the temp file after successful read
                fs::remove(tempFilePath);
            }

            // Create an empty file for the generator to write to
            std::ofstream newFile(filePath);
            newFile.close();

            // Log success
            if (!vehicles.empty()) {
                std::ostringstream oss;
                oss << "Read " << vehicles.size() << " vehicles from lane " << laneId;
                DebugLogger::log(oss.str());
            }

            // Successfully read, break the retry loop
            break;

        } catch (const std::exception& e) {
            DebugLogger::log("Error reading file " + filePath + ": " + e.what(), DebugLogger::LogLevel::ERROR);

            // Wait a bit before retry
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    return vehicles;
}

Vehicle* FileHandler::parseVehicleLine(const std::string& line) {
    // Expected format: "vehicleId:laneId" or "vehicleId_L{laneNumber}:laneId"
    size_t pos = line.find(":");
    if (pos == std::string::npos) {
        DebugLogger::log("Error parsing line: " + line, DebugLogger::LogLevel::ERROR);
        return nullptr;
    }

    std::string vehicleId = line.substr(0, pos);
    char laneId = line[pos + 1];

    // Extract lane number from ID (format: V1_L2 where 2 is the lane number)
    int laneNumber = 2; // Default is lane 2
    size_t lanePos = vehicleId.find("_L");
    if (lanePos != std::string::npos && lanePos + 2 < vehicleId.length()) {
        char laneNumChar = vehicleId[lanePos + 2];
        if (laneNumChar >= '1' && laneNumChar <= '3') {
            laneNumber = laneNumChar - '0';
        }
    }

    // Check for direction info for Lane 3 (format: V1_L3_LEFT)
    Destination destination = Destination::STRAIGHT;
    if (laneNumber == 3) {
        // Free lane (L3) should always turn left per assignment
        destination = Destination::LEFT;

        // But also check if the direction is explicitly specified
        size_t dirPos = vehicleId.find("_LEFT");
        if (dirPos != std::string::npos) {
            destination = Destination::LEFT;
        } else if (vehicleId.find("_RIGHT") != std::string::npos) {
            // We should still enforce left turn in the free lane
            DebugLogger::log("Warning: RIGHT turn specified for free lane, enforcing LEFT turn", DebugLogger::LogLevel::WARNING);
            destination = Destination::LEFT;
        } else if (vehicleId.find("_STRAIGHT") != std::string::npos) {
            // We should still enforce left turn in the free lane
            DebugLogger::log("Warning: STRAIGHT specified for free lane, enforcing LEFT turn", DebugLogger::LogLevel::WARNING);
            destination = Destination::LEFT;
        }
    }

    // Determine if it's an emergency vehicle (based on ID pattern)
    bool isEmergency = vehicleId.find("E") != std::string::npos;

    // Create the vehicle
    Vehicle* vehicle = new Vehicle(vehicleId, laneId, laneNumber, isEmergency);

    // Force left turn for Lane 3 vehicles
    if (laneNumber == 3) {
        // We need to access protected member, so we'll rely on the update method
        // to enforce left turns for lane 3
    }

    return vehicle;
}

void FileHandler::writeLaneStatus(char laneId, int laneNumber, int vehicleCount, bool isPriority) {
    std::lock_guard<std::mutex> lock(mutex);
    std::string statusPath = getLaneStatusFilePath();

    std::ofstream file(statusPath, std::ios::app);
    if (file.is_open()) {
        file << laneId << laneNumber << ": " << vehicleCount << " vehicles"
             << (isPriority ? " (PRIORITY)" : "") << std::endl;
        file.close();
    } else {
        DebugLogger::log("Warning: Could not open lane status file for writing", DebugLogger::LogLevel::WARNING);
    }
}

bool FileHandler::checkFilesExist() {
    for (char laneId : {'A', 'B', 'C', 'D'}) {
        std::string filePath = getLaneFilePath(laneId);
        if (!fs::exists(filePath)) {
            return false;
        }
    }
    return true;
}

bool FileHandler::initializeFiles() {
    std::lock_guard<std::mutex> lock(mutex);

    try {
        // Create data directory structure if it doesn't exist
        if (!fs::exists(dataPath)) {
            if (!fs::create_directories(dataPath)) {
                DebugLogger::log("Error: Failed to create directory " + dataPath, DebugLogger::LogLevel::ERROR);
                return false;
            }
            DebugLogger::log("Created directory: " + dataPath);
        }

        // Create lane files if they don't exist
        for (char laneId : {'A', 'B', 'C', 'D'}) {
            std::string filePath = getLaneFilePath(laneId);
            if (!fs::exists(filePath)) {
                std::ofstream file(filePath);
                if (!file.is_open()) {
                    DebugLogger::log("Error: Failed to create file " + filePath, DebugLogger::LogLevel::ERROR);
                    return false;
                }
                file.close();
                DebugLogger::log("Created file: " + filePath);
            } else {
                // Clear existing files
                std::ofstream file(filePath, std::ios::trunc);
                file.close();
                DebugLogger::log("Cleared file: " + filePath);
            }
        }

        // Create or clear lane status file
        std::string statusPath = getLaneStatusFilePath();
        std::ofstream statusFile(statusPath, std::ios::trunc);
        if (!statusFile.is_open()) {
            DebugLogger::log("Error: Failed to create lane status file", DebugLogger::LogLevel::ERROR);
            return false;
        }
        statusFile.close();

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
