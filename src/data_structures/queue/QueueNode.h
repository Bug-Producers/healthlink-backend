#pragma once

/**
 * @brief Represents a single element in the Queue.
 *
 * @tparam T The type of data stored in this node.
 */
template <typename T>
struct QueueNode {
    T data;             // The value stored in this node
    QueueNode* next;    // Pointer to the next node in the queue

    /**
     * @brief Constructor to easily create a node with a given value and next pointer.
     * @param val The value to store in the node.
     * @param nextNode Pointer to the next node (defaults to nullptr).
     */
    QueueNode(T val, QueueNode* nextNode = nullptr) {
        data = val;
        next = nextNode;
    }
};
