#pragma once

#include <string>

/**
 * @brief Represents a star rating submitted by a patient for a doctor.
 *
 * After an appointment, the patient can rate their experience from
 * 1 to 5 stars with an optional comment. These ratings contribute
 * to the doctor's public profile score.
 */
struct Rating {
    std::string id;          // Unique identifier for this rating
    std::string patientId;   // The patient who submitted the rating
    std::string doctorId;    // The doctor being rated
    int stars{0};            // Score from 1 (poor) to 5 (excellent)
    std::string comment;     // Optional written feedback
    std::string createdAt;   // When the rating was submitted
};
