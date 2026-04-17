#pragma once

#include <vector>
#include "TimeSlot.h"

struct DayAvailability {
    bool available;
    std::vector<TimeSlot> slots;
};
