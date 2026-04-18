#pragma once

#include <string>
#include <vector>
#include <functional>
#include <sstream>
#include <crow.h>
#include "../tree/Tree.h"
#include "../../utils/StringUtils.h"

// Prevent Windows headers from hijacking the HTTP Method macro
#ifdef _WIN32
#undef DELETE
#endif

// Clean type alias for readability
using HttpHandler = std::function<crow::response(const crow::request&)>;

/**
 * @brief Represents a single piece of the URL path along with its registered HTTP methods.
 */
struct RouteSegment {
    std::string pathName;
    
    // Lambdas that hold the actual controller logic
    HttpHandler getHandler;
    HttpHandler postHandler;
    HttpHandler putHandler;
    HttpHandler deleteHandler;

    RouteSegment(const std::string& name = "") : pathName(name) {}
    
    // Required by Tree.h to find matching child nodes easily
    bool operator==(const RouteSegment& other) const { return pathName == other.pathName; }
};

/**
 * @brief Manages API routing using a Tree data structure.
 * 
 * Instead of flat routing, paths are parsed into segments and
 * attached to a tree, ensuring true structural routing logic
 * as required by the backend guidelines.
 */
class ApiRouter {
private:
    Tree<RouteSegment>* routeTree;

public:
    /**
     * @brief Constructor: Sets up the root of our URL Tree
     */
    ApiRouter() { routeTree = new Tree<RouteSegment>(RouteSegment("/")); }

    /**
     * @brief Destructor: Tears down the Tree securely
     */
    ~ApiRouter() { delete routeTree; }

    /**
     * @brief Digs into the tree layer-by-layer, adding new path nodes if they don't exist yet,
     * and finally assigns the HTTP function to the last segment's block.
     */
    void addRoute(const std::string& method, const std::string& path, HttpHandler handler) {
        auto segments = StringUtils::split(path);
        TreeNode<RouteSegment>* current = routeTree->getRoot();

        for (const auto& segment : segments) {
            TreeNode<RouteSegment>* nextNode = nullptr;
            auto* node = current->children.getHead();
            while (node) {
                if (node->data->data.pathName == segment) {
                    nextNode = node->data;
                    break;
                }
                node = node->next;
            }

            if (!nextNode) {
                nextNode = routeTree->addChild(current, RouteSegment(segment));
            }
            current = nextNode;
        }

        if (method == "GET") current->data.getHandler = std::move(handler);
        else if (method == "POST") current->data.postHandler = std::move(handler);
        else if (method == "PUT") current->data.putHandler = std::move(handler);
        else if (method == "DELETE") current->data.deleteHandler = std::move(handler);
    }

    void get(const std::string& path, HttpHandler handler) { addRoute("GET", path, std::move(handler)); }
    void post(const std::string& path, HttpHandler handler) { addRoute("POST", path, std::move(handler)); }
    void put(const std::string& path, HttpHandler handler) { addRoute("PUT", path, std::move(handler)); }
    void del(const std::string& path, HttpHandler handler) { addRoute("DELETE", path, std::move(handler)); }

    /**
     * @brief Recursively navigates the tree nodes to resolve raw URIs and execute matching lambdas.
     */
    crow::response handleRequest(const crow::request& req) const {
        auto segments = StringUtils::split(req.url);
        TreeNode<RouteSegment>* current = routeTree->getRoot();

        for (const auto& segment : segments) {
            TreeNode<RouteSegment>* matchedNode = nullptr;
            TreeNode<RouteSegment>* wildNode = nullptr;

            // Search children for an exact match or a wildcard match ("*")
            auto* node = current->children.getHead();
            while (node) {
                if (node->data->data.pathName == segment) {
                    matchedNode = node->data;
                    break;
                }
                if (node->data->data.pathName == "*") {
                    wildNode = node->data;
                }
                node = node->next;
            }

            // Exact match takes priority, fallback to wildcard, error if nowhere to go
            if (matchedNode) current = matchedNode;
            else if (wildNode) current = wildNode;
            else return crow::response(404, "Route not found in Tree");
        }

        // We reached the end of the URL — choose the right function based on HTTP method
        HttpHandler handler;
        if (req.method == crow::HTTPMethod::GET) handler = current->data.getHandler;
        else if (req.method == crow::HTTPMethod::POST) handler = current->data.postHandler;
        else if (req.method == crow::HTTPMethod::PUT) handler = current->data.putHandler;
        else if (req.method == crow::HTTPMethod::DELETE) handler = current->data.deleteHandler;

        if (handler) return handler(req);
        return crow::response(405, "Method not allowed by Tree Route");
    }
};
