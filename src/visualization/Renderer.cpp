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

// In Renderer.cpp - fixed drawRoadsAndLanes implementation
void Renderer::drawRoadsAndLanes() {
    // Define constants for readability
    const int ROAD_WIDTH = 150;
    const int LANE_WIDTH = 50;

    // Draw intersection (dark gray)
    SDL_SetRenderDrawColor(renderer, 70, 70, 70, 255); // INTERSECTION_COLOR
    SDL_FRect intersectionRect = {
        static_cast<float>(windowWidth/2 - ROAD_WIDTH/2),
        static_cast<float>(windowHeight/2 - ROAD_WIDTH/2),
        static_cast<float>(ROAD_WIDTH),
        static_cast<float>(ROAD_WIDTH)
    };
    SDL_RenderFillRect(renderer, &intersectionRect);

    // Draw horizontal road (dark gray)
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255); // ROAD_COLOR
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

    // Highlight free lanes (lane 3) with a different color
    SDL_SetRenderDrawColor(renderer, 60, 70, 60, 255); // Slightly different color for lane 3

    // A3 - Free lane
    SDL_FRect a3Lane = {
        static_cast<float>(windowWidth/2 + LANE_WIDTH),
        0,
        static_cast<float>(LANE_WIDTH),
        static_cast<float>(windowHeight/2 - ROAD_WIDTH/2)
    };
    SDL_RenderFillRect(renderer, &a3Lane);

    // B3 - Free lane
    SDL_FRect b3Lane = {
        static_cast<float>(windowWidth/2 - 2*LANE_WIDTH),
        static_cast<float>(windowHeight/2 + ROAD_WIDTH/2),
        static_cast<float>(LANE_WIDTH),
        static_cast<float>(windowHeight/2)
    };
    SDL_RenderFillRect(renderer, &b3Lane);

    // C3 - Free lane
    SDL_FRect c3Lane = {
        static_cast<float>(windowWidth/2 + ROAD_WIDTH/2),
        static_cast<float>(windowHeight/2 + LANE_WIDTH),
        static_cast<float>(windowWidth/2 - ROAD_WIDTH/2),
        static_cast<float>(LANE_WIDTH)
    };
    SDL_RenderFillRect(renderer, &c3Lane);

    // D3 - Free lane
    SDL_FRect d3Lane = {
        0,
        static_cast<float>(windowHeight/2 - 2*LANE_WIDTH),
        static_cast<float>(windowWidth/2 - ROAD_WIDTH/2),
        static_cast<float>(LANE_WIDTH)
    };
    SDL_RenderFillRect(renderer, &d3Lane);

    // Highlight priority lane (AL2) if it has priority
    if (trafficManager) {
        Lane* al2Lane = trafficManager->findLane('A', 2);
        if (al2Lane && al2Lane->getPriority() > 0) {
            // Highlight AL2 lane with orange
            SDL_SetRenderDrawColor(renderer, 255, 165, 0, 100); // Semi-transparent orange

            SDL_FRect priorityLaneRect = {
                static_cast<float>(windowWidth/2),
                0,
                static_cast<float>(LANE_WIDTH),
                static_cast<float>(windowHeight/2 - ROAD_WIDTH/2)
            };
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_RenderFillRect(renderer, &priorityLaneRect);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        }
    }

    // Draw lane dividers (white dashed lines)
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // LANE_MARKER_COLOR

    // Horizontal lane dividers
    for (int i = 1; i < 3; i++) {
        int y = windowHeight/2 - ROAD_WIDTH/2 + i * LANE_WIDTH;
        for (int x = 0; x < windowWidth; x += 30) {
            if (x < windowWidth/2 - ROAD_WIDTH/2 || x > windowWidth/2 + ROAD_WIDTH/2) {
                SDL_RenderLine(renderer, x, y, x + 15, y);
            }
        }
    }

    // Vertical lane dividers
    for (int i = 1; i < 3; i++) {
        int x = windowWidth/2 - ROAD_WIDTH/2 + i * LANE_WIDTH;
        for (int y = 0; y < windowHeight; y += 30) {
            if (y < windowHeight/2 - ROAD_WIDTH/2 || y > windowHeight/2 + ROAD_WIDTH/2) {
                SDL_RenderLine(renderer, x, y, x, y + 15);
            }
        }
    }

    // Add queue length indicators for each lane
    drawQueueLengthIndicators();

    // Draw lane labels
    drawLaneLabels();
}



// In Renderer.cpp - queue length indicators implementation
void Renderer::drawQueueLengthIndicators() {
    if (!trafficManager) return;

    // For each lane, draw a colored bar showing queue length
    for (auto* lane : trafficManager->getLanes()) {
        int count = lane->getVehicleCount();
        char laneId = lane->getLaneId();
        int laneNum = lane->getLaneNumber();

        // Skip lane 1 (output lanes)
        if (laneNum == 1) continue;

        // Determine color based on lane
        SDL_Color color;

        if (laneId == 'A' && laneNum == 2) {
            // AL2 - priority lane
            if (lane->getPriority() > 0) {
                color = {255, 140, 0, 255}; // Orange for priority
            } else {
                color = {220, 180, 0, 255}; // Yellow-gold for normal
            }
        } else if (laneNum == 3) {
            // Free lanes
            color = {0, 180, 0, 255}; // Green
        } else {
            // Normal lanes
            color = {0, 120, 220, 255}; // Blue
        }

        // Determine bar position based on lane
        SDL_FRect barRect;
        int maxBarSize = 100; // Maximum size of indicator bar
        int barSize = std::min(count * 5, maxBarSize); // 5 pixels per vehicle, max 100

        switch (laneId) {
            case 'A':
                // A lanes - horizontal bar on left side of lane
                barRect = {
                    static_cast<float>(windowWidth/2 + (laneNum-2)*LANE_WIDTH - 10),
                    static_cast<float>(windowHeight/4),
                    10,
                    static_cast<float>(barSize)
                };
                break;
            case 'B':
                // B lanes - horizontal bar on right side of lane
                barRect = {
                    static_cast<float>(windowWidth/2 - (laneNum-2)*LANE_WIDTH),
                    static_cast<float>(3*windowHeight/4 - barSize),
                    10,
                    static_cast<float>(barSize)
                };
                break;
            case 'C':
                // C lanes - vertical bar below lane
                barRect = {
                    static_cast<float>(3*windowWidth/4 - barSize),
                    static_cast<float>(windowHeight/2 + (laneNum-2)*LANE_WIDTH),
                    static_cast<float>(barSize),
                    10
                };
                break;
            case 'D':
                // D lanes - vertical bar above lane
                barRect = {
                    static_cast<float>(windowWidth/4),
                    static_cast<float>(windowHeight/2 - (laneNum-2)*LANE_WIDTH - 10),
                    static_cast<float>(barSize),
                    10
                };
                break;
        }

        // Draw the bar with the determined color
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_RenderFillRect(renderer, &barRect);

        // Draw border
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderRect(renderer, &barRect);

        // Draw text showing actual count (in real implementation would use SDL_ttf)
        // For now, just draw a placeholder
        if (count > 0) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            float textX, textY;

            switch (laneId) {
                case 'A':
                    textX = barRect.x - 15;
                    textY = barRect.y + barRect.h/2 - 5;
                    break;
                case 'B':
                    textX = barRect.x + 15;
                    textY = barRect.y + barRect.h/2 - 5;
                    break;
                case 'C':
                    textX = barRect.x + barRect.w/2 - 5;
                    textY = barRect.y - 15;
                    break;
                case 'D':
                    textX = barRect.x + barRect.w/2 - 5;
                    textY = barRect.y + 15;
                    break;
            }

            SDL_FRect textRect = {textX, textY, 10, 10};
            SDL_RenderFillRect(renderer, &textRect);
        }
    }
}

// In Renderer.cpp - fixed drawLaneLabels implementation
void Renderer::drawLaneLabels() {
    // Define constants for readability
    const int LANE_WIDTH = 50;

    // Keep original lane labels but enhance with queue counts
    if (!trafficManager) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // TEXT_COLOR

        // Basic lane labels without counts
        drawText("A", windowWidth/2, 10, {0, 0, 0, 255});
        drawText("A1", windowWidth/2 - LANE_WIDTH, windowHeight/4, {0, 0, 0, 255});
        drawText("A2", windowWidth/2, windowHeight/4, {0, 0, 0, 255});
        drawText("A3", windowWidth/2 + LANE_WIDTH, windowHeight/4, {0, 0, 0, 255});

        // Road B labels
        drawText("B", windowWidth/2, windowHeight - 30, {0, 0, 0, 255});
        drawText("B1", windowWidth/2 + LANE_WIDTH, 3*windowHeight/4, {0, 0, 0, 255});
        drawText("B2", windowWidth/2, 3*windowHeight/4, {0, 0, 0, 255});
        drawText("B3", windowWidth/2 - LANE_WIDTH, 3*windowHeight/4, {0, 0, 0, 255});

        // Road C labels
        drawText("C", windowWidth - 30, windowHeight/2, {0, 0, 0, 255});
        drawText("C1", 3*windowWidth/4, windowHeight/2 - LANE_WIDTH, {0, 0, 0, 255});
        drawText("C2", 3*windowWidth/4, windowHeight/2, {0, 0, 0, 255});
        drawText("C3", 3*windowWidth/4, windowHeight/2 + LANE_WIDTH, {0, 0, 0, 255});

        // Road D labels
        drawText("D", 10, windowHeight/2, {0, 0, 0, 255});
        drawText("D1", windowWidth/4, windowHeight/2 + LANE_WIDTH, {0, 0, 0, 255});
        drawText("D2", windowWidth/4, windowHeight/2, {0, 0, 0, 255});
        drawText("D3", windowWidth/4, windowHeight/2 - LANE_WIDTH, {0, 0, 0, 255});

        return;
    }

    // Enhanced labels with vehicle counts
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // TEXT_COLOR

    // Road A labels
    drawText("A", windowWidth/2, 10, {0, 0, 0, 255});

    Lane* a1Lane = trafficManager->findLane('A', 1);
    Lane* a2Lane = trafficManager->findLane('A', 2);
    Lane* a3Lane = trafficManager->findLane('A', 3);

    std::string a1Text = "A1";
    std::string a2Text = "A2";
    std::string a3Text = "A3";

    // Add vehicle counts if lanes exist
    if (a1Lane) a1Text += " (" + std::to_string(a1Lane->getVehicleCount()) + ")";
    if (a2Lane) {
        a2Text += " (" + std::to_string(a2Lane->getVehicleCount()) + ")";
        // Highlight A2 if it has priority
        if (a2Lane->getPriority() > 0) {
            drawText(a2Text, windowWidth/2, windowHeight/4, {255, 165, 0, 255}); // PRIORITY_INDICATOR_COLOR
        } else {
            drawText(a2Text, windowWidth/2, windowHeight/4, {0, 0, 0, 255});
        }
    } else {
        drawText(a2Text, windowWidth/2, windowHeight/4, {0, 0, 0, 255});
    }
    if (a3Lane) a3Text += " (" + std::to_string(a3Lane->getVehicleCount()) + ")";

    drawText(a1Text, windowWidth/2 - LANE_WIDTH, windowHeight/4, {0, 0, 0, 255});
    drawText(a3Text, windowWidth/2 + LANE_WIDTH, windowHeight/4, {0, 0, 0, 255});

    // Road B labels with vehicle counts
    drawText("B", windowWidth/2, windowHeight - 30, {0, 0, 0, 255});

    Lane* b1Lane = trafficManager->findLane('B', 1);
    Lane* b2Lane = trafficManager->findLane('B', 2);
    Lane* b3Lane = trafficManager->findLane('B', 3);

    std::string b1Text = "B1";
    std::string b2Text = "B2";
    std::string b3Text = "B3";

    if (b1Lane) b1Text += " (" + std::to_string(b1Lane->getVehicleCount()) + ")";
    if (b2Lane) b2Text += " (" + std::to_string(b2Lane->getVehicleCount()) + ")";
    if (b3Lane) b3Text += " (" + std::to_string(b3Lane->getVehicleCount()) + ")";

    drawText(b1Text, windowWidth/2 + LANE_WIDTH, 3*windowHeight/4, {0, 0, 0, 255});
    drawText(b2Text, windowWidth/2, 3*windowHeight/4, {0, 0, 0, 255});
    drawText(b3Text, windowWidth/2 - LANE_WIDTH, 3*windowHeight/4, {0, 0, 0, 255});

    // Road C labels with vehicle counts
    drawText("C", windowWidth - 30, windowHeight/2, {0, 0, 0, 255});

    Lane* c1Lane = trafficManager->findLane('C', 1);
    Lane* c2Lane = trafficManager->findLane('C', 2);
    Lane* c3Lane = trafficManager->findLane('C', 3);

    std::string c1Text = "C1";
    std::string c2Text = "C2";
    std::string c3Text = "C3";

    if (c1Lane) c1Text += " (" + std::to_string(c1Lane->getVehicleCount()) + ")";
    if (c2Lane) c2Text += " (" + std::to_string(c2Lane->getVehicleCount()) + ")";
    if (c3Lane) c3Text += " (" + std::to_string(c3Lane->getVehicleCount()) + ")";

    drawText(c1Text, 3*windowWidth/4, windowHeight/2 - LANE_WIDTH, {0, 0, 0, 255});
    drawText(c2Text, 3*windowWidth/4, windowHeight/2, {0, 0, 0, 255});
    drawText(c3Text, 3*windowWidth/4, windowHeight/2 + LANE_WIDTH, {0, 0, 0, 255});

    // Road D labels with vehicle counts
    drawText("D", 10, windowHeight/2, {0, 0, 0, 255});

    Lane* d1Lane = trafficManager->findLane('D', 1);
    Lane* d2Lane = trafficManager->findLane('D', 2);
    Lane* d3Lane = trafficManager->findLane('D', 3);

    std::string d1Text = "D1";
    std::string d2Text = "D2";
    std::string d3Text = "D3";

    if (d1Lane) d1Text += " (" + std::to_string(d1Lane->getVehicleCount()) + ")";
    if (d2Lane) d2Text += " (" + std::to_string(d2Lane->getVehicleCount()) + ")";
    if (d3Lane) d3Text += " (" + std::to_string(d3Lane->getVehicleCount()) + ")";

    drawText(d1Text, windowWidth/4, windowHeight/2 + LANE_WIDTH, {0, 0, 0, 255});
    drawText(d2Text, windowWidth/4, windowHeight/2, {0, 0, 0, 255});
    drawText(d3Text, windowWidth/4, windowHeight/2 - LANE_WIDTH, {0, 0, 0, 255});
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
    if (!trafficManager) return;

    // Draw semi-transparent background
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_FRect overlayRect = {10, 10, 250, 200};
    SDL_RenderFillRect(renderer, &overlayRect);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    // Draw border
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderRect(renderer, &overlayRect);

    int y = 20;
    drawText("Traffic Simulator Stats", 20, y, {255, 255, 255, 255});
    y += 25;

    // Traffic light state
    auto* trafficLight = trafficManager->getTrafficLight();
    if (trafficLight) {
        auto state = trafficLight->getCurrentState();
        std::string stateStr = "Light: ";
        SDL_Color stateColor = {255, 255, 255, 255};

        switch (state) {
            case TrafficLight::State::ALL_RED:
                stateStr += "All Red";
                stateColor = {255, 100, 100, 255};
                break;
            case TrafficLight::State::A_GREEN:
                stateStr += "A Green";
                stateColor = {100, 255, 100, 255};
                break;
            case TrafficLight::State::B_GREEN:
                stateStr += "B Green";
                stateColor = {100, 255, 100, 255};
                break;
            case TrafficLight::State::C_GREEN:
                stateStr += "C Green";
                stateColor = {100, 255, 100, 255};
                break;
            case TrafficLight::State::D_GREEN:
                stateStr += "D Green";
                stateColor = {100, 255, 100, 255};
                break;
        }
        drawText(stateStr, 20, y, stateColor);
        y += 20;
    }

    // Priority status
    Lane* al2Lane = trafficManager->findLane('A', 2);
    if (al2Lane) {
        int count = al2Lane->getVehicleCount();
        std::string priorityStr = "A2: " + std::to_string(count) + " vehicles";

        if (al2Lane->getPriority() > 0) {
            priorityStr += " (PRIORITY)";
            drawText(priorityStr, 20, y, {255, 165, 0, 255});
        } else {
            if (count > 5 && count <= 10) {
                priorityStr += " (5-10 ZONE)";
                drawText(priorityStr, 20, y, {220, 220, 0, 255});
            } else {
                drawText(priorityStr, 20, y, {255, 255, 255, 255});
            }
        }
        y += 20;
    }

    // Free lanes (lane 3) stats
    int totalFreeLaneVehicles = 0;

    Lane* a3Lane = trafficManager->findLane('A', 3);
    Lane* b3Lane = trafficManager->findLane('B', 3);
    Lane* c3Lane = trafficManager->findLane('C', 3);
    Lane* d3Lane = trafficManager->findLane('D', 3);

    if (a3Lane) totalFreeLaneVehicles += a3Lane->getVehicleCount();
    if (b3Lane) totalFreeLaneVehicles += b3Lane->getVehicleCount();
    if (c3Lane) totalFreeLaneVehicles += c3Lane->getVehicleCount();
    if (d3Lane) totalFreeLaneVehicles += d3Lane->getVehicleCount();

    drawText("Free Lanes: " + std::to_string(totalFreeLaneVehicles) +
             " vehicles (all LEFT turn)", 20, y, {0, 200, 0, 255});
    y += 20;

    // Total vehicles
    int totalVehicles = 0;
    for (auto* lane : trafficManager->getLanes()) {
        totalVehicles += lane->getVehicleCount();
    }
    drawText("Total Vehicles: " + std::to_string(totalVehicles), 20, y, {255, 255, 255, 255});
    y += 25;

    // Formula explanation
    drawText("Light Duration = |V| * 2s", 20, y, {200, 200, 255, 255});
    y += 20;
    drawText("|V| = avg. vehicles per normal lane", 20, y, {200, 200, 255, 255});
    y += 20;

    // Show logs if there's space
    std::vector<std::string> logs = DebugLogger::getRecentLogs(3);
    if (!logs.empty()) {
        y += 10;
        drawText("Recent logs:", 20, y, {180, 180, 180, 255});
        y += 15;

        for (const auto& log : logs) {
            std::string shortLog = log;
            if (shortLog.length() > 25) {
                shortLog = shortLog.substr(0, 22) + "...";
            }
            drawText(shortLog, 20, y, {150, 150, 150, 255});
            y += 15;
        }
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
