// src/traffic/vehicle.cpp
#include "vehicle.h"

Vehicle::Vehicle(SDL_Renderer* renderer, int id, LaneId currentLane)
    : m_renderer(renderer)
    , m_id(id)
    , m_currentLane(currentLane)
    , m_targetLane(currentLane)
    , m_position(0.0f, 0.0f)
    , m_isPriority(false) {
}

void Vehicle::render() const {
    SDL_FRect rect = {
        m_position.x - VEHICLE_WIDTH/2,
        m_position.y - VEHICLE_LENGTH/2,
        VEHICLE_WIDTH,
        VEHICLE_LENGTH
    };

    SDL_SetRenderDrawColor(m_renderer,
        m_isPriority ? 255 : 0,  // Red for priority
        m_isPriority ? 0 : 255,  // Green for normal
        0, 255);

    SDL_RenderFillRect(m_renderer, &rect);
}
