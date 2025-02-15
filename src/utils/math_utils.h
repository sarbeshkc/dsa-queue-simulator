// src/utils/math_utils.h
#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <cmath>

struct Vector2D {
  float x, y;

  Vector2D(float x_ = 0.0f, float y_ = 0.0f) : x(x_), y(y_) {}

  // Add vector operations
  Vector2D operator+(const Vector2D &other) const {
    return Vector2D(x + other.x, y + other.y);
  }

  Vector2D operator-(const Vector2D &other) const {
    return Vector2D(x - other.x, y - other.y);
  }
};

#endif // MATH_UTILS_H
