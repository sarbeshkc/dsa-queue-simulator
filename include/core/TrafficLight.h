// FILE: include/core/TrafficLight.h
#ifndef TRAFFIC_LIGHT_H
#define TRAFFIC_LIGHT_H

#include <cstdint>
#include <vector>
#include <string>
#include <SDL3/SDL.h>
#include "core/Lane.h"

class TrafficLight {
public:
    enum class State {
        ALL_RED = 0,
        A_GREEN = 1,
        B_GREEN = 2,
        C_GREEN = 3,
        D_GREEN = 4
    };

    TrafficLight();
    ~TrafficLight();

    // Updates the traffic light state based on lane priorities
    void update(const std::vector<Lane*>& lanes);

    // Renders the traffic lights
    void render(SDL_Renderer* renderer);

    // Returns the current traffic light state
    State getCurrentState() const { return currentState; }

    // Returns the next traffic light state
    State getNextState() const { return nextState; }

    // Sets the next traffic light state
    void setNextState(State state);

    // Checks if the specific lane gets green light
    bool isGreen(char lane) const;

private:
    State currentState;
    State nextState;

    // Timing for the green and red states
    const int allRedDuration = 2000; // 2 seconds for all red

    // Last state change time in milliseconds
    uint32_t lastStateChangeTime;

    // Priority mode flag
    bool isPriorityMode;

    // Track when normal mode should resume (after priority mode drops to 5 vehicles)
    bool shouldResumeNormalMode;

    // Helper function to calculate average vehicle count
    float calculateAverageVehicleCount(const std::vector<Lane*>& lanes);

    // Helper drawing functions
    void drawLightForA(SDL_Renderer* renderer, bool isRed);
    void drawLightForB(SDL_Renderer* renderer, bool isRed);
    void drawLightForC(SDL_Renderer* renderer, bool isRed);
    void drawLightForD(SDL_Renderer* renderer, bool isRed);
};

#endif // TRAFFIC_LIGHT_H
