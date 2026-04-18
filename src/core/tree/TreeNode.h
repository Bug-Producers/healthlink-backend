#pragma once

#include "../linked_list/LinkedList.h"

/**
 * @brief Represents a single node in the Tree.
 *
 * @tparam T The type of data stored in this node.
 */
template <typename T>
struct TreeNode {
    T data;                                // The value stored in this node (e.g., a route segment)
    LinkedList<TreeNode<T>*> children;     // A linked list of child pointers

    /**
     * @brief Constructor to easily create a node with a given value.
     * @param val The value to store in the node.
     */
    TreeNode(T val) {
        data = val;
    }

    /**
     * @brief Destructor recursively deletes all children to prevent memory leaks.
     */
    ~TreeNode() {
        auto current = children.getHead();
        while (current != nullptr) {
            delete current->data; // This deletes the child TreeNode, triggering its own destructor recursively
            current = current->next;
        }
    }
};
