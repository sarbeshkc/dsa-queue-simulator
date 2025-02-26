// FILE: src/traffic_generator.cpp
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <random>
#include <chrono>
#include <thread>
#include <filesystem>
#include <ctime>
#include <mutex>
#include <iomanip>
#include <atomic>
#include <csignal>
#include <map>  // Added missing map header

// Include Windows-specific headers if on Windows
#ifdef _WIN32
#include <windows.h>
#endif

// Namespaces
namespace fs = std::filesystem;

// Constants for the generator
const std::string DATA_DIR = "data/lanes";
const int GENERATION_INTERVAL_MS = 800;
const int MAX_VEHICLES_PER_BATCH = 50;
const int PRIORITY_THRESHOLD_HIGH = 10;
const int PRIORITY_THRESHOLD_LOW = 5;

// Vehicle direction (for lane assignment)
enum class Direction {
    LEFT,
    STRAIGHT,
    RIGHT
};

// Global atomic flag to control continuous generation
std::atomic<bool> keepRunning(true);

// Signal handler for clean shutdown
void signalHandler(int signum) {
    keepRunning = false;
    std::cout << "\nReceived termination signal. Stopping generator...\n";
}

// Set up colored console output
void setupConsole() {
#ifdef _WIN32
    // Enable ANSI escape codes on Windows
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
}

// Simple console log with color
void console_log(const std::string& message, const std::string& color = "\033[1;36m") {
    std::time_t now = std::time(nullptr);
    std::tm timeinfo;

#ifdef _WIN32
    localtime_s(&timeinfo, &now);
#else
    localtime_r(&now, &timeinfo);
#endif

    char timestamp[64];
    std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeinfo);

    std::cout << color << "[" << timestamp << "]\033[0m " << message << std::endl;
}

// Ensure data directories exist
void ensure_directories() {
    if (!fs::exists(DATA_DIR)) {
        fs::create_directories(DATA_DIR);
        console_log("Created directory: " + DATA_DIR);
    }
}

// Write a vehicle to lane file
void write_vehicle(const std::string& id, char lane, int laneNumber, Direction dir = Direction::LEFT) {
    static std::mutex fileMutex;
    std::lock_guard<std::mutex> lock(fileMutex);

    // Skip invalid lane numbers
    if (laneNumber < 1 || laneNumber > 3) {
        return;
    }

    std::string filepath = DATA_DIR + "/lane" + lane + ".txt";
    std::ofstream file(filepath, std::ios::app);

    if (file.is_open()) {
        // Format: vehicleId_L{laneNumber}:lane
        file << id << "_L" << laneNumber;

        // Add direction info based on lane and specific rules
        if (laneNumber == 3) {
            // Lane 3 always turns left
            file << "_LEFT";
        } else if (laneNumber == 2) {
            // Lane 2 can go straight or right
            if (dir == Direction::STRAIGHT) {
                file << "_STRAIGHT";
            } else {
                file << "_RIGHT";
            }
        } else if (laneNumber == 1) {
            // Lane 1 is incoming lane - no special handling needed
        }

        file << ":" << lane << std::endl;
        file.close();

        // Format log message with colors based on lane type
        std::string color = "\033[1;32m"; // Default green
        std::string dirStr = "";

        if (laneNumber == 3) {
            color = "\033[1;32m"; // Green for free lane
            dirStr = " (LEFT turn)";
        } else if (laneNumber == 2 && lane == 'A') {
            color = "\033[1;33m"; // Yellow for priority lane
            dirStr = (dir == Direction::STRAIGHT) ? " (STRAIGHT)" : " (RIGHT turn)";
        } else if (laneNumber == 2) {
            color = "\033[1;37m"; // White for normal lane 2
            dirStr = (dir == Direction::STRAIGHT) ? " (STRAIGHT)" : " (RIGHT turn)";
        } else {
            color = "\033[1;36m"; // Cyan for lane 1
        }

        console_log("Added " + id + " to lane " + lane + std::to_string(laneNumber) + dirStr, color);
    } else {
        console_log("ERROR: Could not open file " + filepath, "\033[1;31m");
    }
}

// Generate a random lane (A, B, C, D) - North, East, South, West
char random_lane() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 3);
    return 'A' + dist(gen);
}

// Generate a lane number with improved distribution (1, 2, or 3)
int random_lane_number() {
    static std::random_device rd;
    static std::mt19937 gen(rd());

    // Improved distribution: 20% lane 1, 50% lane 2, 30% lane 3
    std::vector<double> weights = {0.2, 0.5, 0.3};
    std::discrete_distribution<int> dist(weights.begin(), weights.end());

    return dist(gen) + 1; // Returns 1, 2, or 3
}

// Generate direction (straight, left, right) based on lane rules
Direction random_direction(int laneNumber) {
    static std::random_device rd;
    static std::mt19937 gen(rd());

    if (laneNumber == 3) {
        // Lane 3 always goes left
        return Direction::LEFT;
    } else if (laneNumber == 2) {
        // Lane 2 can go straight (60%) or right (40%)
        std::vector<double> weights = {0.0, 0.6, 0.4}; // [LEFT, STRAIGHT, RIGHT]
        std::discrete_distribution<int> dist(weights.begin(), weights.end());
        return static_cast<Direction>(dist(gen));
    } else {
        // Lane 1 is incoming lane
        return Direction::STRAIGHT;
    }
}

// Clear existing files
void clear_files() {
    for (char lane = 'A'; lane <= 'D'; lane++) {
        std::string filepath = DATA_DIR + "/lane" + lane + ".txt";
        std::ofstream file(filepath, std::ios::trunc);
        file.close();
        console_log("Cleared file: " + filepath);
    }
}

// Display status of current generation
void display_status(int current, int total, int a2_count) {
    const int barWidth = 40;
    float progress = static_cast<float>(current) / total;
    int pos = static_cast<int>(barWidth * progress);

    std::cout << "\r\033[1;33m[";
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }

    std::cout << "] " << int(progress * 100.0) << "% "
              << "Vehicles: " << current << "/" << total
              << " (A2: " << a2_count << ")\033[0m" << std::flush;
}

// Count current vehicles in each lane from files
std::map<std::string, int> count_vehicles_in_lanes() {
    std::map<std::string, int> counts;

    for (char lane = 'A'; lane <= 'D'; lane++) {
        std::string filepath = DATA_DIR + "/lane" + lane + ".txt";
        std::ifstream file(filepath);

        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                // Extract lane number
                size_t lanePos = line.find("_L");
                if (lanePos != std::string::npos && lanePos + 2 < line.length()) {
                    char laneNumChar = line[lanePos + 2];
                    if (laneNumChar >= '1' && laneNumChar <= '3') {
                        std::string laneKey = std::string(1, lane) + laneNumChar;
                        counts[laneKey]++;
                    }
                }
            }
            file.close();
        }
    }

    return counts;
}

// Display current lane statistics
void display_lane_stats() {
    auto counts = count_vehicles_in_lanes();

    std::cout << "\033[1;34m";
    std::cout << "┌────────────────────────────────────┐\n";
    std::cout << "│          Lane Statistics           │\n";
    std::cout << "├────────┬───────┬───────┬───────────┤\n";
    std::cout << "│  Road  │  L1   │  L2   │  L3(Free) │\n";
    std::cout << "├────────┼───────┼───────┼───────────┤\n";

    int total = 0;
    for (char lane = 'A'; lane <= 'D'; lane++) {
        std::string laneLabel;
        switch (lane) {
            case 'A': laneLabel = "A(North)"; break;
            case 'B': laneLabel = "B(East) "; break;
            case 'C': laneLabel = "C(South)"; break;
            case 'D': laneLabel = "D(West) "; break;
        }

        std::cout << "│ " << laneLabel << " │";

        for (int i = 1; i <= 3; i++) {
            std::string laneKey = std::string(1, lane) + std::to_string(i);
            int count = counts[laneKey];
            total += count;

            // Highlight A2 (priority lane) if over threshold
            if (lane == 'A' && i == 2 && count > PRIORITY_THRESHOLD_HIGH) {
                std::cout << " \033[1;33m" << std::setw(5) << count << "\033[1;34m │";
            } else {
                std::cout << " " << std::setw(5) << count << " │";
            }
        }
        std::cout << "\n";
    }

    std::cout << "├────────┴───────┴───────┴───────────┤\n";
    std::cout << "│ Total vehicles: " << std::setw(20) << total << " │\n";
    std::cout << "└────────────────────────────────────┘\033[0m\n";
}

int main() {
    try {
        // Set up signal handler for clean termination
        std::signal(SIGINT, signalHandler);

        // Set up console for colored output
        setupConsole();

        console_log("✅ Traffic generator starting", "\033[1;35m");

        // Create directories and clear files
        ensure_directories();
        clear_files();

        // Random generators
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> delay_dist(0.7, 1.3); // For randomized intervals

        // Global tracking variables
        int total_vehicles = 0;
        int a2_count = 0;
        int current_batch = 0;

        // First generate A2 priority lane vehicles
        console_log("🚦 Generating priority lane vehicles (A2)", "\033[1;33m");
        for (int i = 0; i < 12 && keepRunning; i++) {
            std::string id = "V" + std::to_string(total_vehicles + 1);

            // Alternate between straight and right turns for lane A2
            Direction dir = (i % 2 == 0) ? Direction::STRAIGHT : Direction::RIGHT;
            write_vehicle(id, 'A', 2, dir); // Lane A2 with direction

            total_vehicles++;
            a2_count++;
            current_batch++;

            // Display progress
            display_status(current_batch, MAX_VEHICLES_PER_BATCH, a2_count);

            // Wait between vehicles with slight randomization
            std::this_thread::sleep_for(
                std::chrono::milliseconds(
                    static_cast<int>(GENERATION_INTERVAL_MS * delay_dist(gen))
                )
            );
        }

        std::cout << std::endl;
        console_log("🚗 Generating continuous traffic flow", "\033[1;34m");

        // Display starting lane stats
        display_lane_stats();

        // Stats display timer
        auto lastStatsTime = std::chrono::steady_clock::now();
        bool in_priority_mode = false;

        // Continuous generation until terminated
        while (keepRunning) {
            char lane = random_lane();
            int lane_num = random_lane_number();
            Direction dir = random_direction(lane_num);

            // For testing priority condition, occasionally bias toward lane A2
            if (gen() % 10 == 0) {
                lane = 'A';
                lane_num = 2;
                dir = (gen() % 2 == 0) ? Direction::STRAIGHT : Direction::RIGHT;
            }

            // Also ensure good distribution for lane 3 (free lane)
            if (gen() % 15 == 0) {
                lane = random_lane();
                lane_num = 3; // Force lane 3 (free lane)
                dir = Direction::LEFT; // Lane 3 always turns left
            }

            std::string id = "V" + std::to_string(total_vehicles + 1);

            // Write vehicle to file with appropriate direction
            write_vehicle(id, lane, lane_num, dir);

            // Update counters
            total_vehicles++;
            current_batch++;
            if (lane == 'A' && lane_num == 2) {
                a2_count++;
            }

            // Display progress
            display_status(current_batch, MAX_VEHICLES_PER_BATCH, a2_count);

            // Periodically display lane stats (every 5 seconds)
            auto currentTime = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(currentTime - lastStatsTime).count() >= 5) {
                std::cout << std::endl;
                display_lane_stats();
                lastStatsTime = currentTime;
            }

            // Reset batch counter when it reaches max
            if (current_batch >= MAX_VEHICLES_PER_BATCH) {
                current_batch = 0;
                std::cout << std::endl;
                console_log("♻️ New batch starting", "\033[1;34m");
                display_lane_stats();
            }

            // Check priority lane count and log state changes
            auto counts = count_vehicles_in_lanes();
            int a2_count_current = counts["A2"];

            if (!in_priority_mode && a2_count_current > PRIORITY_THRESHOLD_HIGH) {
                in_priority_mode = true;
                console_log("⚠️ Priority mode activated (A2: " + std::to_string(a2_count_current) +
                          " vehicles)", "\033[1;31m");
            } else if (in_priority_mode && a2_count_current < PRIORITY_THRESHOLD_LOW) {
                in_priority_mode = false;
                console_log("✅ Priority mode deactivated (A2: " + std::to_string(a2_count_current) +
                          " vehicles)", "\033[1;32m");
            }

            // Wait between vehicles with slight randomization
            std::this_thread::sleep_for(
                std::chrono::milliseconds(
                    static_cast<int>(GENERATION_INTERVAL_MS * delay_dist(gen))
                )
            );
        }

        std::cout << std::endl;
        console_log("✅ Traffic generator completed. Generated " +
                   std::to_string(total_vehicles) + " vehicles.", "\033[1;35m");

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "\033[1;31mError: " << e.what() << "\033[0m" << std::endl;
        return 1;
    }
}
