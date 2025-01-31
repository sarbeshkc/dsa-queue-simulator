#include "main.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_log.h"
#include "SDL3_ttf/SDL_ttf.h"
#include "text.h"
#include <SDL3/SDL_render.h>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include "../generator/traffic_generator.h"
#include "../traffic/vehicle.h"

namespace GameVariable {
constexpr int WindowHeight = 720;
constexpr int WindowLength = 1280;
constexpr int TextPixel = 30;
constexpr SDL_Color white = {255, 255, 255, 255};

} // namespace GameVariable

App::App()
    : m_window("Traffic Simulator", GameVariable::WindowLength,
               GameVariable::WindowHeight)
    , m_generator(m_window.renderer())
    , m_lastSpawnTime(SDL_GetTicks())
    , m_running(true) {
}

App::~App() {
    for (auto* vehicle : m_vehicles) {
        delete vehicle;
    }
}

void App::process_event() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            m_running = false;
        }
        // For testing: press SPACE to spawn a vehicle
        else if (event.type == SDL_EVENT_KEY_DOWN) {
            if (event.key.scancode == SDL_SCANCODE_SPACE) {  // Fixed: Using scancode in SDL3
                Vehicle* vehicle = m_generator.generateVehicle();
                if (vehicle) {
                    m_vehicles.push_back(vehicle);
                }
            }
        }
    }
}

void App::update() {
    // Automatic vehicle spawning every 2 seconds
    Uint64 currentTime = SDL_GetTicks();
    if (currentTime - m_lastSpawnTime >= 2000) {
        Vehicle* vehicle = m_generator.generateVehicle();
        if (vehicle) {
            m_vehicles.push_back(vehicle);
        }
        m_lastSpawnTime = currentTime;
    }

    // Update all vehicles
    float deltaTime = 1.0f / 60.0f;

    auto it = m_vehicles.begin();
    while (it != m_vehicles.end()) {
        Vehicle* vehicle = *it;
        vehicle->update(deltaTime);

        Vector2D pos = vehicle->getPosition();
        if (pos.x < -100 || pos.x > GameVariable::WindowLength + 100 ||
            pos.y < -100 || pos.y > GameVariable::WindowHeight + 100) {
            delete vehicle;
            it = m_vehicles.erase(it);
        } else {
            ++it;
        }
    }
}

void App::render() {
    m_window.clear();

    for (const auto* vehicle : m_vehicles) {
        vehicle->render();
    }

    m_window.present();
}

void App::run() {
    m_running = true;
    while (m_running) {
        process_event();
        update();
        render();
        SDL_Delay(16); // Cap to roughly 60 FPS
    }
}

int main(int argc, const char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL init failed: %s\n", SDL_GetError());
        return 1;
    }

    try {
        App app;
        app.run();
    } catch (const std::exception& e) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error: %s\n", e.what());
    }

    SDL_Quit();
    return 0;
}
