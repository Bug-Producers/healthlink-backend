#pragma once

#include <stdexcept>
#include "QueueNode.h"

/**
 * @brief A standard FIFO (First-In, First-Out) Queue using linked nodes.
 *
 * This template class manages memory by creating and deleting nodes directly.
 *
 * @tparam T The type of data stored in the queue (e.g., int, string, custom objects).
 */
template <typename T>
class Queue {
private:
    QueueNode<T>* frontNode; // Pointer to the front of the queue (where we pop)
    QueueNode<T>* backNode;  // Pointer to the back of the queue (where we push)
    size_t count;            // Tracks the number of elements

public:
    /**
     * @brief Constructs an empty Queue.
     */
    Queue() {
        frontNode = nullptr;
        backNode = nullptr;
        count = 0;
    }

    /**
     * @brief Destructor: Cleans up memory when the queue is destroyed.
     */
    ~Queue() {
        while (!isEmpty()) {
            pop();
        }
    }

    /**
     * @brief Adds a new item to the back of the queue.
     * @param value The item to be added.
     */
    void push(const T& value) {
        QueueNode<T>* newNode = new QueueNode<T>(value);

        if (isEmpty()) {
            frontNode = newNode;
            backNode = newNode;
        } else {
            backNode->next = newNode;
            backNode = newNode;
        }
        count++;
    }

    /**
     * @brief Removes the item from the front of the queue.
     * @throws std::out_of_range If the queue is empty.
     */
    void pop() {
        if (isEmpty()) {
            throw std::out_of_range("Cannot pop from an empty queue!");
        }

        QueueNode<T>* temp = frontNode;
        frontNode = frontNode->next;
        delete temp;
        count--;

        // If the queue is now empty, we must also nullify the back pointer
        if (frontNode == nullptr) {
            backNode = nullptr;
        }
    }

    /**
     * @brief Retrieves the item at the front of the queue.
     * @return Reference to the front item.
     * @throws std::out_of_range If the queue is empty.
     */
    T& front() {
        if (isEmpty()) {
            throw std::out_of_range("Cannot look at the front of an empty queue!");
        }
        return frontNode->data;
    }

    /**
     * @brief Checks if the queue is currently empty.
     * @return True if the queue has no items, false otherwise.
     */
    bool isEmpty() const {
        return frontNode == nullptr;
    }

    /**
     * @brief Gets the total number of items currently in the queue.
     * @return The size of the queue.
     */
    size_t size() const {
        return count;
    }
};
