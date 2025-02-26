#ifndef QUEUE_H
#define QUEUE_H

#include <vector>
#include <mutex>
#include <stdexcept>

// A thread-safe queue implementation for the traffic simulation
template<typename T>
class Queue {
public:
    Queue() = default;
    ~Queue() = default;

    // Add element to the queue
    void enqueue(const T& element) {
        std::lock_guard<std::mutex> lock(mutex);
        elements.push_back(element);
    }

    // Remove and return the front element
    T dequeue() {
        std::lock_guard<std::mutex> lock(mutex);

        if (elements.empty()) {
            throw std::runtime_error("Queue is empty");
        }

        T element = elements.front();
        elements.erase(elements.begin());

        return element;
    }

    // Peek at the front element without removing it
    T peek() const {
        std::lock_guard<std::mutex> lock(mutex);

        if (elements.empty()) {
            throw std::runtime_error("Queue is empty");
        }

        return elements.front();
    }

    // Check if the queue is empty
    bool isEmpty() const {
        std::lock_guard<std::mutex> lock(mutex);
        return elements.empty();
    }

    // Get the size of the queue
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex);
        return elements.size();
    }

    // Clear the queue
    void clear() {
        std::lock_guard<std::mutex> lock(mutex);
        elements.clear();
    }

    // Get all elements for iteration (e.g., for rendering)
    std::vector<T> getAllElements() const {
        std::lock_guard<std::mutex> lock(mutex);
        return elements;
    }

private:
    std::vector<T> elements;
    mutable std::mutex mutex;
};

#endif // QUEUE_H
