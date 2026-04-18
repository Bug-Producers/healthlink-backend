#pragma once

#include <string>

/**
 * @brief Represents a cash payment recorded by a doctor after a patient visit.
 *
 * The doctor registers each payment
 * through their dashboard to keep a clean financial trail.
 */
struct Payment {
    std::string id;              // Unique identifier for this payment
    std::string doctorId;        // The doctor who received the payment
    std::string patientId;       // The patient who paid
    std::string appointmentId;   // The appointment this payment is linked to
    double amount{0.0};          // Amount paid in USD
    std::string createdAt;       // Timestamp of when it was recorded 
};
