#pragma once

#include <string>

/**
 * @brief Represents a registered patient in the system.
 *
 * Stores personal, contact, and demographic data.
 */
struct Patient {
    std::string id;            // Unique identifier for the patient
    std::string name;          // Full name of the patient
    std::string email;         // Contact email address
    std::string dateOfBirth;   // Date of birth in YYYY-MM-DD format
    std::string gender;        // Biological or identified gender (e.g., Male, Female, Other)
};
