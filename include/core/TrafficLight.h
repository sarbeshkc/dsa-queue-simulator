// include/core/TrafficLight.h
#pragma once
#include <SDL3/SDL.h>
#include "Constants.h"
#include <cstdint>

class TrafficLight {
private:
    LightState state;                // Current light state
    LightState nextState;            // Next scheduled state
    float transitionProgress;        // Progress of state transition (0-1)
    float transitionDuration;        // Duration of transition animation
    float stateTimer;                // Timer for current state
    bool isTransitioning;            // Whether in transition between states
    float currentStateDuration;      // Duration for current state
    bool isPriorityMode;             // Whether in priority mode
    bool isForced;                   // Whether state is being forced
    LaneId controlledLane;           // The lane this light controls

    // Private methods
    void startTransition(LightState newState);
    float getNextStateDuration() const;

public:
    TrafficLight(LaneId lane = LaneId::AL2_PRIORITY);  // Default to priority lane

    // Core state management
    void update(float deltaTime);
    void setState(LightState newState);
    LightState getState() const { return state; }
    void forceState(LightState newState, bool force = true);
    void setPriorityMode(bool enabled);
    LaneId getControlledLane() const { return controlledLane; }

    // State query methods
    bool isInTransition() const { return isTransitioning; }
    float getTransitionProgress() const { return transitionProgress; }
    float getStateTimer() const { return stateTimer; }
    float getStateDuration() const;

    // Rendering
    void render(SDL_Renderer* renderer, float x, float y) const;
};
