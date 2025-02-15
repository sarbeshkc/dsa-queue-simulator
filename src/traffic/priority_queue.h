// src/traffic/priority_queue.h
#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include "vehicle.h"
#include <chrono>

class PriorityQueue {
private:
    struct Node {
        Vehicle* vehicle;
        float priority;
        std::chrono::steady_clock::time_point entryTime;
        Node* next;

        Node(Vehicle* v, float p)
            : vehicle(v)
            , priority(p)
            , entryTime(std::chrono::steady_clock::now())
            , next(nullptr) {}
    };

    Node* head;
    int size;
    float processingTime;
    bool isPriorityLane;
    static constexpr float PROCESS_TIME = 2.0f; // 2 seconds per vehicle
    static constexpr int PRIORITY_THRESHOLD = 10;
    static constexpr int PRIORITY_RESET_THRESHOLD = 5;

public:
    PriorityQueue(bool isPriority = false)
        : head(nullptr)
        , size(0)
        , processingTime(0.0f)
        , isPriorityLane(isPriority) {}

    ~PriorityQueue() {
        while (head != nullptr) {
            Node* temp = head;
            head = head->next;
            delete temp;
        }
    }

    // Adds a vehicle to the queue with priority handling
    void enqueue(Vehicle* vehicle, float basePriority = 0.0f) {
        float priority = basePriority;

        // Increase priority if queue length exceeds threshold
        if (isPriorityLane && size >= PRIORITY_THRESHOLD) {
            priority += 1000.0f;
        }

        Node* newNode = new Node(vehicle, priority);

        // Insert based on priority
        if (head == nullptr || priority > head->priority) {
            newNode->next = head;
            head = newNode;
        } else {
            Node* current = head;
            while (current->next != nullptr &&
                   current->next->priority >= priority) {
                current = current->next;
            }
            newNode->next = current->next;
            current->next = newNode;
        }
        size++;
    }

    // Removes and returns the highest priority vehicle
    Vehicle* dequeue() {
        if (empty()) return nullptr;

        Node* temp = head;
        Vehicle* vehicle = temp->vehicle;
        head = head->next;
        delete temp;
        size--;

        return vehicle;
    }

    // Queue state queries
    bool empty() const { return head == nullptr; }
    int getSize() const { return size; }
    bool isPriority() const { return isPriorityLane; }

    // Gets wait time of front vehicle
    float getWaitTime() const {
        if (empty()) return 0.0f;
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>
                       (now - head->entryTime).count();
        return static_cast<float>(duration);
    }

    // Processing time management
    void updateProcessingTime(float deltaTime) {
        if (!empty()) {
            processingTime += deltaTime;
        }
    }

    bool readyToProcess() const {
        return processingTime >= PROCESS_TIME;
    }

    void resetProcessingTime() {
        processingTime = 0.0f;
    }

    // Priority management
    bool needsPriorityProcessing() const {
        return isPriorityLane && size >= PRIORITY_THRESHOLD;
    }

    bool canExitPriorityMode() const {
        return size <= PRIORITY_RESET_THRESHOLD;
    }
};

#endif // PRIORITY_QUEUE_H
