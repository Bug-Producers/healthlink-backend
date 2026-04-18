#pragma once

#include <string>
#include <stdexcept>
#include "TreeNode.h"

/**
 * @brief A generic Tree structure used to logically group data hierarchically (e.g. API endpoints).
 *
 * @tparam T The type of data stored in the tree (e.g., string for route segments).
 */
template <typename T>
class Tree {
private:
    TreeNode<T>* root; // Pointer to the root of the tree

public:
    /**
     * @brief Constructs a new Tree with a root node value.
     * @param rootValue The value for the top-level node (e.g., "/api")
     */
    Tree(T rootValue) {
        root = new TreeNode<T>(rootValue);
    }

    /**
     * @brief Destructor: Automatically cleans up all nodes starting from the root.
     */
    ~Tree() {
        delete root;
    }

    /**
     * @brief Gets the root node of the tree.
     */
    TreeNode<T>* getRoot() const {
        return root;
    }

    /**
     * @brief Adds a child to a specific parent node.
     * @param parent The node to attach the child to.
     * @param childValue The value of the new child node.
     * @return Pointer to the newly created child node.
     */
    TreeNode<T>* addChild(TreeNode<T>* parent, const T& childValue) {
        if (parent == nullptr) {
            throw std::invalid_argument("Parent node cannot be null");
        }

        // Check if a child with this value already exists to avoid duplicates
        auto current = parent->children.getHead();
        while (current != nullptr) {
            if (current->data->data == childValue) {
                return current->data; // Return existing child
            }
            current = current->next;
        }

        // If it doesn't exist, create a new child and add it to the parent's children list
        TreeNode<T>* newChild = new TreeNode<T>(childValue);
        parent->children.push(newChild);

        return newChild;
    }
};
