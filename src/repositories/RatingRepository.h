#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <future>
#include <chrono>
#include <ctime>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>

#include "../models/Rating.h"
#include "../core/linked_list/LinkedList.h"
#include "../services/MongoService.h"

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

/**
 * @brief Manages rating records.
 */
class RatingRepository {
private:
    MongoService mongo_{"healthlink"};
    std::unordered_map<std::string, LinkedList<Rating>> cache_;
    mutable std::mutex mtx_;

    static std::string generateId() {
        static int counter = 0;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        return "rat_" + std::to_string(ms) + "_" + std::to_string(++counter);
    }

    static std::string nowTimestamp() {
        auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm tm{};
#ifdef _WIN32
        gmtime_s(&tm, &time);
#else
        gmtime_r(&time, &tm);
#endif
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
        return std::string{buf};
    }

    Rating fromBson(bsoncxx::document::view doc) {
        Rating r{};
        r.id = std::string{doc["id"].get_string().value};
        r.patientId = std::string{doc["patientId"].get_string().value};
        r.doctorId = std::string{doc["doctorId"].get_string().value};
        r.stars = doc["stars"].get_int32().value;
        r.comment = std::string{doc["comment"].get_string().value};
        r.createdAt = std::string{doc["createdAt"].get_string().value};
        return r;
    }

    bsoncxx::document::value toBson(const Rating& r) {
        return make_document(
            kvp("id", r.id),
            kvp("patientId", r.patientId),
            kvp("doctorId", r.doctorId),
            kvp("stars", r.stars),
            kvp("comment", r.comment),
            kvp("createdAt", r.createdAt)
        );
    }

public:
    RatingRepository() {}

    /**
     * @brief Submits a new rating.
     */
    std::future<Rating> addRating(
        const std::string& patientId,
        const std::string& doctorId,
        int stars,
        const std::string& comment
    ) {
        return std::async(std::launch::async, [=, this]() -> Rating {
            if (stars < 1 || stars > 5) {
                throw std::runtime_error("Rating must be between 1 and 5 stars.");
            }

            Rating r{};
            r.id        = generateId();
            r.patientId = patientId;
            r.doctorId  = doctorId;
            r.stars     = stars;
            r.comment   = comment;
            r.createdAt = nowTimestamp();

            mongo_.insertOne("ratings", toBson(r).view());

            // Add to the cache so the average updates right away
            std::lock_guard<std::mutex> lock{mtx_};
            cache_[doctorId].push(r);

            return r;
        });
    }

    /**
     * @brief Fetches every rating for a doctor.
     */
    std::future<LinkedList<Rating>> getByDoctor(const std::string& doctorId) {
        return std::async(std::launch::async, [this, doctorId]() -> LinkedList<Rating> {
            auto docs = mongo_.findMany("ratings",
                make_document(kvp("doctorId", doctorId)).view()
            );

            LinkedList<Rating> results{};
            LinkedList<Rating> fresh{};

            for (auto& raw : docs) {
                Rating r = fromBson(raw.view());
                results.push(r);
                fresh.push(r);
            }

            std::lock_guard<std::mutex> lock{mtx_};
            cache_[doctorId] = std::move(fresh);

            return results;
        });
    }

    /**
     * @brief Calculates the average rating.
     */
    std::future<double> getAverageRating(const std::string& doctorId) {
        return std::async(std::launch::async, [this, doctorId]() -> double {
            auto ratings = getByDoctor(doctorId).get();

            if (ratings.isEmpty()) return 0.0;

            double sum = 0.0;
            int count  = 0;
            auto* node = ratings.getHead();
            while (node) {
                sum += node->data.stars;
                count++;
                node = node->next;
            }

            return sum / count;
        });
    }
};
