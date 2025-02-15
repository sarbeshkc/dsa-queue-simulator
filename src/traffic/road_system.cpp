// src/traffic/road_system.cpp
#include "road_system.h"
#include <cmath>
#include "../utils/math_utils.h"

RoadSystem::RoadSystem(SDL_Renderer* renderer)
    : m_renderer(renderer) {
    initializeLanes();
    initializeTrafficLights();
}

void RoadSystem::initializeLanes() {
    // Constants for positioning
    const float CENTER_X = 640.0f;  // Screen center X
    const float CENTER_Y = 360.0f;  // Screen center Y
    const float LANE_WIDTH = ROAD_WIDTH / 3.0f;

    // Initialize all lanes with their positions
    for (int i = 0; i < 12; ++i) {
        LaneId id = static_cast<LaneId>(i);
        Vector2D start = calculateLaneStart(id);
        Vector2D end = calculateLaneEnd(id);
        bool isPriority = (id == LaneId::AL2_PRIORITY ||
                         id == LaneId::BL2_PRIORITY ||
                         id == LaneId::CL2_PRIORITY ||
                         id == LaneId::DL2_PRIORITY);

        m_lanes[id] = std::make_unique<Lane>(m_renderer, id, isPriority);
    }
}

// src/traffic/road_system.cpp
void RoadSystem::initializeTrafficLights() {
    const float CENTER_X = 640.0f;
    const float CENTER_Y = 360.0f;
    const float OFFSET = 90.0f;

    // Create traffic lights at intersection corners
    m_trafficLights.push_back(std::make_unique<TrafficLight>(
        m_renderer, CENTER_X - OFFSET, CENTER_Y - OFFSET));
    m_trafficLights.push_back(std::make_unique<TrafficLight>(
        m_renderer, CENTER_X + OFFSET, CENTER_Y - OFFSET));
    m_trafficLights.push_back(std::make_unique<TrafficLight>(
        m_renderer, CENTER_X + OFFSET, CENTER_Y + OFFSET));
    m_trafficLights.push_back(std::make_unique<TrafficLight>(
        m_renderer, CENTER_X - OFFSET, CENTER_Y + OFFSET));
}

Vector2D RoadSystem::calculateLaneStart(LaneId id) const {
    const float CENTER_X = 640.0f;
    const float CENTER_Y = 360.0f;
    const float LANE_WIDTH = ROAD_WIDTH / 3.0f;
    const float HALF_ROAD = ROAD_WIDTH / 2.0f;

    // Calculate start position based on lane ID
    switch (id) {
        case LaneId::AL1_INCOMING:
            return Vector2D(CENTER_X - HALF_ROAD + LANE_WIDTH/2, 0);
        case LaneId::AL2_PRIORITY:
            return Vector2D(CENTER_X - HALF_ROAD + LANE_WIDTH*1.5f, 0);
        case LaneId::AL3_FREELANE:
            return Vector2D(CENTER_X - HALF_ROAD + LANE_WIDTH*2.5f, 0);
        // Add other cases for remaining lanes...
        default:
            return Vector2D(0, 0);
    }
}

Vector2D RoadSystem::calculateLaneEnd(LaneId id) const {
    const float CENTER_X = 640.0f;
    const float CENTER_Y = 360.0f;
    const float LANE_WIDTH = ROAD_WIDTH / 3.0f;
    const float HALF_ROAD = ROAD_WIDTH / 2.0f;

    // Calculate end position based on lane ID
    switch (id) {
        case LaneId::AL1_INCOMING:
            return Vector2D(CENTER_X - HALF_ROAD + LANE_WIDTH/2, CENTER_Y - HALF_ROAD);
        case LaneId::AL2_PRIORITY:
            return Vector2D(CENTER_X - HALF_ROAD + LANE_WIDTH*1.5f, CENTER_Y - HALF_ROAD);
        case LaneId::AL3_FREELANE:
            return Vector2D(CENTER_X - HALF_ROAD + LANE_WIDTH*2.5f, CENTER_Y - HALF_ROAD);
        // Add other cases for remaining lanes...
        default:
            return Vector2D(0, 0);
    }
}

void RoadSystem::update(float deltaTime) {
    // Update all lanes
    for (auto& [id, lane] : m_lanes) {
        lane->update(deltaTime);
    }

    // Update traffic lights
    for (auto& light : m_trafficLights) {
        light->update(deltaTime);
    }
}

void RoadSystem::render() const {
    // Render road background
    // Draw basic road structure
    SDL_SetRenderDrawColor(m_renderer, 50, 50, 50, 255);

    // Draw horizontal and vertical roads
    SDL_FRect horizontalRoad = {
        0, 360.0f - ROAD_WIDTH/2,
        1280.0f, ROAD_WIDTH
    };
    SDL_FRect verticalRoad = {
        640.0f - ROAD_WIDTH/2, 0,
        ROAD_WIDTH, 720.0f
    };

    SDL_RenderFillRect(m_renderer, &horizontalRoad);
    SDL_RenderFillRect(m_renderer, &verticalRoad);

    // Render all lanes
    for (const auto& [id, lane] : m_lanes) {
        lane->render();
    }

    // Render traffic lights
    for (const auto& light : m_trafficLights) {
        light->render();
    }
}
