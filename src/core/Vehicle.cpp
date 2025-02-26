// FILE: src/core/Vehicle.cpp
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
      queuePos(0),
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
    // A is North (top), B is East (right), C is South (bottom), D is West (left)
    switch (lane) {
        case 'A': currentDirection = Direction::DOWN; break;  // Coming from North, moving South
        case 'B': currentDirection = Direction::LEFT; break;  // Coming from East, moving West
        case 'C': currentDirection = Direction::UP; break;    // Coming from South, moving North
        case 'D': currentDirection = Direction::RIGHT; break; // Coming from West, moving East
        default:
            DebugLogger::log("Invalid lane ID: " + std::string(1, lane), DebugLogger::LogLevel::ERROR);
            currentDirection = Direction::DOWN;
            break;
    }

    // Determine destination based on lane number and rules from assignment
    if (laneNumber == 3) {
        // Free lane (L3) always turns left
        destination = Destination::LEFT;
        DebugLogger::log("Vehicle " + id + " on lane " + lane + std::to_string(laneNumber) + " will turn LEFT (free lane rule)");
    }
    else if (laneNumber == 2) {
        // Lane 2 can go straight or turn right (based on vehicle ID hash)
        // Use the last digit of the id for randomization (this ensures reproducibility)
        int idHash = 0;
        for (char c : id) idHash += c;

        // Check for explicit direction indication in ID
        if (id.find("_RIGHT") != std::string::npos) {
            destination = Destination::RIGHT;
        } else if (id.find("_STRAIGHT") != std::string::npos) {
            destination = Destination::STRAIGHT;
        } else {
            // 60% straight, 40% right turn if not specified
            destination = (idHash % 10 < 6) ? Destination::STRAIGHT : Destination::RIGHT;
        }

        // Log the chosen direction
        std::string destStr = (destination == Destination::STRAIGHT) ? "STRAIGHT" : "RIGHT";
        DebugLogger::log("Vehicle " + id + " on lane " + lane + std::to_string(laneNumber) + " will go " + destStr);
    }
    else if (laneNumber == 1) {
        // Lane 1 is incoming lane, straight is default
        destination = Destination::STRAIGHT;
    }

    // Set initial position with wider lane spacing for better visualization
    const float laneOffset = 25.0f; // Increased from 15.0f for better separation

    // Set initial position based on lane and direction
    switch (currentDirection) {
        case Direction::DOWN: // From North (A)
            turnPosX = centerX + (laneNumber - 2) * laneOffset;
            turnPosY = 20.0f;  // Start at top of screen
            break;
        case Direction::UP: // From South (C)
            turnPosX = centerX - (laneNumber - 2) * laneOffset;
            turnPosY = windowHeight - 20.0f;  // Start at bottom of screen
            break;
        case Direction::LEFT: // From East (B)
            turnPosX = windowWidth - 20.0f;  // Start at right of screen
            turnPosY = centerY + (laneNumber - 2) * laneOffset;
            break;
        case Direction::RIGHT: // From West (D)
            turnPosX = 20.0f;  // Start at left of screen
            turnPosY = centerY - (laneNumber - 2) * laneOffset;
            break;
    }

    // Set initial animation position
    animPos = (currentDirection == Direction::UP || currentDirection == Direction::DOWN) ?
              turnPosY : turnPosX;

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

    // Adjust intersection boundaries for better visualization
    const float intersectionHalf = 50.0f;
    const float leftEdge = centerX - intersectionHalf;
    const float rightEdge = centerX + intersectionHalf;
    const float topEdge = centerY - intersectionHalf;
    const float bottomEdge = centerY + intersectionHalf;

    // Lane offsets - more spacing for better visualization
    const float laneOffset = 25.0f; // Increased from 15.0f

    // Add the starting position as first waypoint
    waypoints.push_back({turnPosX, turnPosY});

    // Add approach to intersection waypoint
    switch (currentDirection) {
        case Direction::DOWN: // From North (A)
            waypoints.push_back({turnPosX, topEdge - 5.0f});
            break;
        case Direction::UP: // From South (C)
            waypoints.push_back({turnPosX, bottomEdge + 5.0f});
            break;
        case Direction::LEFT: // From East (B)
            waypoints.push_back({rightEdge + 5.0f, turnPosY});
            break;
        case Direction::RIGHT: // From West (D)
            waypoints.push_back({leftEdge - 5.0f, turnPosY});
            break;
    }

    // Add path through intersection based on destination and rules from assignment
    if (destination == Destination::STRAIGHT) {
        // For going straight: From L2 to opposite road's L1
        switch (currentDirection) {
            case Direction::DOWN:  // A to C (North to South)
                waypoints.push_back({turnPosX, bottomEdge + 5.0f});  // Exit point
                waypoints.push_back({turnPosX, windowHeight + 30.0f});  // Off screen
                break;
            case Direction::UP:    // C to A (South to North)
                waypoints.push_back({turnPosX, topEdge - 5.0f});
                waypoints.push_back({turnPosX, -30.0f});
                break;
            case Direction::LEFT:  // B to D (East to West)
                waypoints.push_back({leftEdge - 5.0f, turnPosY});
                waypoints.push_back({-30.0f, turnPosY});
                break;
            case Direction::RIGHT: // D to B (West to East)
                waypoints.push_back({rightEdge + 5.0f, turnPosY});
                waypoints.push_back({windowWidth + 30.0f, turnPosY});
                break;
        }
    }
    else if (destination == Destination::LEFT) {
        // For L3 left turns: Always goes to the target road's L1
        float centerPointX, centerPointY, exitPointX, exitPointY;

        switch (currentDirection) {
            case Direction::DOWN:  // A to D - From North to West's L1
                centerPointX = centerX - 20.0f;
                centerPointY = centerY - 20.0f;
                exitPointX = leftEdge - 5.0f;
                exitPointY = centerY - laneOffset;  // Lane 1 position (D)

                waypoints.push_back({centerPointX, centerPointY});  // Turn center
                waypoints.push_back({exitPointX, exitPointY});      // Exit point
                waypoints.push_back({-30.0f, exitPointY});          // Off screen
                break;

            case Direction::UP:    // C to B - From South to East's L1
                centerPointX = centerX + 20.0f;
                centerPointY = centerY + 20.0f;
                exitPointX = rightEdge + 5.0f;
                exitPointY = centerY + laneOffset;  // Lane 1 position (B)

                waypoints.push_back({centerPointX, centerPointY});
                waypoints.push_back({exitPointX, exitPointY});
                waypoints.push_back({windowWidth + 30.0f, exitPointY});
                break;

            case Direction::LEFT:  // B to A - From East to North's L1
                centerPointX = centerX + 20.0f;
                centerPointY = centerY - 20.0f;
                exitPointX = centerX + laneOffset;  // Lane 1 position (A)
                exitPointY = topEdge - 5.0f;

                waypoints.push_back({centerPointX, centerPointY});
                waypoints.push_back({exitPointX, exitPointY});
                waypoints.push_back({exitPointX, -30.0f});
                break;

            case Direction::RIGHT: // D to C - From West to South's L1
                centerPointX = centerX - 20.0f;
                centerPointY = centerY + 20.0f;
                exitPointX = centerX - laneOffset;  // Lane 1 position (C)
                exitPointY = bottomEdge + 5.0f;

                waypoints.push_back({centerPointX, centerPointY});
                waypoints.push_back({exitPointX, exitPointY});
                waypoints.push_back({exitPointX, windowHeight + 30.0f});
                break;
        }
    }
    else if (destination == Destination::RIGHT) {
        // For L2 right turns: Goes to the clockwise road's L1
        float centerPointX, centerPointY, exitPointX, exitPointY;

        switch (currentDirection) {
            case Direction::DOWN:  // A to B - From North to East's L1
                centerPointX = centerX + 20.0f;
                centerPointY = centerY - 20.0f;
                exitPointX = rightEdge + 5.0f;
                exitPointY = centerY - laneOffset;  // Lane 1 position (B)

                waypoints.push_back({centerPointX, centerPointY});
                waypoints.push_back({exitPointX, exitPointY});
                waypoints.push_back({windowWidth + 30.0f, exitPointY});
                break;

            case Direction::UP:    // C to D - From South to West's L1
                centerPointX = centerX - 20.0f;
                centerPointY = centerY + 20.0f;
                exitPointX = leftEdge - 5.0f;
                exitPointY = centerY + laneOffset;  // Lane 1 position (D)

                waypoints.push_back({centerPointX, centerPointY});
                waypoints.push_back({exitPointX, exitPointY});
                waypoints.push_back({-30.0f, exitPointY});
                break;

            case Direction::LEFT:  // B to C - From East to South's L1
                centerPointX = centerX + 20.0f;
                centerPointY = centerY + 20.0f;
                exitPointX = centerX + laneOffset;  // Lane 1 position (C)
                exitPointY = bottomEdge + 5.0f;

                waypoints.push_back({centerPointX, centerPointY});
                waypoints.push_back({exitPointX, exitPointY});
                waypoints.push_back({exitPointX, windowHeight + 30.0f});
                break;

            case Direction::RIGHT: // D to A - From West to North's L1
                centerPointX = centerX - 20.0f;
                centerPointY = centerY - 20.0f;
                exitPointX = centerX - laneOffset;  // Lane 1 position (A)
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

void Vehicle::setDestination(Destination dest) {
    if (this->destination != dest) {
        this->destination = dest;

        // When destination changes, reinitialize waypoints to update the path
        initializeWaypoints();

        // Log the destination change
        std::ostringstream oss;
        std::string destStr;
        switch (dest) {
            case Destination::STRAIGHT: destStr = "STRAIGHT"; break;
            case Destination::LEFT: destStr = "LEFT"; break;
            case Destination::RIGHT: destStr = "RIGHT"; break;
        }
        oss << "Vehicle " << id << " destination set to " << destStr;
        DebugLogger::log(oss.str());
    }
}

Destination Vehicle::getDestination() const {
    return destination;
}

float Vehicle::easeInOutQuad(float t) const {
    return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
}

void Vehicle::update(uint32_t delta, bool isGreenLight, float targetPos) {
    // Fine-tune speed for smoother animation
    const float SPEED = 0.02f * delta;
    const float VEHICLE_SPACING = 30.0f; // Increased spacing between vehicles in queue

    // Free lane (lane 3) always has green light
    bool canMove = isGreenLight || laneNumber == 3;

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

            // If close enough to waypoint, move to next
            if (distance < 3.0f) {
                currentWaypoint++;

                // If entering the intersection and turning, flag as turning
                if (currentWaypoint == 2 &&
                    (destination == Destination::LEFT || destination == Destination::RIGHT)) {
                    turning = true;
                    turnProgress = 0.0f;
                    state = VehicleState::IN_INTERSECTION;

                    // Log turn start
                    std::ostringstream oss;
                    oss << "Vehicle " << id << " is now turning "
                        << (destination == Destination::LEFT ? "LEFT" : "RIGHT");
                    DebugLogger::log(oss.str());
                }

                // If exiting the intersection
                if (currentWaypoint == 3) {
                    turning = false;
                    state = VehicleState::EXITING;

                    // Update lane and number based on destination and direction
                    // This follows the assignment rules for lane changes
                    switch (currentDirection) {
                        case Direction::DOWN:  // From North (A)
                            if (destination == Destination::LEFT) {
                                lane = 'D';  // Go to West
                                laneNumber = 1;
                                currentDirection = Direction::RIGHT;
                                DebugLogger::log("Vehicle " + id + " now on D1 (turned LEFT from A)");
                            }
                            else if (destination == Destination::RIGHT) {
                                lane = 'B';  // Go to East
                                laneNumber = 1;
                                currentDirection = Direction::LEFT;
                                DebugLogger::log("Vehicle " + id + " now on B1 (turned RIGHT from A)");
                            }
                            else {
                                // Going straight to South
                                lane = 'C';
                                laneNumber = 1;
                                DebugLogger::log("Vehicle " + id + " now on C1 (going STRAIGHT from A)");
                            }
                            break;

                        case Direction::UP:    // From South (C)
                            if (destination == Destination::LEFT) {
                                lane = 'B';  // Go to East
                                laneNumber = 1;
                                currentDirection = Direction::LEFT;
                                DebugLogger::log("Vehicle " + id + " now on B1 (turned LEFT from C)");
                            }
                            else if (destination == Destination::RIGHT) {
                                lane = 'D';  // Go to West
                                laneNumber = 1;
                                currentDirection = Direction::RIGHT;
                                DebugLogger::log("Vehicle " + id + " now on D1 (turned RIGHT from C)");
                            }
                            else {
                                // Going straight to North
                                lane = 'A';
                                laneNumber = 1;
                                DebugLogger::log("Vehicle " + id + " now on A1 (going STRAIGHT from C)");
                            }
                            break;

                        case Direction::LEFT:  // From East (B)
                            if (destination == Destination::LEFT) {
                                lane = 'A';  // Go to North
                                laneNumber = 1;
                                currentDirection = Direction::DOWN;
                                DebugLogger::log("Vehicle " + id + " now on A1 (turned LEFT from B)");
                            }
                            else if (destination == Destination::RIGHT) {
                                lane = 'C';  // Go to South
                                laneNumber = 1;
                                currentDirection = Direction::UP;
                                DebugLogger::log("Vehicle " + id + " now on C1 (turned RIGHT from B)");
                            }
                            else {
                                // Going straight to West
                                lane = 'D';
                                laneNumber = 1;
                                DebugLogger::log("Vehicle " + id + " now on D1 (going STRAIGHT from B)");
                            }
                            break;

                        case Direction::RIGHT: // From West (D)
                            if (destination == Destination::LEFT) {
                                lane = 'C';  // Go to South
                                laneNumber = 1;
                                currentDirection = Direction::UP;
                                DebugLogger::log("Vehicle " + id + " now on C1 (turned LEFT from D)");
                            }
                            else if (destination == Destination::RIGHT) {
                                lane = 'A';  // Go to North
                                laneNumber = 1;
                                currentDirection = Direction::DOWN;
                                DebugLogger::log("Vehicle " + id + " now on A1 (turned RIGHT from D)");
                            }
                            else {
                                // Going straight to East
                                lane = 'B';
                                laneNumber = 1;
                                DebugLogger::log("Vehicle " + id + " now on B1 (going STRAIGHT from D)");
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

                // Update animation position
                animPos = (currentDirection == Direction::UP || currentDirection == Direction::DOWN) ?
                         turnPosY : turnPosX;
            }

            // Update turn progress for visualization
            if (turning) {
                turnProgress = std::min(1.0f, turnProgress + 0.002f * delta);
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
        // Red light - handle queue positioning
        if (currentWaypoint <= 1) {  // Vehicle is approaching or at the stop line
            // Get the stop line waypoint
            auto& stopLine = waypoints[1];

            // Calculate target position based on queue position
            float queueOffsetDistance = VEHICLE_SPACING * queuePos;
            float queueStopX = stopLine.x;
            float queueStopY = stopLine.y;

            // Adjust target position based on direction of travel
            switch (currentDirection) {
                case Direction::DOWN:  // From North (A)
                    queueStopY -= queueOffsetDistance;
                    break;
                case Direction::UP:    // From South (C)
                    queueStopY += queueOffsetDistance;
                    break;
                case Direction::LEFT:  // From East (B)
                    queueStopX += queueOffsetDistance;
                    break;
                case Direction::RIGHT: // From West (D)
                    queueStopX -= queueOffsetDistance;
                    break;
            }

            // Calculate direction and distance to queue position
            float dx = queueStopX - turnPosX;
            float dy = queueStopY - turnPosY;
            float distance = std::sqrt(dx*dx + dy*dy);

            // Only move if far enough from target position (prevents jitter)
            if (distance > 2.0f) {
                // Normalize direction
                dx /= distance;
                dy /= distance;

                // Move toward queue position
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

void Vehicle::render(SDL_Renderer* renderer, SDL_Texture* vehicleTexture, int queuePos) {
    // Store queue position for use in update method
    this->queuePos = queuePos;

    // ENHANCED VEHICLE VISUALIZATION
    // Set different colors for each lane and destination for clear visual distinction
    SDL_Color color;

    if (isEmergency) {
        // Emergency vehicles are bright red with high visibility
        color = {255, 0, 0, 255}; // Bright red
    }
    else {
        // Base color determined by lane identity
        switch (lane) {
            case 'A': // North Road
                if (laneNumber == 1) color = {100, 149, 237, 255}; // Cornflower Blue for A1
                else if (laneNumber == 2) color = {255, 140, 0, 255}; // Orange for A2 (Priority)
                else color = {50, 205, 50, 255}; // Lime Green for A3 (Free)
                break;

            case 'B': // East Road
                if (laneNumber == 1) color = {75, 0, 130, 255}; // Indigo for B1
                else if (laneNumber == 2) color = {218, 165, 32, 255}; // Goldenrod for B2
                else color = {34, 139, 34, 255}; // Forest Green for B3 (Free)
                break;

            case 'C': // South Road
                if (laneNumber == 1) color = {30, 144, 255, 255}; // Dodger Blue for C1
                else if (laneNumber == 2) color = {210, 105, 30, 255}; // Chocolate for C2
                else color = {60, 179, 113, 255}; // Medium Sea Green for C3 (Free)
                break;

            case 'D': // West Road
                if (laneNumber == 1) color = {138, 43, 226, 255}; // Blue Violet for D1
                else if (laneNumber == 2) color = {205, 133, 63, 255}; // Peru for D2
                else color = {46, 139, 87, 255}; // Sea Green for D3 (Free)
                break;

            default:
                color = {192, 192, 192, 255}; // Silver (default)
                break;
        }
    }

    // Brighter colors when turning for better visibility
    if (turning) {
        color.r = std::min(255, color.r + 40);
        color.g = std::min(255, color.g + 40);
        color.b = std::min(255, color.b + 40);
    }

    // Set color for vehicle body
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    // Enhanced vehicle dimensions for better visibility
    float vehicleWidth = 10.0f;  // Wider for better visibility
    float vehicleLength = 20.0f; // Longer for better visibility

    // Draw vehicle rectangle based on orientation
    SDL_FRect vehicleRect;

    if (turning) {
        // For turning vehicles, adjust dimensions gradually
        float progress = turnProgress;
        float width = vehicleWidth;
        float length = vehicleLength;

        // During turn, gradually change dimensions for smoother appearance
        if (currentDirection == Direction::UP || currentDirection == Direction::DOWN) {
            // Transitioning from vertical to horizontal
            if (destination == Destination::LEFT || destination == Destination::RIGHT) {
                width = vehicleWidth * (1.0f - progress) + vehicleLength * progress;
                length = vehicleLength * (1.0f - progress) + vehicleWidth * progress;
            }
        } else {
            // Transitioning from horizontal to vertical
            if (destination == Destination::LEFT || destination == Destination::RIGHT) {
                width = vehicleLength * (1.0f - progress) + vehicleWidth * progress;
                length = vehicleWidth * (1.0f - progress) + vehicleLength * progress;
            }
        }

        vehicleRect = {turnPosX - width/2, turnPosY - length/2, width, length};
    } else {
        // Non-turning vehicles have fixed orientation based on direction
        switch (currentDirection) {
            case Direction::DOWN:
            case Direction::UP:
                // Vertical roads (taller than wide)
                vehicleRect = {turnPosX - vehicleWidth/2, turnPosY - vehicleLength/2, vehicleWidth, vehicleLength};
                break;
            case Direction::LEFT:
            case Direction::RIGHT:
                // Horizontal roads (wider than tall)
                vehicleRect = {turnPosX - vehicleLength/2, turnPosY - vehicleWidth/2, vehicleLength, vehicleWidth};
                break;
        }
    }

    // Draw the vehicle body
    SDL_RenderFillRect(renderer, &vehicleRect);

    // Draw lane number indicators as distinctive marks
    // This makes it very clear which lane the vehicle is in
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White for indicators

    // Choose indicator position based on vehicle orientation
    float indicatorSize = 3.0f;
    float indicatorSpacing = 6.0f;

    for (int i = 0; i < laneNumber; i++) {
        SDL_FRect indicator;

        switch (currentDirection) {
            case Direction::DOWN:
                indicator = {
                    vehicleRect.x + vehicleRect.w/2 - indicatorSize/2,
                    vehicleRect.y + i * indicatorSpacing + 4.0f,
                    indicatorSize,
                    indicatorSize
                };
                break;
            case Direction::UP:
                indicator = {
                    vehicleRect.x + vehicleRect.w/2 - indicatorSize/2,
                    vehicleRect.y + vehicleRect.h - i * indicatorSpacing - 4.0f - indicatorSize,
                    indicatorSize,
                    indicatorSize
                };
                break;
            case Direction::LEFT:
                indicator = {
                    vehicleRect.x + vehicleRect.w - i * indicatorSpacing - 4.0f - indicatorSize,
                    vehicleRect.y + vehicleRect.h/2 - indicatorSize/2,
                    indicatorSize,
                    indicatorSize
                };
                break;
            case Direction::RIGHT:
                indicator = {
                    vehicleRect.x + i * indicatorSpacing + 4.0f,
                    vehicleRect.y + vehicleRect.h/2 - indicatorSize/2,
                    indicatorSize,
                    indicatorSize
                };
                break;
        }

        SDL_RenderFillRect(renderer, &indicator);
    }

    // Draw destination indicator - very visible arrows showing where vehicle is going
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Bright yellow for direction indicators

    if (destination == Destination::LEFT) {
        // Left turn arrow
        SDL_FPoint arrow[3];

        switch (currentDirection) {
            case Direction::DOWN: // A→D (North to West)
                arrow[0] = {vehicleRect.x, vehicleRect.y + 8.0f}; // Point
                arrow[1] = {vehicleRect.x + 6.0f, vehicleRect.y + 8.0f - 4.0f}; // Top wing
                arrow[2] = {vehicleRect.x + 6.0f, vehicleRect.y + 8.0f + 4.0f}; // Bottom wing
                break;

            case Direction::UP: // C→B (South to East)
                arrow[0] = {vehicleRect.x + vehicleRect.w, vehicleRect.y + vehicleRect.h - 8.0f}; // Point
                arrow[1] = {vehicleRect.x + vehicleRect.w - 6.0f, vehicleRect.y + vehicleRect.h - 8.0f - 4.0f}; // Top wing
                arrow[2] = {vehicleRect.x + vehicleRect.w - 6.0f, vehicleRect.y + vehicleRect.h - 8.0f + 4.0f}; // Bottom wing
                break;

            case Direction::LEFT: // B→A (East to North)
                arrow[0] = {vehicleRect.x + 8.0f, vehicleRect.y}; // Point
                arrow[1] = {vehicleRect.x + 8.0f - 4.0f, vehicleRect.y + 6.0f}; // Left wing
                arrow[2] = {vehicleRect.x + 8.0f + 4.0f, vehicleRect.y + 6.0f}; // Right wing
                break;

            case Direction::RIGHT: // D→C (West to South)
                arrow[0] = {vehicleRect.x + vehicleRect.w - 8.0f, vehicleRect.y + vehicleRect.h}; // Point
                arrow[1] = {vehicleRect.x + vehicleRect.w - 8.0f - 4.0f, vehicleRect.y + vehicleRect.h - 6.0f}; // Left wing
                arrow[2] = {vehicleRect.x + vehicleRect.w - 8.0f + 4.0f, vehicleRect.y + vehicleRect.h - 6.0f}; // Right wing
                break;
        }

        SDL_RenderFillTriangleF(renderer, arrow[0].x, arrow[0].y, arrow[1].x, arrow[1].y, arrow[2].x, arrow[2].y);
    }
    else if (destination == Destination::RIGHT) {
        // Right turn arrow
        SDL_FPoint arrow[3];

        switch (currentDirection) {
            case Direction::DOWN: // A→B (North to East)
                arrow[0] = {vehicleRect.x + vehicleRect.w, vehicleRect.y + 8.0f}; // Point
                arrow[1] = {vehicleRect.x + vehicleRect.w - 6.0f, vehicleRect.y + 8.0f - 4.0f}; // Top wing
                arrow[2] = {vehicleRect.x + vehicleRect.w - 6.0f, vehicleRect.y + 8.0f + 4.0f}; // Bottom wing
                break;

            case Direction::UP: // C→D (South to West)
                arrow[0] = {vehicleRect.x, vehicleRect.y + vehicleRect.h - 8.0f}; // Point
                arrow[1] = {vehicleRect.x + 6.0f, vehicleRect.y + vehicleRect.h - 8.0f - 4.0f}; // Top wing
                arrow[2] = {vehicleRect.x + 6.0f, vehicleRect.y + vehicleRect.h - 8.0f + 4.0f}; // Bottom wing
                break;

            case Direction::LEFT: // B→C (East to South)
                arrow[0] = {vehicleRect.x + 8.0f, vehicleRect.y + vehicleRect.h}; // Point
                arrow[1] = {vehicleRect.x + 8.0f - 4.0f, vehicleRect.y + vehicleRect.h - 6.0f}; // Left wing
                arrow[2] = {vehicleRect.x + 8.0f + 4.0f, vehicleRect.y + vehicleRect.h - 6.0f}; // Right wing
                break;

            case Direction::RIGHT: // D→A (West to North)
                arrow[0] = {vehicleRect.x + vehicleRect.w - 8.0f, vehicleRect.y}; // Point
                arrow[1] = {vehicleRect.x + vehicleRect.w - 8.0f - 4.0f, vehicleRect.y + 6.0f}; // Left wing
                arrow[2] = {vehicleRect.x + vehicleRect.w - 8.0f + 4.0f, vehicleRect.y + 6.0f}; // Right wing
                break;
        }

        SDL_RenderFillTriangleF(renderer, arrow[0].x, arrow[0].y, arrow[1].x, arrow[1].y, arrow[2].x, arrow[2].y);
    }
    else {
        // Straight indicator (double parallel lines)
        SDL_FRect lineA, lineB;
        float lineWidth = 2.0f;
        float lineLength = 8.0f;
        float lineSpacing = 4.0f;

        switch (currentDirection) {
            case Direction::DOWN:
                lineA = {vehicleRect.x + vehicleRect.w/3, vehicleRect.y + 5.0f, lineWidth, lineLength};
                lineB = {vehicleRect.x + 2*vehicleRect.w/3, vehicleRect.y + 5.0f, lineWidth, lineLength};
                break;
            case Direction::UP:
                lineA = {vehicleRect.x + vehicleRect.w/3, vehicleRect.y + vehicleRect.h - 5.0f - lineLength, lineWidth, lineLength};
                lineB = {vehicleRect.x + 2*vehicleRect.w/3, vehicleRect.y + vehicleRect.h - 5.0f - lineLength, lineWidth, lineLength};
                break;
            case Direction::LEFT:
                lineA = {vehicleRect.x + vehicleRect.w - 5.0f - lineLength, vehicleRect.y + vehicleRect.h/3, lineLength, lineWidth};
                lineB = {vehicleRect.x + vehicleRect.w - 5.0f - lineLength, vehicleRect.y + 2*vehicleRect.h/3, lineLength, lineWidth};
                break;
            case Direction::RIGHT:
                lineA = {vehicleRect.x + 5.0f, vehicleRect.y + vehicleRect.h/3, lineLength, lineWidth};
                lineB = {vehicleRect.x + 5.0f, vehicleRect.y + 2*vehicleRect.h/3, lineLength, lineWidth};
                break;
        }

        SDL_RenderFillRect(renderer, &lineA);
        SDL_RenderFillRect(renderer, &lineB);
    }

    // Draw lane ID text (letter+number)
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White
    // drawText not implemented - would need custom text rendering

    // We can draw a lane identifier with points/shapes instead
    if (isEmergency) {
        // Draw a cross symbol for emergency vehicles
        float crossSize = 6.0f;
        SDL_FRect crossV, crossH;

        // Position at center of vehicle
        crossH = {turnPosX - crossSize/2, turnPosY - 1.0f, crossSize, 2.0f};
        crossV = {turnPosX - 1.0f, turnPosY - crossSize/2, 2.0f, crossSize};

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White
        SDL_RenderFillRect(renderer, &crossH);
        SDL_RenderFillRect(renderer, &crossV);
    }
}

// Helper for drawing triangles (SDL3 compatible)
void Vehicle::SDL_RenderFillTriangleF(SDL_Renderer* renderer, float x1, float y1, float x2, float y2, float x3, float y3) {
    // Convert to array of SDL_Vertex for SDL_RenderGeometry
    SDL_Vertex vertices[3];

    // Create a color in SDL_FColor format
    SDL_FColor fcolor = {
        1.0f,  // r (normalized to 0.0-1.0)
        1.0f,  // g
        1.0f,  // b
        1.0f   // a
    };

    // First vertex
    vertices[0].position.x = x1;
    vertices[0].position.y = y1;
    vertices[0].color = fcolor;

    // Second vertex
    vertices[1].position.x = x2;
    vertices[1].position.y = y2;
    vertices[1].color = fcolor;

    // Third vertex
    vertices[2].position.x = x3;
    vertices[2].position.y = y3;
    vertices[2].color = fcolor;

    // Draw the triangle
    SDL_RenderGeometry(renderer, NULL, vertices, 3, NULL, 0);
}
