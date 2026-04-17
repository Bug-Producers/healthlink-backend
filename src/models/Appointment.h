#pragma once

#include <string>
#include "AppointmentStatus.h"

struct Appointment {
    std::string id;
    std::string doctorId;
    std::string patientId;
    std::string date;      // YYYY-MM-DD
    std::string startTime; // HH:MM
    std::string endTime;   // HH:MM
    int duration;
    AppointmentStatus status;
};
