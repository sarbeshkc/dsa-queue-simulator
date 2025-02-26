#ifndef DEBUG_OVERLAY_H
#define DEBUG_OVERLAY_H

#include <vector>
#include <string>
#include <SDL3/SDL.h>

// Forward declarations
class TrafficManager;

class DebugOverlay {
public:
    DebugOverlay();
    ~DebugOverlay();

    // Initialize the overlay
    void initialize(TrafficManager* trafficManager);

    // Update overlay data
    void update();

    // Render the overlay
    void render(SDL_Renderer* renderer);

    // Toggle overlay visibility
    void toggleVisibility();

    // Check if overlay is visible
    bool isVisible() const;

    // Add a message to the overlay
    void addMessage(const std::string& message);

    // Clear all messages
    void clearMessages();

private:
    bool visible;
    TrafficManager* trafficManager;
    std::vector<std::string> messages;

    // Render vehicle counts for each lane
    void renderVehicleCounts(SDL_Renderer* renderer);

    // Render traffic light state
    void renderTrafficLightState(SDL_Renderer* renderer);

    // Render priority lane information
    void renderPriorityInfo(SDL_Renderer* renderer);

    // Render custom messages
    void renderMessages(SDL_Renderer* renderer);

    // Helper to render text
    void renderText(SDL_Renderer* renderer, const std::string& text, int x, int y, SDL_Color color);
};

#endif // DEBUG_OVERLAY_H
