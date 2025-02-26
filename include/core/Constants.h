#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <string>
#include <SDL3/SDL.h>

namespace Constants {
    // Window settings
    constexpr int WINDOW_WIDTH = 800;
    constexpr int WINDOW_HEIGHT = 800;
    const std::string WINDOW_TITLE = "Traffic Junction Simulator";
    constexpr float SCALE = 1.1f;

    // Road settings
    constexpr int ROAD_WIDTH = 150;
    constexpr int LANE_WIDTH = 50;
    constexpr int ARROW_SIZE = 15;

    // Vehicle settings
    constexpr int MAX_VEHICLE_ID = 20;
    constexpr float VEHICLE_LENGTH = 20.0f;
    constexpr float VEHICLE_WIDTH = 10.0f;
    constexpr float VEHICLE_GAP = 15.0f;
    constexpr float TURN_DURATION = 1500.0f;
    constexpr float BEZIER_CONTROL_OFFSET = 80.0f;
    constexpr float TURN_SPEED = 0.0008f;
    constexpr float MOVE_SPEED = 0.2f;

    // Traffic light settings
    constexpr int ALL_RED_DURATION = 2000; // 2 seconds
    constexpr int GREEN_DURATION = 3000;   // 3 seconds

    // Queue settings
    constexpr int MAX_QUEUE_SIZE = 100;

    // Priority settings
    constexpr int PRIORITY_THRESHOLD_HIGH = 10; // Enter priority mode when > 10 vehicles
    constexpr int PRIORITY_THRESHOLD_LOW = 5;   // Exit priority mode when < 5 vehicles

    // File paths
    const std::string DATA_PATH = "data/lanes";
    const std::string LOG_FILE = "traffic_simulator.log";
    const std::string VEHICLE_FILE = "vehicles.data";
    const std::string FONT_PATH = "/usr/share/fonts/TTF/DejaVuSans.ttf";
    const std::string CAR_TEXTURE_PATH = "assets/car.jpg";

    // Colors
    constexpr SDL_Color ROAD_COLOR = {50, 50, 50, 255};
    constexpr SDL_Color LANE_MARKER_COLOR = {255, 255, 255, 255};
    constexpr SDL_Color RED_LIGHT_COLOR = {255, 0, 0, 255};
    constexpr SDL_Color GREEN_LIGHT_COLOR = {11, 156, 50, 255};
    constexpr SDL_Color NORMAL_VEHICLE_COLOR = {0, 0, 255, 255};
    constexpr SDL_Color EMERGENCY_VEHICLE_COLOR = {255, 0, 0, 255};
    constexpr SDL_Color PRIORITY_INDICATOR_COLOR = {255, 165, 0, 255};
    constexpr SDL_Color TEXT_COLOR = {0, 0, 0, 255};
    constexpr SDL_Color DEBUG_TEXT_COLOR = {255, 255, 255, 255};
    constexpr SDL_Color DEBUG_BACKGROUND_COLOR = {0, 0, 0, 128};
}

#endif // CONSTANTS_H
