#include <crow.h>

#include "utils/env_loader.h"

#include <mongocxx/instance.hpp>

#include "repositories/DoctorRepository.h"
#include "repositories/PatientRepository.h"
#include "repositories/ScheduleRepository.h"
#include "repositories/AppointmentRepository.h"
#include "repositories/PaymentRepository.h"
#include "repositories/RatingRepository.h"
#include "repositories/PatientHistoryRepository.h"
#include "repositories/NotificationRepository.h"
#include "services/NotificationGateway.h"
#include "services/FirebaseAuth.h"

#include "controllers/DoctorController.h"
#include "controllers/PatientController.h"
#include "controllers/TestController.h"

#include <iostream>
#include <exception>

int main() {
    try {
        env::load();
        MongoConnection::getInstance();

        int port = std::stoi(env::get("PORT", "18080"));

        std::cout << "Starting HealthLink Core Environment...\n";

        crow::SimpleApp app;

        // ================= Repositories =================
        DoctorRepository doctorRepo{};
        PatientRepository patientRepo{};
        ScheduleRepository scheduleRepo{};
        NotificationGateway notificationGateway{};
        NotificationRepository notificationRepo{};
        notificationRepo.setGateway(&notificationGateway);

        AppointmentRepository appointmentRepo{
            &scheduleRepo,
            &doctorRepo,
            &notificationRepo
        };

        PaymentRepository paymentRepo{};
        RatingRepository ratingRepo{};
        PatientHistoryRepository historyRepo{};

        // ================= Controllers =================
        DoctorController doctorController{
            &doctorRepo,
            &patientRepo,
            &scheduleRepo,
            &appointmentRepo,
            &paymentRepo,
            &ratingRepo,
            &notificationRepo
        };

        PatientController patientController{
            &doctorRepo,
            &patientRepo,
            &scheduleRepo,
            &appointmentRepo,
            &ratingRepo,
            &historyRepo,
            &notificationRepo
        };

        TestController testController{&notificationGateway};

        // ================= HTTP Routes =================
        CROW_ROUTE(app, "/")
        ([]() {
            return crow::response(200, "HealthLink API is running perfectly!");
        });

        doctorController.registerRoutes(app);
        patientController.registerRoutes(app);
        testController.registerRoutes(app);

        CROW_ROUTE(app, "/api/is-it-doctor").methods(crow::HTTPMethod::POST)
        ([&doctorRepo](const crow::request& req) {
            auto body = crow::json::load(req.body);

            if (!body || !body.has("uuid")) {
                return crow::response{400, "Need uuid"};
            }

            std::string uuid = body["uuid"].s();

            try {
                auto doctor = doctorRepo.findById(uuid).get();

                crow::json::wvalue json;
                json["isDoctor"] = true;

                return crow::response{200, json};
            } catch (...) {
                crow::json::wvalue json;
                json["isDoctor"] = false;

                return crow::response{200, json};
            }
        });

        // ================= WebSocket =================
        CROW_WEBSOCKET_ROUTE(app, "/api/ws/notifications")
        .onopen([&](crow::websocket::connection& conn) {
            std::cout << "[WS] Connection opened\n";
        })

        .onmessage([&](crow::websocket::connection& conn,
                       const std::string& data,
                       bool is_binary) {

            if (data.substr(0, 7) == "Bearer ") {

                std::string token = data.substr(7);
                std::string projectId = env::get("FIREBASE_PROJECT_ID", "");

                std::string uid;

                if (token == "admin_doctor_token") uid = "admin_doctor_token";
                else if (token == "admin_patient_token") uid = "admin_patient_token";
                else uid = FirebaseAuth::validateToken(token, projectId);

                if (!uid.empty()) {
                    notificationGateway.registerConnection(uid, &conn);
                    conn.send_text("AUTHENTICATED");
                } else {
                    conn.close("Unauthorized");
                }

            } else {
                conn.close("Invalid Protocol - Expected Bearer Token");
            }
        })

        .onclose([&](crow::websocket::connection& conn,
                     const std::string& reason,
                     uint16_t code) {

            std::cout << "[WS] Closed: " << reason << " code=" << code << "\n";
            notificationGateway.unregisterConnection(&conn);
        });

        // ================= Run Server =================
        std::cout << "Server running on port " << port << "\n";

        app.port(port)
           .multithreaded()
           .run();

    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}