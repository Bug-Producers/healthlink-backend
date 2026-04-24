#pragma once

#include <crow.h>
#include <unordered_map>
#include <mutex>
#include <string>
#include <iostream>

/**
 * @brief Manages live WebSocket connections for real-time notifications.
 */
class NotificationGateway {
private:
    // Maps userId -> active WebSocket connection
    std::unordered_map<std::string, crow::websocket::connection*> connections_;
    // Reverse map for cleanup on close
    std::unordered_map<crow::websocket::connection*, std::string> reverseConnections_;
    std::mutex mtx_;

public:
    NotificationGateway() = default;

    /**
     * @brief Registers a new active connection for a user.
     */
    void registerConnection(const std::string& userId, crow::websocket::connection* conn) {
        std::lock_guard<std::mutex> lock(mtx_);
        connections_[userId] = conn;
        reverseConnections_[conn] = userId;
        std::cout << "[WS] User " << userId << " authenticated. Total: " << connections_.size() << "\n";
    }

    /**
     * @brief Removes a connection (e.g. on close).
     */
    void unregisterConnection(crow::websocket::connection* conn) {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = reverseConnections_.find(conn);
        if (it != reverseConnections_.end()) {
            std::string uid = it->second;
            connections_.erase(uid);
            reverseConnections_.erase(it);
            std::cout << "[WS] User " << uid << " disconnected. Remaining: " << connections_.size() << "\n";
        }
    }

    /**
     * @brief Attempts to push a real-time message to a user if they are online.
     */
    /**
     * @brief Attempts to push a real-time message to a user if they are online.
     */
    void sendRealTime(const std::string& userId, const std::string& payload) {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = connections_.find(userId);
        if (it != connections_.end() && it->second) {
            try {
                it->second->send_text(payload);
                std::cout << "[WS] Real-time notification delivered to " << userId << "\n";
            } catch (const std::exception& e) {
                std::cerr << "[WS] Failed to send to " << userId << ": " << e.what() << "\n";
            }
        }
    }

    /**
     * @brief Broadcasts a message to ALL connected users instantly.
     */
    void broadcast(const std::string& message) {
        std::lock_guard<std::mutex> lock(mtx_);
        for (auto& pair : connections_) {
            if (pair.second) {
                try {
                    pair.second->send_text("SYSTEM ALERT: " + message);
                } catch (...) {
                    // Ignore failures on individual broadcasts
                }
            }
        }
        std::cout << "[WS] System alert broadcast to " << connections_.size() << " users.\n";
    }
};
