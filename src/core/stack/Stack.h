#pragma once

#include <stdexcept>
#include "StackNode.h"

/**
 * @brief A standard LIFO (Last-In, First-Out) Stack using linked nodes.
 *
 * This template class manages memory by creating and deleting nodes directly.
 *
 * @tparam T The type of data stored in the stack (e.g., int, string, custom objects).
 */
template <typename T>
class Stack {
private:
    StackNode<T>* topNode; // Pointer to the top of the stack
    size_t count;          // Tracks the number of elements

public:
    /**
     * @brief Constructs an empty Stack.
     */
    Stack() {
        topNode = nullptr;
        count = 0;
    }

    /**
     * @brief Destructor: Cleans up memory when the stack is destroyed.
     */
    ~Stack() {
        while (!isEmpty()) {
            pop();
        }
    }

    /**
     * @brief Pushes a new item onto the top of the stack.
     * @param value The item to be added.
     */
    void push(const T& value) {
        // Create a new node. It holds the new value, and its 'next' points to the old topNode.
        StackNode<T>* newNode = new StackNode<T>(value, topNode);

        // Update topNode to point to our newly created node.
        topNode = newNode;
        count++;
    }

    /**
     * @brief Removes the item from the top of the stack.
     * @throws std::out_of_range If the stack is empty.
     */
    void pop() {
        if (isEmpty()) {
            throw std::out_of_range("Cannot pop from an empty stack!");
        }
        StackNode<T>* temp = topNode;
        topNode = topNode->next;
        delete temp;
        count--;
    }

    /**
     * @brief Retrieves the item at the top of the stack without removing it.
     * @return A reference to the top item.
     * @throws std::out_of_range If the stack is empty.
     */
    T& top() {
        if (isEmpty()) {
            throw std::out_of_range("Cannot look at the top of an empty stack!");
        }
        return topNode->data;
    }

    /**
     * @brief Checks if the stack is currently empty.
     * @return True if the stack has no items, false otherwise.
     */
    bool isEmpty() const {
        return topNode == nullptr;
    }

    /**
     * @brief Gets the total number of items currently in the stack.
     * @return The size of the stack.
     */
    size_t size() const {
        return count;
    }
};
