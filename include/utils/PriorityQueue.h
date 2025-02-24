// include/utils/PriorityQueue.h with recursive mutex
#pragma once
#include "Queue.h"
#include <vector>

template <typename T>
class PriorityQueue : public Queue<T> {
private:
  struct PriorityNode : public Queue<T>::Node {
    int priority;
    PriorityNode(const T &value, int p) : Queue<T>::Node(value), priority(p) {}
  };

public:
  // Inheriting constructors
  using Queue<T>::Queue;

  // We need to explicitly delete copy constructor and assignment
  PriorityQueue(const PriorityQueue&) = delete;
  PriorityQueue& operator=(const PriorityQueue&) = delete;

  // And explicitly define move constructor and assignment
  PriorityQueue(PriorityQueue&& other) noexcept : Queue<T>(std::move(other)) {}

  PriorityQueue& operator=(PriorityQueue&& other) noexcept {
    Queue<T>::operator=(std::move(other));
    return *this;
  }

  // Enqueue with priority - higher priority values are dequeued first
  void enqueuePriority(const T &value, int priority) {
    std::lock_guard<std::recursive_mutex> lock(this->mutex);

    auto newNode = std::make_shared<PriorityNode>(value, priority);

    if (this->isEmpty() ||
        static_cast<PriorityNode *>(this->front.get())->priority < priority) {
      newNode->next = this->front;
      this->front = newNode;
    } else {
      auto current = this->front;
      while (current->next &&
             static_cast<PriorityNode *>(current->next.get())->priority >=
                 priority) {
        current = current->next;
      }
      newNode->next = current->next;
      current->next = newNode;
    }
    this->size++;
  }

  // Get the priority of the first item (highest priority) without removing it
  int peekPriority() const {
    std::lock_guard<std::recursive_mutex> lock(this->mutex);

    if (this->isEmpty()) {
      throw std::runtime_error("Queue is empty");
    }
    return static_cast<PriorityNode *>(this->front.get())->priority;
  }

  // Get the priority of an item at a specific index
  int getPriorityAt(size_t index) const {
    std::lock_guard<std::recursive_mutex> lock(this->mutex);

    if (index >= this->size) {
      throw std::out_of_range("Index out of bounds");
    }

    auto current = this->front;
    for (size_t i = 0; i < index; i++) {
      current = current->next;
    }
    return static_cast<PriorityNode *>(current.get())->priority;
  }

  // Update the priority of an item that matches the given value
  // Returns true if at least one item was updated
  bool updatePriority(const T &value, int newPriority) {
    std::lock_guard<std::recursive_mutex> lock(this->mutex);

    if (this->isEmpty()) {
      return false;
    }

    // Find and remove all matching items
    std::vector<T> removedItems;
    auto current = this->front;
    auto prev = std::shared_ptr<typename Queue<T>::Node>(nullptr);

    while (current) {
      if (current->data == value) {
        if (prev) {
          prev->next = current->next;
        } else {
          this->front = current->next;
        }

        if (current == this->rear) {
          this->rear = prev;
        }

        removedItems.push_back(current->data);
        this->size--;

        auto next = current->next;
        current = next;
      } else {
        prev = current;
        current = current->next;
      }
    }

    // Re-add all removed items with the new priority
    for (const auto& item : removedItems) {
      enqueuePriority(item, newPriority);
    }

    return !removedItems.empty();
  }
};
