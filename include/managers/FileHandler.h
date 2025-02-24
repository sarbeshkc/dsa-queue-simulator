// include/managers/FileHandler.h (modified)
#pragma once
#include "core/Vehicle.h"
#include "core/Constants.h"
#include <memory>
#include <vector>
#include <string>
#include <map>
#include <filesystem>
#include <chrono>
#include <mutex>
#include <thread>

class FileHandler {
public:
    FileHandler();
    ~FileHandler() = default;

    // Core file operations
    std::vector<std::pair<LaneId, std::shared_ptr<Vehicle>>> readNewVehicles();
    std::vector<std::pair<LaneId, std::shared_ptr<Vehicle>>> getReadyVehicles();
    void clearLaneFiles();

    // State queries
    bool isLaneFileAvailable(LaneId laneId) const;
    size_t getVehicleCountInFile(LaneId laneId) const;
    std::chrono::system_clock::time_point getLastModifiedTime(LaneId laneId) const;

private:
    std::map<LaneId, std::filesystem::path> laneFiles;
    std::filesystem::path dataDir;
    std::filesystem::path vehicleDataFile; // Single file for all vehicles
    std::mutex fileMutex;
    std::thread watcherThread;
    std::vector<std::pair<LaneId, std::shared_ptr<Vehicle>>> vehiclesReady;

    static constexpr int FILE_CHECK_INTERVAL_MS = 100;
    static const std::string BASE_PATH;

    // File operation methods
    void initializeFileSystem();
    void validateFileSystem() const;
    std::vector<std::shared_ptr<Vehicle>> parseVehicleData(const std::string& data, LaneId laneId);
    std::shared_ptr<Vehicle> parseVehicleLine(const std::string& line, LaneId laneId);

    // File watching methods
    void startFileWatching();
    void watchForFileChanges();

    // Helper methods
    std::filesystem::path getLaneFilePath(LaneId laneId) const;
    void ensureDirectoryExists(const std::filesystem::path& dir);
    void logFileOperation(const std::string& operation, const std::filesystem::path& filepath) const;
    void handleFileError(const std::string& operation, const std::filesystem::path& filepath, const std::exception& e) const;
};
