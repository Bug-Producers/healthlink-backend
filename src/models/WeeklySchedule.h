#pragma once

#include <string>
#include <unordered_map>
#include "../core/linked_list/LinkedList.h"
#include "TimeSlot.h"

/**
 * @brief Represents the recurring weekly availability schedule of a doctor.
 */
struct WeeklySchedule {
    std::string doctorId;                                                // Unique ID of the doctor to whom this schedule belongs
    std::unordered_map<std::string, LinkedList<TimeSlot>> availability;  // Map of days (e.g. "monday") to a list of available time slots
};
