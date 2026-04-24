#pragma once

#include <string>
#include <crow.h>

#include "../services/FirebaseAuth.h"
#include "../repositories/DoctorRepository.h"
#include "../repositories/PatientRepository.h"
#include "../repositories/ScheduleRepository.h"
#include "../repositories/AppointmentRepository.h"
#include "../repositories/RatingRepository.h"
#include "../repositories/PatientHistoryRepository.h"
#include "../repositories/NotificationRepository.h"
#include "../core/router/ApiRouter.h"
#include "../utils/StringUtils.h"

/**
 * @brief Handles all API routes that patients interact with.
 */
class PatientController {
private:
    DoctorRepository* doctorRepo_;
    PatientRepository* patientRepo_;
    ScheduleRepository* scheduleRepo_;
    AppointmentRepository* appointmentRepo_;
    RatingRepository* ratingRepo_;
    PatientHistoryRepository* historyRepo_;
    NotificationRepository* notificationRepo_;

public:
    PatientController(
        DoctorRepository* doctorRepo,
        PatientRepository* patientRepo,
        ScheduleRepository* scheduleRepo,
        AppointmentRepository* appointmentRepo,
        RatingRepository* ratingRepo,
        PatientHistoryRepository* historyRepo,
        NotificationRepository* notificationRepo
    ) {
        doctorRepo_ = doctorRepo;
        patientRepo_ = patientRepo;
        scheduleRepo_ = scheduleRepo;
        appointmentRepo_ = appointmentRepo;
        ratingRepo_ = ratingRepo;
        historyRepo_ = historyRepo;
        notificationRepo_ = notificationRepo;
    }

    /**
     * @brief Hooks up all patient-facing routes to the ApiRouter.
     */
    void registerRoutes(ApiRouter& router) {

        // GET /api/patients/doctors
        router.get("/api/patients/doctors", [this](const crow::request& req) -> crow::response {
            auto uid = FirebaseAuth::authenticate(req);
            if (uid.empty()) return crow::response{401, "Unauthorized"};

            try {
                auto doctors = doctorRepo_->findAll().get();
                std::vector<crow::json::wvalue> list; // Crow JSON internal array requirement

                auto* node = doctors.getHead();
                while (node) {
                    crow::json::wvalue d;
                    d["uuid"] = node->data.uuid;
                    d["name"] = node->data.name;
                    d["city"] = node->data.city;
                    d["hospitalOrClinicName"] = node->data.hospitalOrClinicName;
                    d["rating"] = node->data.rating;
                    d["expYears"] = node->data.expYears;
                    d["profileImage"] = node->data.profileImage;
                    d["department"]["name"] = node->data.department.name;
                    list.push_back(std::move(d));
                    node = node->next;
                }

                crow::json::wvalue json;
                json["doctors"] = std::move(list);
                return crow::response{200, json};
            } catch (const std::exception& e) {
                return crow::response{500, e.what()};
            }
        });

        // GET /api/patients/doctors/<string>
        router.get("/api/patients/doctors/*", [this](const crow::request& req) -> crow::response {
            auto uid = FirebaseAuth::authenticate(req);
            if (uid.empty()) return crow::response{401, "Unauthorized"};

            auto segments = StringUtils::split(req.url);
            if (segments.size() < 4) return crow::response(400);
            std::string doctorId = segments.back();

            try {
                auto doctor = doctorRepo_->findById(doctorId).get();
                auto avg    = ratingRepo_->getAverageRating(doctorId).get();

                crow::json::wvalue json;
                json["uuid"] = doctor.uuid;
                json["name"] = doctor.name;
                json["city"] = doctor.city;
                json["country"] = doctor.country;
                json["hospitalOrClinicName"] = doctor.hospitalOrClinicName;
                json["rating"] = avg;
                json["expYears"] = doctor.expYears;
                json["patients"] = doctor.patients;
                json["about"] = doctor.about;
                json["profileImage"] = doctor.profileImage;
                json["appointmentDuration"] = doctor.appointmentDuration;
                json["bufferTime"] = doctor.bufferTime;
                json["department"]["name"] = doctor.department.name;
                json["department"]["count"] = doctor.department.count;
                return crow::response{200, json};
            } catch (const std::exception& e) {
                return crow::response{404, e.what()};
            }
        });

        // GET /api/patients/<id>
        router.get("/api/patients/*", [this](const crow::request& req) -> crow::response {
            auto uid = FirebaseAuth::authenticate(req);
            if (uid.empty()) return crow::response{401, "Unauthorized"};

            auto segments = StringUtils::split(req.url);
            if (segments.size() < 4) return crow::response(400);
            std::string patientId = segments.back();

            // We do a small hack: PatientController router matches /api/patients/doctors/* first because it has an exact matched segment "doctors"
            // So /api/patients/<id> will naturally fall through to this exact wildcard match if it's not matching doctors or departments.
            
            try {
                auto patient = patientRepo_->findById(patientId).get();
                crow::json::wvalue json;
                json["id"]          = patient.id;
                json["name"]        = patient.name;
                json["email"]       = patient.email;
                json["dateOfBirth"] = patient.dateOfBirth;
                json["gender"]      = patient.gender;
                json["profileImage"]= patient.profileImage;
                return crow::response{200, json};
            } catch (const std::exception& e) {
                return crow::response{404, e.what()};
            }
        });

        // GET /api/patients/departments/<string>
        router.get("/api/patients/departments/*", [this](const crow::request& req) -> crow::response {
            auto uid = FirebaseAuth::authenticate(req);
            if (uid.empty()) return crow::response{401, "Unauthorized"};

            auto segments = StringUtils::split(req.url);
            if (segments.size() < 4) return crow::response(400);
            std::string deptName = segments.back();

            try {
                auto doctors = doctorRepo_->findByDepartment(deptName).get();
                std::vector<crow::json::wvalue> list;

                auto* node = doctors.getHead();
                while (node) {
                    crow::json::wvalue d;
                    d["uuid"]         = node->data.uuid;
                    d["name"]         = node->data.name;
                    d["rating"]       = node->data.rating;
                    d["expYears"]     = node->data.expYears;
                    d["profileImage"] = node->data.profileImage;
                    list.push_back(std::move(d));
                    node = node->next;
                }

                crow::json::wvalue json;
                json["department"] = deptName;
                json["doctors"]    = std::move(list);
                return crow::response{200, json};
            } catch (const std::exception& e) {
                return crow::response{500, e.what()};
            }
        });

        // GET /api/patients/doctors/<id>/slots/<day>
        router.get("/api/patients/doctors/*/slots/*", [this](const crow::request& req) -> crow::response {
            auto uid = FirebaseAuth::authenticate(req);
            if (uid.empty()) return crow::response{401, "Unauthorized"};

            auto segments = StringUtils::split(req.url);
            if (segments.size() < 6) return crow::response(400);
            std::string doctorId = segments[3];
            std::string day = segments[5];

            try {
                auto slots = scheduleRepo_->getAvailableSlots(doctorId, day).get();
                std::vector<crow::json::wvalue> list;

                auto* node = slots.getHead();
                while (node) {
                    crow::json::wvalue s;
                    s["startTime"] = node->data.startTime;
                    s["endTime"]   = node->data.endTime;
                    list.push_back(std::move(s));
                    node = node->next;
                }

                crow::json::wvalue json;
                json["doctorId"] = doctorId;
                json["day"]      = day;
                json["slots"]    = std::move(list);
                return crow::response{200, json};
            } catch (const std::exception& e) {
                return crow::response{500, e.what()};
            }
        });

        // POST /api/patients/appointments/book
        router.post("/api/patients/appointments/book", [this](const crow::request& req) -> crow::response {
            auto uid = FirebaseAuth::authenticate(req);
            if (uid.empty()) return crow::response{401, "Unauthorized"};

            auto body = crow::json::load(req.body);
            if (!body || !body.has("doctorId") || !body.has("date") ||
                !body.has("dayOfWeek") || !body.has("frameStart") || !body.has("frameEnd")) {
                return crow::response{400, "Need doctorId, date, dayOfWeek, frameStart, frameEnd"};
            }

            try {
                auto patient = patientRepo_->findById(uid).get();

                auto apt = appointmentRepo_->bookAppointment(
                    std::string{body["doctorId"].s()},
                    patient,
                    std::string{body["date"].s()},
                    std::string{body["dayOfWeek"].s()},
                    std::string{body["frameStart"].s()},
                    std::string{body["frameEnd"].s()}
                ).get();

                crow::json::wvalue json;
                json["id"]        = apt.id;
                json["doctorId"]  = apt.doctor.uuid;
                json["date"]      = apt.date;
                json["startTime"] = apt.startTime;
                json["endTime"]   = apt.endTime;
                json["duration"]  = apt.duration;
                json["status"]    = static_cast<int>(apt.status);
                json["message"]   = "Booked! Your appointment is at " + apt.startTime;
                return crow::response{201, json};
            } catch (const std::exception& e) {
                return crow::response{400, e.what()};
            }
        });

        // GET /api/patients/appointments
        router.get("/api/patients/appointments", [this](const crow::request& req) -> crow::response {
            auto uid = FirebaseAuth::authenticate(req);
            if (uid.empty()) return crow::response{401, "Unauthorized"};

            try {
                auto appointments = appointmentRepo_->findByPatient(uid).get();
                std::vector<crow::json::wvalue> list;

                auto* node = appointments.getHead();
                while (node) {
                    crow::json::wvalue a;
                    a["id"]        = node->data.id;
                    a["doctorId"]  = node->data.doctor.uuid;
                    a["date"]      = node->data.date;
                    a["startTime"] = node->data.startTime;
                    a["endTime"]   = node->data.endTime;
                    a["duration"]  = node->data.duration;
                    a["status"]    = static_cast<int>(node->data.status);
                    list.push_back(std::move(a));
                    node = node->next;
                }

                crow::json::wvalue json;
                json["appointments"] = std::move(list);
                return crow::response{200, json};
            } catch (const std::exception& e) {
                return crow::response{500, e.what()};
            }
        });

        // DELETE /api/patients/appointments/<string>
        router.del("/api/patients/appointments/*", [this](const crow::request& req) -> crow::response {
            auto uid = FirebaseAuth::authenticate(req);
            if (uid.empty()) return crow::response{401, "Unauthorized"};

            auto segments = StringUtils::split(req.url);
            if (segments.size() < 4) return crow::response(400);
            std::string aptId = segments.back();

            try {
                bool ok = appointmentRepo_->cancel(aptId).get();
                if (ok) return crow::response{200, "Appointment cancelled"};
                return crow::response{404, "Appointment not found"};
            } catch (const std::exception& e) {
                return crow::response{500, e.what()};
            }
        });

        // POST /api/patients/ratings
        router.post("/api/patients/ratings", [this](const crow::request& req) -> crow::response {
            auto uid = FirebaseAuth::authenticate(req);
            if (uid.empty()) return crow::response{401, "Unauthorized"};

            auto body = crow::json::load(req.body);
            if (!body || !body.has("doctorId") || !body.has("stars")) {
                return crow::response{400, "Need doctorId and stars (1-5)"};
            }

            try {
                std::string comment = body.has("comment") ? std::string{body["comment"].s()} : "";

                auto rating = ratingRepo_->addRating(
                    uid,
                    std::string{body["doctorId"].s()},
                    static_cast<int>(body["stars"].i()),
                    comment
                ).get();

                crow::json::wvalue json;
                json["id"]        = rating.id;
                json["doctorId"]  = rating.doctorId;
                json["stars"]     = rating.stars;
                json["comment"]   = rating.comment;
                json["createdAt"] = rating.createdAt;
                return crow::response{201, json};
            } catch (const std::exception& e) {
                return crow::response{400, e.what()};
            }
        });

        // GET /api/patients/history
        router.get("/api/patients/history", [this](const crow::request& req) -> crow::response {
            auto uid = FirebaseAuth::authenticate(req);
            if (uid.empty()) return crow::response{401, "Unauthorized"};

            try {
                auto history = historyRepo_->getHistory(uid).get();
                std::vector<std::string> reports; // JSON array format

                auto& stack = history.medicalReports;
                auto stackCopy = stack;

                while (!stackCopy.isEmpty()) {
                    reports.push_back(stackCopy.top());
                    stackCopy.pop();
                }

                crow::json::wvalue json;
                json["patientId"] = history.patientId;
                json["reports"]   = reports;
                return crow::response{200, json};
            } catch (const std::exception& e) {
                return crow::response{500, e.what()};
            }
        });

        // POST /api/patients/history
        router.post("/api/patients/history", [this](const crow::request& req) -> crow::response {
            auto uid = FirebaseAuth::authenticate(req);
            if (uid.empty()) return crow::response{401, "Unauthorized"};

            auto body = crow::json::load(req.body);
            if (!body || !body.has("patientId") || !body.has("report")) {
                return crow::response{400, "Need patientId and report string"};
            }

            try {
                std::string pId = body["patientId"].s();
                std::string report = body["report"].s();

                bool ok = historyRepo_->addReport(pId, report).get();
                if (ok) return crow::response{201, "Report added"};
                return crow::response{500, "Failed to add report"};
            } catch (const std::exception& e) {
                return crow::response{500, e.what()};
            }
        });

        // GET /api/patients/notifications
        router.get("/api/patients/notifications", [this](const crow::request& req) -> crow::response {
            auto uid = FirebaseAuth::authenticate(req);
            if (uid.empty()) return crow::response{401, "Unauthorized"};

            try {
                auto notifs = notificationRepo_->getNotifications(uid).get();
                std::vector<crow::json::wvalue> list;

                auto* node = notifs.getHead();
                while (node) {
                    crow::json::wvalue n;
                    n["id"]        = node->data.id;
                    n["message"]   = node->data.message;
                    n["createdAt"] = node->data.createdAt;
                    n["read"]      = node->data.read;
                    list.push_back(std::move(n));
                    node = node->next;
                }

                crow::json::wvalue json;
                json["notifications"] = std::move(list);
                return crow::response{200, json};
            } catch (const std::exception& e) {
                return crow::response{500, e.what()};
            }
        });
    }
};
