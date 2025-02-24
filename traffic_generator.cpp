#include <iostream>
#include <fstream>
#include <random>
#include <chrono>
#include <thread>
#include <filesystem>
#include <string>
#include <mutex>
#include <vector>
#include <map>

// Define constants similar to those in the main project
namespace Constants {
    static constexpr int MAX_QUEUE_SIZE = 100;
    static constexpr int PRIORITY_THRESHOLD = 10;
    static constexpr int NORMAL_THRESHOLD = 5;

    // Lane IDs (matching the main project)
    enum class LaneId {
        AL1_INCOMING = 0,
        AL2_PRIORITY = 1,
        AL3_FREELANE = 2,
        BL1_INCOMING = 3,
        BL2_NORMAL = 4,
        BL3_FREELANE = 5,
        CL1_INCOMING = 6,
        CL2_NORMAL = 7,
        CL3_FREELANE = 8,
        DL1_INCOMING = 9,
        DL2_NORMAL = 10,
        DL3_FREELANE = 11
    };

    // Direction values
    enum class Direction {
        STRAIGHT = 0,
        LEFT = 1,
        RIGHT = 2
    };
}

// Generator class to create vehicle data
class Generator {
private:
    std::mt19937 rng;  // Random number generator
    std::map<Constants::LaneId, std::filesystem::path> laneFiles;  // Lane file paths
    uint32_t nextVehicleId;  // ID counter for vehicles
    std::filesystem::path dataDir;  // Directory for data files
    std::mutex fileMutex;  // Thread safety for file operations

    // Settings for each lane
    struct LaneConfig {
        double spawnRate;
        int maxVehicles;
        bool canGoStraight;
        bool canGoLeft;
        bool canGoRight;
    };
    std::map<Constants::LaneId, LaneConfig> laneConfigs;

    // Helper methods
    void initializeLaneFiles() {
        using namespace Constants;

        laneFiles = {
            {LaneId::AL1_INCOMING, (dataDir / "lane_a1.txt").lexically_normal()},
            {LaneId::AL2_PRIORITY, (dataDir / "lane_a2.txt").lexically_normal()},
            {LaneId::AL3_FREELANE, (dataDir / "lane_a3.txt").lexically_normal()},
            {LaneId::BL1_INCOMING, (dataDir / "lane_b1.txt").lexically_normal()},
            {LaneId::BL2_NORMAL,   (dataDir / "lane_b2.txt").lexically_normal()},
            {LaneId::BL3_FREELANE, (dataDir / "lane_b3.txt").lexically_normal()},
            {LaneId::CL1_INCOMING, (dataDir / "lane_c1.txt").lexically_normal()},
            {LaneId::CL2_NORMAL,   (dataDir / "lane_c2.txt").lexically_normal()},
            {LaneId::CL3_FREELANE, (dataDir / "lane_c3.txt").lexically_normal()},
            {LaneId::DL1_INCOMING, (dataDir / "lane_d1.txt").lexically_normal()},
            {LaneId::DL2_NORMAL,   (dataDir / "lane_d2.txt").lexically_normal()},
            {LaneId::DL3_FREELANE, (dataDir / "lane_d3.txt").lexically_normal()},
        };
    }

    void setupLaneConfigs() {
        using namespace Constants;

        // First lanes (can go straight or right)
        LaneConfig firstLaneConfig;
        firstLaneConfig.spawnRate = 0.12;      // 12% spawn rate
        firstLaneConfig.maxVehicles = 12;      // Max 12 vehicles
        firstLaneConfig.canGoStraight = true;  // Can go straight
        firstLaneConfig.canGoRight = true;     // Can turn right
        firstLaneConfig.canGoLeft = false;     // Cannot turn left

        // Normal second lanes (straight only)
        LaneConfig normalSecondLaneConfig;
        normalSecondLaneConfig.spawnRate = 0.12;      // 12% spawn rate
        normalSecondLaneConfig.maxVehicles = 12;      // Max 12 vehicles
        normalSecondLaneConfig.canGoStraight = true;  // Can go straight
        normalSecondLaneConfig.canGoRight = false;    // Cannot turn
        normalSecondLaneConfig.canGoLeft = false;     // Cannot turn left

        // Priority lane config (AL2)
        LaneConfig priorityLaneConfig;
        priorityLaneConfig.spawnRate = 0.15;    // 15% spawn rate
        priorityLaneConfig.maxVehicles = 15;    // Max 15 vehicles
        priorityLaneConfig.canGoStraight = true;// Can go straight
        priorityLaneConfig.canGoRight = false;  // Cannot turn
        priorityLaneConfig.canGoLeft = false;   // Cannot turn left

        // Free lane config (L3)
        LaneConfig freeLaneConfig;
        freeLaneConfig.spawnRate = 0.08;        // 8% spawn rate
        freeLaneConfig.maxVehicles = 8;         // Max 8 vehicles
        freeLaneConfig.canGoStraight = false;   // Cannot go straight
        freeLaneConfig.canGoRight = false;      // Cannot turn right
        freeLaneConfig.canGoLeft = true;        // Can only turn left

        // Set configurations
        laneConfigs[LaneId::AL1_INCOMING] = firstLaneConfig;
        laneConfigs[LaneId::BL1_INCOMING] = firstLaneConfig;
        laneConfigs[LaneId::CL1_INCOMING] = firstLaneConfig;
        laneConfigs[LaneId::DL1_INCOMING] = firstLaneConfig;

        laneConfigs[LaneId::AL2_PRIORITY] = priorityLaneConfig;
        laneConfigs[LaneId::BL2_NORMAL] = normalSecondLaneConfig;
        laneConfigs[LaneId::CL2_NORMAL] = normalSecondLaneConfig;
        laneConfigs[LaneId::DL2_NORMAL] = normalSecondLaneConfig;

        laneConfigs[LaneId::AL3_FREELANE] = freeLaneConfig;
        laneConfigs[LaneId::BL3_FREELANE] = freeLaneConfig;
        laneConfigs[LaneId::CL3_FREELANE] = freeLaneConfig;
        laneConfigs[LaneId::DL3_FREELANE] = freeLaneConfig;
    }

    Constants::Direction getRandomDirection(const LaneConfig& config) {
        using namespace Constants;

        std::uniform_real_distribution<> dist(0.0, 1.0);
        float random = dist(rng);

        if (config.canGoLeft && (config.canGoStraight || config.canGoRight)) {
            // Lane can go left + others
            if (random < 0.7) {
                return Direction::LEFT;
            } else if (random < 0.85 && config.canGoStraight) {
                return Direction::STRAIGHT;
            } else if (config.canGoRight) {
                return Direction::RIGHT;
            } else {
                return Direction::LEFT;
            }
        }
        else if (config.canGoLeft) {
            // Lane can only go left
            return Direction::LEFT;
        }
        else if (config.canGoStraight && config.canGoRight) {
            // Lane can go straight or right
            return (random < 0.7) ? Direction::STRAIGHT : Direction::RIGHT;
        }
        else if (config.canGoStraight) {
            // Lane can only go straight
            return Direction::STRAIGHT;
        }
        else if (config.canGoRight) {
            // Lane can only go right
            return Direction::RIGHT;
        }

        // Default
        return Direction::STRAIGHT;
    }

    size_t countVehiclesInFile(const std::filesystem::path& filepath) const {
        try {
            std::ifstream file(filepath);
            if (!file) return 0;

            size_t count = 0;
            std::string line;
            while (std::getline(file, line)) {
                if (!line.empty()) count++;
            }
            return count;
        } catch (const std::exception& e) {
            std::cerr << "Error counting vehicles: " << e.what() << std::endl;
            return 0;
        }
    }

    bool shouldGenerateVehicle(const LaneConfig& config, size_t currentCount) {
        std::uniform_real_distribution<> dist(0.0, 1.0);
        float random = dist(rng);

        // Check if we're below the max and the random number is within spawn rate
        return (currentCount < config.maxVehicles) && (random < config.spawnRate);
    }

    void writeVehicleToFile(const std::filesystem::path& filepath, uint32_t id,
                           Constants::Direction dir) {
        std::lock_guard<std::mutex> lock(fileMutex);
        try {
            std::ofstream file(filepath, std::ios::app);
            if (!file) {
                throw std::runtime_error("Cannot open file: " + filepath.string());
            }

            // Write in format: id,direction;
            char dirChar;
            switch (dir) {
                case Constants::Direction::STRAIGHT: dirChar = 'S'; break;
                case Constants::Direction::LEFT: dirChar = 'L'; break;
                case Constants::Direction::RIGHT: dirChar = 'R'; break;
                default: dirChar = 'S';
            }

            file << id << "," << dirChar << ";\n";
            file.flush();

        } catch (const std::exception& e) {
            std::cerr << "Error writing to file " << filepath << ": " << e.what() << std::endl;
        }
    }

    void clearAllFiles() {
        for (const auto& [_, filepath] : laneFiles) {
            try {
                std::ofstream file(filepath, std::ios::trunc);
            } catch (const std::exception& e) {
                std::cerr << "Error clearing file: " << e.what() << std::endl;
            }
        }
    }

    // Log generation info
    void logGeneration(Constants::LaneId lane, uint32_t vehicleId, Constants::Direction dir,
                      size_t currentCount, int maxCount) {
        char laneChar;
        int laneNum;

        switch (lane) {
            case Constants::LaneId::AL1_INCOMING:
                laneChar = 'A'; laneNum = 1; break;
            case Constants::LaneId::AL2_PRIORITY:
                laneChar = 'A'; laneNum = 2; break;
            case Constants::LaneId::AL3_FREELANE:
                laneChar = 'A'; laneNum = 3; break;
            case Constants::LaneId::BL1_INCOMING:
                laneChar = 'B'; laneNum = 1; break;
            case Constants::LaneId::BL2_NORMAL:
                laneChar = 'B'; laneNum = 2; break;
            case Constants::LaneId::BL3_FREELANE:
                laneChar = 'B'; laneNum = 3; break;
            case Constants::LaneId::CL1_INCOMING:
                laneChar = 'C'; laneNum = 1; break;
            case Constants::LaneId::CL2_NORMAL:
                laneChar = 'C'; laneNum = 2; break;
            case Constants::LaneId::CL3_FREELANE:
                laneChar = 'C'; laneNum = 3; break;
            case Constants::LaneId::DL1_INCOMING:
                laneChar = 'D'; laneNum = 1; break;
            case Constants::LaneId::DL2_NORMAL:
                laneChar = 'D'; laneNum = 2; break;
            case Constants::LaneId::DL3_FREELANE:
                laneChar = 'D'; laneNum = 3; break;
            default:
                laneChar = '?'; laneNum = 0;
        }

        std::string dirStr;
        switch (dir) {
            case Constants::Direction::STRAIGHT: dirStr = "STRAIGHT"; break;
            case Constants::Direction::LEFT: dirStr = "LEFT"; break;
            case Constants::Direction::RIGHT: dirStr = "RIGHT"; break;
            default: dirStr = "UNKNOWN";
        }

        std::cout << "Generated vehicle " << vehicleId
                  << " in lane " << laneChar << laneNum
                  << " with direction " << dirStr
                  << " (Count: " << currentCount << "/" << maxCount << ")" << std::endl;
    }

public:
    Generator() : nextVehicleId(1) {
        try {
            // Initialize random generator
            std::random_device rd;
            rng.seed(rd());

            // Set up data directory
            dataDir = (std::filesystem::current_path() / "data" / "lanes").lexically_normal();
            std::cout << "Generator using path: " << dataDir << std::endl;
            std::filesystem::create_directories(dataDir);

            // Initialize files and configs
            initializeLaneFiles();
            setupLaneConfigs();
            clearAllFiles();

        } catch (const std::exception& e) {
            std::cerr << "Generator initialization failed: " << e.what() << std::endl;
            throw;
        }
    }

    // Generate traffic for all lanes
    void generateTraffic() {
        try {
            bool anyVehicleGenerated = false;

            for (const auto& [laneId, filepath] : laneFiles) {
                const auto& config = laneConfigs[laneId];
                size_t currentVehicles = countVehiclesInFile(filepath);

                if (shouldGenerateVehicle(config, currentVehicles)) {
                    Constants::Direction dir = getRandomDirection(config);
                    writeVehicleToFile(filepath, nextVehicleId, dir);
                    logGeneration(laneId, nextVehicleId, dir, currentVehicles + 1, config.maxVehicles);
                    nextVehicleId++;
                    anyVehicleGenerated = true;
                }
            }

            if (anyVehicleGenerated) {
                // Brief pause after generating a vehicle
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

        } catch (const std::exception& e) {
            std::cerr << "Error in traffic generation: " << e.what() << std::endl;
        }
    }

    // Display generator status
    void displayStatus() const {
        std::cout << "\n---- Traffic Generator Status ----\n";

        for (const auto& [laneId, filepath] : laneFiles) {
            const auto& config = laneConfigs.at(laneId);
            size_t count = countVehiclesInFile(filepath);

            // Format lane ID
            char laneChar;
            int laneNum;
            switch (laneId) {
                case Constants::LaneId::AL1_INCOMING:
                    laneChar = 'A'; laneNum = 1; break;
                case Constants::LaneId::AL2_PRIORITY:
                    laneChar = 'A'; laneNum = 2; break;
                case Constants::LaneId::AL3_FREELANE:
                    laneChar = 'A'; laneNum = 3; break;
                case Constants::LaneId::BL1_INCOMING:
                    laneChar = 'B'; laneNum = 1; break;
                case Constants::LaneId::BL2_NORMAL:
                    laneChar = 'B'; laneNum = 2; break;
                case Constants::LaneId::BL3_FREELANE:
                    laneChar = 'B'; laneNum = 3; break;
                case Constants::LaneId::CL1_INCOMING:
                    laneChar = 'C'; laneNum = 1; break;
                case Constants::LaneId::CL2_NORMAL:
                    laneChar = 'C'; laneNum = 2; break;
                case Constants::LaneId::CL3_FREELANE:
                    laneChar = 'C'; laneNum = 3; break;
                case Constants::LaneId::DL1_INCOMING:
                    laneChar = 'D'; laneNum = 1; break;
                case Constants::LaneId::DL2_NORMAL:
                    laneChar = 'D'; laneNum = 2; break;
                case Constants::LaneId::DL3_FREELANE:
                    laneChar = 'D'; laneNum = 3; break;
                default:
                    laneChar = '?'; laneNum = 0;
            }

            std::cout << "Lane " << laneChar << laneNum
                     << " | Vehicles: " << count << "/" << config.maxVehicles
                     << " | Spawn Rate: " << (config.spawnRate * 100) << "% | "
                     << "Directions: "
                     << (config.canGoStraight ? "S" : "")
                     << (config.canGoLeft ? "L" : "")
                     << (config.canGoRight ? "R" : "")
                     << std::endl;
        }

        std::cout << "--------------------------------\n";
    }
};

int main() {
    try {
        Generator generator;

        std::cout << "Traffic Generator Started\n"
                  << "========================\n\n";

        // Run generator
        while (true) {
            generator.generateTraffic();
            generator.displayStatus();

            // Wait a bit before next generation cycle
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        }

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
