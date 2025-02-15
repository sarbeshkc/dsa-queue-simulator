// src/core/main.cpp
#include "window.h"
#include "../traffic/traffic_manager.h"
#include "../traffic/road_system.h"
#include <stdexcept>
#include <iostream>

class SimulationApp {
public:
    SimulationApp()
        : m_window(nullptr)
        , m_running(true)
        , m_elapsedTime(0.0f) {

        // Initialize SDL
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            throw std::runtime_error(
                std::string("SDL initialization failed: ") + SDL_GetError()
            );
        }

        // Create window and components
        m_window = std::make_unique<Window>("Traffic Simulation", 1280, 720);
        m_roadSystem = std::make_unique<RoadSystem>(m_window->getRenderer());
        m_trafficManager = std::make_unique<TrafficManager>(m_window->getRenderer());
    }

    ~SimulationApp() {
        SDL_Quit();
    }

    void run() {
        Uint64 lastTime = SDL_GetTicks();

        // Main game loop
        while (m_running) {
            // Calculate delta time
            Uint64 currentTime = SDL_GetTicks();
            float deltaTime = (currentTime - lastTime) / 1000.0f;
            lastTime = currentTime;

            // Update simulation
            processEvents();
            update(deltaTime);
            render();

            // Cap frame rate to approximately 60 FPS
            SDL_Delay(16);
        }
    }

private:
    std::unique_ptr<Window> m_window;
    std::unique_ptr<RoadSystem> m_roadSystem;
    std::unique_ptr<TrafficManager> m_trafficManager;
    bool m_running;
    float m_elapsedTime;

 void processEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                m_running = false;
                break;
            case SDL_EVENT_KEY_DOWN:
                // In SDL3, we use event.key.sym instead of event.key.keysym.sym
                handleKeyPress(event.key.type);
                break;
        }
    }
}

    void update(float deltaTime) {
        m_elapsedTime += deltaTime;

        // Update core systems
        m_roadSystem->update(deltaTime);
        m_trafficManager->update(deltaTime);

        // Process any new vehicles from files
        processNewVehicles();
    }

    void render() {
        m_window->clear();

        // Render simulation components
        m_roadSystem->render();
        m_trafficManager->render();

        // Display queue statistics
        renderStatistics();

        m_window->present();
    }

    void handleKeyPress(SDL_Keycode key) {
        switch (key) {
            case SDLK_ESCAPE:
                m_running = false;
                break;
            case SDLK_SPACE:
                // Could add pause functionality
                break;
        }
    }

    void processNewVehicles() {
        // Check for new vehicles from generator
        // This is where we'll implement the file communication
    }

    void renderStatistics() {
        // This will display queue lengths and wait times
        // Implementation coming in the next part
    }
};

int main(int argc, char* argv[]) {
    try {
        SimulationApp app;
        app.run();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
