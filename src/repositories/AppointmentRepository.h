#pragma once

#include <string>
#include <mutex>
#include <future>
#include <sstream>
#include <iomanip>
#include <chrono>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>

#include "../models/Appointment.h"
#include "../models/WeeklySchedule.h"
#include "../core/linked_list/LinkedList.h"
#include "../core/queue/Queue.h"
#include "../services/MongoService.h"
#include "ScheduleRepository.h"
#include "DoctorRepository.h"
#include "NotificationRepository.h"

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

/**
 * @brief Handles the full appointment lifecycle
 */
class AppointmentRepository {
private:
    MongoService mongo_{"healthlink"};
    ScheduleRepository* scheduleRepo_;
    DoctorRepository* doctorRepo_;
    NotificationRepository* notificationRepo_;
    mutable std::mutex mtx_;

    /**
     * @brief Makes a unique ID.
     */
    static std::string generateId() {
        static int counter = 0;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        return "apt_" + std::to_string(ms) + "_" + std::to_string(++counter);
    }

    /**
     * @brief Turns "10:30" into 630.
     */
    static int timeToMinutes(const std::string& time) {
        return std::stoi(time.substr(0, 2)) * 60 + std::stoi(time.substr(3, 2));
    }

    /**
     * @brief Turns 630 back into "10:30".
     */
    static std::string minutesToTime(int totalMinutes) {
        int h = totalMinutes / 60;
        int m = totalMinutes % 60;
        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(2) << h
            << ":" << std::setfill('0') << std::setw(2) << m;
        return oss.str();
    }

    Appointment fromBson(bsoncxx::document::view doc) {
        Appointment a{};
        a.id = std::string{doc["id"].get_string().value};
        a.date = std::string{doc["date"].get_string().value};
        a.startTime = std::string{doc["startTime"].get_string().value};
        a.endTime = std::string{doc["endTime"].get_string().value};
        a.duration = doc["duration"].get_int32().value;
        a.status = static_cast<AppointmentStatus>(doc["status"].get_int32().value);

        // We store doctor and patient as flat IDs
        if (doc.find("doctorId") != doc.end())
            a.doctor.uuid = std::string{doc["doctorId"].get_string().value};
        if (doc.find("patientId") != doc.end())
            a.patient.id = std::string{doc["patientId"].get_string().value};

        return a;
    }

    bsoncxx::document::value toBson(const Appointment& a) {
        return make_document(
            kvp("id", a.id),
            kvp("doctorId", a.doctor.uuid),
            kvp("patientId", a.patient.id),
            kvp("date", a.date),
            kvp("startTime", a.startTime),
            kvp("endTime", a.endTime),
            kvp("duration", a.duration),
            kvp("status", static_cast<int>(a.status))
        );
    }

public:
    /**
     * @brief We need the schedule and doctor repos to calculate booking times.
     */
    AppointmentRepository(ScheduleRepository* scheduleRepo, DoctorRepository* doctorRepo, NotificationRepository* notificationRepo) {
        scheduleRepo_ = scheduleRepo;
        doctorRepo_ = doctorRepo;
        notificationRepo_ = notificationRepo;
    }

    /**
     * @brief Books an appointment.
     */
    std::future<Appointment> bookAppointment(
        const std::string& doctorId,
        const Patient& patient,
        const std::string& date,
        const std::string& dayOfWeek,
        const std::string& frameStart,
        const std::string& frameEnd
    ) {
        return std::async(std::launch::async, [=, this]() -> Appointment {
            Doctor doctor = doctorRepo_->findById(doctorId).get();

            // Find available slots for this day
            auto slots = scheduleRepo_->getAvailableSlots(doctorId, dayOfWeek).get();

            // Walk the LinkedList to find the slot matching the requested time frame
            TimeSlot* matchingSlot = nullptr;
            auto* node = slots.getHead();
            while (node) {
                if (node->data.startTime == frameStart && node->data.endTime == frameEnd) {
                    matchingSlot = &(node->data);
                    break;
                }
                node = node->next;
            }

            if (!matchingSlot) {
                throw std::runtime_error(
                    "No slot found for " + frameStart + " - " + frameEnd + " on " + dayOfWeek
                );
            }

            int slotStart   = timeToMinutes(frameStart);
            int slotEnd     = timeToMinutes(frameEnd);

            // Fetch existing appointments from DB to find the latest booked time in this exact frame
            auto existingDocs = mongo_.findMany("appointments",
                make_document(
                    kvp("doctorId", doctorId),
                    kvp("date", date),
                    kvp("status", static_cast<int>(AppointmentStatus::Booked)) // Ignore cancelled ones
                ).view()
            );

            int maxEndTimeMinutes = slotStart;

            for (auto& raw : existingDocs) {
                auto doc = raw.view();
                int existStart = timeToMinutes(std::string{doc["startTime"].get_string().value});
                int existEnd = timeToMinutes(std::string{doc["endTime"].get_string().value});
                
                // If this existing appointment falls tightly inside our targeted time block
                if (existStart >= slotStart && existStart < slotEnd) {
                    if (existEnd > maxEndTimeMinutes) {
                        maxEndTimeMinutes = existEnd;
                    }
                }
            }

            int allocStart = maxEndTimeMinutes;
            if (maxEndTimeMinutes > slotStart) {
                // Add the doctor's buffer time since this isn't the very first appt in the slot
                allocStart += doctor.bufferTime;
            }

            int allocEnd   = allocStart + doctor.appointmentDuration;

            // Make sure we haven't run past the end of the frame
            if (allocEnd > slotEnd) {
                throw std::runtime_error("This time frame is fully booked — no more room.");
            }

            // Build the appointment with the calculated time
            Appointment apt{};
            apt.id        = generateId();
            apt.doctor    = doctor;
            apt.patient   = patient;
            apt.date      = date;
            apt.startTime = minutesToTime(allocStart);
            apt.endTime   = minutesToTime(allocEnd);
            apt.duration  = doctor.appointmentDuration;
            apt.status    = AppointmentStatus::Booked;

            // Persist to MongoDB
            mongo_.insertOne("appointments", toBson(apt).view());

            notificationRepo_->pushNotification(patient.id, "Your appointment with " + doctor.name + " is definitively booked for " + date + " at " + apt.startTime);
            notificationRepo_->pushNotification(doctorId, "New incoming booking! " + patient.name + " confirmed an appointment on " + date + " at " + apt.startTime);

            return apt;
        });
    }

    /**
     * @brief Lists all appointments for a patient.
     */
    std::future<LinkedList<Appointment>> findByPatient(const std::string& patientId) {
        return std::async(std::launch::async, [this, patientId]() -> LinkedList<Appointment> {
            auto docs = mongo_.findMany("appointments",
                make_document(kvp("patientId", patientId)).view()
            );

            LinkedList<Appointment> results{};
            for (auto& raw : docs) {
                results.push(fromBson(raw.view()));
            }
            return results;
        });
    }

    /**
     * @brief Lists all appointments for a doctor.
     */
    std::future<LinkedList<Appointment>> findByDoctor(const std::string& doctorId) {
        return std::async(std::launch::async, [this, doctorId]() -> LinkedList<Appointment> {
            auto docs = mongo_.findMany("appointments",
                make_document(kvp("doctorId", doctorId)).view()
            );

            LinkedList<Appointment> results{};
            for (auto& raw : docs) {
                results.push(fromBson(raw.view()));
            }
            return results;
        });
    }

    /**
     * @brief Marks an appointment as cancelled.
     */
    std::future<bool> cancel(const std::string& appointmentId) {
        return std::async(std::launch::async, [this, appointmentId]() -> bool {
            auto doc = mongo_.findOne("appointments", make_document(kvp("id", appointmentId)).view());
            if (doc) {
                auto apt = fromBson(doc->view());
                notificationRepo_->pushNotification(apt.patient.id, "Your appointment on " + apt.date + " was successfully canceled.");
                notificationRepo_->pushNotification(apt.doctor.uuid, "Appointment Cancelled by patient on " + apt.date);
            }

            return mongo_.updateOne("appointments",
                make_document(kvp("id", appointmentId)).view(),
                make_document(kvp("$set", make_document(
                    kvp("status", static_cast<int>(AppointmentStatus::Cancelled))
                ))).view()
            );
        });
    }

    /**
     * @brief Changes the status of an appointment.
     */
    std::future<bool> updateStatus(const std::string& appointmentId, AppointmentStatus newStatus) {
        return std::async(std::launch::async, [this, appointmentId, newStatus]() -> bool {
            auto doc = mongo_.findOne("appointments", make_document(kvp("id", appointmentId)).view());
            if (doc) {
                auto apt = fromBson(doc->view());
                // Optionally notify based on the new status
                if (newStatus == AppointmentStatus::Completed) {
                    notificationRepo_->pushNotification(apt.patient.id, "Your appointment on " + apt.date + " was marked completed.");
                } else if (newStatus == AppointmentStatus::Cancelled) {
                    notificationRepo_->pushNotification(apt.patient.id, "Your appointment on " + apt.date + " was canceled.");
                }
            }

            return mongo_.updateOne("appointments",
                make_document(kvp("id", appointmentId)).view(),
                make_document(kvp("$set", make_document(
                    kvp("status", static_cast<int>(newStatus))
                ))).view()
            );
        });
    }
};
