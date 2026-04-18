#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <future>
#include <vector>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/builder/basic/array.hpp>

#include "../models/PatientHistory.h"
#include "../core/stack/Stack.h"
#include "../services/MongoService.h"

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

/**
 * @brief Manages patient medical histories.
 */
class PatientHistoryRepository {
private:
    MongoService mongo_{"healthlink"};
    std::unordered_map<std::string, PatientHistory> cache_;
    mutable std::mutex mtx_;

    /**
     * @brief Rebuilds a PatientHistory from its BSON representation.
     */
    PatientHistory fromBson(bsoncxx::document::view doc) {
        PatientHistory ph{};
        ph.id        = std::string{doc["id"].get_string().value};
        ph.patientId = std::string{doc["patientId"].get_string().value};

        if (doc.find("medicalReports") != doc.end()) {
            // Collect into a temporary list first
            std::vector<std::string> temp;
            for (auto& report : doc["medicalReports"].get_array().value) {
                temp.push_back(std::string{report.get_string().value});
            }

            // Push in reverse so the last one ends up on top
            for (auto it = temp.rbegin(); it != temp.rend(); ++it) {
                ph.medicalReports.push(*it);
            }
        }

        return ph;
    }

public:
    PatientHistoryRepository() {}

    /**
     * @brief Gets the full medical history for a patient.
     */
    std::future<PatientHistory> getHistory(const std::string& patientId) {
        return std::async(std::launch::async, [this, patientId]() -> PatientHistory {
            // Check the cache first
            {
                std::lock_guard<std::mutex> lock{mtx_};
                auto it = cache_.find(patientId);
                if (it != cache_.end()) return it->second;
            }

            // Ask the database
            auto result = mongo_.findOne("patient_history",
                make_document(kvp("patientId", patientId)).view()
            );

            if (!result) {
                // No history yet — return a blank one
                PatientHistory empty{};
                empty.id        = "hist_" + patientId;
                empty.patientId = patientId;
                return empty;
            }

            PatientHistory ph = fromBson(result->view());

            // Cache it for next time
            std::lock_guard<std::mutex> lock{mtx_};
            cache_[patientId] = ph;
            return ph;
        });
    }

    /**
     * @brief Adds a new medical report to a patient's history.
     */
    std::future<bool> addReport(const std::string& patientId, const std::string& report) {
        return std::async(std::launch::async, [this, patientId, report]() -> bool {
            // Check if the patient already has a history document
            auto existing = mongo_.findOne("patient_history",
                make_document(kvp("patientId", patientId)).view()
            );

            if (existing) {
                // Append the new report to the existing array
                mongo_.pushToArray("patient_history",
                    make_document(kvp("patientId", patientId)).view(),
                    "medicalReports",
                    report
                );
            } else {
                // First report ever — create the whole document
                bsoncxx::builder::basic::array reportsArr{};
                reportsArr.append(report);

                auto doc = make_document(
                    kvp("id",              "hist_" + patientId),
                    kvp("patientId",       patientId),
                    kvp("medicalReports",  reportsArr)
                );

                mongo_.insertOne("patient_history", doc.view());
            }

            // Push onto the in-memory Stack too
            std::lock_guard<std::mutex> lock{mtx_};
            if (cache_.find(patientId) == cache_.end()) {
                PatientHistory ph{};
                ph.id        = "hist_" + patientId;
                ph.patientId = patientId;
                cache_[patientId] = ph;
            }
            cache_[patientId].medicalReports.push(report);

            return true;
        });
    }
};
