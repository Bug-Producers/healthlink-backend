#pragma once

#include <crow.h>
#include <string>
#include <functional>
#include <sstream>

#include "../tree/Tree.h"
#include "../../utils/StringUtils.h"

using HttpHandler = std::function<crow::response(const crow::request&)>;

struct RouteSegment {
    std::string pathName;

    HttpHandler getHandler;
    HttpHandler postHandler;
    HttpHandler putHandler;
    HttpHandler deleteHandler;

    RouteSegment(const std::string& name = "") : pathName(name) {}

    bool operator==(const RouteSegment& other) const {
        return pathName == other.pathName;
    }
};

class ApiRouter {
private:
    Tree<RouteSegment>* routeTree;

    std::string normalizePath(const std::string& url) const {
        std::string path = url;

        // remove query string
        auto qpos = path.find('?');
        if (qpos != std::string::npos)
            path = path.substr(0, qpos);

        if (path.empty()) return "/";

        return path;
    }

public:
    ApiRouter() {
        routeTree = new Tree<RouteSegment>(RouteSegment("/"));
    }

    ~ApiRouter() {
        delete routeTree;
    }

    void addRoute(const std::string& method,
                  const std::string& path,
                  HttpHandler handler) {

        auto segments = StringUtils::split(path);
        TreeNode<RouteSegment>* current = routeTree->getRoot();

        for (const auto& segment : segments) {
            if (segment.empty()) continue;

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

        if (method == "GET")         current->data.getHandler    = std::move(handler);
        else if (method == "POST")   current->data.postHandler   = std::move(handler);
        else if (method == "PUT")    current->data.putHandler    = std::move(handler);
        else if (method == "DELETE") current->data.deleteHandler = std::move(handler);
    }

    void get(const std::string& path, HttpHandler handler) {
        addRoute("GET", path, std::move(handler));
    }

    void post(const std::string& path, HttpHandler handler) {
        addRoute("POST", path, std::move(handler));
    }

    void put(const std::string& path, HttpHandler handler) {
        addRoute("PUT", path, std::move(handler));
    }

    void del(const std::string& path, HttpHandler handler) {
        addRoute("DELETE", path, std::move(handler));
    }

    crow::response handleRequest(const crow::request& req) const {
        std::string path = normalizePath(req.url);

        auto segments = StringUtils::split(path);
        TreeNode<RouteSegment>* current = routeTree->getRoot();

        for (const auto& segment : segments) {
            if (segment.empty()) continue;

            TreeNode<RouteSegment>* matchedNode = nullptr;
            TreeNode<RouteSegment>* wildNode = nullptr;

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

            if (matchedNode) {
                current = matchedNode;
            } else if (wildNode) {
                current = wildNode;
            } else {
                return crow::response(404, "Route not found");
            }
        }

        HttpHandler handler;

        switch (req.method) {
            case crow::HTTPMethod::Get:
                handler = current->data.getHandler;
                break;

            case crow::HTTPMethod::Post:
                handler = current->data.postHandler;
                break;

            case crow::HTTPMethod::Put:
                handler = current->data.putHandler;
                break;

            case crow::HTTPMethod::Delete:
                handler = current->data.deleteHandler;
                break;

            default:
                return crow::response(405, "Unsupported method");
        }

        if (!handler) {
            return crow::response(405, "Method not allowed");
        }

        return handler(req);
    }
};