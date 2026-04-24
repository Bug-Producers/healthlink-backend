#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <future>
#include <vector>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/builder/basic/array.hpp>

#include "../models/WeeklySchedule.h"
#include "../models/TimeSlot.h"
#include "../core/linked_list/LinkedList.h"
#include "../services/MongoService.h"

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

/**
 * @brief Manages doctor weekly availability schedules.
 */
class ScheduleRepository {
private:
    MongoService mongo_{"healthlink"};
    std::unordered_map<std::string, WeeklySchedule> cache_;
    mutable std::mutex mtx_;

    /**
     * @brief Unpacks a BSON document into a WeeklySchedule.
     */
    WeeklySchedule fromBson(bsoncxx::document::view doc) {
        WeeklySchedule ws{};
        ws.doctorId = std::string{doc["doctorId"].get_string().value};

        if (doc.find("availability") != doc.end()) {
            auto avail = doc["availability"].get_document().view();

            for (auto& day : avail) {
                std::string dayName{day.key()};
                LinkedList<TimeSlot> slots{};

                if (day.type() == bsoncxx::type::k_array) {
                    for (auto& slotDoc : day.get_array().value) {
                        TimeSlot ts{};
                        auto sv = slotDoc.get_document().view();
                        ts.startTime = std::string{sv["startTime"].get_string().value};
                        ts.endTime = std::string{sv["endTime"].get_string().value};
                        slots.push(ts);
                    }
                }

                ws.availability[dayName] = std::move(slots);
            }
        }

        return ws;
    }

    /**
     * @brief Packs a WeeklySchedule into BSON.
     */
    bsoncxx::document::value toBson(const WeeklySchedule& ws) {
        bsoncxx::builder::basic::document builder{};
        builder.append(kvp("doctorId", ws.doctorId));

        bsoncxx::builder::basic::document availDoc{};
        for (auto& [dayName, slotList] : ws.availability) {
            bsoncxx::builder::basic::array slotsArr{};

            // Walk the LinkedList head to tail
            auto* node = slotList.getHead();
            while (node) {
                slotsArr.append(make_document(
                    kvp("startTime", node->data.startTime),
                    kvp("endTime", node->data.endTime)
                ));
                node = node->next;
            }

            availDoc.append(kvp(dayName, slotsArr));
        }

        builder.append(kvp("availability", availDoc));
        return builder.extract();
    }

public:
    ScheduleRepository() {}

    /**
     * @brief Gets a doctor's full weekly schedule.
     */
    std::future<WeeklySchedule> getSchedule(const std::string& doctorId) {
        return std::async(std::launch::async, [this, doctorId]() -> WeeklySchedule {
            {
                std::lock_guard<std::mutex> lock{mtx_};
                auto it = cache_.find(doctorId);
                if (it != cache_.end()) return it->second;
            }

            auto result = mongo_.findOne("schedules", make_document(kvp("doctorId", doctorId)).view());
            if (!result) {
                // No schedule yet — give them an empty one to fill in
                WeeklySchedule empty{};
                empty.doctorId = doctorId;
                return empty;
            }

            WeeklySchedule ws = fromBson(result->view());

            std::lock_guard<std::mutex> lock{mtx_};
            cache_[doctorId] = ws;
            return ws;
        });
    }

    /**
     * @brief Replaces a doctor's entire weekly schedule.
     */
    std::future<bool> updateSchedule(const std::string& doctorId, const WeeklySchedule& schedule) {
        return std::async(std::launch::async, [this, doctorId, schedule]() -> bool {
            bool ok = mongo_.replaceOne("schedules",
                make_document(kvp("doctorId", doctorId)).view(),
                toBson(schedule).view(),
                true   // upsert: create if it doesn't exist yet
            );

            if (ok) {
                std::lock_guard<std::mutex> lock{mtx_};
                cache_[doctorId] = schedule;
            }
            return ok;
        });
    }

    /**
     * @brief Returns the time slots available on a specific day.
     */
    std::future<LinkedList<TimeSlot>> getAvailableSlots(
        const std::string& doctorId,
        const std::string& dayOfWeek
    ) {
        return std::async(std::launch::async, [this, doctorId, dayOfWeek]() -> LinkedList<TimeSlot> {
            auto schedule = getSchedule(doctorId).get();

            auto it = schedule.availability.find(dayOfWeek);
            if (it != schedule.availability.end()) {
                return it->second;
            }

            // Doctor doesn't work that day
            return LinkedList<TimeSlot>{};
        });
    }
};
