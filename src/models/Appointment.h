#pragma once

#include <string>
#include "AppointmentStatus.h"
#include "Doctor.h"
#include "Patient.h"

/**
 * @brief Represents a scheduled medical appointment.
 *
 * Contains information about the doctor, patient, scheduling details, and the current status of the appointment.
 */
struct Appointment {
    std::string id;             // Unique identifier for the appointment
    Doctor doctor;              // The doctor assigned to the appointment
    Patient patient;            // The patient who booked the appointment
    std::string date;           // Date of the appointment in YYYY-MM-DD format
    std::string startTime;      // Start time of the appointment in HH:MM format
    std::string endTime;        // End time of the appointment in HH:MM format
    int duration;               // Duration of the appointment in minutes
    AppointmentStatus status;   // Current status of the appointment (e.g., Booked, Completed)
};
