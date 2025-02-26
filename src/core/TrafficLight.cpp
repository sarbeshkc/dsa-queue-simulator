// FILE: src/core/TrafficLight.cpp
#include "core/TrafficLight.h"
#include "utils/DebugLogger.h"
#include <sstream>
#include <cmath>
#include <SDL3/SDL.h>
#include "core/Constants.h"

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

    // Check priority conditions with improved threshold handling
    if (al2Lane) {
        int vehicleCount = al2Lane->getVehicleCount();

        // Enter priority mode when vehicles > PRIORITY_THRESHOLD_HIGH
        if (!isPriorityMode && vehicleCount > Constants::PRIORITY_THRESHOLD_HIGH) {
            isPriorityMode = true;
            std::ostringstream oss;
            oss << "PRIORITY MODE ACTIVATED: A2 has " << vehicleCount << " vehicles";
            DebugLogger::log(oss.str(), DebugLogger::LogLevel::INFO);

            // Force transition to A_GREEN via ALL_RED if not already on A
            if (currentState != State::A_GREEN) {
                nextState = State::ALL_RED;
                // Force state change immediately if already at ALL_RED
                if (currentState == State::ALL_RED) {
                    lastStateChangeTime = 0; // Force duration elapsed
                }
            }
        }
        // Exit priority mode only when vehicles < PRIORITY_THRESHOLD_LOW
        else if (isPriorityMode && vehicleCount < Constants::PRIORITY_THRESHOLD_LOW) {
            shouldResumeNormalMode = true;
            std::ostringstream oss;
            oss << "PRIORITY MODE DEACTIVATED: A2 now has " << vehicleCount << " vehicles";
            DebugLogger::log(oss.str(), DebugLogger::LogLevel::INFO);
        }
    }

    // Calculate appropriate duration based on assignment formula
    int stateDuration;
    if (currentState == State::ALL_RED) {
        stateDuration = allRedDuration; // 2 seconds for ALL_RED
    } else {
        // Calculate average using lane counts
        float averageVehicleCount = calculateAverageVehicleCount(lanes);

        // Set duration using formula: Total time = |V| * t (2 seconds per vehicle)
        stateDuration = static_cast<int>(averageVehicleCount * 2000);

        // Apply minimum and maximum limits for reasonable times
        if (stateDuration < 3000) stateDuration = 3000; // Min 3 seconds
        if (stateDuration > 15000) stateDuration = 15000; // Max 15 seconds

        // Log the calculation
        std::ostringstream oss;
        oss << "Traffic light timing: |V| = " << averageVehicleCount
            << ", Duration = " << stateDuration / 1000.0f << " seconds";
        DebugLogger::log(oss.str());
    }

    // State transition with improved priority handling
    if (elapsedTime >= stateDuration) {
        // Change to next state
        currentState = nextState;

        // Handle priority mode properly
        if (isPriorityMode && !shouldResumeNormalMode) {
            // In priority mode: alternate between A_GREEN and ALL_RED only
            if (currentState == State::ALL_RED) {
                nextState = State::A_GREEN;
            } else {
                nextState = State::ALL_RED;
            }
        } else {
            // Exit priority mode if needed
            if (shouldResumeNormalMode) {
                isPriorityMode = false;
                shouldResumeNormalMode = false;
                DebugLogger::log("Resuming normal traffic light sequence");
            }

            // Normal rotation pattern: ALL_RED → A → ALL_RED → B → ALL_RED → C → ALL_RED → D → ...
            if (currentState == State::ALL_RED) {
                // Cycle through green states
                switch (nextState) {
                    case State::A_GREEN: nextState = State::B_GREEN; break;
                    case State::B_GREEN: nextState = State::C_GREEN; break;
                    case State::C_GREEN: nextState = State::D_GREEN; break;
                    case State::D_GREEN: nextState = State::A_GREEN; break;
                    default: nextState = State::A_GREEN; break;
                }
            } else {
                // Always go to ALL_RED after any green state
                nextState = State::ALL_RED;
            }
        }

        // Log state change clearly
        std::string stateStr;
        switch (currentState) {
            case State::ALL_RED: stateStr = "ALL_RED"; break;
            case State::A_GREEN: stateStr = "A_GREEN"; break;
            case State::B_GREEN: stateStr = "B_GREEN"; break;
            case State::C_GREEN: stateStr = "C_GREEN"; break;
            case State::D_GREEN: stateStr = "D_GREEN"; break;
        }

        std::ostringstream oss;
        oss << "Traffic light changed to: " << stateStr
            << (isPriorityMode ? " (PRIORITY MODE)" : "");
        DebugLogger::log(oss.str());

        lastStateChangeTime = currentTime;
    }
}

// Calculate average vehicle count with proper formula
float TrafficLight::calculateAverageVehicleCount(const std::vector<Lane*>& lanes) {
    int normalLaneCount = 0;
    int totalVehicleCount = 0;

    for (auto* lane : lanes) {
        // Only count lane 2 (normal lanes)
        // In priority mode, exclude the priority lane (A2) from calculation
        if (lane->getLaneNumber() == 2 &&
            !(isPriorityMode && lane->getLaneId() == 'A')) {
            normalLaneCount++;
            totalVehicleCount += lane->getVehicleCount();
        }
    }

    // Calculate average: |V| = (1/n) * Σ|Li|
    float average = (normalLaneCount > 0) ?
        static_cast<float>(totalVehicleCount) / normalLaneCount : 0.0f;

    // Return at least 1 to ensure some duration
    return std::max(1.0f, average);
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

void TrafficLight::drawLightForA(SDL_Renderer* renderer, bool isRed) {
    const int LIGHT_SIZE = 20;
    const int LIGHT_BOX_WIDTH = 25;
    const int LIGHT_BOX_HEIGHT = 60;
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 800;

    // Position for road A
    int x = WINDOW_WIDTH/2 + 40;
    int y = WINDOW_HEIGHT/2 - 120;

    // Traffic light box
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255); // Dark gray
    SDL_FRect lightBox = {(float)x, (float)y, LIGHT_BOX_WIDTH, LIGHT_BOX_HEIGHT};
    SDL_RenderFillRect(renderer, &lightBox);

    // Black border
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &lightBox);

    // Red light
    SDL_SetRenderDrawColor(renderer, isRed ? 255 : 80, 0, 0, 255);
    SDL_FRect redLight = {(float)(x + 2.5), (float)(y + 5), LIGHT_SIZE, LIGHT_SIZE};
    SDL_RenderFillRect(renderer, &redLight);

    // Green light
    SDL_SetRenderDrawColor(renderer, 0, isRed ? 80 : 255, 0, 255);
    SDL_FRect greenLight = {(float)(x + 2.5), (float)(y + 35), LIGHT_SIZE, LIGHT_SIZE};
    SDL_RenderFillRect(renderer, &greenLight);

    // Black borders around lights
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &redLight);
    SDL_RenderRect(renderer, &greenLight);
}

void TrafficLight::drawLightForB(SDL_Renderer* renderer, bool isRed) {
    const int LIGHT_SIZE = 20;
    const int LIGHT_BOX_WIDTH = 25;
    const int LIGHT_BOX_HEIGHT = 60;
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 800;

    // Position for road B
    int x = WINDOW_WIDTH/2 - 65;
    int y = WINDOW_HEIGHT/2 + 60;

    // Traffic light box
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255); // Dark gray
    SDL_FRect lightBox = {(float)x, (float)y, LIGHT_BOX_WIDTH, LIGHT_BOX_HEIGHT};
    SDL_RenderFillRect(renderer, &lightBox);

    // Black border
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &lightBox);

    // Red light
    SDL_SetRenderDrawColor(renderer, isRed ? 255 : 80, 0, 0, 255);
    SDL_FRect redLight = {(float)(x + 2.5), (float)(y + 5), LIGHT_SIZE, LIGHT_SIZE};
    SDL_RenderFillRect(renderer, &redLight);

    // Green light
    SDL_SetRenderDrawColor(renderer, 0, isRed ? 80 : 255, 0, 255);
    SDL_FRect greenLight = {(float)(x + 2.5), (float)(y + 35), LIGHT_SIZE, LIGHT_SIZE};
    SDL_RenderFillRect(renderer, &greenLight);

    // Black borders around lights
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &redLight);
    SDL_RenderRect(renderer, &greenLight);
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
