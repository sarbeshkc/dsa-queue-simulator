// queue.cpp
#include <stdexcept>
#include "queue.h"

template <typename T>
void Queue<T>::enqueue(const T& item) {
    m_elements.push_back(item);
}

template <typename T>
T Queue<T>::dequeue() {
    if (empty()) {
        throw std::runtime_error("Queue is empty");
    }

    T front = m_elements.front();
    m_elements.erase(m_elements.begin());
    return front;
}

template <typename T>
const T& Queue<T>::front() const {
    if (empty()) {
        throw std::runtime_error("Queue is empty");
    }
    return m_elements.front();
}

template <typename T>
bool Queue<T>::empty() const {
    return m_elements.empty();
}

template <typename T>
size_t Queue<T>::size() const {
    return m_elements.size();
}

template <typename T>
bool Queue<T>::canProcess() const {
    return m_processingTime >= PROCESS_TIME;
}

template <typename T>
void Queue<T>::updateProcessingTime(float deltaTime) {
    if (!empty()) {
        m_processingTime += deltaTime;
    }
}

template <typename T>
void Queue<T>::resetProcessingTime() {
    m_processingTime = 0.0f;
}

// LaneQueue implementation
LaneQueue::LaneQueue(bool isPriority)
    : m_isPriority(isPriority) {}

bool LaneQueue::needsPriorityProcessing() const {
    return m_isPriority && size() >= PRIORITY_THRESHOLD;
}

bool LaneQueue::canExitPriorityMode() const {
    return size() <= PRIORITY_RESET;
}
