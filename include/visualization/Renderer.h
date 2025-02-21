#pragma once

#define _USE_MATH_DEFINES  // Required for M_PI constant

// Core SDL includes
#include <SDL3/SDL.h>

// Project includes
#include "managers/TrafficManager.h"
#include "visualization/DebugOverlay.h"
#include "core/Constants.h"

// Standard library includes
#include <memory>
#include <map>
#include <cmath>

/**
 * @class Renderer
 * @brief Handles visualization for the traffic simulation system
 */
class Renderer {
private:
    // Core SDL components
    SDL_Window* window;
    SDL_Renderer* renderer;

    // Debug components
    DebugOverlay debugOverlay;
    bool debugMode;
    bool showGrid;

    // Rendering constants
    static constexpr float VEHICLE_WIDTH = 40.0f;
    static constexpr float VEHICLE_HEIGHT = 30.0f;
    static constexpr float LIGHT_SIZE = 20.0f;
    static constexpr float ARROW_SIZE = 30.0f;
    static constexpr float DASH_LENGTH = 20.0f;
    static constexpr float GAP_LENGTH = 20.0f;
    static constexpr float HOUSING_PADDING = 5.0f;

    // Environment rendering methods
    void renderBackground();
    void renderGrassAreas();
    void renderRoads();
    void renderRoadEdges();
    void renderLanes();
    void renderIntersection();
    void renderCrosswalks();
    void renderStopLines();

    // Traffic elements rendering
    void renderDirectionalArrows();
    void renderTrafficLights(const std::map<LaneId, TrafficLight>& lights);

    void renderTrafficLight(float x, float y, float rotation, LightState state);
    void renderVehicles(const std::map<uint32_t, VehicleState>& vehicles);
    void renderVehicle(float x, float y, Direction dir, bool isPriority, float angle, bool isMoving);
    void renderPriorityLane();
    void renderPriorityLaneIndicator();
    void renderTurningGuides();

    // Debug visualization
    void renderLaneIdentifiers();
    void renderVehicleCount(const TrafficManager& trafficManager);
    void drawDebugGrid();

    // Helper methods
    void drawArrow(float x, float y, float angle, Direction dir);
    void renderCircle(float x, float y, float radius);
    void renderDashedLine(float x1, float y1, float x2, float y2);
    void renderRoundedRect(float x, float y, float w, float h, float radius);
    SDL_FPoint rotatePoint(float x, float y, float cx, float cy, float angle);
    float calculateTurningAngle(const VehicleState& state) const;
    SDL_Color getLaneColor(LaneId laneId, bool isActive) const;

public:
    // Constructor and destructor
    Renderer();
    ~Renderer();

    // Core methods
    bool initialize();
    void render(const TrafficManager& trafficManager);
    void cleanup();
    bool isInitialized() const { return window != nullptr && renderer != nullptr; }

    // Configuration methods
    void setDebugMode(bool enabled) { debugMode = enabled; }
    void toggleGridDisplay() { showGrid = !showGrid; }
    void updateWindowSize(int width, int height);
};
