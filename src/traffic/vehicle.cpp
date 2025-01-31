#include "vehicle.h"
#include <SDL3/SDL.h>

Vehicle::Vehicle(SDL_Renderer* renderer, int vehicle_id, LaneId startLane,
                Vector2D startPos, Direction facing)
    : m_renderer(renderer)
    , m_id(vehicle_id)
    , m_position(startPos)
    , m_velocity(0.0F, 0.0F)
    , m_status(VehicleStatus::MOVING)
    , m_currentLane(startLane)
    , m_targetLane(startLane)
    , m_facing(facing)
    , m_turnIntent(TurnBehaviour::STRAIGHT)
    , m_waitTime(0.0F)
    , m_isPriority(false) {
    checkPriority();
}

Vehicle::~Vehicle() = default;

void Vehicle::render() const {
    float x = m_position.x - 10;
    float y = m_position.y - 20;
    float w = 20;
    float h = 40;

    SDL_Vertex vertices[6];

    // Set colors based on priority
    Uint8 r = m_isPriority ? 255 : 0;
    Uint8 g = m_isPriority ? 0 : 255;
    Uint8 b = 0;
    Uint8 a = 255;

    // Define vertices for vehicle body (two triangles forming a rectangle)
    vertices[0].position.x = x;
    vertices[0].position.y = y;
    vertices[0].color.r = r;
    vertices[0].color.g = g;
    vertices[0].color.b = b;
    vertices[0].color.a = a;

    vertices[1].position.x = x + w;
    vertices[1].position.y = y;
    vertices[1].color.r = r;
    vertices[1].color.g = g;
    vertices[1].color.b = b;
    vertices[1].color.a = a;

    vertices[2].position.x = x;
    vertices[2].position.y = y + h;
    vertices[2].color.r = r;
    vertices[2].color.g = g;
    vertices[2].color.b = b;
    vertices[2].color.a = a;

    vertices[3].position.x = x + w;
    vertices[3].position.y = y;
    vertices[3].color.r = r;
    vertices[3].color.g = g;
    vertices[3].color.b = b;
    vertices[3].color.a = a;

    vertices[4].position.x = x + w;
    vertices[4].position.y = y + h;
    vertices[4].color.r = r;
    vertices[4].color.g = g;
    vertices[4].color.b = b;
    vertices[4].color.a = a;

    vertices[5].position.x = x;
    vertices[5].position.y = y + h;
    vertices[5].color.r = r;
    vertices[5].color.g = g;
    vertices[5].color.b = b;
    vertices[5].color.a = a;

    SDL_RenderGeometry(m_renderer, nullptr, vertices, 6, nullptr, 0);

    // Direction indicator
    SDL_Vertex directionIndicator[3];

    // Yellow color for direction indicator
    Uint8 indicator_r = 255;
    Uint8 indicator_g = 255;
    Uint8 indicator_b = 0;
    Uint8 indicator_a = 255;

    switch(m_facing) {
        case Direction::NORTH:
            directionIndicator[0].position.x = m_position.x;
            directionIndicator[0].position.y = m_position.y - 25;
            directionIndicator[1].position.x = m_position.x - 5;
            directionIndicator[1].position.y = m_position.y - 15;
            directionIndicator[2].position.x = m_position.x + 5;
            directionIndicator[2].position.y = m_position.y - 15;
            break;
        case Direction::SOUTH:
            directionIndicator[0].position.x = m_position.x;
            directionIndicator[0].position.y = m_position.y + 25;
            directionIndicator[1].position.x = m_position.x - 5;
            directionIndicator[1].position.y = m_position.y + 15;
            directionIndicator[2].position.x = m_position.x + 5;
            directionIndicator[2].position.y = m_position.y + 15;
            break;
        case Direction::EAST:
            directionIndicator[0].position.x = m_position.x + 15;
            directionIndicator[0].position.y = m_position.y;
            directionIndicator[1].position.x = m_position.x + 5;
            directionIndicator[1].position.y = m_position.y - 5;
            directionIndicator[2].position.x = m_position.x + 5;
            directionIndicator[2].position.y = m_position.y + 5;
            break;
        case Direction::WEST:
            directionIndicator[0].position.x = m_position.x - 15;
            directionIndicator[0].position.y = m_position.y;
            directionIndicator[1].position.x = m_position.x - 5;
            directionIndicator[1].position.y = m_position.y - 5;
            directionIndicator[2].position.x = m_position.x - 5;
            directionIndicator[2].position.y = m_position.y + 5;
            break;
    }

    // Set color for all direction indicator vertices
    for (int i = 0; i < 3; i++) {
        directionIndicator[i].color.r = indicator_r;
        directionIndicator[i].color.g = indicator_g;
        directionIndicator[i].color.b = indicator_b;
        directionIndicator[i].color.a = indicator_a;
    }

    SDL_RenderGeometry(m_renderer, nullptr, directionIndicator, 3, nullptr, 0);
}

void Vehicle::update(float deltaTime) {
    updateState();
    updatePosition(deltaTime);

    if (m_status == VehicleStatus::WAITING) {
        m_waitTime += deltaTime;
    }
}

void Vehicle::updatePosition(float deltaTime) {
    float speed = 100.0F;

    if (m_status == VehicleStatus::MOVING || m_status == VehicleStatus::TURNING) {
        switch (m_facing) {
            case Direction::NORTH:
                m_velocity = Vector2D(0.0F, -speed);
                break;
            case Direction::SOUTH:
                m_velocity = Vector2D(0.0F, speed);
                break;
            case Direction::EAST:
                m_velocity = Vector2D(speed, 0.0F);
                break;
            case Direction::WEST:
                m_velocity = Vector2D(-speed, 0.0F);
                break;
        }

        m_position.x += m_velocity.x * deltaTime;
        m_position.y += m_velocity.y * deltaTime;
    }
}

void Vehicle::updateState() {
    if (checkCollision()) {
        m_status = VehicleStatus::STOPPED;
    } else if (m_status == VehicleStatus::STOPPED) {
        m_status = VehicleStatus::MOVING;
    }

    if (m_status == VehicleStatus::TURNING && m_currentLane != m_targetLane) {
        m_currentLane = m_targetLane;
        m_status = VehicleStatus::MOVING;
    }
}

bool Vehicle::checkCollision() const {
    const float margin = 50.0F;
    return (m_position.x < -margin || m_position.x > 1280 + margin ||
            m_position.y < -margin || m_position.y > 720 + margin);
}

void Vehicle::changeLane(LaneId newLane) {
    if (m_currentLane != newLane) {
        m_targetLane = newLane;
        m_status = VehicleStatus::TURNING;
    }
}

void Vehicle::setTurnDirection(TurnBehaviour turn) {
    m_turnIntent = turn;
    m_status = VehicleStatus::TURNING;

    switch(turn) {
        case TurnBehaviour::TURNING_LEFT:
            switch(m_facing) {
                case Direction::NORTH: m_facing = Direction::WEST; break;
                case Direction::SOUTH: m_facing = Direction::EAST; break;
                case Direction::EAST: m_facing = Direction::NORTH; break;
                case Direction::WEST: m_facing = Direction::SOUTH; break;
            }
            break;
        case TurnBehaviour::TURNING_RIGHT:
            switch(m_facing) {
                case Direction::NORTH: m_facing = Direction::EAST; break;
                case Direction::SOUTH: m_facing = Direction::WEST; break;
                case Direction::EAST: m_facing = Direction::SOUTH; break;
                case Direction::WEST: m_facing = Direction::NORTH; break;
            }
            break;
        default:
            break;
    }
}

void Vehicle::checkPriority() {
    m_isPriority = (m_currentLane == LaneId::AL2_PRIORITY ||
                    m_currentLane == LaneId::BL2_PRIORITY ||
                    m_currentLane == LaneId::CL2_PRIORITY ||
                    m_currentLane == LaneId::DL2_PRIORITY);
}

bool Vehicle::needsTurnLeft(LaneId oldLane, LaneId newLane) const {
    return (oldLane != newLane);
}
