#pragma once

#include <string>
#include "../core/queue/Queue.h"
#include "Patient.h"

/**
 * @brief Represents a specific time block available for a medical appointment.
 */
struct TimeSlot {
    std::string startTime;          // The beginning of the time slot (e.g., 09:00)
    std::string endTime;            // The end of the time slot (e.g., 09:30)
    Queue<Patient> bookings;        // Waitlist queue of patients requesting or booked for this specific time slot
};
