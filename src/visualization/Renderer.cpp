// FILE: src/visualization/Renderer.cpp
#include "visualization/Renderer.h"
#include "core/Lane.h"
#include "core/Vehicle.h"
#include "core/TrafficLight.h"
#include "managers/TrafficManager.h"
#include "utils/DebugLogger.h"
#include "core/Constants.h"

#include <sstream>
#include <algorithm>
#include <cmath>

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
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255); // Darker background
    SDL_RenderClear(renderer);

    // Draw roads and lanes
    drawRoadsAndLanes();

    // Draw traffic lights
    drawTrafficLights();

    // Draw vehicles
    drawVehicles();

    // Draw lane labels and direction indicators
    drawLaneLabels();

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
    const int ROAD_WIDTH = Constants::ROAD_WIDTH;
    const int LANE_WIDTH = Constants::LANE_WIDTH;

    // Draw dark gray background for the whole road network
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
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

    // Draw clear lane dividers for all roads
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White for lane dividers

    // Draw road labels - A, B, C, D for better orientation
    int textSize = 16;

    // Road A (North)
    drawText("A (North)", windowWidth/2 - 40, 10, {255, 255, 255, 255});

    // Road B (East)
    drawText("B (East)", windowWidth - 80, windowHeight/2 - 10, {255, 255, 255, 255});

    // Road C (South)
    drawText("C (South)", windowWidth/2 - 40, windowHeight - 30, {255, 255, 255, 255});

    // Road D (West)
    drawText("D (West)", 10, windowHeight/2 - 10, {255, 255, 255, 255});

    // Horizontal lane dividers (3 lanes per road)
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    // Draw center double-yellow line
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Yellow

    // Horizontal center double line
    SDL_FRect hCenterLine1 = {
        0, static_cast<float>(windowHeight/2 - 1),
        static_cast<float>(windowWidth), 2.0f
    };
    SDL_FRect hCenterLine2 = {
        0, static_cast<float>(windowHeight/2 - 5),
        static_cast<float>(windowWidth), 2.0f
    };
    SDL_RenderFillRect(renderer, &hCenterLine1);
    SDL_RenderFillRect(renderer, &hCenterLine2);

    // Vertical center double line
    SDL_FRect vCenterLine1 = {
        static_cast<float>(windowWidth/2 - 1), 0,
        2.0f, static_cast<float>(windowHeight)
    };
    SDL_FRect vCenterLine2 = {
        static_cast<float>(windowWidth/2 - 5), 0,
        2.0f, static_cast<float>(windowHeight)
    };
    SDL_RenderFillRect(renderer, &vCenterLine1);
    SDL_RenderFillRect(renderer, &vCenterLine2);

    // Switch back to white for lane dividers
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    // Horizontal lane dividers
    for (int i = 1; i < 3; i++) {
        float y1 = windowHeight/2 - ROAD_WIDTH/2 + i * LANE_WIDTH;
        float y2 = windowHeight/2 + i * LANE_WIDTH;

        // Top road lanes (going down)
        for (int x = 0; x < windowWidth/2 - ROAD_WIDTH/2; x += 30) {
            SDL_RenderLine(renderer, x, y1, x + 15, y1);
        }

        // Bottom road lanes (going up)
        for (int x = windowWidth/2 + ROAD_WIDTH/2; x < windowWidth; x += 30) {
            SDL_RenderLine(renderer, x, y2, x + 15, y2);
        }
    }

    // Vertical lane dividers
    for (int i = 1; i < 3; i++) {
        float x1 = windowWidth/2 - ROAD_WIDTH/2 + i * LANE_WIDTH;
        float x2 = windowWidth/2 + i * LANE_WIDTH;

        // Left road lanes (going right)
        for (int y = 0; y < windowHeight/2 - ROAD_WIDTH/2; y += 30) {
            SDL_RenderLine(renderer, x1, y, x1, y + 15);
        }

        // Right road lanes (going left)
        for (int y = windowHeight/2 + ROAD_WIDTH/2; y < windowHeight; y += 30) {
            SDL_RenderLine(renderer, x2, y, x2, y + 15);
        }
    }

    // Highlight lane types with subtle background colors
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // A2 (Priority Lane) - light orange highlight
    if (trafficManager && trafficManager->isLanePrioritized('A', 2)) {
        SDL_SetRenderDrawColor(renderer, 255, 165, 0, 80); // Semi-transparent orange
        SDL_FRect priorityLane = {
            static_cast<float>(windowWidth/2),
            0,
            static_cast<float>(LANE_WIDTH),
            static_cast<float>(windowHeight/2 - ROAD_WIDTH/2)
        };
        SDL_RenderFillRect(renderer, &priorityLane);
    }

    // Free Lanes (L3) - light green highlights
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 50); // Semi-transparent green

    // A3 (North Road)
    SDL_FRect freeALane = {
        static_cast<float>(windowWidth/2 + LANE_WIDTH),
        0,
        static_cast<float>(LANE_WIDTH),
        static_cast<float>(windowHeight/2 - ROAD_WIDTH/2)
    };
    SDL_RenderFillRect(renderer, &freeALane);

    // B3 (East Road)
    SDL_FRect freeBLane = {
        static_cast<float>(windowWidth/2 + ROAD_WIDTH/2),
        static_cast<float>(windowHeight/2 + LANE_WIDTH),
        static_cast<float>(windowWidth - (windowWidth/2 + ROAD_WIDTH/2)),
        static_cast<float>(LANE_WIDTH)
    };
    SDL_RenderFillRect(renderer, &freeBLane);

    // C3 (South Road)
    SDL_FRect freeCLane = {
        static_cast<float>(windowWidth/2 - 2*LANE_WIDTH),
        static_cast<float>(windowHeight/2 + ROAD_WIDTH/2),
        static_cast<float>(LANE_WIDTH),
        static_cast<float>(windowHeight - (windowHeight/2 + ROAD_WIDTH/2))
    };
    SDL_RenderFillRect(renderer, &freeCLane);

    // D3 (West Road)
    SDL_FRect freeDLane = {
        0,
        static_cast<float>(windowHeight/2 - 2*LANE_WIDTH),
        static_cast<float>(windowWidth/2 - ROAD_WIDTH/2),
        static_cast<float>(LANE_WIDTH)
    };
    SDL_RenderFillRect(renderer, &freeDLane);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    // Draw lane labels directly on the road
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    // A Road (North) lane labels
    drawText("A1", windowWidth/2 - LANE_WIDTH/2 - 10, windowHeight/4, {255, 255, 255, 255});
    drawText("A2", windowWidth/2 + LANE_WIDTH/2 - 10, windowHeight/4, {255, 255, 255, 255});
    drawText("A3", windowWidth/2 + LANE_WIDTH + LANE_WIDTH/2 - 10, windowHeight/4, {255, 255, 255, 255});

    // B Road (East) lane labels
    drawText("B1", 3*windowWidth/4, windowHeight/2 - LANE_WIDTH/2 - 10, {255, 255, 255, 255});
    drawText("B2", 3*windowWidth/4, windowHeight/2 + LANE_WIDTH/2 - 10, {255, 255, 255, 255});
    drawText("B3", 3*windowWidth/4, windowHeight/2 + LANE_WIDTH + LANE_WIDTH/2 - 10, {255, 255, 255, 255});

    // C Road (South) lane labels
    drawText("C1", windowWidth/2 + LANE_WIDTH/2 - 10, 3*windowHeight/4, {255, 255, 255, 255});
    drawText("C2", windowWidth/2 - LANE_WIDTH/2 - 10, 3*windowHeight/4, {255, 255, 255, 255});
    drawText("C3", windowWidth/2 - LANE_WIDTH - LANE_WIDTH/2 - 10, 3*windowHeight/4, {255, 255, 255, 255});

    // D Road (West) lane labels
    drawText("D1", windowWidth/4, windowHeight/2 + LANE_WIDTH/2 - 10, {255, 255, 255, 255});
    drawText("D2", windowWidth/4, windowHeight/2 - LANE_WIDTH/2 - 10, {255, 255, 255, 255});
    drawText("D3", windowWidth/4, windowHeight/2 - LANE_WIDTH - LANE_WIDTH/2 - 10, {255, 255, 255, 255});

    // Draw small arrows indicating traffic direction for each lane
    // North road (A) - traffic going down
    drawDirectionArrow(windowWidth/2 - LANE_WIDTH/2, windowHeight/4, Direction::DOWN, {255, 255, 255, 255}); // A1
    drawDirectionArrow(windowWidth/2 + LANE_WIDTH/2, windowHeight/4, Direction::DOWN, {255, 255, 255, 255}); // A2
    drawDirectionArrow(windowWidth/2 + LANE_WIDTH + LANE_WIDTH/2, windowHeight/4, Direction::DOWN, {255, 255, 255, 255}); // A3

    // East road (B) - traffic going left
    drawDirectionArrow(3*windowWidth/4, windowHeight/2 - LANE_WIDTH/2, Direction::LEFT, {255, 255, 255, 255}); // B1
    drawDirectionArrow(3*windowWidth/4, windowHeight/2 + LANE_WIDTH/2, Direction::LEFT, {255, 255, 255, 255}); // B2
    drawDirectionArrow(3*windowWidth/4, windowHeight/2 + LANE_WIDTH + LANE_WIDTH/2, Direction::LEFT, {255, 255, 255, 255}); // B3

    // South road (C) - traffic going up
    drawDirectionArrow(windowWidth/2 + LANE_WIDTH/2, 3*windowHeight/4, Direction::UP, {255, 255, 255, 255}); // C1
    drawDirectionArrow(windowWidth/2 - LANE_WIDTH/2, 3*windowHeight/4, Direction::UP, {255, 255, 255, 255}); // C2
    drawDirectionArrow(windowWidth/2 - LANE_WIDTH - LANE_WIDTH/2, 3*windowHeight/4, Direction::UP, {255, 255, 255, 255}); // C3

    // West road (D) - traffic going right
    drawDirectionArrow(windowWidth/4, windowHeight/2 + LANE_WIDTH/2, Direction::RIGHT, {255, 255, 255, 255}); // D1
    drawDirectionArrow(windowWidth/4, windowHeight/2 - LANE_WIDTH/2, Direction::RIGHT, {255, 255, 255, 255}); // D2
    drawDirectionArrow(windowWidth/4, windowHeight/2 - LANE_WIDTH - LANE_WIDTH/2, Direction::RIGHT, {255, 255, 255, 255}); // D3

    // Draw stop lines at intersection
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    // Top stop line (A road)
    SDL_FRect topStop = {
        static_cast<float>(windowWidth/2 - ROAD_WIDTH/2),
        static_cast<float>(windowHeight/2 - ROAD_WIDTH/2 - 4),
        static_cast<float>(ROAD_WIDTH),
        4.0f
    };
    SDL_RenderFillRect(renderer, &topStop);

    // Bottom stop line (C road)
    SDL_FRect bottomStop = {
        static_cast<float>(windowWidth/2 - ROAD_WIDTH/2),
        static_cast<float>(windowHeight/2 + ROAD_WIDTH/2),
        static_cast<float>(ROAD_WIDTH),
        4.0f
    };
    SDL_RenderFillRect(renderer, &bottomStop);

    // Left stop line (D road)
    SDL_FRect leftStop = {
        static_cast<float>(windowWidth/2 - ROAD_WIDTH/2 - 4),
        static_cast<float>(windowHeight/2 - ROAD_WIDTH/2),
        4.0f,
        static_cast<float>(ROAD_WIDTH)
    };
    SDL_RenderFillRect(renderer, &leftStop);

    // Right stop line (B road)
    SDL_FRect rightStop = {
        static_cast<float>(windowWidth/2 + ROAD_WIDTH/2),
        static_cast<float>(windowHeight/2 - ROAD_WIDTH/2),
        4.0f,
        static_cast<float>(ROAD_WIDTH)
    };
    SDL_RenderFillRect(renderer, &rightStop);
}



// Helper method to draw direction arrows
// FILE: src/visualization/Renderer.cpp
// Implementation of drawDirectionArrow method

// FILE: src/visualization/Renderer.cpp
// Corrected implementation of drawDirectionArrow method

void Renderer::drawDirectionArrow(int x, int y, Direction dir, SDL_Color color) {
    SDL_SetRenderDrawColor(this->renderer, color.r, color.g, color.b, color.a);

    const int arrowSize = 12;

    SDL_FPoint points[3];

    switch (dir) {
        case Direction::UP:
            points[0] = {static_cast<float>(x), static_cast<float>(y - arrowSize/2)};
            points[1] = {static_cast<float>(x - arrowSize/2), static_cast<float>(y + arrowSize/2)};
            points[2] = {static_cast<float>(x + arrowSize/2), static_cast<float>(y + arrowSize/2)};
            break;

        case Direction::DOWN:
            points[0] = {static_cast<float>(x), static_cast<float>(y + arrowSize/2)};
            points[1] = {static_cast<float>(x - arrowSize/2), static_cast<float>(y - arrowSize/2)};
            points[2] = {static_cast<float>(x + arrowSize/2), static_cast<float>(y - arrowSize/2)};
            break;

        case Direction::LEFT:
            points[0] = {static_cast<float>(x - arrowSize/2), static_cast<float>(y)};
            points[1] = {static_cast<float>(x + arrowSize/2), static_cast<float>(y - arrowSize/2)};
            points[2] = {static_cast<float>(x + arrowSize/2), static_cast<float>(y + arrowSize/2)};
            break;

        case Direction::RIGHT:
            points[0] = {static_cast<float>(x + arrowSize/2), static_cast<float>(y)};
            points[1] = {static_cast<float>(x - arrowSize/2), static_cast<float>(y - arrowSize/2)};
            points[2] = {static_cast<float>(x - arrowSize/2), static_cast<float>(y + arrowSize/2)};
            break;
    }

    // Draw outline
    SDL_RenderLine(this->renderer, points[0].x, points[0].y, points[1].x, points[1].y);
    SDL_RenderLine(this->renderer, points[1].x, points[1].y, points[2].x, points[2].y);
    SDL_RenderLine(this->renderer, points[2].x, points[2].y, points[0].x, points[0].y);

    // Create SDL vertices for filled triangle
    SDL_Vertex vertices[3];
    SDL_FColor fcolor = {
        static_cast<float>(color.r) / 255.0f,
        static_cast<float>(color.g) / 255.0f,
        static_cast<float>(color.b) / 255.0f,
        static_cast<float>(color.a) / 255.0f
    };

    // Set vertices
    for (int i = 0; i < 3; i++) {
        vertices[i].position = points[i];
        vertices[i].color = fcolor;
    }

    // Draw filled triangle
    SDL_RenderGeometry(this->renderer, NULL, vertices, 3, NULL, 0);
}

void Renderer::drawLaneLabels() {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    const int ROAD_WIDTH = Constants::ROAD_WIDTH;
    const int LANE_WIDTH = Constants::LANE_WIDTH;

    // Road A (North - Top) labels
    drawText("A (North)", windowWidth/2, 10, {255, 255, 255, 255});
    drawText("A1", windowWidth/2 - LANE_WIDTH, windowHeight/4, {0, 140, 255, 255});
    drawText("A2 (Priority)", windowWidth/2, windowHeight/4, {255, 140, 0, 255});
    drawText("A3 (Free)", windowWidth/2 + LANE_WIDTH, windowHeight/4, {0, 220, 60, 255});

    // Road B (East - Right) labels
    drawText("B (East)", windowWidth - 60, windowHeight/2, {255, 255, 255, 255});
    drawText("B1", 3*windowWidth/4, windowHeight/2 - LANE_WIDTH, {0, 140, 255, 255});
    drawText("B2", 3*windowWidth/4, windowHeight/2, {255, 255, 255, 255});
    drawText("B3 (Free)", 3*windowWidth/4, windowHeight/2 + LANE_WIDTH, {0, 220, 60, 255});

    // Road C (South - Bottom) labels
    drawText("C (South)", windowWidth/2, windowHeight - 30, {255, 255, 255, 255});
    drawText("C1", windowWidth/2 + LANE_WIDTH, 3*windowHeight/4, {0, 140, 255, 255});
    drawText("C2", windowWidth/2, 3*windowHeight/4, {255, 255, 255, 255});
    drawText("C3 (Free)", windowWidth/2 - LANE_WIDTH, 3*windowHeight/4, {0, 220, 60, 255});

    // Road D (West - Left) labels
    drawText("D (West)", 50, windowHeight/2, {255, 255, 255, 255});
    drawText("D1", windowWidth/4, windowHeight/2 + LANE_WIDTH, {0, 140, 255, 255});
    drawText("D2", windowWidth/4, windowHeight/2, {255, 255, 255, 255});
    drawText("D3 (Free)", windowWidth/4, windowHeight/2 - LANE_WIDTH, {0, 220, 60, 255});

    // Draw direction arrows
    // North (Road A) direction arrows
    drawArrow(windowWidth/2 - LANE_WIDTH*1.5, windowHeight/4 + 15, // Lane A1 arrow (straight)
             windowWidth/2 - LANE_WIDTH*1.5, windowHeight/4 + 30,
             windowWidth/2 - LANE_WIDTH*1.5 - 5, windowHeight/4 + 25,
             {0, 140, 255, 255});

    // Draw split arrows for Lane A2 (priority) - straight or right
    drawArrow(windowWidth/2, windowHeight/4 + 15, // Lane A2 arrow straight
             windowWidth/2, windowHeight/4 + 30,
             windowWidth/2 - 5, windowHeight/4 + 25,
             {255, 140, 0, 255});
    drawArrow(windowWidth/2 + 10, windowHeight/4 + 15, // Lane A2 arrow right
             windowWidth/2 + 20, windowHeight/4 + 20,
             windowWidth/2 + 15, windowHeight/4 + 10,
             {255, 140, 0, 255});

    // Draw left arrow for Lane A3 (free)
    drawArrow(windowWidth/2 + LANE_WIDTH*1.5 - 10, windowHeight/4 + 15, // Lane A3 arrow left
             windowWidth/2 + LANE_WIDTH*1.5 - 20, windowHeight/4 + 20,
             windowWidth/2 + LANE_WIDTH*1.5 - 15, windowHeight/4 + 10,
             {0, 220, 60, 255});

    // East (Road B) direction arrows - similar logic
    drawArrow(3*windowWidth/4 - 15, windowHeight/2 - LANE_WIDTH*1.5, // Lane B1 arrow (straight)
             3*windowWidth/4 - 30, windowHeight/2 - LANE_WIDTH*1.5,
             3*windowWidth/4 - 25, windowHeight/2 - LANE_WIDTH*1.5 - 5,
             {0, 140, 255, 255});

    // Draw split arrows for Lane B2 - straight or right
    drawArrow(3*windowWidth/4 - 15, windowHeight/2, // Lane B2 arrow straight
             3*windowWidth/4 - 30, windowHeight/2,
             3*windowWidth/4 - 25, windowHeight/2 - 5,
             {255, 255, 255, 255});
    drawArrow(3*windowWidth/4 - 15, windowHeight/2 + 10, // Lane B2 arrow right
             3*windowWidth/4 - 20, windowHeight/2 + 20,
             3*windowWidth/4 - 10, windowHeight/2 + 15,
             {255, 255, 255, 255});

    // Draw left arrow for Lane B3 (free)
    drawArrow(3*windowWidth/4 - 15, windowHeight/2 + LANE_WIDTH*1.5 - 10, // Lane B3 arrow left
             3*windowWidth/4 - 20, windowHeight/2 + LANE_WIDTH*1.5 - 20,
             3*windowWidth/4 - 10, windowHeight/2 + LANE_WIDTH*1.5 - 15,
             {0, 220, 60, 255});

    // South (Road C) direction arrows
    drawArrow(windowWidth/2 + LANE_WIDTH*1.5, 3*windowHeight/4 - 15, // Lane C1 arrow (straight)
             windowWidth/2 + LANE_WIDTH*1.5, 3*windowHeight/4 - 30,
             windowWidth/2 + LANE_WIDTH*1.5 + 5, 3*windowHeight/4 - 25,
             {0, 140, 255, 255});

    // Draw split arrows for Lane C2 - straight or right
    drawArrow(windowWidth/2, 3*windowHeight/4 - 15, // Lane C2 arrow straight
             windowWidth/2, 3*windowHeight/4 - 30,
             windowWidth/2 + 5, 3*windowHeight/4 - 25,
             {255, 255, 255, 255});
    drawArrow(windowWidth/2 - 10, 3*windowHeight/4 - 15, // Lane C2 arrow right
             windowWidth/2 - 20, 3*windowHeight/4 - 20,
             windowWidth/2 - 15, 3*windowHeight/4 - 10,
             {255, 255, 255, 255});

    // Draw left arrow for Lane C3 (free)
    drawArrow(windowWidth/2 - LANE_WIDTH*1.5 + 10, 3*windowHeight/4 - 15, // Lane C3 arrow left
             windowWidth/2 - LANE_WIDTH*1.5 + 20, 3*windowHeight/4 - 20,
             windowWidth/2 - LANE_WIDTH*1.5 + 15, 3*windowHeight/4 - 10,
             {0, 220, 60, 255});

    // West (Road D) direction arrows
    drawArrow(windowWidth/4 + 15, windowHeight/2 + LANE_WIDTH*1.5, // Lane D1 arrow (straight)
             windowWidth/4 + 30, windowHeight/2 + LANE_WIDTH*1.5,
             windowWidth/4 + 25, windowHeight/2 + LANE_WIDTH*1.5 + 5,
             {0, 140, 255, 255});

    // Draw split arrows for Lane D2 - straight or right
    drawArrow(windowWidth/4 + 15, windowHeight/2, // Lane D2 arrow straight
             windowWidth/4 + 30, windowHeight/2,
             windowWidth/4 + 25, windowHeight/2 + 5,
             {255, 255, 255, 255});
    drawArrow(windowWidth/4 + 15, windowHeight/2 - 10, // Lane D2 arrow right
             windowWidth/4 + 20, windowHeight/2 - 20,
             windowWidth/4 + 10, windowHeight/2 - 15,
             {255, 255, 255, 255});

    // Draw left arrow for Lane D3 (free)
    drawArrow(windowWidth/4 + 15, windowHeight/2 - LANE_WIDTH*1.5 + 10, // Lane D3 arrow left
             windowWidth/4 + 20, windowHeight/2 - LANE_WIDTH*1.5 + 20,
             windowWidth/4 + 10, windowHeight/2 - LANE_WIDTH*1.5 + 15,
             {0, 220, 60, 255});
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
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200); // More opaque background
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_FRect overlayRect = {10, 10, 280, 180}; // Larger overlay
    SDL_RenderFillRect(renderer, &overlayRect);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    // Add border
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderRect(renderer, &overlayRect);

    // Draw statistics
    drawStatistics();

    // Draw title
    drawText("Traffic Junction Simulator", 20, 20, {255, 255, 255, 255});
    drawText("Press D to toggle debug overlay", 20, 40, {200, 200, 200, 255});

    // Draw recent logs
    std::vector<std::string> logs = DebugLogger::getRecentLogs(5);
    int y = 170;

    for (const auto& log : logs) {
        std::string truncatedLog = log.length() > 50 ? log.substr(0, 47) + "..." : log;
        drawText(truncatedLog, 10, y, {200, 200, 200, 255});
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
    int y = 60;

    while (std::getline(stream, line)) {
        // Check if line contains priority info
        if (line.find("PRIORITY") != std::string::npos) {
            drawText(line, 20, y, {255, 140, 0, 255}); // Highlight priority lanes
        } else if (line.find("A2") != std::string::npos) {
            drawText(line, 20, y, {255, 200, 0, 255}); // Highlight A2 lane
        } else {
            drawText(line, 20, y, {255, 255, 255, 255});
        }
        y += 20;
    }

    // Show current traffic light state
    SDL_Color stateColor = {255, 255, 255, 255};
    std::string stateText = "Traffic Light: ";

    auto* trafficLight = trafficManager->getTrafficLight();
    if (trafficLight) {
        auto currentState = trafficLight->getCurrentState();
        switch (currentState) {
            case TrafficLight::State::ALL_RED:
                stateText += "All Red";
                stateColor = {255, 100, 100, 255};
                break;
            case TrafficLight::State::A_GREEN:
                stateText += "A Green (North)";
                stateColor = {100, 255, 100, 255};
                break;
            case TrafficLight::State::B_GREEN:
                stateText += "B Green (East)";
                stateColor = {100, 255, 100, 255};
                break;
            case TrafficLight::State::C_GREEN:
                stateText += "C Green (South)";
                stateColor = {100, 255, 100, 255};
                break;
            case TrafficLight::State::D_GREEN:
                stateText += "D Green (West)";
                stateColor = {100, 255, 100, 255};
                break;
        }
    }

    drawText(stateText, 20, y, stateColor);
}

void Renderer::drawText(const std::string& text, int x, int y, SDL_Color color) {
    // Since we don't have SDL_ttf configured, draw a colored rectangle
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_FRect textRect = {static_cast<float>(x), static_cast<float>(y),
                         static_cast<float>(8 * text.length()), 15};

    // Draw colored rectangle representing text
    SDL_RenderFillRect(renderer, &textRect);

    // Draw text outline
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &textRect);
}

void Renderer::drawArrow(int x1, int y1, int x2, int y2, int x3, int y3, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    // Draw triangle outline
    SDL_RenderLine(renderer, x1, y1, x2, y2);
    SDL_RenderLine(renderer, x2, y2, x3, y3);
    SDL_RenderLine(renderer, x3, y3, x1, y1);

    // Create vertices for filled triangle with SDL_FColor for SDL3 compatibility
    SDL_Vertex vertices[3];

    // Convert SDL_Color to SDL_FColor for vertices
    SDL_FColor fcolor = {
        static_cast<float>(color.r) / 255.0f,
        static_cast<float>(color.g) / 255.0f,
        static_cast<float>(color.b) / 255.0f,
        static_cast<float>(color.a) / 255.0f
    };

    // First vertex
    vertices[0].position.x = x1;
    vertices[0].position.y = y1;
    vertices[0].color = fcolor;

    // Second vertex
    vertices[1].position.x = x2;
    vertices[1].position.y = y2;
    vertices[1].color = fcolor;

    // Third vertex
    vertices[2].position.x = x3;
    vertices[2].position.y = y3;
    vertices[2].color = fcolor;

    // Draw the filled triangle
    SDL_RenderGeometry(renderer, NULL, vertices, 3, NULL, 0);
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
