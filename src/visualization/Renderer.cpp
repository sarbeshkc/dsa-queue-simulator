#include "visualization/Renderer.h"
#include "core/Lane.h"
#include "core/Vehicle.h"
#include "core/TrafficLight.h"
#include "managers/TrafficManager.h"
#include "utils/DebugLogger.h"

#include <sstream>
#include <algorithm>
#include <cmath>

// Constants
const int ROAD_WIDTH = 150;
const int LANE_WIDTH = 50;
const SDL_Color ROAD_COLOR = {50, 50, 50, 255};
const SDL_Color LANE_MARKER_COLOR = {255, 255, 255, 255};
const SDL_Color INTERSECTION_COLOR = {70, 70, 70, 255};
const SDL_Color PRIORITY_INDICATOR_COLOR = {255, 165, 0, 255};
const SDL_Color TEXT_COLOR = {0, 0, 0, 255};

Renderer::Renderer()
    : window(nullptr),
      renderer(nullptr),
      carTexture(nullptr),
      surface(nullptr),
      active(false),
      showDebugOverlay(true),
      frameRateLimit(60),
      lastFrameTime(0),
      windowWidth(800),
      windowHeight(800),
      trafficManager(nullptr) {}

Renderer::~Renderer() {
    cleanup();
}

bool Renderer::initialize(int width, int height, const std::string& title) {
    windowWidth = width;
    windowHeight = height;

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        DebugLogger::log("Failed to initialize SDL: " + std::string(SDL_GetError()), DebugLogger::LogLevel::ERROR);
        return false;
    }

    // Create window
    window = SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_OPENGL);
    if (!window) {
        DebugLogger::log("Failed to create window: " + std::string(SDL_GetError()), DebugLogger::LogLevel::ERROR);
        return false;
    }

    // Create renderer
    renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        DebugLogger::log("Failed to create renderer: " + std::string(SDL_GetError()), DebugLogger::LogLevel::ERROR);
        return false;
    }

    // Load textures
    if (!loadTextures()) {
        DebugLogger::log("Failed to load textures", DebugLogger::LogLevel::ERROR);
        return false;
    }

    active = true;
    DebugLogger::log("Renderer initialized successfully");

    return true;
}

bool Renderer::loadTextures() {
    // Create a simple surface directly with a solid color to avoid SDL_MapRGB issues
    surface = SDL_CreateSurface(20, 10, SDL_PIXELFORMAT_RGBA8888);
    if (!surface) {
        DebugLogger::log("Failed to create surface: " + std::string(SDL_GetError()), DebugLogger::LogLevel::ERROR);
        return false;
    }

    // Fill with blue color using a simpler approach
    // Create a color value manually
    Uint32 blueColor = 0x0000FFFF; // RGBA format: blue with full alpha

    // Fill the entire surface with this color
    SDL_FillSurfaceRect(surface, NULL, blueColor);

    carTexture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    surface = nullptr;

    if (!carTexture) {
        DebugLogger::log("Failed to create car texture: " + std::string(SDL_GetError()), DebugLogger::LogLevel::ERROR);
        return false;
    }

    return true;
}

void Renderer::startRenderLoop() {
    if (!active || !trafficManager) {
        DebugLogger::log("Cannot start render loop - renderer not active or trafficManager not set", DebugLogger::LogLevel::ERROR);
        return;
    }

    DebugLogger::log("Starting render loop");

    uint32_t lastUpdate = SDL_GetTicks();
    const int updateInterval = 16; // ~60 FPS

    while (active) {
        uint32_t currentTime = SDL_GetTicks();
        uint32_t deltaTime = currentTime - lastUpdate;

        if (deltaTime >= updateInterval) {
            // Process events
            active = processEvents();

            // Update traffic manager
            trafficManager->update(deltaTime);

            // Render frame
            renderFrame();

            lastUpdate = currentTime;
        }

        // Delay to maintain frame rate
        uint32_t frameDuration = SDL_GetTicks() - currentTime;
        if (frameRateLimit > 0) {
            uint32_t targetFrameTime = 1000 / frameRateLimit;
            if (frameDuration < targetFrameTime) {
                SDL_Delay(targetFrameTime - frameDuration);
            }
        }
    }
}

bool Renderer::processEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                return false;

            case SDL_EVENT_KEY_DOWN: {
                // Check based on the key scancode instead of using SDLK constants
                SDL_Scancode scancode = event.key.scancode;

                // D key scancode is usually 7 (for SDL_SCANCODE_D)
                if (scancode == SDL_SCANCODE_D) {
                    toggleDebugOverlay();
                }
                // Escape key scancode is usually 41 (for SDL_SCANCODE_ESCAPE)
                else if (scancode == SDL_SCANCODE_ESCAPE) {
                    return false;
                }
                break;
            }
        }
    }

    return true;
}

void Renderer::renderFrame() {
    if (!active || !renderer || !trafficManager) {
        return;
    }

    // Clear screen
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    // Draw roads and lanes
    drawRoadsAndLanes();

    // Draw traffic lights
    drawTrafficLights();

    // Draw vehicles
    drawVehicles();

    // Draw debug overlay if enabled
    if (showDebugOverlay) {
        drawDebugOverlay();
    }

    // Present render
    SDL_RenderPresent(renderer);

    // Update frame time
    lastFrameTime = SDL_GetTicks();
}

void Renderer::drawRoadsAndLanes() {
    // Draw intersection (dark gray)
    SDL_SetRenderDrawColor(renderer, INTERSECTION_COLOR.r, INTERSECTION_COLOR.g, INTERSECTION_COLOR.b, INTERSECTION_COLOR.a);
    SDL_FRect intersectionRect = {
        static_cast<float>(windowWidth/2 - ROAD_WIDTH/2),
        static_cast<float>(windowHeight/2 - ROAD_WIDTH/2),
        static_cast<float>(ROAD_WIDTH),
        static_cast<float>(ROAD_WIDTH)
    };
    SDL_RenderFillRect(renderer, &intersectionRect);

    // Draw horizontal road (dark gray)
    SDL_FRect horizontalRoad = {
        0, static_cast<float>(windowHeight/2 - ROAD_WIDTH/2),
        static_cast<float>(windowWidth), static_cast<float>(ROAD_WIDTH)
    };
    SDL_RenderFillRect(renderer, &horizontalRoad);

    // Draw vertical road (dark gray)
    SDL_FRect verticalRoad = {
        static_cast<float>(windowWidth/2 - ROAD_WIDTH/2), 0,
        static_cast<float>(ROAD_WIDTH), static_cast<float>(windowHeight)
    };
    SDL_RenderFillRect(renderer, &verticalRoad);

    // Draw lane dividers (white dashed lines)
    SDL_SetRenderDrawColor(renderer, LANE_MARKER_COLOR.r, LANE_MARKER_COLOR.g, LANE_MARKER_COLOR.b, LANE_MARKER_COLOR.a);

    // Horizontal lane dividers
    for (int i = 1; i < 3; i++) {
        int y = windowHeight/2 - ROAD_WIDTH/2 + i * LANE_WIDTH;
        for (int x = 0; x < windowWidth; x += 30) {
            SDL_RenderLine(renderer, x, y, x + 15, y);
        }
    }

    // Vertical lane dividers
    for (int i = 1; i < 3; i++) {
        int x = windowWidth/2 - ROAD_WIDTH/2 + i * LANE_WIDTH;
        for (int y = 0; y < windowHeight; y += 30) {
            SDL_RenderLine(renderer, x, y, x, y + 15);
        }
    }

    // Draw lane labels
    drawLaneLabels();

    // If AL2 is priority lane and has more than 10 vehicles, highlight it
    if (trafficManager) {
        Lane* priorityLane = trafficManager->getPriorityLane();
        if (priorityLane && priorityLane->getVehicleCount() > 10) {
            // Highlight AL2 lane in orange
            SDL_SetRenderDrawColor(renderer,
                PRIORITY_INDICATOR_COLOR.r,
                PRIORITY_INDICATOR_COLOR.g,
                PRIORITY_INDICATOR_COLOR.b,
                PRIORITY_INDICATOR_COLOR.a);

            SDL_FRect priorityLaneRect = {
                static_cast<float>(windowWidth/2 - ROAD_WIDTH/2 + LANE_WIDTH),
                0,
                static_cast<float>(LANE_WIDTH),
                static_cast<float>(windowHeight/2 - ROAD_WIDTH/2)
            };
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_RenderFillRect(renderer, &priorityLaneRect);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        }
    }
}

void Renderer::drawLaneLabels() {
    SDL_SetRenderDrawColor(renderer, TEXT_COLOR.r, TEXT_COLOR.g, TEXT_COLOR.b, TEXT_COLOR.a);

    // Road A labels
    drawText("A", windowWidth/2, 10, TEXT_COLOR);
    drawText("A1", windowWidth/2 - LANE_WIDTH, windowHeight/4, TEXT_COLOR);
    drawText("A2", windowWidth/2, windowHeight/4, TEXT_COLOR);
    drawText("A3", windowWidth/2 + LANE_WIDTH, windowHeight/4, TEXT_COLOR);

    // Road B labels
    drawText("B", windowWidth/2, windowHeight - 30, TEXT_COLOR);
    drawText("B1", windowWidth/2 + LANE_WIDTH, 3*windowHeight/4, TEXT_COLOR);
    drawText("B2", windowWidth/2, 3*windowHeight/4, TEXT_COLOR);
    drawText("B3", windowWidth/2 - LANE_WIDTH, 3*windowHeight/4, TEXT_COLOR);

    // Road C labels
    drawText("C", windowWidth - 30, windowHeight/2, TEXT_COLOR);
    drawText("C1", 3*windowWidth/4, windowHeight/2 - LANE_WIDTH, TEXT_COLOR);
    drawText("C2", 3*windowWidth/4, windowHeight/2, TEXT_COLOR);
    drawText("C3", 3*windowWidth/4, windowHeight/2 + LANE_WIDTH, TEXT_COLOR);

    // Road D labels
    drawText("D", 10, windowHeight/2, TEXT_COLOR);
    drawText("D1", windowWidth/4, windowHeight/2 + LANE_WIDTH, TEXT_COLOR);
    drawText("D2", windowWidth/4, windowHeight/2, TEXT_COLOR);
    drawText("D3", windowWidth/4, windowHeight/2 - LANE_WIDTH, TEXT_COLOR);
}

void Renderer::drawTrafficLights() {
    if (!trafficManager) {
        return;
    }

    TrafficLight* trafficLight = trafficManager->getTrafficLight();
    if (!trafficLight) {
        return;
    }

    // Draw traffic lights
    trafficLight->render(renderer);
}

void Renderer::drawVehicles() {
    if (!trafficManager) {
        return;
    }

    // Get all lanes from traffic manager
    const std::vector<Lane*>& lanes = trafficManager->getLanes();

    // Draw vehicles in each lane
    for (Lane* lane : lanes) {
        if (!lane) {
            continue;
        }

        const std::vector<Vehicle*>& vehicles = lane->getVehicles();
        int queuePos = 0;

        for (Vehicle* vehicle : vehicles) {
            if (vehicle) {
                vehicle->render(renderer, carTexture, queuePos);
                queuePos++;
            }
        }
    }
}

void Renderer::drawDebugOverlay() {
    // Draw semi-transparent background
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_FRect overlayRect = {10, 10, 250, 150};
    SDL_RenderFillRect(renderer, &overlayRect);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    // Draw statistics
    drawStatistics();

    // Draw recent logs
    std::vector<std::string> logs = DebugLogger::getRecentLogs(5);
    int y = 170;

    for (const auto& log : logs) {
        drawText(log.substr(0, 30), 10, y, {200, 200, 200, 255});
        y += 20;
    }
}

void Renderer::drawStatistics() {
    if (!trafficManager) {
        return;
    }

    // Get statistics from traffic manager
    std::string stats = trafficManager->getStatistics();

    // Split into lines
    std::istringstream stream(stats);
    std::string line;
    int y = 20;

    while (std::getline(stream, line)) {
        drawText(line, 20, y, {255, 255, 255, 255});
        y += 20;
    }
}

void Renderer::drawText(const std::string& text, int x, int y, SDL_Color color) {
    // Since we don't have SDL_ttf configured, draw a colored rectangle
    // and note that we would normally render text here
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_FRect textRect = {static_cast<float>(x), static_cast<float>(y),
                         static_cast<float>(8 * text.length()), 15};

    // SDL3 uses SDL_RenderRect instead of SDL_RenderDrawRect
    SDL_RenderRect(renderer, &textRect);

    // In a real implementation with SDL_ttf, we would render the text here
}

void Renderer::drawArrow(int x1, int y1, int x2, int y2, int x3, int y3, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    // Draw triangle outline
    SDL_RenderLine(renderer, x1, y1, x2, y2);
    SDL_RenderLine(renderer, x2, y2, x3, y3);
    SDL_RenderLine(renderer, x3, y3, x1, y1);

    // Fill triangle (simple implementation)
    // Sort vertices by y
    if (y1 > y2) { std::swap(y1, y2); std::swap(x1, x2); }
    if (y1 > y3) { std::swap(y1, y3); std::swap(x1, x3); }
    if (y2 > y3) { std::swap(y2, y3); std::swap(x2, x3); }

    // Compute slopes
    float dx1 = (y2 - y1) ? static_cast<float>(x2 - x1) / (y2 - y1) : 0;
    float dx2 = (y3 - y1) ? static_cast<float>(x3 - x1) / (y3 - y1) : 0;
    float dx3 = (y3 - y2) ? static_cast<float>(x3 - x2) / (y3 - y2) : 0;

    float sx1 = x1, sx2 = x1;

    // Fill top to middle
    for (int y = y1; y < y2; y++) {
        SDL_RenderLine(renderer, static_cast<int>(sx1), y, static_cast<int>(sx2), y);
        sx1 += dx1;
        sx2 += dx2;
    }

    sx1 = x2;

    // Fill middle to bottom
    for (int y = y2; y <= y3; y++) {
        SDL_RenderLine(renderer, static_cast<int>(sx1), y, static_cast<int>(sx2), y);
        sx1 += dx3;
        sx2 += dx2;
    }
}

void Renderer::cleanup() {
    if (carTexture) {
        SDL_DestroyTexture(carTexture);
        carTexture = nullptr;
    }

    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }

    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    DebugLogger::log("Renderer resources cleaned up");
}

bool Renderer::isActive() const {
    return active;
}

void Renderer::toggleDebugOverlay() {
    showDebugOverlay = !showDebugOverlay;
    DebugLogger::log("Debug overlay " + std::string(showDebugOverlay ? "enabled" : "disabled"));
}

void Renderer::setFrameRateLimit(int fps) {
    frameRateLimit = fps;
}

void Renderer::setTrafficManager(TrafficManager* manager) {
    trafficManager = manager;
}
