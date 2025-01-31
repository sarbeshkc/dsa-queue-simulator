#include "traffic_generator.h"

TrafficGenerator::TrafficGenerator(SDL_Renderer* renderer)
    : m_renderer(renderer)
    , m_rng(std::random_device{}())
{}

Vehicle* TrafficGenerator::generateVehicle() {
    // Pick a random spawn point (0-3 for NORTH, SOUTH, EAST, WEST)
    int spawnPoint = m_rng() % 4;

    Vector2D startPos;
    Direction facing;
    LaneId lane;

    switch(spawnPoint) {
        case 0: // NORTH spawn point
            startPos = Vector2D(640, -20);
            facing = Direction::SOUTH;
            lane = LaneId::AL1_INCOMING;
            break;
        case 1: // SOUTH spawn point
            startPos = Vector2D(640, 740);
            facing = Direction::NORTH;
            lane = LaneId::CL1_INCOMING;
            break;
        case 2: // EAST spawn point
            startPos = Vector2D(1300, 360);
            facing = Direction::WEST;
            lane = LaneId::BL1_INCOMING;
            break;
        case 3: // WEST spawn point
            startPos = Vector2D(-20, 360);
            facing = Direction::EAST;
            lane = LaneId::DL1_INCOMING;
            break;
    }

    return new Vehicle(m_renderer, m_vehicleCount++, lane, startPos, facing);
}
