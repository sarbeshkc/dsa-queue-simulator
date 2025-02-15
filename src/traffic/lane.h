// src/traffic/lane.h
#ifndef LANE_H
#define LANE_H

#include <SDL3/SDL.h>
#include "../common/types.h"
#include "vehicle.h"
#include <queue>

class Lane {
public:
    // Constructor that matches how it's being called in the code
    Lane(SDL_Renderer* renderer, LaneId id, Vector2D start = Vector2D(),
         Vector2D end = Vector2D(), bool isPriority = false);

    // Core functionality
    void update(float deltaTime);
    void render() const;

    // Vehicle management
    void addVehicle(Vehicle* vehicle);
    Vehicle* processNextVehicle();
    int getQueueLength() const;
    bool isPriorityLane() const;

private:
    SDL_Renderer* m_renderer;
    LaneId m_id;
    Vector2D m_startPos;
    Vector2D m_endPos;
    bool m_isPriority;
    float m_processingTimer;
    std::queue<Vehicle*> m_queue;

    static constexpr float PROCESS_TIME = 2.0f;
};

#endif // LANE_H
