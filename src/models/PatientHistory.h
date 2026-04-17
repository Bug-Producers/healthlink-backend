#pragma once

#include <string>
#include <vector>

struct PatientHistory {
    std::string id;
    std::string patientId;
    std::vector<std::string> allergies;
    std::vector<std::string> chronicDiseases;
    std::vector<std::string> pastSurgeries;
    std::vector<std::string> currentMedications;
    std::string familyMedicalHistory;
    std::string notes;
};
