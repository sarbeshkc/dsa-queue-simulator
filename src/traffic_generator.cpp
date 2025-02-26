#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <random>
#include <chrono>
#include <thread>
#include <filesystem>
#include <ctime>
#include <iomanip>
#include<sstream>

// Namespaces
namespace fs = std::filesystem;

// Constants
const std::string DATA_DIR = "data/lanes";
const int GENERATION_INTERVAL_MS = 1000;
const int MAX_VEHICLES = 30;

// Vehicle structure
struct VehicleInfo {
    std::string id;
    char sourceLane;        // A, B, C, D
    int sourceLaneNumber;   // 1, 2, 3
    bool isEmergency;
};

// Simple console log with timestamp
void console_log(const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << "[" << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << "] "
       << message;

    std::cout << ss.str() << std::endl;
}

// Function to ensure the data directory exists
void ensure_directories() {
    try {
        if (!fs::exists(DATA_DIR)) {
            fs::create_directories(DATA_DIR);
            console_log("Created directory: " + DATA_DIR);
        }
    } catch (const std::exception& e) {
        console_log("Error creating directory: " + std::string(e.what()));
    }
}

// Write a vehicle to lane file with format: VehicleID:LaneID
void write_vehicle(const VehicleInfo& vehicle) {
    std::string filepath = DATA_DIR + "/lane" + vehicle.sourceLane + ".txt";
    std::ofstream file(filepath, std::ios::app);

    if (file.is_open()) {
        // Format: VehicleID:LaneID (the simulation will extract lane number from ID)
        file << vehicle.id << ":" << vehicle.sourceLane << std::endl;
        file.close();

        std::string laneStr = std::string(1, vehicle.sourceLane) + std::to_string(vehicle.sourceLaneNumber);
        console_log("Added " + vehicle.id + " to lane " + laneStr);
    } else {
        console_log("Failed to open file: " + filepath);
    }
}

// Generate a random lane (A, B, C, D)
char random_lane() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 3);
    return 'A' + dist(gen);
}

// Generate a random lane number (1, 2, 3)
int random_lane_number() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(1, 3);
    return dist(gen);
}

// Clear existing files to ensure a clean start
void clear_files() {
    console_log("Clearing lane files for a fresh start");

    for (char lane = 'A'; lane <= 'D'; lane++) {
        std::string filepath = DATA_DIR + "/lane" + lane + ".txt";
        try {
            std::ofstream file(filepath, std::ios::trunc);
            if (file.is_open()) {
                file.close();
                console_log("Cleared file: " + filepath);
            } else {
                console_log("Failed to open file for clearing: " + filepath);
            }
        } catch (const std::exception& e) {
            console_log("Error clearing file: " + std::string(e.what()));
        }
    }
}

// Create a new vehicle with unique ID and lane info
VehicleInfo create_vehicle(int id, char lane = '\0', int laneNumber = 0, bool isEmergency = false) {
    VehicleInfo vehicle;

    // Use provided lane/number or generate random ones
    vehicle.sourceLane = (lane != '\0') ? lane : random_lane();
    vehicle.sourceLaneNumber = (laneNumber > 0) ? laneNumber : random_lane_number();
    vehicle.isEmergency = isEmergency;

    // Create ID with lane number embedded (matching the expected format in the simulator)
    if (isEmergency) {
        vehicle.id = "EMG" + std::to_string(id) + "_L" + std::to_string(vehicle.sourceLaneNumber);
    } else {
        vehicle.id = "V" + std::to_string(id) + "_L" + std::to_string(vehicle.sourceLaneNumber);
    }

    return vehicle;
}

// Create a priority lane vehicle (specifically for A2)
VehicleInfo create_priority_vehicle(int id) {
    return create_vehicle(id, 'A', 2);
}

// Create a free lane vehicle (lane 3 for all roads)
VehicleInfo create_free_lane_vehicle(int id, char lane) {
    return create_vehicle(id, lane, 3);
}

// Create an emergency vehicle (with EMG prefix)
VehicleInfo create_emergency_vehicle(int id) {
    char lane = random_lane();
    int laneNumber = random_lane_number();
    return create_vehicle(id, lane, laneNumber, true);
}

// Main function
int main() {
    try {
        console_log("Traffic generator starting");

        // Create directories and clear files
        ensure_directories();
        clear_files();

        // Counter for generated vehicles
        int vehicle_count = 0;

        // Generate vehicles for priority lane A2
        console_log("Generating vehicles for lane A2 (priority lane)");
        for (int i = 0; i < 12 && vehicle_count < MAX_VEHICLES; i++) {
            VehicleInfo vehicle = create_priority_vehicle(vehicle_count + 1);
            write_vehicle(vehicle);
            vehicle_count++;

            // Wait between vehicles to avoid file contention
            std::this_thread::sleep_for(std::chrono::milliseconds(GENERATION_INTERVAL_MS));
        }

        // Generate vehicles for free lanes (lane 3)
        console_log("Generating vehicles for free lanes (lane 3)");
        for (char lane = 'A'; lane <= 'D' && vehicle_count < MAX_VEHICLES; lane++) {
            VehicleInfo vehicle = create_free_lane_vehicle(vehicle_count + 1, lane);
            write_vehicle(vehicle);
            vehicle_count++;

            // Wait between vehicles
            std::this_thread::sleep_for(std::chrono::milliseconds(GENERATION_INTERVAL_MS));
        }

        // Generate a couple of emergency vehicles
        if (vehicle_count < MAX_VEHICLES - 2) {
            console_log("Generating emergency vehicles");
            for (int i = 0; i < 2 && vehicle_count < MAX_VEHICLES; i++) {
                VehicleInfo vehicle = create_emergency_vehicle(vehicle_count + 1);
                write_vehicle(vehicle);
                vehicle_count++;

                std::this_thread::sleep_for(std::chrono::milliseconds(GENERATION_INTERVAL_MS));
            }
        }

        // Generate remaining vehicles randomly
        console_log("Generating random vehicles");
        while (vehicle_count < MAX_VEHICLES) {
            VehicleInfo vehicle = create_vehicle(vehicle_count + 1);
            write_vehicle(vehicle);
            vehicle_count++;

            // Wait between vehicles
            std::this_thread::sleep_for(std::chrono::milliseconds(GENERATION_INTERVAL_MS));
        }

        console_log("Generated " + std::to_string(vehicle_count) + " vehicles");
        console_log("Traffic generator completed");

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
