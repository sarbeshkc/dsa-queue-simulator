// traffic_light.h
#ifndef TRAFFIC_LIGHT_H
#define TRAFFIC_LIGHT_H

#include <SDL3/SDL.h>

enum class LightState {
    RED,
    GREEN
};

class TrafficLight {
public:
    TrafficLight(SDL_Renderer* renderer, float x, float y);

    void update(float deltaTime);
    void render() const;
    void setState(LightState state);

    bool isGreen() const { return m_state == LightState::GREEN; }

    // Synchronization
    void synchronizeWith(TrafficLight* other);

private:
    SDL_Renderer* m_renderer;
    LightState m_state;
    float m_timer;
    float m_x, m_y;
    TrafficLight* m_syncedLight;

    // Timing constants
    static constexpr float STATE_DURATION = 10.0f;  // 10 seconds per state
};

#endif // TRAFFIC_LIGHT_H
