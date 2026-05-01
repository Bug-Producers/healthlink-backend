#pragma once

#include <string>
#include <crow.h>

#include "../services/FirebaseAuth.h"
#include "../repositories/DoctorRepository.h"
#include "../repositories/PatientRepository.h"
#include "../repositories/ScheduleRepository.h"
#include "../repositories/AppointmentRepository.h"
#include "../repositories/PaymentRepository.h"
#include "../repositories/RatingRepository.h"
#include "../repositories/NotificationRepository.h"

/**
 * @brief Handles all API routes that doctors interact with.
 */
class DoctorController {
private:
    DoctorRepository* doctorRepo_;
    PatientRepository* patientRepo_;
    ScheduleRepository* scheduleRepo_;
    AppointmentRepository* appointmentRepo_;
    PaymentRepository* paymentRepo_;
    RatingRepository* ratingRepo_;
    NotificationRepository* notificationRepo_;

public:
    DoctorController(
        DoctorRepository* doctorRepo,
        PatientRepository* patientRepo,
        ScheduleRepository* scheduleRepo,
        AppointmentRepository* appointmentRepo,
        PaymentRepository* paymentRepo,
        RatingRepository* ratingRepo,
        NotificationRepository* notificationRepo
    ) {
        doctorRepo_ = doctorRepo;
        patientRepo_ = patientRepo;
        scheduleRepo_ = scheduleRepo;
        appointmentRepo_ = appointmentRepo;
        paymentRepo_ = paymentRepo;
        ratingRepo_ = ratingRepo;
        notificationRepo_ = notificationRepo;
    }

    /**
     * @brief Hooks up all doctor-related routes to the app.
     */
    template<typename App>
    void registerRoutes(App& app) {

        // POST /api/doctors/register
        CROW_ROUTE(app, "/api/doctors/register")
        .methods(crow::HTTPMethod::POST)
        ([this](const crow::request& req) {
            auto uid = FirebaseAuth::authenticate(req);
            if (uid.empty()) return crow::response{401, "Unauthorized"};

            auto body = crow::json::load(req.body);
            if (!body) return crow::response{400, "Invalid JSON body"};

            try {
                Doctor doctor{};
                doctor.uuid = uid;

                if (body.has("name")) doctor.name = body["name"].s();
                if (body.has("city")) doctor.city = body["city"].s();
                if (body.has("country")) doctor.country = body["country"].s();
                if (body.has("hospitalOrClinicName")) doctor.hospitalOrClinicName = body["hospitalOrClinicName"].s();
                if (body.has("about")) doctor.about = body["about"].s();
                if (body.has("appointmentDuration")) doctor.appointmentDuration = body["appointmentDuration"].i();
                if (body.has("bufferTime")) doctor.bufferTime = body["bufferTime"].i();

                if (body.has("department") && body["department"].has("name")) {
                    doctor.department.name = body["department"]["name"].s();
                    doctor.department.count = 1;
                }

                doctor.rating = 0;
                doctor.expYears = body.has("expYears") ? body["expYears"].i() : 0;
                doctor.patients = 0;
                doctor.profileImage = "";

                bool ok = doctorRepo_->createDoctor(doctor).get();
                if (ok) return crow::response{201, "Doctor profile created"};
                return crow::response{500, "Failed to create profile"};
            } catch (const std::exception& e) {
                return crow::response{500, e.what()};
            }
        });

        // GET /api/doctors/profile
        CROW_ROUTE(app, "/api/doctors/profile")
        .methods(crow::HTTPMethod::GET)
        ([this](const crow::request& req) {
            auto uid = FirebaseAuth::authenticate(req);
            if (uid.empty()) return crow::response{401, "Unauthorized"};

            try {
                auto doctor = doctorRepo_->findById(uid).get();
                crow::json::wvalue json;
                json["uuid"] = doctor.uuid;
                json["name"] = doctor.name;
                json["city"] = doctor.city;
                json["country"] = doctor.country;
                json["hospitalOrClinicName"] = doctor.hospitalOrClinicName;
                json["rating"] = doctor.rating;
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

        // PUT /api/doctors/profile
        CROW_ROUTE(app, "/api/doctors/profile")
        .methods(crow::HTTPMethod::PUT)
        ([this](const crow::request& req) {
            auto uid = FirebaseAuth::authenticate(req);
            if (uid.empty()) return crow::response{401, "Unauthorized"};

            auto body = crow::json::load(req.body);
            if (!body) return crow::response{400, "Invalid JSON body"};

            try {
                auto doctor = doctorRepo_->findById(uid).get();

                if (body.has("name")) doctor.name = body["name"].s();
                if (body.has("city")) doctor.city = body["city"].s();
                if (body.has("country")) doctor.country = body["country"].s();
                if (body.has("hospitalOrClinicName")) doctor.hospitalOrClinicName = body["hospitalOrClinicName"].s();
                if (body.has("about")) doctor.about = body["about"].s();
                if (body.has("appointmentDuration")) doctor.appointmentDuration = body["appointmentDuration"].i();
                if (body.has("bufferTime")) doctor.bufferTime = body["bufferTime"].i();

                bool ok = doctorRepo_->updateProfile(uid, doctor).get();
                if (ok) return crow::response{200, "Profile updated"};
                return crow::response{500, "Failed to update profile"};
            } catch (const std::exception& e) {
                return crow::response{500, e.what()};
            }
        });

        // PUT /api/doctors/profile/image
        CROW_ROUTE(app, "/api/doctors/profile/image")
        .methods(crow::HTTPMethod::PUT)
        ([this](const crow::request& req) {
            auto uid = FirebaseAuth::authenticate(req);
            if (uid.empty()) return crow::response{401, "Unauthorized"};

            auto body = crow::json::load(req.body);
            if (!body || !body.has("image")) {
                return crow::response{400, "Send {\"image\": \"<base64 data>\"}"};
            }

            try {
                std::string imageData = body["image"].s();
                bool ok = doctorRepo_->updateProfileImage(uid, imageData).get();
                if (ok) return crow::response{200, "Profile image updated"};
                return crow::response{500, "Failed to save image"};
            } catch (const std::exception& e) {
                return crow::response{500, e.what()};
            }
        });

        // GET /api/doctors/schedule
        CROW_ROUTE(app, "/api/doctors/schedule")
        .methods(crow::HTTPMethod::GET)
        ([this](const crow::request& req) {
            auto uid = FirebaseAuth::authenticate(req);
            if (uid.empty()) return crow::response{401, "Unauthorized"};

            try {
                auto schedule = scheduleRepo_->getSchedule(uid).get();
                crow::json::wvalue json;
                json["doctorId"] = schedule.doctorId;

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

        // PUT /api/doctors/schedule
        CROW_ROUTE(app, "/api/doctors/schedule")
        .methods(crow::HTTPMethod::PUT)
        ([this](const crow::request& req) {
            auto uid = FirebaseAuth::authenticate(req);
            if (uid.empty()) return crow::response{401, "Unauthorized"};

            auto body = crow::json::load(req.body);
            if (!body || !body.has("availability")) {
                return crow::response{400, "Send {\"availability\": {\"monday\": [{...}], ...}}"};
            }

            try {
                WeeklySchedule ws{};
                ws.doctorId = uid;

                for (auto& day : body["availability"]) {
                    std::string dayName = day.key();
                    LinkedList<TimeSlot> slots{};

                    for (auto& slotJson : day) {
                        TimeSlot ts{};
                        ts.startTime = slotJson["startTime"].s();
                        ts.endTime   = slotJson["endTime"].s();
                        slots.push(ts);
                    }

                    ws.availability[dayName] = std::move(slots);
                }

                bool ok = scheduleRepo_->updateSchedule(uid, ws).get();
                if (ok) return crow::response{200, "Schedule updated"};
                return crow::response{500, "Failed to update schedule"};
            } catch (const std::exception& e) {
                return crow::response{500, e.what()};
            }
        });

        // GET /api/doctors/appointments
        CROW_ROUTE(app, "/api/doctors/appointments")
        .methods(crow::HTTPMethod::GET)
        ([this](const crow::request& req) {
            auto uid = FirebaseAuth::authenticate(req);
            if (uid.empty()) return crow::response{401, "Unauthorized"};

            try {
                auto appointments = appointmentRepo_->findByDoctor(uid).get();
                std::vector<crow::json::wvalue> list;

                auto* node = appointments.getHead();
                while (node) {
                    crow::json::wvalue a;
                    a["id"]        = node->data.id;
                    a["patientId"] = node->data.patient.id;
                    try {
                        auto pat = patientRepo_->findById(node->data.patient.id).get();
                        a["patientName"] = pat.name;
                        a["patientImage"] = pat.profileImage;
                    } catch (...) {
                        a["patientName"] = "Unknown";
                    }
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

        // PATCH /api/doctors/appointments/<id>
        CROW_ROUTE(app, "/api/doctors/appointments/<string>")
        .methods(crow::HTTPMethod::PATCH)
        ([this](const crow::request& req, std::string aptId) {
            auto uid = FirebaseAuth::authenticate(req);
            if (uid.empty()) return crow::response{401, "Unauthorized"};

            auto body = crow::json::load(req.body);
            if (!body || !body.has("status")) return crow::response{400, "Need status integer"};

            try {
                int statusInt = body["status"].i();
                AppointmentStatus newStatus = static_cast<AppointmentStatus>(statusInt);
                
                bool ok = appointmentRepo_->updateStatus(aptId, newStatus).get();
                if (ok) return crow::response{200, "Appointment status updated"};
                return crow::response{404, "Appointment not found"};
            } catch (const std::exception& e) {
                return crow::response{500, e.what()};
            }
        });

        // POST /api/doctors/payments
        CROW_ROUTE(app, "/api/doctors/payments")
        .methods(crow::HTTPMethod::POST)
        ([this](const crow::request& req) {
            auto uid = FirebaseAuth::authenticate(req);
            if (uid.empty()) return crow::response{401, "Unauthorized"};

            auto body = crow::json::load(req.body);
            if (!body || !body.has("patientId") || !body.has("appointmentId") || !body.has("amount")) {
                return crow::response{400, "Need patientId, appointmentId, and amount"};
            }

            try {
                auto payment = paymentRepo_->recordPayment(
                    uid,
                    std::string{body["patientId"].s()},
                    std::string{body["appointmentId"].s()},
                    body["amount"].d()
                ).get();

                crow::json::wvalue json;
                json["id"]            = payment.id;
                json["doctorId"]      = payment.doctorId;
                json["patientId"]     = payment.patientId;
                json["appointmentId"] = payment.appointmentId;
                json["amount"]        = payment.amount;
                json["createdAt"]     = payment.createdAt;
                return crow::response{201, json};
            } catch (const std::exception& e) {
                return crow::response{500, e.what()};
            }
        });

        // GET /api/doctors/payments
        CROW_ROUTE(app, "/api/doctors/payments")
        .methods(crow::HTTPMethod::GET)
        ([this](const crow::request& req) {
            auto uid = FirebaseAuth::authenticate(req);
            if (uid.empty()) return crow::response{401, "Unauthorized"};

            try {
                auto payments = paymentRepo_->getByDoctor(uid).get();
                std::vector<crow::json::wvalue> list;

                auto* node = payments.getHead();
                while (node) {
                    crow::json::wvalue p;
                    p["id"]            = node->data.id;
                    p["patientId"]     = node->data.patientId;
                    p["appointmentId"] = node->data.appointmentId;
                    p["amount"]        = node->data.amount;
                    p["createdAt"]     = node->data.createdAt;
                    list.push_back(std::move(p));
                    node = node->next;
                }

                crow::json::wvalue json;
                json["payments"] = std::move(list);
                return crow::response{200, json};
            } catch (const std::exception& e) {
                return crow::response{500, e.what()};
            }
        });

        // GET /api/doctors/revenue
        CROW_ROUTE(app, "/api/doctors/revenue")
        .methods(crow::HTTPMethod::GET)
        ([this](const crow::request& req) {
            auto uid = FirebaseAuth::authenticate(req);
            if (uid.empty()) return crow::response{401, "Unauthorized"};

            try {
                auto stats = paymentRepo_->getRevenueStats(uid).get();
                crow::json::wvalue json;
                json["totalEarnings"] = stats.totalEarnings;
                json["totalPayments"] = stats.totalPayments;
                
                std::vector<crow::json::wvalue> dailyArray;
                for (auto& [dayKey, total] : stats.dailyBreakdown) {
                    crow::json::wvalue d;
                    d["date"] = dayKey;
                    d["total"] = total;
                    dailyArray.push_back(std::move(d));
                }
                json["dailyBreakdown"] = std::move(dailyArray);
                
                return crow::response{200, json};
            } catch (const std::exception& e) {
                return crow::response{500, e.what()};
            }
        });

        // GET /api/doctors/ratings
        CROW_ROUTE(app, "/api/doctors/ratings")
        .methods(crow::HTTPMethod::GET)
        ([this](const crow::request& req) {
            auto uid = FirebaseAuth::authenticate(req);
            if (uid.empty()) return crow::response{401, "Unauthorized"};

            try {
                auto ratings = ratingRepo_->getByDoctor(uid).get();
                double avg   = ratingRepo_->getAverageRating(uid).get();

                std::vector<crow::json::wvalue> list;
                auto* node = ratings.getHead();
                while (node) {
                    crow::json::wvalue r;
                    r["id"]        = node->data.id;
                    r["patientId"] = node->data.patientId;
                    r["stars"]     = node->data.stars;
                    r["comment"]   = node->data.comment;
                    r["createdAt"] = node->data.createdAt;
                    list.push_back(std::move(r));
                    node = node->next;
                }

                crow::json::wvalue json;
                json["averageRating"] = avg;
                json["ratings"]       = std::move(list);
                return crow::response{200, json};
            } catch (const std::exception& e) {
                return crow::response{500, e.what()};
            }
        });

        // GET /api/doctors/notifications
        CROW_ROUTE(app, "/api/doctors/notifications")
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

        // PATCH /api/doctors/notifications/<id>
        CROW_ROUTE(app, "/api/doctors/notifications/<string>")
        .methods(crow::HTTPMethod::PATCH)
        ([this](const crow::request& req, std::string notifId) {
            auto uid = FirebaseAuth::authenticate(req);
            if (uid.empty()) return crow::response{401, "Unauthorized"};

            try {
                bool ok = notificationRepo_->markAsRead(notifId).get();
                if (ok) return crow::response{200, "Notification marked as read"};
                return crow::response{404, "Notification not found"};
            } catch (const std::exception& e) {
                return crow::response{500, e.what()};
            }
        });
    }
};