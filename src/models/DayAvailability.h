#pragma once

#include "../core/linked_list/LinkedList.h"
#include "TimeSlot.h"

/**
 * @brief Represents a doctor's availability for a specific day.
 *
 * It tracks whether the doctor is available at all on that day, and if so, what specific time slots are available.
 */
struct DayAvailability {
    bool available;                  // True if the doctor is working on this day, false otherwise
    LinkedList<TimeSlot> slots;      // A list of available time slots for booking on this day
};
