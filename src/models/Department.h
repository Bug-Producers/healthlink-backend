#pragma once

#include <string>
#include <vector>
#include "Doctor.h"

struct Department {
    std::string name;
    int count;
    std::vector<Doctor> doctors;
};
