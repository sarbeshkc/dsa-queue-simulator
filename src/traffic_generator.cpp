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

// Vehicle direction (for lane 3)
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

    // Skip invalid lane combinations
    // Only allow lanes 2 and 3 - lane 1 handled automatically by the vehicle itself
    if (laneNumber == 1) {
        return;
    }

    std::string filepath = DATA_DIR + "/lane" + lane + ".txt";
    std::ofstream file(filepath, std::ios::app);

    if (file.is_open()) {
        // Format: vehicleId_L{laneNumber}:lane
        file << id << "_L" << laneNumber;

        // Add direction info for lane 3 (free lane)
        if (laneNumber == 3) {
            switch (dir) {
                case Direction::LEFT: file << "_LEFT"; break;
                case Direction::RIGHT: file << "_RIGHT"; break;
                default: file << "_STRAIGHT"; break;
            }
        }

        file << ":" << lane << std::endl;
        file.close();

        console_log("Added " + id + " to lane " + lane + std::to_string(laneNumber), "\033[1;32m");
    } else {
        console_log("ERROR: Could not open file " + filepath, "\033[1;31m");
    }
}

// Generate a random lane (A, B, C, D)
char random_lane() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 3);
    return 'A' + dist(gen);
}

// Generate a lane number (2 or 3)
int random_lane_number() {
    static std::random_device rd;
    static std::mt19937 gen(rd());

    // Only lanes 2 and 3 (lane 1 is automatically handled by the vehicle)
    std::vector<double> weights = {0.7, 0.3}; // 70% lane 2, 30% lane 3
    std::discrete_distribution<int> dist(weights.begin(), weights.end());

    return dist(gen) + 2; // Returns 2 or 3
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
            write_vehicle(id, 'A', 2); // Always lane A2
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

        // Continuous generation until terminated
        while (keepRunning) {
            char lane = random_lane();
            int lane_num = random_lane_number(); // Only returns 2 or 3

            // For testing, occasionally bias toward lane A2
            if (gen() % 20 == 0) {
                lane = 'A';
                lane_num = 2;
            }

            std::string id = "V" + std::to_string(total_vehicles + 1);

            // For lane 3, always turn left per assignment
            if (lane_num == 3) {
                write_vehicle(id, lane, lane_num, Direction::LEFT);
            } else {
                write_vehicle(id, lane, lane_num);
            }

            // Update counters
            total_vehicles++;
            current_batch++;
            if (lane == 'A' && lane_num == 2) {
                a2_count++;
            }

            // Display progress
            display_status(current_batch, MAX_VEHICLES_PER_BATCH, a2_count);

            // Reset batch counter when it reaches max
            if (current_batch >= MAX_VEHICLES_PER_BATCH) {
                current_batch = 0;
                std::cout << std::endl;
                console_log("♻️ New batch starting", "\033[1;34m");
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
