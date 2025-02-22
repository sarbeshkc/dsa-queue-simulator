// Constants.h
#pragma once
#include <cstdint>

enum class LaneId {
    AL1_INCOMING,
    AL2_PRIORITY,
    AL3_FREELANE,
    BL1_INCOMING,
    BL2_NORMAL,
    BL3_FREELANE,
    CL1_INCOMING,
    CL2_NORMAL,
    CL3_FREELANE,
    DL1_INCOMING,
    DL2_NORMAL,
    DL3_FREELANE
};

enum class Direction {
    STRAIGHT,
    LEFT,
    RIGHT
};

enum class LightState {
    RED,
    GREEN
};

namespace SimConstants {
    // Window and Display
    static constexpr int WINDOW_WIDTH = 1280;
    static constexpr int WINDOW_HEIGHT = 960;
    static constexpr int CENTER_X = WINDOW_WIDTH / 2;
    static constexpr int CENTER_Y = WINDOW_HEIGHT / 2;

    // Road and Lane Configuration
    static constexpr int ROAD_WIDTH = 360;     // Width for 3 lanes
    static constexpr int LANE_WIDTH = 120;     // Individual lane width
    static constexpr float QUEUE_SPACING = 60.0f;
    static constexpr float QUEUE_START_OFFSET = 200.0f;

    // Vehicle Configuration
    static constexpr float VEHICLE_WIDTH = 40.0f;
    static constexpr float VEHICLE_HEIGHT = 30.0f;
    static constexpr float VEHICLE_BASE_SPEED = 50.0f;
    static constexpr float VEHICLE_TURN_SPEED = 30.0f;

    // Traffic Light Configuration
    static constexpr float LIGHT_SIZE = 40.0f;
    static constexpr float LIGHT_SPACING = 60.0f;

    // Intersection Configuration
    static constexpr float INTERSECTION_RADIUS = 180.0f;
    static constexpr float TURN_GUIDE_RADIUS = 150.0f;

    // System Timing
    static constexpr int FILE_CHECK_INTERVAL = 100;    // ms
    static constexpr int TRAFFIC_UPDATE_INTERVAL = 50; // ms
    static constexpr float VEHICLE_PROCESS_TIME = 3.0f; // seconds

// Add these constants to the class
static constexpr float UPDATE_INTERVAL = 0.016f;         // ~60 FPS
};
