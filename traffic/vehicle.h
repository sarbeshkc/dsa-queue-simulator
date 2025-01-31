#pragma once
#include "SDL_render.h"
#include <SDL3/SDL.h>
#include <cstdint>

// std::unit8_t used because clandg cried
// because it uses more memory than required

enum class VehicleStatus : std::uint8_t {

  // Tracks the current behaviour
  WAITING,
  MOVING,
  TURNING,
  STOPPED,
  TURNING_LEFT,
  TURNING_RIGHT,
  EXISTING,
};

enum class LaneId : std::uint8_t {
  // lane 1
  AL1_INCOMING,
  AL2_PRIORITY,
  AL3_FREELANE,

  // lane 2

  BL1_INCOMING,
  BL2_PRIORITY,
  BL3_FREELANE,

  // lane 3

  CL1_INCOMING,
  CL2_PRIORITY,
  CL3_FREELANE,

  // lane 4

  DL1_INCOMING,
  DL2_PRIORITY,
  DL3_FREELANE,

};

enum class TurnBehaviour : std::uint8_t {

  // Tracks the intended behaviour
  TURNING_LEFT,
  TURNING_RIGHT,
  STRAIGHT,

};

enum class Direction : std::uint8_t {
  // Direction the vehicle is moving in
  NORTH,
  SOUTH,
  EAST,
  WEST,
};

struct Vector2D {
  float x, y;
  Vector2D(float x_ = 0.0F, float y_ = 0.0F) : x(x_), y(y_) {}
};

class Vehicle {
  Vehicle(SDL_Renderer *renderer, int vehicle_id, LaneId startLane,
          Vector2D startPos, Direction facing);
  ~Vehicle();

  // core func

  void update(float deltaTime);
  void render();

  // state management

  void changeLane(LaneId newLane);
  void setTurnDirection(TurnBehaviour turn);
  void checkPriority();

  // getter methods

  VehicleStatus getStatus() const;
  LaneId getCurrentLaneId() const;
  Direction getFacingDirection() const;
  Vector2D getPosition() const;
  float getWaitTime() const;

private:
  // rendering
  SDL_Renderer *m_renderer;

  int m_id;
  Vector2D m_position;
  Vector2D m_velocity;

  // State

  VehicleStatus m_status;
  LaneId m_currentLane;
  LaneId m_targetLane;
  Direction m_facing;
  TurnBehaviour m_turnIntent;

  // Timing and priority

  float m_waitTime;
  bool m_isPriority;

  // Helper Methods
  void updatePosition(float deltaTime);
  void updateState();
  bool checkCollision() const;
};
