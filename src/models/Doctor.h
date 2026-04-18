#pragma once

#include <string>
#include "Department.h"

/**
 * @brief Represents a medical professional (Doctor) in the system.
 *
 * Contains demographic details, professional background, clinic details, and metrics
 * for the doctor profile.
 */
struct Doctor {
    std::string uuid;                   // Unique identifier for the doctor
    std::string name;                   // Full name of the doctor
    std::string city;                   // City where the doctor practices
    std::string country;                // Country where the doctor practices
    std::string hospitalOrClinicName;   // The name of the hospital/clinic where the doctor operates
    double rating;                      // Patient-given average rating (e.g., 4.5 out of 5.0)
    int expYears;                       // Number of years of professional experience
    int patients;                       // Total number of patients treated or registered
    std::string about;                  // Biography or summary profile of the doctor
    std::string profileImage;           // URL or path to the doctor's profile image
    Department department;              // The department or specialty the doctor belongs to
    int appointmentDuration;            // Average duration of a standard appointment (in minutes)
    int bufferTime;                     // Buffer break time between appointments (in minutes)
};
