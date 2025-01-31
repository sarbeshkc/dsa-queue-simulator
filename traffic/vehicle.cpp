#include "vehicle.h"
#include "SDL_rect.h"
#include "SDL_render.h"

Vehicle::Vehicle(SDL_Renderer *renderer, int vehicle_id, LaneId startLane,
                 Vector2D startPos, Direction facing)
    : m_renderer(renderer), m_id(vehicle_id), m_currentLane(startLane),
      m_facing(facing), m_status(VehicleStatus::WAITING), m_waitTime(0.0F),
      m_isPriority(false) {
  if (startLane == LaneId::AL2_PRIORITY) {
    m_isPriority = true;
  }

  m_targetLane = m_currentLane;
  m_turnIntent = TurnBehaviour::STRAIGHT;
}

void Vehicle::update(float deltaTime) {
  updateState();
  updatePosition(deltaTime);

  if (m_status == VehicleStatus ::WAITING) {
    m_waitTime += deltaTime;
  }
}

void Vehicle::changeLane(LaneId newLane) {

  LaneId oldLane = m_currentLane;

  m_targetLane = newLane;

  if (Vehicle::needsTurnLeft(oldLane, newLane)) {
    m_turnIntent = TurnBehaviour::TURNING_LEFT;
    m_status = VehicleStatus::TURNING_LEFT;
  }
}
void Vehicle::render() {
    // Create the vehicle shape coordinates
    float x = m_position.x - 10;
    float y = m_position.y - 20;
    float w = 20;
    float h = 40;

    // Draw the main body
    if (m_isPriority) {
        SDL_SetRenderDrawColor(m_renderer, 255, 0, 0, 255); // Red for priority
    } else {
        SDL_SetRenderDrawColor(m_renderer, 0, 255, 0, 255); // Green for normal
    }

    // Draw filled rectangle using vertices
    SDL_Vertex vertices[6] = {
        {{x, y}, {255, 0, 0, 255}, {0, 0}},         // Top-left
        {{x + w, y}, {255, 0, 0, 255}, {1, 0}},     // Top-right
        {{x, y + h}, {255, 0, 0, 255}, {0, 1}},     // Bottom-left
        {{x + w, y}, {255, 0, 0, 255}, {1, 0}},     // Top-right
        {{x + w, y + h}, {255, 0, 0, 255}, {1, 1}}, // Bottom-right
        {{x, y + h}, {255, 0, 0, 255}, {0, 1}}      // Bottom-left
    };

    // Set vertex colors based on priority
    SDL_Color color = m_isPriority ? SDL_Color{255, 0, 0, 255} : SDL_Color{0, 255, 0, 255};


    SDL_RenderGeometry(m_renderer, nullptr, vertices, 6, nullptr, 0);

    // Add direction indicator (small triangle in front)
    SDL_Vertex triangle[3];

    switch(m_facing) {
        case Direction::NORTH: {
            triangle[0] = {{m_position.x, m_position.y - 25}, {255, 255, 0, 255}, {0, 0}};
            triangle[1] = {{m_position.x - 5, m_position.y - 15}, {255, 255, 0, 255}, {0, 1}};
            triangle[2] = {{m_position.x + 5, m_position.y - 15}, {255, 255, 0, 255}, {1, 1}};
            break;
        }
        case Direction::SOUTH: {
            triangle[0] = {{m_position.x, m_position.y + 25}, {255, 255, 0, 255}, {0, 0}};
            triangle[1] = {{m_position.x - 5, m_position.y + 15}, {255, 255, 0, 255}, {0, 1}};
            triangle[2] = {{m_position.x + 5, m_position.y + 15}, {255, 255, 0, 255}, {1, 1}};
            break;
        }
        case Direction::EAST: {
            triangle[0] = {{m_position.x + 15, m_position.y}, {255, 255, 0, 255}, {0, 0}};
            triangle[1] = {{m_position.x + 5, m_position.y - 5}, {255, 255, 0, 255}, {0, 1}};
            triangle[2] = {{m_position.x + 5, m_position.y + 5}, {255, 255, 0, 255}, {1, 1}};
            break;
        }
        case Direction::WEST: {
            triangle[0] = {{m_position.x - 15, m_position.y}, {255, 255, 0, 255}, {0, 0}};
            triangle[1] = {{m_position.x - 5, m_position.y - 5}, {255, 255, 0, 255}, {0, 1}};
            triangle[2] = {{m_position.x - 5, m_position.y + 5}, {255, 255, 0, 255}, {1, 1}};
            break;
        }
    }

    SDL_RenderGeometry(m_renderer, nullptr, triangle, 3, nullptr, 0);
}
