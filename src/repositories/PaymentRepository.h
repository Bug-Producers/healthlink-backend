#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <future>
#include <chrono>
#include <ctime>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>

#include "../models/Payment.h"
#include "../core/linked_list/LinkedList.h"
#include "../services/MongoService.h"

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

/**
 * @brief Handles payment recording and revenue tracking.
 */
class PaymentRepository {
private:
    MongoService mongo_{"healthlink"};
    std::unordered_map<std::string, LinkedList<Payment>> cache_;
    mutable std::mutex mtx_;

    static std::string generateId() {
        static int counter = 0;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        return "pay_" + std::to_string(ms) + "_" + std::to_string(++counter);
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

    Payment fromBson(bsoncxx::document::view doc) {
        Payment p{};
        p.id = std::string{doc["id"].get_string().value};
        p.doctorId = std::string{doc["doctorId"].get_string().value};
        p.patientId = std::string{doc["patientId"].get_string().value};
        p.appointmentId = std::string{doc["appointmentId"].get_string().value};
        p.amount = doc["amount"].get_double().value;
        p.createdAt = std::string{doc["createdAt"].get_string().value};
        return p;
    }

    bsoncxx::document::value toBson(const Payment& p) {
        return make_document(
            kvp("id", p.id),
            kvp("doctorId", p.doctorId),
            kvp("patientId", p.patientId),
            kvp("appointmentId", p.appointmentId),
            kvp("amount", p.amount),
            kvp("createdAt", p.createdAt)
        );
    }

public:
    PaymentRepository() = default;

    /**
     * @brief Records a new payment from a patient.
     */
    std::future<Payment> recordPayment(
        const std::string& doctorId,
        const std::string& patientId,
        const std::string& appointmentId,
        double amount
    ) {
        return std::async(std::launch::async, [=, this]() -> Payment {
            Payment p{};
            p.id            = generateId();
            p.doctorId = doctorId;
            p.patientId = patientId;
            p.appointmentId = appointmentId;
            p.amount = amount;
            p.createdAt = nowTimestamp();

            mongo_.insertOne("payments", toBson(p).view());

            // Keep the cache current
            std::lock_guard<std::mutex> lock{mtx_};
            cache_[doctorId].push(p);

            return p;
        });
    }

    /**
     * @brief Fetches all payment records for a doctor.
     */
    std::future<LinkedList<Payment>> getByDoctor(const std::string& doctorId) {
        return std::async(std::launch::async, [this, doctorId]() -> LinkedList<Payment> {
            auto docs = mongo_.findMany("payments",
                make_document(kvp("doctorId", doctorId)).view()
            );

            LinkedList<Payment> results{};
            LinkedList<Payment> fresh{};

            for (auto& raw : docs) {
                Payment p = fromBson(raw.view());
                results.push(p);
                fresh.push(p);
            }

            // Refresh the cache
            std::lock_guard<std::mutex> lock{mtx_};
            cache_[doctorId] = std::move(fresh);

            return results;
        });
    }

    /**
     * @brief Simple container for a doctor's revenue numbers.
     */
    struct RevenueStats {
        double totalEarnings{0.0};
        int totalPayments{0};
        std::unordered_map<std::string, double> dailyBreakdown;
    };

    /**
     * @brief Tallies up total earnings and payment count for a doctor.
     */
    std::future<RevenueStats> getRevenueStats(const std::string& doctorId) {
        return std::async(std::launch::async, [this, doctorId]() -> RevenueStats {
            auto payments = getByDoctor(doctorId).get();

            RevenueStats stats{};
            auto* node = payments.getHead();
            while (node) {
                stats.totalEarnings += node->data.amount;
                stats.totalPayments++;
                
                // Group by grabbing YYYY-MM-DD from ISO stamp
                if (node->data.createdAt.length() >= 10) {
                    std::string dayKey = node->data.createdAt.substr(0, 10);
                    stats.dailyBreakdown[dayKey] += node->data.amount;
                }

                node = node->next;
            }
            return stats;
        });
    }
};
