#pragma once

#include <string>
#include <map>
#include "DayAvailability.h"

struct WeeklySchedule {
    std::string doctorId;
    std::string timezone;
    std::map<std::string, DayAvailability> days; // monday to sunday
    std::string updatedAt; // ISO date
};
