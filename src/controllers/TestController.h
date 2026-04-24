#pragma once

#include <crow.h>
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <string>

#include "../core/router/ApiRouter.h"
#include "../services/MongoService.h"
#include "../services/NotificationGateway.h"

/**
 * @brief Handles all development and testing endpoints (e.g., seeding the database).
 */
class TestController {
private:
    NotificationGateway* gateway_;

public:
    TestController(NotificationGateway* gateway) : gateway_(gateway) {}

    void registerRoutes(ApiRouter& router) {
        // Dev seed: Injects massive fake environment data natively
        router.post("/api/dev/seed", [](const crow::request&) {
            MongoService mongo;

            // Generate 30 fake records (15 Doctors, 15 Patients)
            for (int i = 1; i <= 30; i++) {
                if (i <= 15) {
                    std::string drId = "fake_doctor_" + std::to_string(i);
                    std::string payload = R"({
                        "uuid": ")" + drId + R"(",
                        "name": "Dr. Fake )" + std::to_string(i) + R"(",
                        "city": "Test City",
                        "country": "Test Country",
                        "hospitalOrClinicName": "Dev Hospital",
                        "rating": 4.5,
                        "expYears": 15,
                        "patients": 120,
                        "about": "Generated Testing Doctor",
                        "profileImage": "",
                        "appointmentDuration": 30,
                        "bufferTime": 5,
                        "department": { "name": "Cardiology", "count": 1 }
                    })";
                    auto dr = bsoncxx::from_json(payload);
                    mongo.replaceOne("doctors", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("uuid", drId)).view(), dr.view(), true);

                    // Add full availability schedules for each doctor
                    std::string schedPayload = R"({
                        "doctorId": ")" + drId + R"(",
                        "availability": {
                            "Monday": [
                                { "startTime": "09:00", "endTime": "12:00" },
                                { "startTime": "13:00", "endTime": "17:00" }
                            ],
                            "Wednesday": [
                                { "startTime": "10:00", "endTime": "15:00" }
                            ],
                            "Friday": [
                                { "startTime": "08:00", "endTime": "12:00" }
                            ]
                        }
                    })";
                    auto sch = bsoncxx::from_json(schedPayload);
                    mongo.replaceOne("schedules", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("doctorId", drId)).view(), sch.view(), true);

                } else {
                    std::string ptId = "fake_patient_" + std::to_string(i);
                    std::string payload = R"({
                        "id": ")" + ptId + R"(",
                        "name": "Fake Patient )" + std::to_string(i) + R"(",
                        "email": "patient)" + std::to_string(i) + R"(@test.local",
                        "dateOfBirth": "1990-01-01",
                        "gender": "Male"
                    })";
                    auto pt = bsoncxx::from_json(payload);
                    mongo.replaceOne("patients", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("id", ptId)).view(), pt.view(), true);

                    // Pre-load Medical History for testing Stack structure
                    std::string histPayload = R"({
                        "patientId": ")" + ptId + R"(",
                        "medicalReports": [
                            "General Checkup completed. Status: Healthy",
                            "Blood test results returned normal."
                        ]
                    })";
                    auto hist = bsoncxx::from_json(histPayload);
                    mongo.replaceOne("patient_history", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("patientId", ptId)).view(), hist.view(), true);
                }
            }

            // Always ensure the precise root DEV tokens exist
            auto adminDr = bsoncxx::from_json(R"({
                "uuid": "admin_doctor_token",
                "name": "Dr. System Admin",
                "city": "Admin City",
                "country": "Earth",
                "hospitalOrClinicName": "Dev Hospital",
                "rating": 5.0,
                "expYears": 20,
                "patients": 999,
                "about": "System Admin Doctor",
                "profileImage": "",
                "appointmentDuration": 30,
                "bufferTime": 10,
                "department": { "name": "Administration", "count": 1 }
            })");
            mongo.replaceOne("doctors", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("uuid", "admin_doctor_token")).view(), adminDr.view(), true);

            auto adminSched = bsoncxx::from_json(R"({
                "doctorId": "admin_doctor_token",
                "availability": {
                    "Monday": [ { "startTime": "09:00", "endTime": "12:00" } ],
                    "Tuesday": [ { "startTime": "09:00", "endTime": "12:00" } ]
                }
            })");
            mongo.replaceOne("schedules", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("doctorId", "admin_doctor_token")).view(), adminSched.view(), true);

            auto adminPt = bsoncxx::from_json(R"({
                "id": "admin_patient_token",
                "name": "Admin Patient",
                "email": "admin@healthlink.local",
                "dateOfBirth": "1990-01-01",
                "gender": "Male"
            })");
            mongo.replaceOne("patients", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("id", "admin_patient_token")).view(), adminPt.view(), true);

            auto adminHist = bsoncxx::from_json(R"({
                "patientId": "admin_patient_token",
                "medicalReports": [ "Admin Access Granted" ]
            })");
            mongo.replaceOne("patient_history", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("patientId", "admin_patient_token")).view(), adminHist.view(), true);

            return crow::response{201, "Seeded 30 fully relational generated records (with Availability & Medical History) + Master Dev Auth Tokens directly to MongoDB!"};
        });

        // Manual System Alert: Broadcasts to everyone online
        router.post("/api/dev/alert", [this](const crow::request& req) {
            auto body = crow::json::load(req.body);
            if (!body || !body.has("message")) {
                return crow::response{400, "Need message field"};
            }

            std::string msg = body["message"].s();
            gateway_->broadcast(msg);
            
            return crow::response{200, "Alert broadcasted"};
        });
    }
};
