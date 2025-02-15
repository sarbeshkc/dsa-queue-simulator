// src/core/queue.h
#ifndef QUEUE_H
#define QUEUE_H

#include <vector>
#include <stdexcept>
#include "../traffic/vehicle.h"

// Generic Queue template class that forms the base of our queue system.
// This implements the basic FIFO (First In, First Out) behavior required
// for managing vehicles in lanes.
template <typename T>
class Queue {
public:
    // Default constructor initializes an empty queue with no processing time
    Queue() : m_processingTimer(0.0f) {}

    // Add an item to the back of the queue
    void enqueue(const T& item) {
        m_elements.push_back(item);
    }

    // Remove and return the front item from the queue
    // Throws an exception if the queue is empty
    T dequeue() {
        if (empty()) {
            throw std::runtime_error("Cannot dequeue from empty queue");
        }

        T frontItem = m_elements.front();
        m_elements.erase(m_elements.begin());
        return frontItem;
    }

    // Check if queue is empty
    bool empty() const {
        return m_elements.empty();
    }

    // Get number of items in queue
    size_t size() const {
        return m_elements.size();
    }

    // Get front item without removing it
    // Throws an exception if the queue is empty
    const T& front() const {
        if (empty()) {
            throw std::runtime_error("Queue is empty");
        }
        return m_elements.front();
    }

    // Processing time management - required by assignment specification
    // Each vehicle needs 2 seconds of processing time

    // Update the processing timer for current vehicle
    void updateProcessingTime(float deltaTime) {
        if (!empty()) {
            m_processingTimer += deltaTime;
        }
    }

    // Check if enough time has passed to process next vehicle
    bool canProcess() const {
        return m_processingTimer >= PROCESS_TIME;
    }

    // Reset processing timer after handling a vehicle
    void resetProcessingTime() {
        m_processingTimer = 0.0f;
    }

protected:
    std::vector<T> m_elements;        // Container for queue elements
    float m_processingTimer;          // Tracks time spent processing current vehicle

    // Assignment requirement: each vehicle needs 2 seconds of processing
    static constexpr float PROCESS_TIME = 2.0f;
};

// Specialized queue for managing vehicles in lanes
// Adds priority handling capabilities as required by the assignment
class LaneQueue : public Queue<Vehicle*> {
public:
    // Initialize queue with priority status
    explicit LaneQueue(bool isPriority = false)
        : Queue<Vehicle*>()
        , m_isPriority(isPriority) {}

    // Check if this queue needs priority processing
    // According to assignment: priority needed when queue length >= 10
    bool needsPriorityProcessing() const {
        return m_isPriority && this->size() >= PRIORITY_THRESHOLD;
    }

    // Check if we can exit priority mode
    // According to assignment: exit when queue length <= 5
    bool canExitPriorityMode() const {
        return this->size() <= PRIORITY_RESET_THRESHOLD;
    }

    // Check if this is a priority queue
    bool isPriorityQueue() const {
        return m_isPriority;
    }

private:
    bool m_isPriority;   // Indicates if this is a priority lane queue

    // Constants defined by assignment requirements
    static constexpr size_t PRIORITY_THRESHOLD = 10;      // Start priority mode
    static constexpr size_t PRIORITY_RESET_THRESHOLD = 5; // End priority mode
};

#endif // QUEUE_Hendif // QUEUE_H
