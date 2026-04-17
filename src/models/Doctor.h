#pragma once

#include <string>

struct Doctor {
    std::string uuid;
    std::string name;
    std::string city;
    std::string country;
    std::string hospitalOrClinicName;
    double rating;
    int expYears;
    int patients;
    std::string about;
    std::string profileImage;
    std::string departmentName;
    int appointmentDuration; // in minutes
    int bufferTime; // in minutes
};
