// include/utils/Queue.h with recursive mutex
#pragma once
#include <memory>
#include <stdexcept>
#include <mutex>

template<typename T>
class Queue {
protected:
    struct Node {
        T data;
        std::shared_ptr<Node> next;
        Node(const T& value) : data(value), next(nullptr) {}
    };

    std::shared_ptr<Node> front;
    std::shared_ptr<Node> rear;
    size_t size;
    mutable std::recursive_mutex mutex; // Changed to recursive mutex to prevent deadlocks

public:
    Queue() : front(nullptr), rear(nullptr), size(0) {}

    virtual ~Queue() = default;

    // Delete copy constructor and assignment operator due to mutex
    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

    // Add move constructor and move assignment
    Queue(Queue&& other) noexcept
        : front(std::move(other.front)),
          rear(std::move(other.rear)),
          size(other.size) {
        other.size = 0;
    }

    Queue& operator=(Queue&& other) noexcept {
        if (this != &other) {
            front = std::move(other.front);
            rear = std::move(other.rear);
            size = other.size;
            other.size = 0;
        }
        return *this;
    }

    void enqueue(const T& value) {
        std::lock_guard<std::recursive_mutex> lock(mutex);

        auto newNode = std::make_shared<Node>(value);
        if (isEmpty()) {
            front = rear = newNode;
        } else {
            rear->next = newNode;
            rear = newNode;
        }
        size++;
    }

    T dequeue() {
        std::lock_guard<std::recursive_mutex> lock(mutex);

        if (isEmpty()) {
            throw std::runtime_error("Queue is empty");
        }

        T value = front->data;
        front = front->next;
        size--;

        if (isEmpty()) {
            rear = nullptr;
        }

        return value;
    }

    // Method that dequeues without locking (for internal use)
    T dequeueUnlocked() {
        if (isEmpty()) {
            throw std::runtime_error("Queue is empty");
        }

        T value = front->data;
        front = front->next;
        size--;

        if (isEmpty()) {
            rear = nullptr;
        }

        return value;
    }

    bool isEmpty() const {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        return front == nullptr;
    }

    size_t getSize() const {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        return size;
    }

    T peek() const {
        std::lock_guard<std::recursive_mutex> lock(mutex);

        if (isEmpty()) {
            throw std::runtime_error("Queue is empty");
        }
        return front->data;
    }

    // Add index-based peek
    T peek(size_t index) const {
        std::lock_guard<std::recursive_mutex> lock(mutex);

        if (size == 0 || index >= size) {
            throw std::out_of_range("Index out of bounds");
        }

        auto current = front;
        for (size_t i = 0; i < index; i++) {
            current = current->next;
        }
        return current->data;
    }
};
