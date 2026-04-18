#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <future>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>

#include "../models/Doctor.h"
#include "../core/linked_list/LinkedList.h"
#include "../services/MongoService.h"

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

/**
 * @brief Handles all doctor-related data access.
 */
class DoctorRepository {
private:
    MongoService mongo_{"healthlink"};                        // Talks to MongoDB for us
    std::unordered_map<std::string, Doctor> cache_;           // Quick UUID lookup
    mutable std::mutex mtx_;                                  // Guards the cache

    /**
     * @brief Reads a BSON document and fills in a Doctor struct.
     */
    Doctor fromBson(bsoncxx::document::view doc) {
        Doctor d{};
        d.uuid                 = std::string{doc["uuid"].get_string().value};
        d.name                 = std::string{doc["name"].get_string().value};
        d.city                 = std::string{doc["city"].get_string().value};
        d.country              = std::string{doc["country"].get_string().value};
        d.hospitalOrClinicName = std::string{doc["hospitalOrClinicName"].get_string().value};
        d.rating               = doc["rating"].get_double().value;
        d.expYears             = doc["expYears"].get_int32().value;
        d.patients             = doc["patients"].get_int32().value;
        d.about                = std::string{doc["about"].get_string().value};
        d.profileImage         = std::string{doc["profileImage"].get_string().value};
        d.appointmentDuration  = doc["appointmentDuration"].get_int32().value;
        d.bufferTime           = doc["bufferTime"].get_int32().value;

        // Unpack the nested department
        if (doc.find("department") != doc.end()) {
            auto dept = doc["department"].get_document().view();
            d.department.name  = std::string{dept["name"].get_string().value};
            d.department.count = dept["count"].get_int32().value;
        }

        return d;
    }

    /**
     * @brief Packs a Doctor struct into BSON for storage.
     */
    bsoncxx::document::value toBson(const Doctor& d) {
        return make_document(
            kvp("uuid",                 d.uuid),
            kvp("name",                 d.name),
            kvp("city",                 d.city),
            kvp("country",              d.country),
            kvp("hospitalOrClinicName", d.hospitalOrClinicName),
            kvp("rating",               d.rating),
            kvp("expYears",             d.expYears),
            kvp("patients",             d.patients),
            kvp("about",                d.about),
            kvp("profileImage",         d.profileImage),
            kvp("appointmentDuration",  d.appointmentDuration),
            kvp("bufferTime",           d.bufferTime),
            kvp("department", make_document(
                kvp("name",  d.department.name),
                kvp("count", d.department.count)
            ))
        );
    }

public:
    DoctorRepository() {}

    /**
     * @brief Finds a single doctor by UUID.
     */
    std::future<Doctor> findById(const std::string& doctorId) {
        return std::async(std::launch::async, [this, doctorId]() -> Doctor {
            // Check if we already have this doctor cached
            {
                std::lock_guard<std::mutex> lock{mtx_};
                auto it = cache_.find(doctorId);
                if (it != cache_.end()) return it->second;
            }

            // Not cached — ask the service layer
            auto result = mongo_.findOne("doctors", make_document(kvp("uuid", doctorId)).view());
            if (!result) throw std::runtime_error("Doctor not found: " + doctorId);

            Doctor doc = fromBson(result->view());

            // Remember it for next time
            std::lock_guard<std::mutex> lock{mtx_};
            cache_[doctorId] = doc;
            return doc;
        });
    }

    /**
     * @brief Pulls every doctor from the database into a LinkedList.
     */
    std::future<LinkedList<Doctor>> findAll() {
        return std::async(std::launch::async, [this]() -> LinkedList<Doctor> {
            auto docs = mongo_.findAll("doctors");

            LinkedList<Doctor> doctors{};
            std::lock_guard<std::mutex> lock{mtx_};
            for (auto& raw : docs) {
                Doctor d = fromBson(raw.view());
                cache_[d.uuid] = d;
                doctors.push(d);
            }
            return doctors;
        });
    }

    /**
     * @brief Finds all doctors in a specific department.
     */
    std::future<LinkedList<Doctor>> findByDepartment(const std::string& deptName) {
        return std::async(std::launch::async, [this, deptName]() -> LinkedList<Doctor> {
            auto docs = mongo_.findMany("doctors",
                make_document(kvp("department.name", deptName)).view()
            );

            LinkedList<Doctor> results{};
            std::lock_guard<std::mutex> lock{mtx_};
            for (auto& raw : docs) {
                Doctor d = fromBson(raw.view());
                cache_[d.uuid] = d;
                results.push(d);
            }
            return results;
        });
    }

    /**
     * @brief Saves updated profile info for a doctor.
     */
    std::future<bool> updateProfile(const std::string& doctorId, const Doctor& updated) {
        return std::async(std::launch::async, [this, doctorId, updated]() -> bool {
            bool ok = mongo_.replaceOne("doctors",
                make_document(kvp("uuid", doctorId)).view(),
                toBson(updated).view()
            );

            if (ok) {
                std::lock_guard<std::mutex> lock{mtx_};
                cache_[doctorId] = updated;
            }
            return ok;
        });
    }

    /**
     * @brief Updates the profile image for a doctor.
     * @param doctorId Which doctor to update.
     * @param base64ImageData The image encoded as a base64 string.
     */
    std::future<bool> updateProfileImage(const std::string& doctorId, const std::string& base64ImageData) {
        return std::async(std::launch::async, [this, doctorId, base64ImageData]() -> bool {
            bool ok = mongo_.updateOne("doctors",
                make_document(kvp("uuid", doctorId)).view(),
                make_document(kvp("$set", make_document(
                    kvp("profileImage", base64ImageData)
                ))).view()
            );

            // Update the cache too so the next read shows the new image
            if (ok) {
                std::lock_guard<std::mutex> lock{mtx_};
                auto it = cache_.find(doctorId);
                if (it != cache_.end()) {
                    it->second.profileImage = base64ImageData;
                }
            }
            return ok;
        });
    }
};
