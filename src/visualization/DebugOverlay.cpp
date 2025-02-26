#include "visualization/DebugOverlay.h"
#include "managers/TrafficManager.h"
#include "core/TrafficLight.h"
#include "utils/DebugLogger.h"
#include "core/Constants.h"

#include <sstream>

DebugOverlay::DebugOverlay()
    : visible(true),
      trafficManager(nullptr) {}

DebugOverlay::~DebugOverlay() {}

void DebugOverlay::initialize(TrafficManager* manager) {
    trafficManager = manager;
    DebugLogger::log("Debug overlay initialized");
}

void DebugOverlay::update() {
    // Update overlay data if needed
}

void DebugOverlay::render(SDL_Renderer* renderer) {
    if (!visible || !trafficManager) {
        return;
    }

    // Draw semi-transparent background
    // Using Constants directly for SDL3
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_FRect overlayRect = {10, 10, 280, 220};
    SDL_RenderFillRect(renderer, &overlayRect);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    // Render overlay sections
    renderVehicleCounts(renderer);
    renderTrafficLightState(renderer);
    renderPriorityInfo(renderer);
    renderMessages(renderer);
}

void DebugOverlay::toggleVisibility() {
    visible = !visible;
    DebugLogger::log("Debug overlay visibility set to " + std::string(visible ? "visible" : "hidden"));
}

bool DebugOverlay::isVisible() const {
    return visible;
}

void DebugOverlay::addMessage(const std::string& message) {
    messages.push_back(message);

    // Keep only the latest 5 messages
    if (messages.size() > 5) {
        messages.erase(messages.begin());
    }
}

void DebugOverlay::clearMessages() {
    messages.clear();
}

void DebugOverlay::renderVehicleCounts(SDL_Renderer* renderer) {
    if (!trafficManager) {
        return;
    }

    const std::vector<Lane*>& lanes = trafficManager->getLanes();
    int totalVehicles = 0;
    int y = 20;

    // Define text color for SDL3
    SDL_Color debugTextColor = {255, 255, 255, 255};
    SDL_Color priorityColor = {255, 165, 0, 255};

    renderText(renderer, "Lane Statistics:", 20, y, debugTextColor);
    y += 20;

    for (Lane* lane : lanes) {
        if (!lane) {
            continue;
        }

        int count = lane->getVehicleCount();
        totalVehicles += count;

        std::stringstream ss;
        ss << lane->getName() << ": " << count << " vehicles";

        if (trafficManager->isLanePrioritized(lane->getLaneId(), lane->getLaneNumber())) {
            ss << " (PRIORITY)";
            renderText(renderer, ss.str(), 20, y, priorityColor);
        } else {
            renderText(renderer, ss.str(), 20, y, debugTextColor);
        }

        y += 15;
    }

    std::stringstream ss;
    ss << "Total Vehicles: " << totalVehicles;
    renderText(renderer, ss.str(), 20, y, debugTextColor);
}

void DebugOverlay::renderTrafficLightState(SDL_Renderer* renderer) {
    if (!trafficManager) {
        return;
    }

    TrafficLight* light = trafficManager->getTrafficLight();
    if (!light) {
        return;
    }

    std::string stateStr;
    SDL_Color stateColor;

    // Define colors for SDL3
    SDL_Color redColor = {255, 0, 0, 255};
    SDL_Color greenColor = {11, 156, 50, 255};

    switch (light->getCurrentState()) {
        case TrafficLight::State::ALL_RED:
            stateStr = "All Red";
            stateColor = redColor;
            break;
        case TrafficLight::State::A_GREEN:
            stateStr = "A Green";
            stateColor = greenColor;
            break;
        case TrafficLight::State::B_GREEN:
            stateStr = "B Green";
            stateColor = greenColor;
            break;
        case TrafficLight::State::C_GREEN:
            stateStr = "C Green";
            stateColor = greenColor;
            break;
        case TrafficLight::State::D_GREEN:
            stateStr = "D Green";
            stateColor = greenColor;
            break;
    }

    renderText(renderer, "Traffic Light: " + stateStr, 20, 160, stateColor);
}

void DebugOverlay::renderPriorityInfo(SDL_Renderer* renderer) {
    if (!trafficManager) {
        return;
    }

    // Define colors for SDL3
    SDL_Color debugTextColor = {255, 255, 255, 255};
    SDL_Color priorityColor = {255, 165, 0, 255};

    Lane* priorityLane = trafficManager->getPriorityLane();
    if (priorityLane && priorityLane->getVehicleCount() > 10) {
        std::stringstream ss;
        ss << "PRIORITY MODE ACTIVE: AL2 has " << priorityLane->getVehicleCount() << " vehicles";
        renderText(renderer, ss.str(), 20, 180, priorityColor);
    } else if (priorityLane) {
        std::stringstream ss;
        ss << "Priority threshold: " << priorityLane->getVehicleCount() << "/10";
        renderText(renderer, ss.str(), 20, 180, debugTextColor);
    }
}

void DebugOverlay::renderMessages(SDL_Renderer* renderer) {
    // Define text color for SDL3
    SDL_Color debugTextColor = {255, 255, 255, 255};

    int y = 200;

    renderText(renderer, "Recent Events:", 20, y, debugTextColor);
    y += 20;

    for (const auto& message : messages) {
        renderText(renderer, "- " + message, 25, y, debugTextColor);
        y += 15;
    }
}

void DebugOverlay::renderText(SDL_Renderer* renderer, const std::string& text, int x, int y, SDL_Color color) {
    // Since we don't have SDL_ttf configured, we'll draw a colored rectangle
    // to indicate text position
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_FRect textRect = {static_cast<float>(x), static_cast<float>(y),
                          static_cast<float>(text.length() * 7), 12};

    // SDL3 uses SDL_RenderRect instead of SDL_RenderDrawRect
    SDL_RenderRect(renderer, &textRect);

    // In a full implementation with SDL_ttf, this would render the actual text
}
