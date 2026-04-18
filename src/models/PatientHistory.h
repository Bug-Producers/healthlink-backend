#pragma once

#include <string>
#include "../core/stack/Stack.h"

/**
 * @brief Represents the medical history of a specific patient.
 */
struct PatientHistory {
    std::string id;                        // Unique identifier for the medical history record
    std::string patientId;                 // ID of the patient to whom this history belongs
    Stack<std::string> medicalReports;     // Stack storing chronological medical reports (most recent first)
};
