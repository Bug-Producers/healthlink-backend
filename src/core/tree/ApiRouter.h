#pragma once

#include <string>
#include <functional>
#include "Tree.h"

/**
 * @brief Represents a single route segment and the function to execute.
 */
struct RouteSegment {
    std::string pathName;
    std::function<std::string()> handler; // The function to run when this route is hit

    RouteSegment() {
        pathName = "";
        handler = nullptr;
    }

    RouteSegment(std::string name) {
        pathName = name;
        handler = nullptr;
    }

    // Required by Tree.h to find existing children
    bool operator==(const RouteSegment& other) const {
        return pathName == other.pathName;
    }
};

/**
 * @brief Manages API endpoints by storing them in a tree structure.
 *
 * This allows logical grouping of endpoints (e.g., /api/appointments/book)
 */
class ApiRouter {
private:
    Tree<RouteSegment>* routeTree; // The tree storing route segments

public:
    /**
     * @brief Initializes the router with a root node representing the base path.
     */
    ApiRouter() {
        routeTree = new Tree<RouteSegment>(RouteSegment("/"));
    }

    /**
     * @brief Destructor: Cleans up the dynamically allocated tree.
     */
    ~ApiRouter() {
        delete routeTree;
    }

    /**
     * @brief Registers a new API endpoint path into the tree and attaches a function to it.
     * @param path The full endpoint path (e.g., "/api/appointments/book").
     * @param handler The function to execute when this path is requested.
     */
    void registerEndpoint(const std::string& path, std::function<std::string()> handler) {
        TreeNode<RouteSegment>* current = routeTree->getRoot();
        std::string segment = "";

        // Parse the path string and build the tree nodes
        for (size_t i = 0; i < path.length(); ++i) {
            if (path[i] == '/') {
                if (!segment.empty()) {
                    current = routeTree->addChild(current, RouteSegment(segment));
                    segment = "";
                }
            } else {
                segment += path[i];
            }
        }

        // Add the final segment
        if (!segment.empty()) {
            current = routeTree->addChild(current, RouteSegment(segment));
        }

        // Attach the executable function to the final leaf node of this path
        current->data.handler = handler;
    }

    /**
     * @brief Resolves a URL path into its corresponding function.
     * @param path The path to look up (e.g. "/api/doctors/time")
     * @return The function to execute, or nullptr if the path doesn't exist.
     */
    std::function<std::string()> resolveRoute(const std::string& path) const {
        TreeNode<RouteSegment>* current = routeTree->getRoot();
        std::string segment = "";

        for (size_t i = 0; i < path.length(); ++i) {
            if (path[i] == '/') {
                if (!segment.empty()) {
                    current = findChild(current, segment);
                    if (current == nullptr) return nullptr; // Route broken
                    segment = "";
                }
            } else {
                segment += path[i];
            }
        }

        if (!segment.empty()) {
            current = findChild(current, segment);
        }

        if (current != nullptr && current->data.handler != nullptr) {
            return current->data.handler; // Return the executable function!
        }

        return nullptr;
    }

private:
    /**
     * @brief Helper function to find a specific child by its segment value.
     */
    TreeNode<RouteSegment>* findChild(TreeNode<RouteSegment>* parent, const std::string& segment) const {
        if (parent == nullptr) return nullptr;

        auto current = parent->children.getHead();
        while (current != nullptr) {
            if (current->data->data.pathName == segment) {
                return current->data;
            }
            current = current->next;
        }

        return nullptr;
    }
};
