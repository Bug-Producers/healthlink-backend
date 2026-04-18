#pragma once

/**
 * @brief Enumerates the possible statuses of an appointment.
 */
enum class AppointmentStatus {
    Booked,      // The appointment is scheduled and confirmed
    Cancelled,   // The appointment was cancelled by either party
    Completed    // The appointment has taken place and is finished
};
