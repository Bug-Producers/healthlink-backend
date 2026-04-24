#pragma once

#include <string>
#include <future>
#include <chrono>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>

#include "../models/Notification.h"
#include "../core/linked_list/LinkedList.h"
#include "../services/MongoService.h"
#include "../services/NotificationGateway.h"

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

/**
 * @brief Stores and accesses global push notification events seamlessly.
 */
class NotificationRepository {
private:
    MongoService mongo_{"healthlink"};
    NotificationGateway* gateway_{nullptr};

    static std::string generateId() {
        static int counter = 0;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        return "notif_" + std::to_string(ms) + "_" + std::to_string(++counter);
    }

public:
    NotificationRepository() {}

    void setGateway(NotificationGateway* gateway) {
        gateway_ = gateway;
    }

    /**
     * @brief Instantly fires off an async alert payload directly into Mongo
     */
    std::future<bool> pushNotification(const std::string& userId, const std::string& message) {
        return std::async(std::launch::async, [this, userId, message]() -> bool {
            auto doc = make_document(
                kvp("id", generateId()),
                kvp("userId", userId),
                kvp("message", message),
                kvp("createdAt", "Just now"),
                kvp("read", false)
            );
            
            bool ok = mongo_.insertOne("notifications", doc.view());
            if (ok && gateway_) {
                gateway_->sendRealTime(userId, message);
            }
            return ok;
        });
    }

    /**
     * @brief Pulls down a user's unread sequence of alerts
     */
    std::future<LinkedList<Notification>> getNotifications(const std::string& userId) {
        return std::async(std::launch::async, [this, userId]() -> LinkedList<Notification> {
            auto docs = mongo_.findMany("notifications", make_document(kvp("userId", userId)).view());
            LinkedList<Notification> results{};
            for (auto& raw : docs) {
                auto view = raw.view();
                Notification n{};
                n.id = std::string{view["id"].get_string().value};
                n.userId = std::string{view["userId"].get_string().value};
                n.message = std::string{view["message"].get_string().value};
                n.createdAt = std::string{view["createdAt"].get_string().value};
                n.read = view["read"].get_bool().value;
                results.push(n);
            }
            return results;
        });
    }

    /**
     * @brief Marks a notification as read.
     */
    std::future<bool> markAsRead(const std::string& notificationId) {
        return std::async(std::launch::async, [this, notificationId]() -> bool {
            return mongo_.updateOne("notifications",
                make_document(kvp("id", notificationId)).view(),
                make_document(kvp("$set", make_document(
                    kvp("read", true)
                ))).view()
            );
        });
    }
};
