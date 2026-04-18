#pragma once

#include <stdexcept>
#include "Node.h"

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
    LinkedList(const LinkedList& other) {
        head = nullptr;
        count = 0;
        
        if (!other.head) return;
        
        Node<T>* otherCurrent = other.head;
        Node<T>* tail = nullptr;
        
        while (otherCurrent != nullptr) {
            Node<T>* newNode = new Node<T>(otherCurrent->data, nullptr);
            if (!head) {
                head = newNode;
                tail = newNode;
            } else {
                tail->next = newNode;
                tail = newNode;
            }
            count++;
            otherCurrent = otherCurrent->next;
        }
    }

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
        
        head = nullptr;
        count = 0;

        if (!other.head) return *this;
        
        Node<T>* otherCurrent = other.head;
        Node<T>* tail = nullptr;
        
        while (otherCurrent != nullptr) {
            Node<T>* newNode = new Node<T>(otherCurrent->data, nullptr);
            if (!head) {
                head = newNode;
                tail = newNode;
            } else {
                tail->next = newNode;
                tail = newNode;
            }
            count++;
            otherCurrent = otherCurrent->next;
        }
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
     * @brief Removes the first element from the list.
     * @throws std::out_of_range If the list is empty.
     */
    void pop() {
        if (isEmpty()) {
            throw std::out_of_range("List is empty");
        }
        Node<T>* temp = head;
        head = head->next;
        delete temp;
        count--;
    }

    /**
     * @brief Retrieves the data of the first element.
     * @return Reference to the first element's data.
     * @throws std::out_of_range If the list is empty.
     */
    T& peek() {
        if (isEmpty()) {
            throw std::out_of_range("List is empty");
        }
        return head->data;
    }

    /**
     * @brief Retrieves the data of the first element (const version).
     * @return Const reference to the first element's data.
     * @throws std::out_of_range If the list is empty.
     */
    const T& peek() const {
        if (isEmpty()) {
            throw std::out_of_range("List is empty");
        }
        return head->data;
    }

    /**
     * @brief Checks if the list is empty.
     * @return True if the list contains no elements, false otherwise.
     */
    bool isEmpty() const {
        return head == nullptr;
    }

    /**
     * @brief Returns the total number of items in the list.
     * @return The size of the list.
     */
    size_t size() const {
        return count;
    }
};
