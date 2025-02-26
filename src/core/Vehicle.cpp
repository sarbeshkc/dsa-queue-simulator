#include "core/Vehicle.h"
#include "core/Constants.h"
#include "utils/DebugLogger.h"
#include <cmath>
#include <sstream>

Vehicle::Vehicle(const std::string& id, char lane, int laneNumber, bool isEmergency)
    : id(id),
      lane(lane),
      laneNumber(laneNumber),
      isEmergency(isEmergency),
      arrivalTime(time(nullptr)),
      animPos(0.0f),
      turning(false),
      turnProgress(0.0f),
      turnPosX(0.0f),
      turnPosY(0.0f),
      destination(Destination::STRAIGHT),
      currentDirection(Direction::DOWN),
      state(VehicleState::APPROACHING),
      currentWaypoint(0) {

    // Log creation
    std::ostringstream oss;
    oss << "Created vehicle " << id << " in lane " << lane << laneNumber;
    DebugLogger::log(oss.str());

    // Window dimensions
    const int windowWidth = 800;
    const int windowHeight = 800;
    const int centerX = windowWidth / 2;
    const int centerY = windowHeight / 2;

    // Determine current direction based on lane
    switch (lane) {
        case 'A': currentDirection = Direction::DOWN; break; // A is top road, moving down
        case 'B': currentDirection = Direction::UP; break;   // B is bottom road, moving up
        case 'C': currentDirection = Direction::LEFT; break; // C is right road, moving left
        case 'D': currentDirection = Direction::RIGHT; break;// D is left road, moving right
    }

    // Set initial position based on lane and direction
    switch (currentDirection) {
        case Direction::DOWN:
            turnPosX = centerX + (laneNumber - 2) * 15.0f;
            turnPosY = 20.0f;  // Start at top of screen
            break;
        case Direction::UP:
            turnPosX = centerX - (laneNumber - 2) * 15.0f;
            turnPosY = windowHeight - 20.0f;  // Start at bottom of screen
            break;
        case Direction::LEFT:
            turnPosX = windowWidth - 20.0f;  // Start at right of screen
            turnPosY = centerY + (laneNumber - 2) * 15.0f;
            break;
        case Direction::RIGHT:
            turnPosX = 20.0f;  // Start at left of screen
            turnPosY = centerY - (laneNumber - 2) * 15.0f;
            break;
    }

    // Set initial animation position
    animPos = (currentDirection == Direction::DOWN || currentDirection == Direction::UP) ?
              turnPosY : turnPosX;

    // Determine destination based on lane number and ID (for randomness)
    int idHash = 0;
    for (char c : id) idHash += c;

    if (laneNumber == 3) {
        // Free lane (L3) always turns left
        destination = Destination::LEFT;
    }
    else if (laneNumber == 2) {
        // Lane 2 has three possible destinations: straight, left turn, or right turn
        // Using a weighted random choice based on ID
        int choice = idHash % 10;
        if (choice < 4) {
            destination = Destination::STRAIGHT; // 40% chance to go straight
        } else if (choice < 7) {
            destination = Destination::LEFT;     // 30% chance to turn left
        } else {
            destination = Destination::RIGHT;    // 30% chance to turn right
        }
    }

    // Initialize waypoints for path planning
    initializeWaypoints();
}

Vehicle::~Vehicle() {
    std::ostringstream oss;
    oss << "Destroyed vehicle " << id;
    DebugLogger::log(oss.str());
}

void Vehicle::initializeWaypoints() {
    // Window dimensions
    const int windowWidth = 800;
    const int windowHeight = 800;
    const int centerX = windowWidth / 2;
    const int centerY = windowHeight / 2;

    // Clear existing waypoints
    waypoints.clear();

    // Intersection boundaries (smaller for smoother movement)
    const float intersectionHalf = 45.0f;
    const float leftEdge = centerX - intersectionHalf;
    const float rightEdge = centerX + intersectionHalf;
    const float topEdge = centerY - intersectionHalf;
    const float bottomEdge = centerY + intersectionHalf;

    // Lane offsets - smaller for more compact lanes
    const float laneOffset = 12.0f;

    // Add the starting position as first waypoint
    waypoints.push_back({turnPosX, turnPosY});

    // Add approach to intersection waypoint
    switch (currentDirection) {
        case Direction::DOWN:
            waypoints.push_back({turnPosX, topEdge - 5.0f});
            break;
        case Direction::UP:
            waypoints.push_back({turnPosX, bottomEdge + 5.0f});
            break;
        case Direction::LEFT:
            waypoints.push_back({rightEdge + 5.0f, turnPosY});
            break;
        case Direction::RIGHT:
            waypoints.push_back({leftEdge - 5.0f, turnPosY});
            break;
    }

    // Add path through intersection based on destination
    if (destination == Destination::STRAIGHT) {
        // For going straight, add waypoints to pass through intersection
        switch (currentDirection) {
            case Direction::DOWN:  // A to B
                waypoints.push_back({turnPosX, bottomEdge + 5.0f});  // Exit point
                waypoints.push_back({turnPosX, windowHeight + 30.0f});  // Off screen
                break;
            case Direction::UP:    // B to A
                waypoints.push_back({turnPosX, topEdge - 5.0f});
                waypoints.push_back({turnPosX, -30.0f});
                break;
            case Direction::LEFT:  // C to D
                waypoints.push_back({leftEdge - 5.0f, turnPosY});
                waypoints.push_back({-30.0f, turnPosY});
                break;
            case Direction::RIGHT: // D to C
                waypoints.push_back({rightEdge + 5.0f, turnPosY});
                waypoints.push_back({windowWidth + 30.0f, turnPosY});
                break;
        }
    }
    else if (destination == Destination::LEFT) {
        // For left turns, add center point and exit points
        float centerPointX, centerPointY, exitPointX, exitPointY;

        switch (currentDirection) {
            case Direction::DOWN:  // A to D - Left turn puts vehicle in lane 1
                centerPointX = centerX - 15.0f;
                centerPointY = centerY - 15.0f;
                exitPointX = leftEdge - 5.0f;
                exitPointY = centerY - laneOffset;  // Lane 1 position

                waypoints.push_back({centerPointX, centerPointY});  // Turn center
                waypoints.push_back({exitPointX, exitPointY});      // Exit point
                waypoints.push_back({-30.0f, exitPointY});          // Off screen
                break;

            case Direction::UP:    // B to C - Left turn puts vehicle in lane 1
                centerPointX = centerX + 15.0f;
                centerPointY = centerY + 15.0f;
                exitPointX = rightEdge + 5.0f;
                exitPointY = centerY + laneOffset;  // Lane 1 position

                waypoints.push_back({centerPointX, centerPointY});
                waypoints.push_back({exitPointX, exitPointY});
                waypoints.push_back({windowWidth + 30.0f, exitPointY});
                break;

            case Direction::LEFT:  // C to A - Left turn puts vehicle in lane 1
                centerPointX = centerX + 15.0f;
                centerPointY = centerY - 15.0f;
                exitPointX = centerX + laneOffset;  // Lane 1 position
                exitPointY = topEdge - 5.0f;

                waypoints.push_back({centerPointX, centerPointY});
                waypoints.push_back({exitPointX, exitPointY});
                waypoints.push_back({exitPointX, -30.0f});
                break;

            case Direction::RIGHT: // D to B - Left turn puts vehicle in lane 1
                centerPointX = centerX - 15.0f;
                centerPointY = centerY + 15.0f;
                exitPointX = centerX - laneOffset;  // Lane 1 position
                exitPointY = bottomEdge + 5.0f;

                waypoints.push_back({centerPointX, centerPointY});
                waypoints.push_back({exitPointX, exitPointY});
                waypoints.push_back({exitPointX, windowHeight + 30.0f});
                break;
        }
    }
    else if (destination == Destination::RIGHT) {
        // For right turns, add center point and exit points
        float centerPointX, centerPointY, exitPointX, exitPointY;

        switch (currentDirection) {
            case Direction::DOWN:  // A to C - Right turn puts vehicle in lane 1
                centerPointX = centerX + 15.0f;
                centerPointY = centerY + 15.0f;
                exitPointX = rightEdge + 5.0f;
                exitPointY = centerY + laneOffset;  // Lane 1 position

                waypoints.push_back({centerPointX, centerPointY});
                waypoints.push_back({exitPointX, exitPointY});
                waypoints.push_back({windowWidth + 30.0f, exitPointY});
                break;

            case Direction::UP:    // B to D - Right turn puts vehicle in lane 1
                centerPointX = centerX - 15.0f;
                centerPointY = centerY - 15.0f;
                exitPointX = leftEdge - 5.0f;
                exitPointY = centerY - laneOffset;  // Lane 1 position

                waypoints.push_back({centerPointX, centerPointY});
                waypoints.push_back({exitPointX, exitPointY});
                waypoints.push_back({-30.0f, exitPointY});
                break;

            case Direction::LEFT:  // C to B - Right turn puts vehicle in lane 1
                centerPointX = centerX - 15.0f;
                centerPointY = centerY + 15.0f;
                exitPointX = centerX - laneOffset;  // Lane 1 position
                exitPointY = bottomEdge + 5.0f;

                waypoints.push_back({centerPointX, centerPointY});
                waypoints.push_back({exitPointX, exitPointY});
                waypoints.push_back({exitPointX, windowHeight + 30.0f});
                break;

            case Direction::RIGHT: // D to A - Right turn puts vehicle in lane 1
                centerPointX = centerX + 15.0f;
                centerPointY = centerY - 15.0f;
                exitPointX = centerX + laneOffset;  // Lane 1 position
                exitPointY = topEdge - 5.0f;

                waypoints.push_back({centerPointX, centerPointY});
                waypoints.push_back({exitPointX, exitPointY});
                waypoints.push_back({exitPointX, -30.0f});
                break;
        }
    }

    // Set current waypoint index
    currentWaypoint = 0;
    turning = false;
}

std::string Vehicle::getId() const {
    return id;
}

char Vehicle::getLane() const {
    return lane;
}

void Vehicle::setLane(char lane) {
    this->lane = lane;
}

int Vehicle::getLaneNumber() const {
    return laneNumber;
}

void Vehicle::setLaneNumber(int number) {
    this->laneNumber = number;
}

bool Vehicle::isEmergencyVehicle() const {
    return isEmergency;
}

time_t Vehicle::getArrivalTime() const {
    return arrivalTime;
}

float Vehicle::getAnimationPos() const {
    return animPos;
}

void Vehicle::setAnimationPos(float pos) {
    this->animPos = pos;
}

bool Vehicle::isTurning() const {
    return turning;
}

void Vehicle::setTurning(bool turning) {
    this->turning = turning;
}

float Vehicle::getTurnProgress() const {
    return turnProgress;
}

void Vehicle::setTurnProgress(float progress) {
    this->turnProgress = progress;
}

float Vehicle::getTurnPosX() const {
    return turnPosX;
}

void Vehicle::setTurnPosX(float x) {
    this->turnPosX = x;
}

float Vehicle::getTurnPosY() const {
    return turnPosY;
}

void Vehicle::setTurnPosY(float y) {
    this->turnPosY = y;
}

float Vehicle::easeInOutQuad(float t) const {
    return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
}

// In Vehicle.cpp - update method fix to prevent overlapping and improve turning
void Vehicle::update(uint32_t delta, bool isGreenLight, float targetPos) {
    // Very slow speed for smoother animation
    const float SPEED = 0.015f * delta;
    // Minimum distance between vehicles in queue
    const float MIN_VEHICLE_DISTANCE = 25.0f;

    // Free lane (lane 3) always has green light
    bool canMove = isGreenLight || laneNumber == 3;

    // If in lane 3, ensure we're turning left
    if (laneNumber == 3 && destination != Destination::LEFT) {
        destination = Destination::LEFT;
        initializeWaypoints(); // Recalculate waypoints for left turn
    }

    if (canMove) {
        // We have more waypoints to travel
        if (currentWaypoint < waypoints.size() - 1) {
            // Get current and next waypoint
            auto& current = waypoints[currentWaypoint];
            auto& next = waypoints[currentWaypoint + 1];

            // Calculate direction vector
            float dx = next.x - turnPosX;
            float dy = next.y - turnPosY;

            // Calculate distance to next waypoint
            float distance = std::sqrt(dx*dx + dy*dy);

            // Check if we need to maintain distance from vehicle ahead (only if not turning)
            if (!turning && state == VehicleState::APPROACHING) {
                float distToTarget = 0.0f;

                // For vertical roads (A, B)
                if (currentDirection == Direction::UP || currentDirection == Direction::DOWN) {
                    distToTarget = std::abs(turnPosY - targetPos);
                }
                // For horizontal roads (C, D)
                else {
                    distToTarget = std::abs(turnPosX - targetPos);
                }

                // Don't move if too close to vehicle ahead
                if (distToTarget < MIN_VEHICLE_DISTANCE && targetPos > 0) {
                    return;
                }
            }

            // If close enough to waypoint, move to next
            if (distance < 3.0f) {
                currentWaypoint++;

                // If entering the intersection and turning, flag as turning
                if (currentWaypoint == 2 &&
                    (destination == Destination::LEFT || destination == Destination::RIGHT)) {
                    turning = true;
                    turnProgress = 0.0f;
                    state = VehicleState::IN_INTERSECTION;
                }

                // If exiting the intersection
                if (currentWaypoint == 3) {
                    turning = false;
                    state = VehicleState::EXITING;

                    // Update lane and number based on destination and direction
                    switch (currentDirection) {
                        case Direction::DOWN:  // From A
                            if (destination == Destination::LEFT) {
                                lane = 'D';
                                laneNumber = 1;  // Exit into lane 1
                                currentDirection = Direction::RIGHT;
                            }
                            else if (destination == Destination::RIGHT) {
                                lane = 'C';
                                laneNumber = 1;  // Exit into lane 1
                                currentDirection = Direction::LEFT;
                            }
                            else {
                                // Going straight, lane number stays the same
                                lane = 'B';
                            }
                            break;

                        case Direction::UP:    // From B
                            if (destination == Destination::LEFT) {
                                lane = 'C';
                                laneNumber = 1;  // Exit into lane 1
                                currentDirection = Direction::LEFT;
                            }
                            else if (destination == Destination::RIGHT) {
                                lane = 'D';
                                laneNumber = 1;  // Exit into lane 1
                                currentDirection = Direction::RIGHT;
                            }
                            else {
                                // Going straight, lane number stays the same
                                lane = 'A';
                            }
                            break;

                        case Direction::LEFT:  // From C
                            if (destination == Destination::LEFT) {
                                lane = 'A';
                                laneNumber = 1;  // Exit into lane 1
                                currentDirection = Direction::DOWN;
                            }
                            else if (destination == Destination::RIGHT) {
                                lane = 'B';
                                laneNumber = 1;  // Exit into lane 1
                                currentDirection = Direction::UP;
                            }
                            else {
                                // Going straight, lane number stays the same
                                lane = 'D';
                            }
                            break;

                        case Direction::RIGHT: // From D
                            if (destination == Destination::LEFT) {
                                lane = 'B';
                                laneNumber = 1;  // Exit into lane 1
                                currentDirection = Direction::UP;
                            }
                            else if (destination == Destination::RIGHT) {
                                lane = 'A';
                                laneNumber = 1;  // Exit into lane 1
                                currentDirection = Direction::DOWN;
                            }
                            else {
                                // Going straight, lane number stays the same
                                lane = 'C';
                            }
                            break;
                    }
                }
            }

            // Move toward next waypoint
            if (distance > 0) {
                // Normalize direction vector
                dx /= distance;
                dy /= distance;

                // Move toward waypoint
                turnPosX += dx * SPEED;
                turnPosY += dy * SPEED;

                // Update animation position for future reference
                animPos = (currentDirection == Direction::UP || currentDirection == Direction::DOWN) ?
                         turnPosY : turnPosX;
            }

            // Update turn progress for visualization
            if (turning) {
                turnProgress = std::min(1.0f, turnProgress + 0.001f * delta);
            }
        }

        // Check if we've reached the last waypoint
        if (currentWaypoint == waypoints.size() - 1) {
            // Get screen dimensions
            const int windowWidth = 800;
            const int windowHeight = 800;

            // Check if off-screen
            if (turnPosX < -30.0f || turnPosX > windowWidth + 30.0f ||
                turnPosY < -30.0f || turnPosY > windowHeight + 30.0f) {
                // Flag for removal
                state = VehicleState::EXITED;
            }
        }
    }
    else {
        // Red light - move until we reach the stop line
        if (currentWaypoint == 0) {
            // Get the stop line waypoint
            auto& stopLine = waypoints[1];

            // Calculate direction and distance to stop line
            float dx = stopLine.x - turnPosX;
            float dy = stopLine.y - turnPosY;
            float distance = std::sqrt(dx*dx + dy*dy);

            // Only approach if not too close to stop line
            if (distance > 5.0f) {
                // Normalize direction
                dx /= distance;
                dy /= distance;

                // Move toward stop line
                turnPosX += dx * SPEED;
                turnPosY += dy * SPEED;

                // Update animation position
                animPos = (currentDirection == Direction::UP || currentDirection == Direction::DOWN) ?
                         turnPosY : turnPosX;
            }
        }
    }
}

void Vehicle::calculateTurnPath(float startX, float startY, float controlX, float controlY,
                              float endX, float endY, float progress) {
    // Quadratic bezier curve calculation for smooth turning
    float oneMinusT = 1.0f - progress;

    // Calculate position on the curve
    turnPosX = oneMinusT * oneMinusT * startX +
               2.0f * oneMinusT * progress * controlX +
               progress * progress * endX;

    turnPosY = oneMinusT * oneMinusT * startY +
               2.0f * oneMinusT * progress * controlY +
               progress * progress * endY;
}

// In Vehicle.cpp - render method improvement for better visualization
// In Vehicle.cpp - render method fix (undeclared variables)
void Vehicle::render(SDL_Renderer* renderer, SDL_Texture* vehicleTexture, int queuePos) {
    // Set vehicle color based on lane, lane number, and turning status
    SDL_Color color;

    if (isEmergency) {
        color = {255, 0, 0, 255}; // Red for emergency
    }
    else if (laneNumber == 2 && lane == 'A') {
        // AL2 is priority lane - highlight with orange
        color = {255, 140, 0, 255}; // Brighter orange for priority lane
    }
    else if (laneNumber == 3) {
        // Free lane - green with left turn indicator
        color = {0, 220, 60, 255}; // Brighter green for free lane
    }
    else if (laneNumber == 1) {
        // Lane 1 - blue/cyan
        color = {0, 140, 255, 255}; // Bright blue for lane 1
    }
    else {
        // Regular vehicles, choose by destination
        switch (static_cast<int>(destination)) {
            case static_cast<int>(Destination::STRAIGHT):
                color = {120, 120, 200, 255}; break; // Light blue for straight
            case static_cast<int>(Destination::LEFT):
                color = {200, 120, 120, 255}; break; // Light red for left turn
            case static_cast<int>(Destination::RIGHT):
                color = {120, 200, 120, 255}; break; // Light green for right turn
            default:
                color = {180, 180, 180, 255}; break; // Gray default
        }
    }

    // If turning, make the color a bit brighter
    if (turning) {
        color.r = std::min(255, color.r + 40);
        color.g = std::min(255, color.g + 40);
        color.b = std::min(255, color.b + 40);

        // Add a turning indicator halo
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 100); // Yellow semi-transparent
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        // Draw a slightly larger rectangle behind the vehicle
        // Use class member variables for width/length instead of local vars
        SDL_FRect haloRect = {
            turnPosX - (VEHICLE_WIDTH/2) - 3.0f,
            turnPosY - (VEHICLE_LENGTH/2) - 3.0f,
            VEHICLE_WIDTH + 6.0f,
            VEHICLE_LENGTH + 6.0f
        };
        SDL_RenderFillRect(renderer, &haloRect);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }

    // Set color for vehicle
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    // Vehicle dimensions - use class constants
    float width = VEHICLE_WIDTH;
    float length = VEHICLE_LENGTH;

    // Draw vehicle rectangle based on orientation
    SDL_FRect vehicleRect;

    if (turning) {
        // For turning vehicles, adjust dimensions gradually
        float progress = turnProgress;
        float adjustedWidth = width;
        float adjustedLength = length;

        // During turn, gradually change dimensions for smoother appearance
        if (currentDirection == Direction::UP || currentDirection == Direction::DOWN) {
            // Transitioning from vertical to horizontal
            if (destination == Destination::LEFT || destination == Destination::RIGHT) {
                adjustedWidth = width * (1.0f - progress) + length * progress;
                adjustedLength = length * (1.0f - progress) + width * progress;
            }
        } else {
            // Transitioning from horizontal to vertical
            if (destination == Destination::LEFT || destination == Destination::RIGHT) {
                adjustedWidth = length * (1.0f - progress) + width * progress;
                adjustedLength = width * (1.0f - progress) + length * progress;
            }
        }

        vehicleRect = {turnPosX - adjustedWidth/2, turnPosY - adjustedLength/2, adjustedWidth, adjustedLength};
    } else {
        // Non-turning vehicles have fixed orientation based on direction
        switch (currentDirection) {
            case Direction::DOWN:
            case Direction::UP:
                // Vertical roads (taller than wide)
                vehicleRect = {turnPosX - width/2, turnPosY - length/2, width, length};
                break;
            case Direction::LEFT:
            case Direction::RIGHT:
                // Horizontal roads (wider than tall)
                vehicleRect = {turnPosX - length/2, turnPosY - width/2, length, width};
                break;
        }
    }

    // Draw the vehicle
    SDL_RenderFillRect(renderer, &vehicleRect);

    // Add a darker front to the vehicle for better visibility
    SDL_SetRenderDrawColor(renderer,
                          color.r * 0.7f,
                          color.g * 0.7f,
                          color.b * 0.7f,
                          color.a);

    SDL_FRect frontRect;
    if (!turning) {
        switch (currentDirection) {
            case Direction::DOWN:
                // Front of vehicle (bottom part for downward movement)
                frontRect = {vehicleRect.x, vehicleRect.y + vehicleRect.h * 0.6f, vehicleRect.w, vehicleRect.h * 0.4f};
                break;
            case Direction::UP:
                // Front of vehicle (top part for upward movement)
                frontRect = {vehicleRect.x, vehicleRect.y, vehicleRect.w, vehicleRect.h * 0.4f};
                break;
            case Direction::LEFT:
                // Front of vehicle (left part for leftward movement)
                frontRect = {vehicleRect.x, vehicleRect.y, vehicleRect.w * 0.4f, vehicleRect.h};
                break;
            case Direction::RIGHT:
                // Front of vehicle (right part for rightward movement)
                frontRect = {vehicleRect.x + vehicleRect.w * 0.6f, vehicleRect.y, vehicleRect.w * 0.4f, vehicleRect.h};
                break;
        }

        SDL_RenderFillRect(renderer, &frontRect);
    }

    // Add indicator based on destination
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White

    float indicatorSize = 2.0f;
    float indicatorOffset = 3.0f;

    // Make indicators more visible and distinctive
    if (destination == Destination::LEFT) {
        // Left turn indicator (small white triangle near the front)
        SDL_FRect leftIndicator = {
            vehicleRect.x + indicatorOffset,
            vehicleRect.y + indicatorOffset,
            indicatorSize, indicatorSize
        };
        SDL_RenderFillRect(renderer, &leftIndicator);
    }
    else if (destination == Destination::RIGHT) {
        // Right turn indicator (small white triangle near the front)
        SDL_FRect rightIndicator = {
            vehicleRect.x + vehicleRect.w - indicatorOffset - indicatorSize,
            vehicleRect.y + indicatorOffset,
            indicatorSize, indicatorSize
        };
        SDL_RenderFillRect(renderer, &rightIndicator);
    }
    else {
        // Straight indicator (center dot)
        SDL_FRect indicator = {
            vehicleRect.x + vehicleRect.w/2 - indicatorSize/2,
            vehicleRect.y + indicatorOffset,
            indicatorSize, indicatorSize
        };
        SDL_RenderFillRect(renderer, &indicator);
    }

    // Add queue position number for debugging
    if (queuePos > 0) {
        // Draw a small number indicator
        char posStr[3];
        snprintf(posStr, sizeof(posStr), "%d", queuePos);

        // In a real implementation, we would render text here
        // For now, just draw a different colored dot
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Yellow
        SDL_FRect posIndicator = {
            vehicleRect.x + vehicleRect.w/2 - 1.0f,
            vehicleRect.y + vehicleRect.h/2 - 1.0f,
            2.0f, 2.0f
        };
        SDL_RenderFillRect(renderer, &posIndicator);
    }
}
