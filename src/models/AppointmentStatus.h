#pragma once

/**
 * @brief Enumerates the possible statuses of an appointment.
 */
enum class AppointmentStatus {
    Booked = 0,      // The appointment is scheduled and confirmed
    Completed = 1,   // The appointment has taken place and is finished
    Cancelled = 2    // The appointment was cancelled by either party
};
