// src/traffic/vehicle.h
#ifndef VEHICLE_H
#define VEHICLE_H

#include <SDL3/SDL.h>
#include "../utils/math_utils.h"
#include "../common/types.h"

class Vehicle {
public:
    Vehicle(SDL_Renderer* renderer, int id, LaneId currentLane);  // Declaration only

    // Getters
    int getId() const { return m_id; }
    LaneId getCurrentLaneId() const { return m_currentLane; }
    Vector2D getPosition() const { return m_position; }
    bool isPriority() const { return m_isPriority; }

    // Setters
    void setPosition(float x, float y) { m_position.x = x; m_position.y = y; }
    void setTargetLane(LaneId lane) { m_targetLane = lane; }
    void setPriority(bool priority) { m_isPriority = priority; }

    void render() const;  // Declaration only

    float getX() const { return m_position.x; }
    float getY() const { return m_position.y; }
    LaneId getLane() const { return m_currentLane; }

    // Constants
    static constexpr float VEHICLE_WIDTH = 20.0f;
    static constexpr float VEHICLE_LENGTH = 40.0f;

private:
    SDL_Renderer* m_renderer;
    int m_id;
    LaneId m_currentLane;
    LaneId m_targetLane;
    Vector2D m_position;
    bool m_isPriority;
};

#endif // VEHICLE_H
