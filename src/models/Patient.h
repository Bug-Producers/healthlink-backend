#pragma once

#include <string>

struct Patient {
    std::string id;
    std::string name;
    std::string email;
    std::string phone;
    std::string dateOfBirth; // YYYY-MM-DD
    std::string gender;
    std::string address;
    std::string bloodGroup;
};
