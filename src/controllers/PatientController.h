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
     * @brief Hooks up all patient-facing routes to the app.
     */
    template<typename App>
    void registerRoutes(App& app) {

        // GET /api/patients/doctors
        CROW_ROUTE(app, "/api/patients/doctors")
        .methods(crow::HTTPMethod::GET)
        ([this](const crow::request& req) {
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
                    d["country"] = node->data.country;
                    d["hospitalOrClinicName"] = node->data.hospitalOrClinicName;
                    d["rating"] = node->data.rating;
                    d["expYears"] = node->data.expYears;
                    d["patients"] = node->data.patients;
                    d["about"] = node->data.about;
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

        // GET /api/patients/history (and aliases)
        auto getHistoryHandler = [this](const crow::request& req) {
            auto uid = FirebaseAuth::authenticate(req);
            if (uid.empty()) return crow::response{401, "Unauthorized"};

            std::string patientId = uid;
            auto targetPatientId = req.url_params.get("patientId");
            if (targetPatientId) {
                patientId = targetPatientId;
            }

            try {
                auto history = historyRepo_->getHistory(patientId).get();
                std::vector<crow::json::wvalue> reportsList;

                auto& stack = history.medicalReports;
                auto stackCopy = stack;

                while (!stackCopy.isEmpty()) {
                    crow::json::wvalue entry;
                    entry["report"] = stackCopy.top();
                    entry["patientId"] = history.patientId;
                    reportsList.push_back(std::move(entry));
                    stackCopy.pop();
                }

                crow::json::wvalue json;
                json["patientId"] = history.patientId;
                json["history"]   = std::move(reportsList);
                return crow::response{200, json};
            } catch (const std::exception& e) {
                return crow::response{500, e.what()};
            }
        };

        CROW_ROUTE(app, "/api/patients/history").methods(crow::HTTPMethod::GET)(getHistoryHandler);
        CROW_ROUTE(app, "/api/history").methods(crow::HTTPMethod::GET)(getHistoryHandler);
        CROW_ROUTE(app, "/history").methods(crow::HTTPMethod::GET)(getHistoryHandler);

        auto postHistoryHandler = [this](const crow::request& req) {
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
        };

        CROW_ROUTE(app, "/api/patients/history").methods(crow::HTTPMethod::POST)(postHistoryHandler);
        CROW_ROUTE(app, "/api/history").methods(crow::HTTPMethod::POST)(postHistoryHandler);
        CROW_ROUTE(app, "/history").methods(crow::HTTPMethod::POST)(postHistoryHandler);

        // GET /api/patients/doctors/<string>
        CROW_ROUTE(app, "/api/patients/doctors/<string>")
        .methods(crow::HTTPMethod::GET)
        ([this](const crow::request& req, std::string doctorId) {
            auto uid = FirebaseAuth::authenticate(req);
            if (uid.empty()) return crow::response{401, "Unauthorized"};

            try {
                auto doctor = doctorRepo_->findById(doctorId).get();
                auto avg    = ratingRepo_->getAverageRating(doctorId).get();
                auto schedule = scheduleRepo_->getSchedule(doctorId).get();

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
                json["appointmentDuration"] = schedule.appointmentDuration;
                json["bufferTime"] = schedule.bufferTime;
                json["department"]["name"] = doctor.department.name;
                json["department"]["count"] = doctor.department.count;
                return crow::response{200, json};
            } catch (const std::exception& e) {
                return crow::response{404, e.what()};
            }
        });

        // GET /api/patients/<id>
        CROW_ROUTE(app, "/api/patients/<string>")
        .methods(crow::HTTPMethod::GET)
        ([this](const crow::request& req, std::string patientId) {
            auto uid = FirebaseAuth::authenticate(req);
            if (uid.empty()) return crow::response{401, "Unauthorized"};

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

        // POST /api/patients/register
        CROW_ROUTE(app, "/api/patients/register")
        .methods(crow::HTTPMethod::POST)
        ([this](const crow::request& req) {
            auto uid = FirebaseAuth::authenticate(req);
            if (uid.empty()) return crow::response{401, "Unauthorized"};

            auto body = crow::json::load(req.body);
            if (!body || !body.has("name") || !body.has("email") ||
                !body.has("dateOfBirth") || !body.has("gender")) {
                return crow::response{400, "Need name, email, dateOfBirth, gender"};
            }

            try {
                Patient patient;
                patient.id = uid;
                patient.name = body["name"].s();
                patient.email = body["email"].s();
                patient.dateOfBirth = body["dateOfBirth"].s();
                patient.gender = body["gender"].s();
                patient.profileImage = body.has("profileImage")
                    ? std::string{body["profileImage"].s()} : "";

                bool ok = patientRepo_->create(patient).get();
                if (ok) {
                    crow::json::wvalue json;
                    json["id"]          = patient.id;
                    json["name"]        = patient.name;
                    json["email"]       = patient.email;
                    json["dateOfBirth"] = patient.dateOfBirth;
                    json["gender"]      = patient.gender;
                    json["profileImage"]= patient.profileImage;
                    return crow::response{201, json};
                }
                return crow::response{500, "Failed to create patient"};
            } catch (const std::exception& e) {
                return crow::response{500, e.what()};
            }
        });

        // PUT /api/patients/profile
        CROW_ROUTE(app, "/api/patients/profile")
        .methods(crow::HTTPMethod::PUT)
        ([this](const crow::request& req) {
            auto uid = FirebaseAuth::authenticate(req);
            if (uid.empty()) return crow::response{401, "Unauthorized"};

            auto body = crow::json::load(req.body);
            if (!body || !body.has("name") || !body.has("dateOfBirth") || !body.has("gender")) {
                return crow::response{400, "Need name, dateOfBirth, gender"};
            }

            try {
                // Fetch existing to retain ID, email, and profileImage
                auto patient = patientRepo_->findById(uid).get();
                patient.name = body["name"].s();
                patient.dateOfBirth = body["dateOfBirth"].s();
                patient.gender = body["gender"].s();

                bool ok = patientRepo_->updateProfile(uid, patient).get();
                if (ok) return crow::response{200, "Profile updated"};
                return crow::response{500, "Failed to update profile"};
            } catch (const std::exception& e) {
                return crow::response{500, e.what()};
            }
        });

        // PUT /api/patients/profile/image
        CROW_ROUTE(app, "/api/patients/profile/image")
        .methods(crow::HTTPMethod::PUT)
        ([this](const crow::request& req) {
            auto uid = FirebaseAuth::authenticate(req);
            if (uid.empty()) return crow::response{401, "Unauthorized"};

            auto body = crow::json::load(req.body);
            if (!body || !body.has("image")) {
                return crow::response{400, "Need base64 image data"};
            }

            try {
                std::string b64 = body["image"].s();
                bool ok = patientRepo_->updateProfileImage(uid, b64).get();
                if (ok) return crow::response{200, "Profile image updated"};
                return crow::response{500, "Failed to update image"};
            } catch (const std::exception& e) {
                return crow::response{500, e.what()};
            }
        });

        // GET /api/patients/doctors/<id>/schedule
        CROW_ROUTE(app, "/api/patients/doctors/<string>/schedule")
        .methods(crow::HTTPMethod::GET)
        ([this](const crow::request& req, std::string doctorId) {
            auto uid = FirebaseAuth::authenticate(req);
            if (uid.empty()) return crow::response{401, "Unauthorized"};

            try {
                auto schedule = scheduleRepo_->getSchedule(doctorId).get();
                crow::json::wvalue json;
                json["doctorId"] = schedule.doctorId;
                json["appointmentDuration"] = schedule.appointmentDuration;
                json["bufferTime"] = schedule.bufferTime;

                crow::json::wvalue availability;
                for (auto& [day, slotList] : schedule.availability) {
                    std::vector<crow::json::wvalue> slots;
                    auto* node = slotList.getHead();
                    while (node) {
                        crow::json::wvalue s;
                        s["startTime"] = node->data.startTime;
                        s["endTime"]   = node->data.endTime;
                        slots.push_back(std::move(s));
                        node = node->next;
                    }
                    availability[day] = std::move(slots);
                }

                json["availability"] = std::move(availability);
                return crow::response{200, json};
            } catch (const std::exception& e) {
                return crow::response{500, e.what()};
            }
        });

        // GET /api/patients/departments/<string>
        CROW_ROUTE(app, "/api/patients/departments/<string>")
        .methods(crow::HTTPMethod::GET)
        ([this](const crow::request& req, std::string deptName) {
            auto uid = FirebaseAuth::authenticate(req);
            if (uid.empty()) return crow::response{401, "Unauthorized"};

            try {
                auto doctors = doctorRepo_->findByDepartment(deptName).get();
                std::vector<crow::json::wvalue> list;

                auto* node = doctors.getHead();
                while (node) {
                    crow::json::wvalue d;
                    d["uuid"] = node->data.uuid;
                    d["name"] = node->data.name;
                    d["rating"] = node->data.rating;
                    d["expYears"] = node->data.expYears;
                    d["profileImage"] = node->data.profileImage;
                    list.push_back(std::move(d));
                    node = node->next;
                }

                crow::json::wvalue json;
                json["department"] = deptName;
                json["doctors"] = std::move(list);
                return crow::response{200, json};
            } catch (const std::exception& e) {
                return crow::response{500, e.what()};
            }
        });

        // GET /api/patients/doctors/<id>/slots/<day>
        CROW_ROUTE(app, "/api/patients/doctors/<string>/slots/<string>")
        .methods(crow::HTTPMethod::GET)
        ([this](const crow::request& req, std::string doctorId, std::string day) {
            auto uid = FirebaseAuth::authenticate(req);
            if (uid.empty()) return crow::response{401, "Unauthorized"};

            try {
                auto slots = scheduleRepo_->getAvailableSlots(doctorId, day).get();
                std::vector<crow::json::wvalue> list;

                auto* node = slots.getHead();
                while (node) {
                    crow::json::wvalue s;
                    s["startTime"] = node->data.startTime;
                    s["endTime"] = node->data.endTime;
                    list.push_back(std::move(s));
                    node = node->next;
                }

                crow::json::wvalue json;
                json["doctorId"] = doctorId;
                json["day"] = day;
                json["slots"] = std::move(list);
                return crow::response{200, json};
            } catch (const std::exception& e) {
                return crow::response{500, e.what()};
            }
        });

        // POST /api/patients/appointments/book
        CROW_ROUTE(app, "/api/patients/appointments/book")
        .methods(crow::HTTPMethod::POST)
        ([this](const crow::request& req) {
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
                json["id"] = apt.id;
                json["doctorId"] = apt.doctor.uuid;
                json["date"] = apt.date;
                json["startTime"] = apt.startTime;
                json["endTime"] = apt.endTime;
                json["duration"] = apt.duration;
                json["status"] = static_cast<int>(apt.status);
                json["message"] = "Booked! Your appointment is at " + apt.startTime;
                return crow::response{201, json};
            } catch (const std::exception& e) {
                return crow::response{400, e.what()};
            }
        });

        // GET /api/patients/appointments
        CROW_ROUTE(app, "/api/patients/appointments")
        .methods(crow::HTTPMethod::GET)
        ([this](const crow::request& req) {
            auto uid = FirebaseAuth::authenticate(req);
            if (uid.empty()) return crow::response{401, "Unauthorized"};

            try {
                auto appointments = appointmentRepo_->findByPatient(uid).get();
                std::vector<crow::json::wvalue> list;

                auto* node = appointments.getHead();
                while (node) {
                    crow::json::wvalue a;
                    a["id"] = node->data.id;
                    a["doctorId"] = node->data.doctor.uuid;
                    try {
                        auto doc = doctorRepo_->findById(node->data.doctor.uuid).get();
                        a["doctorName"] = doc.name;
                        a["doctorImage"] = doc.profileImage;
                    } catch (...) {
                        a["doctorName"] = "Unknown Doctor";
                    }
                    a["date"] = node->data.date;
                    a["startTime"] = node->data.startTime;
                    a["endTime"] = node->data.endTime;
                    a["duration"] = node->data.duration;
                    a["status"] = static_cast<int>(node->data.status);
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
        CROW_ROUTE(app, "/api/patients/appointments/<string>")
        .methods(crow::HTTPMethod::Delete)
        ([this](const crow::request& req, std::string aptId) {
            auto uid = FirebaseAuth::authenticate(req);
            if (uid.empty()) return crow::response{401, "Unauthorized"};

            try {
                bool ok = appointmentRepo_->cancel(aptId).get();
                if (ok) return crow::response{200, "Appointment cancelled"};
                return crow::response{404, "Appointment not found"};
            } catch (const std::exception& e) {
                return crow::response{500, e.what()};
            }
        });

        // POST /api/patients/ratings
        CROW_ROUTE(app, "/api/patients/ratings")
        .methods(crow::HTTPMethod::POST)
        ([this](const crow::request& req) {
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
                json["id"] = rating.id;
                json["doctorId"] = rating.doctorId;
                json["stars"] = rating.stars;
                json["comment"] = rating.comment;
                json["createdAt"] = rating.createdAt;
                return crow::response{201, json};
            } catch (const std::exception& e) {
                return crow::response{400, e.what()};
            }
        });



        // GET /api/patients/notifications
        CROW_ROUTE(app, "/api/patients/notifications")
        .methods(crow::HTTPMethod::GET)
        ([this](const crow::request& req) {
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
