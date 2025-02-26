#include "core/TrafficLight.h"
#include "utils/DebugLogger.h"
#include <algorithm>
#include <iostream>

// Define the threshold constants if not defined in Constants.h
#define PRIORITY_THRESHOLD_HIGH 10
#define PRIORITY_THRESHOLD_LOW 5

TrafficLight::TrafficLight()
    : currentState(State::ALL_RED),
      nextState(State::ALL_RED),
      lastStateChangeTime(SDL_GetTicks()),
      isPriorityMode(false),
      shouldResumeNormalMode(false) {}

TrafficLight::~TrafficLight() {}

void TrafficLight::update(const std::vector<Lane*>& lanes) {
    uint32_t currentTime = SDL_GetTicks();
    uint32_t elapsedTime = currentTime - lastStateChangeTime;

    // Check if AL2 has more than 10 vehicles (priority condition)
    Lane* laneAL2 = nullptr;
    for (auto lane : lanes) {
        if (lane->getLaneId() == 'A' && lane->getLaneNumber() == 2) {
            laneAL2 = lane;
            break;
        }
    }

    // Priority mode logic
    if (laneAL2 != nullptr) {
        int vehicleCount = laneAL2->getVehicleCount();

        // Enter priority mode if AL2 has more than 10 vehicles
        if (!isPriorityMode && vehicleCount > PRIORITY_THRESHOLD_HIGH) {
            isPriorityMode = true;
            shouldResumeNormalMode = false;
            DebugLogger::log("Entering priority mode for AL2 with " + std::to_string(vehicleCount) + " vehicles");

            // Set next state to A_GREEN to prioritize AL2
            if (currentState != State::A_GREEN) {
                nextState = State::A_GREEN;
            }
        }
        // Exit priority mode if AL2 has fewer than 5 vehicles
        else if (isPriorityMode && vehicleCount < PRIORITY_THRESHOLD_LOW) {
            isPriorityMode = false;
            shouldResumeNormalMode = true;
            DebugLogger::log("Exiting priority mode, AL2 now has " + std::to_string(vehicleCount) + " vehicles");
        }
    }

    // Calculate green duration based on vehicle count
    int duration = (currentState == State::ALL_RED) ? allRedDuration : greenDuration;

    // If in priority mode, keep A green for longer proportional to vehicle count
    if (isPriorityMode && currentState == State::A_GREEN && laneAL2 != nullptr) {
        duration = calculateGreenDuration(laneAL2->getVehicleCount());
    }

    // State transition logic
    if (elapsedTime >= duration) {
        currentState = nextState;

        // Calculate next state if not in priority mode
        if (!isPriorityMode) {
            nextState = calculateNextState(lanes);
        }
        // In priority mode, keep going back to A_GREEN after ALL_RED
        else if (currentState == State::ALL_RED) {
            nextState = State::A_GREEN;
        }
        // In priority mode, go to ALL_RED after A_GREEN
        else if (currentState == State::A_GREEN) {
            nextState = State::ALL_RED;
        }

        // Log state changes
        DebugLogger::log("Traffic light state changed from " +
                         std::to_string(static_cast<int>(currentState)) +
                         " to " + std::to_string(static_cast<int>(nextState)));

        lastStateChangeTime = currentTime;
    }
}

TrafficLight::State TrafficLight::calculateNextState(const std::vector<Lane*>& lanes) {
    // If transitioning from a green state, go to ALL_RED first
    if (currentState != State::ALL_RED) {
        return State::ALL_RED;
    }

    // If coming from ALL_RED, determine which lane to serve next

    // If resuming from priority mode, go through regular cycle starting from B
    if (shouldResumeNormalMode) {
        shouldResumeNormalMode = false;
        return State::B_GREEN;
    }

    // Normal rotation: A -> ALL_RED -> B -> ALL_RED -> C -> ALL_RED -> D -> ALL_RED -> A
    switch (currentState) {
        case State::ALL_RED:
            // After current state was ALL_RED, check what the previous state was
            switch (nextState) {
                case State::ALL_RED: return State::A_GREEN; // Initial state or after reset
                case State::A_GREEN: return State::B_GREEN;
                case State::B_GREEN: return State::C_GREEN;
                case State::C_GREEN: return State::D_GREEN;
                case State::D_GREEN: return State::A_GREEN;
                default: return State::A_GREEN; // Fallback
            }
        default:
            return State::ALL_RED; // Any other state transitions to ALL_RED
    }
}

int TrafficLight::calculateGreenDuration(int vehicleCount) {
    // Base formula from assignment: Total time = |V| * t
    // Where t is 2 seconds per vehicle
    const int timePerVehicle = 2000; // 2 seconds in milliseconds

    // Calculate the total green duration based on vehicle count
    // Ensure a minimum duration and cap it to prevent excessive wait times
    int calculatedDuration = std::max(3000, std::min(vehicleCount * timePerVehicle, 15000));

    return calculatedDuration;
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
    // Draw traffic lights based on current state
    drawLightForA(renderer, currentState != State::A_GREEN);
    drawLightForB(renderer, currentState != State::B_GREEN);
    drawLightForC(renderer, currentState != State::C_GREEN);
    drawLightForD(renderer, currentState != State::D_GREEN);

    // Display priority mode indicator if active
    if (isPriorityMode) {
        SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255); // Orange for priority
        SDL_FRect priorityIndicator = {10, 10, 20, 20};
        SDL_RenderFillRect(renderer, &priorityIndicator);

        // Draw "PRIORITY" text next to indicator
        // Note: Text rendering would require TTF implementation
    }
}

void TrafficLight::drawLightForA(SDL_Renderer* renderer, bool isRed) {
    SDL_FRect lightBox = {388, 288, 70, 30};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &lightBox);

    SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
    SDL_FRect innerBox = {389, 289, 68, 28};
    SDL_RenderFillRect(renderer, &innerBox);

    // Right turn light - always green
    SDL_SetRenderDrawColor(renderer, 11, 156, 50, 255);
    SDL_FRect right_Light = {433, 293, 20, 20};
    SDL_RenderFillRect(renderer, &right_Light);

    // Straight light - controlled by traffic signal
    if(isRed)
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    else
        SDL_SetRenderDrawColor(renderer, 11, 156, 50, 255);
    SDL_FRect straight_Light = {393, 293, 20, 20};
    SDL_RenderFillRect(renderer, &straight_Light);
}

void TrafficLight::drawLightForB(SDL_Renderer* renderer, bool isRed) {
    SDL_FRect lightBox = {325, 488, 80, 30};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &lightBox);

    SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
    SDL_FRect innerBox = {326, 489, 78, 28};
    SDL_RenderFillRect(renderer, &innerBox);

    // left lane light -> always green
    SDL_SetRenderDrawColor(renderer, 11, 156, 50, 255);
    SDL_FRect left_Light = {330, 493, 20, 20};
    SDL_RenderFillRect(renderer, &left_Light);

    // middle lane light -> controlled by traffic signal
    if(isRed)
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    else
        SDL_SetRenderDrawColor(renderer, 11, 156, 50, 255);
    SDL_FRect middle_Light = {380, 493, 20, 20};
    SDL_RenderFillRect(renderer, &middle_Light);
}

void TrafficLight::drawLightForC(SDL_Renderer* renderer, bool isRed) {
    SDL_FRect lightBox = {488, 388, 30, 70};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &lightBox);

    SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
    SDL_FRect innerBox = {489, 389, 28, 68};
    SDL_RenderFillRect(renderer, &innerBox);

    // right turn light - always green
    SDL_SetRenderDrawColor(renderer, 11, 156, 50, 255);
    SDL_FRect right_Light = {493, 433, 20, 20};
    SDL_RenderFillRect(renderer, &right_Light);

    // straight light
    if(isRed)
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    else
        SDL_SetRenderDrawColor(renderer, 11, 156, 50, 255);
    SDL_FRect straight_Light = {493, 393, 20, 20};
    SDL_RenderFillRect(renderer, &straight_Light);
}

void TrafficLight::drawLightForD(SDL_Renderer* renderer, bool isRed) {
    SDL_FRect lightBox = {288, 325, 30, 90};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &lightBox);

    SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
    SDL_FRect innerBox = {289, 326, 28, 88};
    SDL_RenderFillRect(renderer, &innerBox);

    // left turn light - always green
    SDL_SetRenderDrawColor(renderer, 11, 156, 50, 255);
    SDL_FRect left_turn_Light = {293, 330, 20, 20};
    SDL_RenderFillRect(renderer, &left_turn_Light);

    // middle lane light
    if(isRed)
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    else
        SDL_SetRenderDrawColor(renderer, 11, 156, 50, 255);
    SDL_FRect middle_Light = {293, 380, 20, 20};
    SDL_RenderFillRect(renderer, &middle_Light);
}
