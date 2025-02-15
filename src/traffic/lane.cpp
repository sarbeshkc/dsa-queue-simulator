// src/traffic/lane.cp// src/traffic/lane.cpp
#include "lane.h"
#include <cmath>
#include <algorithm>

// Initialize a new lane with all its properties
Lane::Lane(SDL_Renderer* renderer, LaneId id, Vector2D start, Vector2D end, bool isPriority)
    : m_renderer(renderer)
    , m_id(id)
    , m_startPos(start)
    , m_endPos(end)
    , m_isPriority(isPriority)
    , m_processingTimer(0.0f) {
}

void Lane::update(float deltaTime) {
    // If we have vehicles in the queue, we need to update our processing timer
    // This implements the assignment's 2-second processing time requirement
    if (!m_queue.empty()) {
        m_processingTimer += deltaTime;
    }

    // Make sure all vehicles are properly positioned in the queue
    updateVehiclePositions();
}

Vehicle* Lane::processNextVehicle() {
    // We can only process a vehicle if:
    // 1. There is a vehicle waiting
    // 2. We've waited the required 2 seconds
    if (m_queue.empty() || m_processingTimer < PROCESS_TIME) {
        return nullptr;
    }

    // Get the next vehicle and remove it from the queue
    Vehicle* vehicle = m_queue.front();
    m_queue.pop();

    // Reset our timer for the next vehicle
    m_processingTimer = 0.0f;

    return vehicle;
}

void Lane::addVehicle(Vehicle* vehicle) {
    if (!vehicle) return;  // Safety check

    // Add to queue and update all vehicle positions
    m_queue.push(vehicle);
    updateVehiclePositions();
}

bool Lane::removeVehicle(Vehicle* vehicle) {
    // Since std::queue doesn't support direct removal,
    // we need to recreate the queue without the target vehicle
    std::queue<Vehicle*> newQueue;
    bool found = false;

    // Go through all vehicles, keeping all except the target
    while (!m_queue.empty()) {
        Vehicle* current = m_queue.front();
        m_queue.pop();

        if (current != vehicle) {
            newQueue.push(current);
        } else {
            found = true;
        }
    }

    // Replace our queue with the new one
    m_queue = std::move(newQueue);

    // If we found and removed a vehicle, update positions
    if (found) {
        updateVehiclePositions();
    }

    return found;
}

int Lane::getQueueLength() const {
    return static_cast<int>(m_queue.size());
}

bool Lane::hasWaitingVehicles() const {
    return !m_queue.empty();
}

bool Lane::isPriorityLane() const {
    return m_isPriority;
}

Vehicle* Lane::getNextVehicle() const {
    return m_queue.empty() ? nullptr : m_queue.front();
}

void Lane::updateVehiclePositions() {
    // Create a temporary queue to process all vehicles
    std::queue<Vehicle*> tempQueue = m_queue;
    size_t position = 0;

    // Position each vehicle in sequence
    while (!tempQueue.empty()) {
        Vehicle* vehicle = tempQueue.front();
        tempQueue.pop();

        if (vehicle) {
            positionVehicleInQueue(vehicle, position++);
        }
    }
}

void Lane::positionVehicleInQueue(Vehicle* vehicle, size_t queuePosition) {
    if (!vehicle) return;

    // Calculate how far along the lane this vehicle should be
    float offset = static_cast<float>(queuePosition) * VEHICLE_SPACING;

    // Calculate the lane's direction vector
    Vector2D direction = {
        m_endPos.x - m_startPos.x,
        m_endPos.y - m_startPos.y
    };

    // Normalize the direction vector
    float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    direction.x /= length;
    direction.y /= length;

    // Calculate the vehicle's new position
    Vector2D newPos = {
        m_startPos.x + direction.x * offset,
        m_startPos.y + direction.y * offset
    };

    // Update the vehicle's position
    vehicle->setPosition(newPos.x, newPos.y);
}

void Lane::render() const {
    // Choose lane color - priority lanes are brighter
    SDL_SetRenderDrawColor(m_renderer,
        m_isPriority ? 255 : 200,  // Brighter for priority lanes
        200, 200, 255);

    // Draw the lane line
    SDL_RenderLine(m_renderer,
        static_cast<int>(m_startPos.x),
        static_cast<int>(m_startPos.y),
        static_cast<int>(m_endPos.x),
        static_cast<int>(m_endPos.y));

    // Render each vehicle in the queue
    std::queue<Vehicle*> renderQueue = m_queue;
    while (!renderQueue.empty()) {
        Vehicle* vehicle = renderQueue.front();
        renderQueue.pop();

        if (vehicle) {
            vehicle->render();
        }
    }
}

bool Lane::isVehicleAtIntersection(const Vehicle* vehicle) const {
    if (!vehicle) return false;
    return calculateDistanceToIntersection(vehicle->getPosition()) < INTERSECTION_THRESHOLD;
}

float Lane::calculateDistanceToIntersection(const Vector2D& pos) const {
    // Calculate straight-line distance to intersection (lane end)
    float dx = m_endPos.x - pos.x;
    float dy = m_endPos.y - pos.y;
    return std::sqrt(dx * dx + dy * dy);
}
