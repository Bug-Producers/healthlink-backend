#pragma once

/**
 * @brief Represents a single element in the Linked List.
 *
 * @tparam T The type of data stored in this node.
 */
template <typename T>
struct Node {
    T data;         // The value stored in this node
    Node* next;     // Pointer to the next node in the list

    /**
     * @brief Constructor to easily create a node with a given value.
     * @param val The value to store in the node.
     * @param nextNode Pointer to the next node (defaults to nullptr).
     */
    Node(T val, Node* nextNode = nullptr) {
        data = val;
        next = nextNode;
    }
};
