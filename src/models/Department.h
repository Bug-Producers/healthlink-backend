#pragma once

#include <string>
#include "../core/linked_list/LinkedList.h"

/**
 * @brief Represents a medical department or specialty in a clinic.
 */
struct Department {
    std::string name;                    // Name of the department (e.g., Cardiology, Pediatrics)
    int count;                           // Total number of active doctors in this department
    LinkedList<std::string> doctorIds;   // Linked list storing the unique IDs of doctors assigned to this department
};
