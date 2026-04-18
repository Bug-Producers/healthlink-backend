#pragma once

#include <string>

/**
 * @brief Represents an in-app push notification for a user.
 */
struct Notification {
    std::string id;
    std::string userId; // patientId or doctorId
    std::string message;
    std::string createdAt;
    bool read;
};
