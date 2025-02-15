// traffic_light.cpp
#include "traffic_light.h"

TrafficLight::TrafficLight(SDL_Renderer* renderer, float x, float y)
    : m_renderer(renderer)
    , m_state(LightState::RED)
    , m_timer(0.0f)
    , m_x(x)
    , m_y(y)
    , m_syncedLight(nullptr)
{}

void TrafficLight::update(float deltaTime) {
    m_timer += deltaTime;

    // Change state after duration
    if (m_timer >= STATE_DURATION) {
        // Toggle state
        setState(m_state == LightState::RED ? LightState::GREEN : LightState::RED);
        m_timer = 0.0f;
    }
}

void TrafficLight::setState(LightState state) {
    m_state = state;

    // Update synced light to opposite state
    if (m_syncedLight) {
        m_syncedLight->m_state = (state == LightState::RED ?
            LightState::GREEN : LightState::RED);
        m_syncedLight->m_timer = 0.0f;
    }
}

void TrafficLight::render() const {
    // Draw traffic light housing
    SDL_FRect housing = {
        m_x - 15, m_y - 40,
        30, 80
    };

    SDL_SetRenderDrawColor(m_renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(m_renderer, &housing);

    // Draw lights
    const float LIGHT_RADIUS = 10.0f;

    // Red light
    SDL_SetRenderDrawColor(m_renderer,
        m_state == LightState::RED ? 255 : 50,  // Bright/dim red
        0, 0, 255);
    SDL_FRect redLight = {
        m_x - LIGHT_RADIUS, m_y - 25,
        LIGHT_RADIUS * 2, LIGHT_RADIUS * 2
    };
    SDL_RenderFillRect(m_renderer, &redLight);

    // Green light
    SDL_SetRenderDrawColor(m_renderer,
        0,
        m_state == LightState::GREEN ? 255 : 50,  // Bright/dim green
        0, 255);
    SDL_FRect greenLight = {
        m_x - LIGHT_RADIUS, m_y + 5,
        LIGHT_RADIUS * 2, LIGHT_RADIUS * 2
    };
    SDL_RenderFillRect(m_renderer, &greenLight);
}

void TrafficLight::synchronizeWith(TrafficLight* other) {
    m_syncedLight = other;
    if (m_syncedLight) {
        m_syncedLight->m_state = (m_state == LightState::RED ?
            LightState::GREEN : LightState::RED);
    }
}
