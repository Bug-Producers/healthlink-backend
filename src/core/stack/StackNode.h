#pragma once

/**
 * @brief Represents a single element in the Stack.
 *
 * @tparam T The type of data stored in this node.
 */
template <typename T>
struct StackNode {
    T data;             // The value stored in this node
    StackNode* next;    // Pointer to the next node in the stack

    /**
     * @brief Constructor to easily create a node with a given value and next pointer.
     * @param val The value to store in the node.
     * @param nextNode Pointer to the next node.
     */
    StackNode(T val, StackNode* nextNode = nullptr) {
        data = val;
        next = nextNode;
    }
};
