#pragma once

#include <stdexcept>
#include "Node.h"
#include "../../utils/CollectionUtils.h"

/**
 * @brief A simple singly linked list template class.
 *
 * @tparam T The type of data stored in the list.
 */
template <typename T>
class LinkedList {
private:
    Node<T>* head;  // Pointer to the first node in the list
    size_t count;   // Tracks the number of elements

public:
    /**
     * @brief Constructs an empty Linked List.
     */
    LinkedList() {
        head = nullptr;
        count = 0;
    }

    /**
     * @brief Copy Constructor (Deep Copy).
     * 
     * Essential for safely copying the list without creating shared memory or dangling pointers.
     * 
     * Usage:
     * @code
     *   LinkedList<int> original;
     *   original.push(5);
     *   LinkedList<int> copy = original; // Safely clones all nodes natively
     * @endcode
     */
    LinkedList(const LinkedList& other) : head(CollectionUtils::cloneNodes(other.head)), count(other.count) {}

    /**
     * @brief Copy Assignment Operator.
     * 
     * Automatically clears existing nodes within the targeted list preventing memory leaks, 
     * before securely transferring the values.
     * 
     * Usage:
     * @code
     *   LinkedList<int> target, source;
     *   target = source; // Wipes target, then safely clones all source nodes
     * @endcode
     */
    LinkedList& operator=(const LinkedList& other) {
        if (this == &other) return *this;
        Node<T>* current = head;
        while (current != nullptr) {
            Node<T>* nextNode = current->next;
            delete current;
            current = nextNode;
        }
        head = CollectionUtils::cloneNodes(other.head);
        count = other.count;
        return *this;
    }

    /**
     * @brief Destructor: Cleans up memory when the list is destroyed.
     * Prevents memory leaks by deleting each node one by one.
     */
    ~LinkedList() {
        Node<T>* current = head;
        while (current != nullptr) {
            Node<T>* nextNode = current->next;
            delete current;
            current = nextNode;
        }
    }

    /**
     * @brief Access the head node directly (useful for iterating over the list manually).
     */
    Node<T>* getHead() const {
        return head;
    }

    /**
     * @brief Adds a new element to the list.
     * @param value The value to add.
     */
    void push(const T& value) {
        Node<T>* newNode = new Node<T>(value, head);
        head = newNode;
        count++;
    }

    /**
     * @brief Checks if the list is empty.
     * @return True if the list contains no elements, false otherwise.
     */
    bool isEmpty() const {
        return head == nullptr;
    }

};
