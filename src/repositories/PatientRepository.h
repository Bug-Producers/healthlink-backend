#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <future>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>

#include "../models/Patient.h"
#include "../services/MongoService.h"

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

/**
 * @brief Manages patient records.
 */
class PatientRepository {
private:
    MongoService mongo_{"healthlink"};
    std::unordered_map<std::string, Patient> cache_;
    mutable std::mutex mtx_;

    Patient fromBson(bsoncxx::document::view doc) {
        Patient p{};
        p.id          = std::string{doc["id"].get_string().value};
        p.name        = std::string{doc["name"].get_string().value};
        p.email       = std::string{doc["email"].get_string().value};
        p.dateOfBirth = std::string{doc["dateOfBirth"].get_string().value};
        p.gender      = std::string{doc["gender"].get_string().value};
        return p;
    }

    bsoncxx::document::value toBson(const Patient& p) {
        return make_document(
            kvp("id",          p.id),
            kvp("name",        p.name),
            kvp("email",       p.email),
            kvp("dateOfBirth", p.dateOfBirth),
            kvp("gender",      p.gender)
        );
    }

public:
    PatientRepository() {}

    /**
     * @brief Looks up a patient by ID.
     */
    std::future<Patient> findById(const std::string& patientId) {
        return std::async(std::launch::async, [this, patientId]() -> Patient {
            {
                std::lock_guard<std::mutex> lock{mtx_};
                auto it = cache_.find(patientId);
                if (it != cache_.end()) return it->second;
            }

            auto result = mongo_.findOne("patients", make_document(kvp("id", patientId)).view());
            if (!result) throw std::runtime_error("Patient not found: " + patientId);

            Patient p = fromBson(result->view());

            std::lock_guard<std::mutex> lock{mtx_};
            cache_[patientId] = p;
            return p;
        });
    }

    /**
     * @brief Registers a new patient.
     */
    std::future<bool> create(const Patient& patient) {
        return std::async(std::launch::async, [this, patient]() -> bool {
            bool ok = mongo_.insertOne("patients", toBson(patient).view());
            if (ok) {
                std::lock_guard<std::mutex> lock{mtx_};
                cache_[patient.id] = patient;
            }
            return ok;
        });
    }
};
