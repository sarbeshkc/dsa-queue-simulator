#include "core/TrafficLight.h"
#include "utils/DebugLogger.h"
#include <sstream>
#include <cmath>
#include <SDL3/SDL.h>

TrafficLight::TrafficLight()
    : currentState(State::ALL_RED),
      nextState(State::A_GREEN),
      lastStateChangeTime(SDL_GetTicks()),
      isPriorityMode(false),
      shouldResumeNormalMode(false) {

    DebugLogger::log("TrafficLight initialized");
}

TrafficLight::~TrafficLight() {
    DebugLogger::log("TrafficLight destroyed");
}

void TrafficLight::update(const std::vector<Lane*>& lanes) {
    uint32_t currentTime = SDL_GetTicks();
    uint32_t elapsedTime = currentTime - lastStateChangeTime;

    // Find AL2 lane to check priority
    Lane* al2Lane = nullptr;
    for (auto* lane : lanes) {
        if (lane->getLaneId() == 'A' && lane->getLaneNumber() == 2) {
            al2Lane = lane;
            break;
        }
    }

    // Check priority conditions
    if (al2Lane) {
        int vehicleCount = al2Lane->getVehicleCount();

        // Enter priority mode
        if (!isPriorityMode && vehicleCount > 10) { // PRIORITY_THRESHOLD_HIGH
            isPriorityMode = true;
            std::ostringstream oss;
            oss << "Priority mode activated: A2 has " << vehicleCount << " vehicles (>10)";
            DebugLogger::log(oss.str());

            // If not already serving A, switch to it
            if (currentState != State::A_GREEN) {
                nextState = State::ALL_RED; // Next go to all red
            }
        }
        // Exit priority mode
        else if (isPriorityMode && vehicleCount < 5) { // PRIORITY_THRESHOLD_LOW
            shouldResumeNormalMode = true;
            std::ostringstream oss;
            oss << "Priority mode deactivated: A2 now has " << vehicleCount << " vehicles (<5)";
            DebugLogger::log(oss.str());
        }
    }

    // Calculate appropriate duration based on the formula from the assignment
    int stateDuration;
    if (currentState == State::ALL_RED) {
        stateDuration = allRedDuration; // 2 seconds for ALL_RED
    } else {
        // Formula: |V| = (1/n) * Σ|Li| for normal lanes
        int normalLaneCount = 0;
        int totalVehicleCount = 0;

        for (auto* lane : lanes) {
            // Only count normal lanes (lane 2) across all roads
            if (lane->getLaneNumber() == 2) {
                normalLaneCount++;
                totalVehicleCount += lane->getVehicleCount();
            }
        }

        // Calculate average vehicle count as per formula
        float averageVehicleCount = (normalLaneCount > 0) ?
            static_cast<float>(totalVehicleCount) / normalLaneCount : 0.0f;

        // Set the duration based on the formula: Total time = |V| * t
        // where t is 2 seconds per vehicle
        stateDuration = static_cast<int>(averageVehicleCount * 2000);

        // Apply minimum and maximum limits
        if (stateDuration < 3000) stateDuration = 3000; // Min 3 seconds
        if (stateDuration > 15000) stateDuration = 15000; // Max 15 seconds

        // Debug log
        std::ostringstream oss;
        oss << "Traffic light timing: |V| = " << averageVehicleCount
            << ", Duration = " << stateDuration / 1000.0f << " seconds";
        DebugLogger::log(oss.str());
    }

    // State transition
    if (elapsedTime >= stateDuration) {
        // Change state
        currentState = nextState;

        // Determine next state
        if (isPriorityMode && !shouldResumeNormalMode) {
            // In priority mode, alternate between A_GREEN and ALL_RED
            if (currentState == State::ALL_RED) {
                nextState = State::A_GREEN;
            } else {
                nextState = State::ALL_RED;
            }
        } else {
            // If we should exit priority mode, do it now
            if (shouldResumeNormalMode) {
                isPriorityMode = false;
                shouldResumeNormalMode = false;
                DebugLogger::log("Resuming normal traffic light sequence");
            }

            // In normal mode, rotate through all lanes
            if (currentState == State::ALL_RED) {
                // Determine which green state to go to next
                switch (nextState) {
                    case State::A_GREEN: nextState = State::B_GREEN; break;
                    case State::B_GREEN: nextState = State::C_GREEN; break;
                    case State::C_GREEN: nextState = State::D_GREEN; break;
                    case State::D_GREEN: nextState = State::A_GREEN; break;
                    default: nextState = State::A_GREEN; break;
                }
            } else {
                // Any green state goes to ALL_RED next
                nextState = State::ALL_RED;
            }
        }

        // Log state change
        std::ostringstream oss;
        oss << "Traffic light changed to state: " << static_cast<int>(currentState);
        DebugLogger::log(oss.str());

        lastStateChangeTime = currentTime;
    }
}

void TrafficLight::render(SDL_Renderer* renderer) {
    // Draw traffic light for each road
    drawLightForA(renderer, !isGreen('A'));
    drawLightForB(renderer, !isGreen('B'));
    drawLightForC(renderer, !isGreen('C'));
    drawLightForD(renderer, !isGreen('D'));

    // Priority mode indicator
    if (isPriorityMode) {
        SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255); // Orange
        SDL_FRect priorityIndicator = {10, 10, 30, 30};
        SDL_RenderFillRect(renderer, &priorityIndicator);

        // Black border
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderRect(renderer, &priorityIndicator);
    }
}

void TrafficLight::setNextState(State state) {
    nextState = state;
}

bool TrafficLight::isGreen(char lane) const {
    switch (lane) {
        case 'A': return currentState == State::A_GREEN;
        case 'B': return currentState == State::B_GREEN;
        case 'C': return currentState == State::C_GREEN;
        case 'D': return currentState == State::D_GREEN;
        default: return false;
    }
}

void TrafficLight::drawLightForA(SDL_Renderer* renderer, bool isRed) {
    const int LIGHT_SIZE = 20;
    const int LIGHT_BOX_WIDTH = 30;
    const int LIGHT_BOX_HEIGHT = 65;
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 800;

    // Position for road A - make it more visible
    int x = WINDOW_WIDTH/2 + 60; // Further out from intersection
    int y = WINDOW_HEIGHT/2 - 140;

    // Add shadow for 3D effect
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 150);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_FRect shadowBox = {(float)x + 3, (float)y + 3, LIGHT_BOX_WIDTH, LIGHT_BOX_HEIGHT};
    SDL_RenderFillRect(renderer, &shadowBox);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    // Traffic light box
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255); // Darker gray for better contrast
    SDL_FRect lightBox = {(float)x, (float)y, LIGHT_BOX_WIDTH, LIGHT_BOX_HEIGHT};
    SDL_RenderFillRect(renderer, &lightBox);

    // Black border
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &lightBox);

    // Red light with glow effect when active
    if (isRed) {
        // Active red light with glow
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 70);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_FRect redGlow = {(float)(x + 5) - 3, (float)(y + 10) - 3, LIGHT_SIZE + 6, LIGHT_SIZE + 6};
        SDL_RenderFillRect(renderer, &redGlow);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Bright red
    } else {
        SDL_SetRenderDrawColor(renderer, 80, 0, 0, 255); // Dark red
    }
    SDL_FRect redLight = {(float)(x + 5), (float)(y + 10), LIGHT_SIZE, LIGHT_SIZE};
    SDL_RenderFillRect(renderer, &redLight);

    // Green light with glow effect when active
    if (!isRed) {
        // Active green light with glow
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 70);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_FRect greenGlow = {(float)(x + 5) - 3, (float)(y + 35) - 3, LIGHT_SIZE + 6, LIGHT_SIZE + 6};
        SDL_RenderFillRect(renderer, &greenGlow);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Bright green
    } else {
        SDL_SetRenderDrawColor(renderer, 0, 80, 0, 255); // Dark green
    }
    SDL_FRect greenLight = {(float)(x + 5), (float)(y + 35), LIGHT_SIZE, LIGHT_SIZE};
    SDL_RenderFillRect(renderer, &greenLight);

    // Black borders around lights
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &redLight);
    SDL_RenderRect(renderer, &greenLight);

    // Add a small 'A' label
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_FRect labelBox = {(float)(x + LIGHT_BOX_WIDTH/2 - 5), (float)(y - 15), 10, 10};
    SDL_RenderFillRect(renderer, &labelBox);
}

void TrafficLight::drawLightForB(SDL_Renderer* renderer, bool isRed) {
    const int LIGHT_SIZE = 20;
    const int LIGHT_BOX_WIDTH = 30;
    const int LIGHT_BOX_HEIGHT = 65;
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 800;

    // Position for road B
    int x = WINDOW_WIDTH/2 - 90; // Further out from intersection
    int y = WINDOW_HEIGHT/2 + 75;

    // Add shadow for 3D effect
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 150);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_FRect shadowBox = {(float)x + 3, (float)y + 3, LIGHT_BOX_WIDTH, LIGHT_BOX_HEIGHT};
    SDL_RenderFillRect(renderer, &shadowBox);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    // Traffic light box
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_FRect lightBox = {(float)x, (float)y, LIGHT_BOX_WIDTH, LIGHT_BOX_HEIGHT};
    SDL_RenderFillRect(renderer, &lightBox);

    // Black border
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &lightBox);

    // Red light with glow effect when active
    if (isRed) {
        // Active red light with glow
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 70);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_FRect redGlow = {(float)(x + 5) - 3, (float)(y + 10) - 3, LIGHT_SIZE + 6, LIGHT_SIZE + 6};
        SDL_RenderFillRect(renderer, &redGlow);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 80, 0, 0, 255);
    }
    SDL_FRect redLight = {(float)(x + 5), (float)(y + 10), LIGHT_SIZE, LIGHT_SIZE};
    SDL_RenderFillRect(renderer, &redLight);

    // Green light with glow effect when active
    if (!isRed) {
        // Active green light with glow
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 70);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_FRect greenGlow = {(float)(x + 5) - 3, (float)(y + 35) - 3, LIGHT_SIZE + 6, LIGHT_SIZE + 6};
        SDL_RenderFillRect(renderer, &greenGlow);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 0, 80, 0, 255);
    }
    SDL_FRect greenLight = {(float)(x + 5), (float)(y + 35), LIGHT_SIZE, LIGHT_SIZE};
    SDL_RenderFillRect(renderer, &greenLight);

    // Black borders around lights
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &redLight);
    SDL_RenderRect(renderer, &greenLight);

    // Add a small 'B' label
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_FRect labelBox = {(float)(x + LIGHT_BOX_WIDTH/2 - 5), (float)(y - 15), 10, 10};
    SDL_RenderFillRect(renderer, &labelBox);
}


void TrafficLight::drawLightForC(SDL_Renderer* renderer, bool isRed) {
    const int LIGHT_SIZE = 20;
    const int LIGHT_BOX_WIDTH = 60;
    const int LIGHT_BOX_HEIGHT = 25;
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 800;

    // Position for road C
    int x = WINDOW_WIDTH/2 + 60;
    int y = WINDOW_HEIGHT/2 - 65;

    // Traffic light box
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255); // Dark gray
    SDL_FRect lightBox = {(float)x, (float)y, LIGHT_BOX_WIDTH, LIGHT_BOX_HEIGHT};
    SDL_RenderFillRect(renderer, &lightBox);

    // Black border
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &lightBox);

    // Red light
    SDL_SetRenderDrawColor(renderer, isRed ? 255 : 80, 0, 0, 255);
    SDL_FRect redLight = {(float)(x + 5), (float)(y + 2.5), LIGHT_SIZE, LIGHT_SIZE};
    SDL_RenderFillRect(renderer, &redLight);

    // Green light
    SDL_SetRenderDrawColor(renderer, 0, isRed ? 80 : 255, 0, 255);
    SDL_FRect greenLight = {(float)(x + 35), (float)(y + 2.5), LIGHT_SIZE, LIGHT_SIZE};
    SDL_RenderFillRect(renderer, &greenLight);

    // Black borders around lights
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &redLight);
    SDL_RenderRect(renderer, &greenLight);
}

void TrafficLight::drawLightForD(SDL_Renderer* renderer, bool isRed) {
    const int LIGHT_SIZE = 20;
    const int LIGHT_BOX_WIDTH = 60;
    const int LIGHT_BOX_HEIGHT = 25;
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 800;

    // Position for road D
    int x = WINDOW_WIDTH/2 - 120;
    int y = WINDOW_HEIGHT/2 + 40;

    // Traffic light box
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255); // Dark gray
    SDL_FRect lightBox = {(float)x, (float)y, LIGHT_BOX_WIDTH, LIGHT_BOX_HEIGHT};
    SDL_RenderFillRect(renderer, &lightBox);

    // Black border
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &lightBox);

    // Red light
    SDL_SetRenderDrawColor(renderer, isRed ? 255 : 80, 0, 0, 255);
    SDL_FRect redLight = {(float)(x + 5), (float)(y + 2.5), LIGHT_SIZE, LIGHT_SIZE};
    SDL_RenderFillRect(renderer, &redLight);

    // Green light
    SDL_SetRenderDrawColor(renderer, 0, isRed ? 80 : 255, 0, 255);
    SDL_FRect greenLight = {(float)(x + 35), (float)(y + 2.5), LIGHT_SIZE, LIGHT_SIZE};
    SDL_RenderFillRect(renderer, &greenLight);

    // Black borders around lights
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &redLight);
    SDL_RenderRect(renderer, &greenLight);
}
