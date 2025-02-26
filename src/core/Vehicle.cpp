#include "core/Vehicle.h"
#include "utils/DebugLogger.h"
#include <sstream>
#include <cmath>
#include <algorithm>

Vehicle::Vehicle(const std::string& id, char lane, int laneNumber, bool isEmergency)
    : id(id.empty() ? "V_unknown" : id),
      lane(lane),
      laneNumber(std::clamp(laneNumber, 1, 3)),  // Ensure lane number is between 1-3
      isEmergency(isEmergency),
      arrivalTime(std::time(nullptr)),
      animPos(0.0f),
      turning(false),
      turnProgress(0.0f),
      turnPosX(0.0f),
      turnPosY(0.0f) {

    // Initialize animation position based on lane
    if (lane == 'A') {
        animPos = 0.0f; // Top of screen
    } else if (lane == 'B') {
        animPos = 800.0f; // Bottom of screen (assuming 800 height)
    } else if (lane == 'C') {
        animPos = 800.0f; // Right of screen (assuming 800 width)
    } else if (lane == 'D') {
        animPos = 0.0f; // Left of screen
    } else {
        // Invalid lane, default to A
        this->lane = 'A';
        animPos = 0.0f;
        DebugLogger::log("Invalid lane provided: " + std::string(1, lane) + ", defaulting to A", DebugLogger::LogLevel::WARNING);
    }

    std::stringstream ss;
    ss << "Created vehicle " << id << " on lane " << this->lane << this->laneNumber
       << (isEmergency ? " (Emergency)" : "");
    DebugLogger::log(ss.str());
}

Vehicle::~Vehicle() {}

std::string Vehicle::getId() const {
    return id;
}

char Vehicle::getLane() const {
    return lane;
}

void Vehicle::setLane(char newLane) {
    if (newLane >= 'A' && newLane <= 'D') {
        lane = newLane;
    } else {
        DebugLogger::log("Attempt to set invalid lane: " + std::string(1, newLane), DebugLogger::LogLevel::WARNING);
    }
}

int Vehicle::getLaneNumber() const {
    return laneNumber;
}

void Vehicle::setLaneNumber(int number) {
    if (number >= 1 && number <= 3) {
        laneNumber = number;
    } else {
        DebugLogger::log("Attempt to set invalid lane number: " + std::to_string(number), DebugLogger::LogLevel::WARNING);
    }
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
    animPos = pos;
}

bool Vehicle::isTurning() const {
    return turning;
}

void Vehicle::setTurning(bool isTurning) {
    turning = isTurning;
}

float Vehicle::getTurnProgress() const {
    return turnProgress;
}

void Vehicle::setTurnProgress(float progress) {
    turnProgress = std::clamp(progress, 0.0f, 1.0f);  // Ensure progress is between 0-1
}

float Vehicle::getTurnPosX() const {
    return turnPosX;
}

void Vehicle::setTurnPosX(float x) {
    turnPosX = x;
}

float Vehicle::getTurnPosY() const {
    return turnPosY;
}

void Vehicle::setTurnPosY(float y) {
    turnPosY = y;
}

void Vehicle::update(uint32_t delta, bool isGreenLight, float targetPos) {
    // Guard against numerical issues with delta (if it's extremely large)
    if (delta > 1000) {
        delta = 1000;  // Cap to a reasonable maximum value
    }

    // If the vehicle is turning, update turn progress
    if (turning) {
        turnProgress += delta * TURN_SPEED;

        // Cap turn progress at 1.0
        if (turnProgress > 1.0f) {
            turnProgress = 1.0f;
        }

        // If turn is complete, reset turning state
        if (turnProgress >= 1.0f) {
            turning = false;
            turnProgress = 0.0f;

            // Update lane and position based on turn destination
            // This would be handled by the TrafficManager in the complete implementation
        }

        return; // Skip normal movement while turning
    }

    // Normal movement logic
    float moveDistance = MOVE_SPEED * delta;

    // Vehicle movement depends on lane and traffic light
    if (lane == 'A') {
        // Moving down from top
        if (isGreenLight || animPos > 400) { // Past the intersection or green light
            animPos += moveDistance;
        } else {
            // Approaching red light, stop at intersection
            float stopPos = 280; // Stop position for lane A
            if (animPos < stopPos) {
                animPos = std::min(animPos + moveDistance, stopPos);
            }
        }
    } else if (lane == 'B') {
        // Moving up from bottom
        if (isGreenLight || animPos < 400) { // Past the intersection or green light
            animPos -= moveDistance;
        } else {
            // Approaching red light, stop at intersection
            float stopPos = 520; // Stop position for lane B
            if (animPos > stopPos) {
                animPos = std::max(animPos - moveDistance, stopPos);
            }
        }
    } else if (lane == 'C') {
        // Moving left from right
        if (isGreenLight || animPos < 400) { // Past the intersection or green light
            animPos -= moveDistance;
        } else {
            // Approaching red light, stop at intersection
            float stopPos = 520; // Stop position for lane C
            if (animPos > stopPos) {
                animPos = std::max(animPos - moveDistance, stopPos);
            }
        }
    } else if (lane == 'D') {
        // Moving right from left
        if (isGreenLight || animPos > 400) { // Past the intersection or green light
            animPos += moveDistance;
        } else {
            // Approaching red light, stop at intersection
            float stopPos = 280; // Stop position for lane D
            if (animPos < stopPos) {
                animPos = std::min(animPos + moveDistance, stopPos);
            }
        }
    }
}

void Vehicle::render(SDL_Renderer* renderer, SDL_Texture* vehicleTexture, int queuePos) {
    if (!renderer) {
        DebugLogger::log("Renderer is null in Vehicle::render", DebugLogger::LogLevel::ERROR);
        return;
    }

    int x = 0, y = 0;
    int w = static_cast<int>(VEHICLE_LENGTH);
    int h = static_cast<int>(VEHICLE_WIDTH);

    // Position the vehicle based on lane and animation position
    if (turning) {
        // Use turning coordinates if the vehicle is turning
        x = static_cast<int>(turnPosX);
        y = static_cast<int>(turnPosY);
    } else {
        if (lane == 'A') {
            // Road A (North to South)
            int offsetX = (laneNumber == 1) ? -50 : (laneNumber == 3) ? 50 : 0;
            x = 400 - w/2 + offsetX; // Center of lane
            y = static_cast<int>(animPos);
        } else if (lane == 'B') {
            // Road B (South to North)
            int offsetX = (laneNumber == 1) ? 50 : (laneNumber == 3) ? -50 : 0;
            x = 400 - w/2 + offsetX;
            y = static_cast<int>(animPos);
        } else if (lane == 'C') {
            // Road C (East to West)
            int offsetY = (laneNumber == 1) ? -50 : (laneNumber == 3) ? 50 : 0;
            x = static_cast<int>(animPos);
            y = 400 - h/2 + offsetY;
        } else if (lane == 'D') {
            // Road D (West to East)
            int offsetY = (laneNumber == 1) ? 50 : (laneNumber == 3) ? -50 : 0;
            x = static_cast<int>(animPos);
            y = 400 - h/2 + offsetY;
        }
    }

    // Use the vehicle texture with appropriate rotation based on lane
    if (vehicleTexture) {
        SDL_FRect destRect = {static_cast<float>(x), static_cast<float>(y),
                             static_cast<float>(w), static_cast<float>(h)};

        // Set rotation angle based on lane
        double angle = 0.0;
        if (lane == 'A') angle = 180.0;       // Facing down
        else if (lane == 'B') angle = 0.0;    // Facing up
        else if (lane == 'C') angle = 90.0;   // Facing left
        else if (lane == 'D') angle = 270.0;  // Facing right

        // Set different color for emergency vehicles
        if (isEmergency) {
            SDL_SetTextureColorMod(vehicleTexture, 255, 0, 0); // Red tint for emergency
        } else {
            SDL_SetTextureColorMod(vehicleTexture, 255, 255, 255); // Normal color
        }

        // Render the vehicle
        SDL_RenderTexture(renderer, vehicleTexture, nullptr, &destRect);
    } else {
        // Fallback to rectangle rendering if texture is unavailable
        if (isEmergency) {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red for emergency
        } else {
            SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // Blue for normal
        }

        SDL_FRect rect = {static_cast<float>(x), static_cast<float>(y),
                         static_cast<float>(w), static_cast<float>(h)};
        SDL_RenderFillRect(renderer, &rect);
    }
}

void Vehicle::calculateTurnPath(float startX, float startY, float controlX, float controlY,
                              float endX, float endY, float progress) {
    float t = easeInOutQuad(progress);

    // Quadratic Bezier curve calculation
    turnPosX = (1-t)*(1-t)*startX + 2*(1-t)*t*controlX + t*t*endX;
    turnPosY = (1-t)*(1-t)*startY + 2*(1-t)*t*controlY + t*t*endY;
}

float Vehicle::easeInOutQuad(float t) const {
    t = std::clamp(t, 0.0f, 1.0f);  // Ensure t is between 0-1
    return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
}
