//FileHandler.h
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

class FileHandler {
public:
    FileHandler();
    ~FileHandler() = default;

    // Core file operations
    std::vector<std::pair<LaneId, std::shared_ptr<Vehicle>>> readNewVehicles();
    void clearLaneFiles();
    bool writeVehicleToLane(LaneId laneId, const std::shared_ptr<Vehicle>& vehicle);

    // State queries
    bool isLaneFileAvailable(LaneId laneId) const;
    size_t getVehicleCountInFile(LaneId laneId) const;
    std::chrono::system_clock::time_point getLastModifiedTime(LaneId laneId) const;

private:
    // File management
    std::map<LaneId, std::filesystem::path> laneFiles;
    std::map<std::filesystem::path, std::chrono::steady_clock::time_point> lastCheckTimes;
    std::map<std::filesystem::path, int64_t> lastReadPositions;
    std::filesystem::path dataDir;
    std::mutex fileMutex;

    // Configuration
    static constexpr int FILE_CHECK_INTERVAL_MS = 100;
    static const std::string BASE_PATH;

    // File operation methods
    void initializeFileSystem();
    void validateFileSystem() const;
    std::vector<std::shared_ptr<Vehicle>> parseVehicleData(const std::string& data, LaneId laneId);
    std::shared_ptr<Vehicle> parseVehicleLine(const std::string& line, LaneId laneId);
    void updateLastReadPosition(const std::filesystem::path& filepath, int64_t position);

    // Helper methods
    std::filesystem::path getLaneFilePath(LaneId laneId) const;
    bool shouldCheckFile(const std::filesystem::path& filepath) const;
    void ensureDirectoryExists(const std::filesystem::path& dir);
    void logFileOperation(const std::string& operation, const std::filesystem::path& filepath) const;

    // File access checks
    bool hasReadAccess(const std::filesystem::path& filepath) const;
    bool hasWriteAccess(const std::filesystem::path& filepath) const;

    // Error handling
    void handleFileError(const std::string& operation, const std::filesystem::path& filepath, const std::exception& e) const;
    void validateFilePath(const std::filesystem::path& filepath) const;

    // Data validation
    bool isValidVehicleData(const std::string& data) const;
    bool isValidVehicleId(uint32_t id) const;
    bool isValidDirection(char dirChar) const;

    // File system utilities
    void createEmptyFile(const std::filesystem::path& filepath);
    bool isFileEmpty(const std::filesystem::path& filepath) const;
    void truncateFile(const std::filesystem::path& filepath);

#ifdef _DEBUG
    // Debug helpers
    void dumpFileContents(const std::filesystem::path& filepath) const;
    void validateFileIntegrity() const;
    void checkFileSizes() const;
#endif
};
