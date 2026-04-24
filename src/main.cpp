#include <crow.h>

#include "core/router/ApiRouter.h"
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

        // ================= Router =================
        ApiRouter router{};

        router.get("/", [](const crow::request&) {
            return crow::response(200, "HealthLink API is running perfectly!");
        });

        doctorController.registerRoutes(router);
        patientController.registerRoutes(router);
        testController.registerRoutes(router);


        CROW_ROUTE(app, "/<path>")
        .methods(
            crow::HTTPMethod::GET,
            crow::HTTPMethod::POST,
            crow::HTTPMethod::PUT,
            crow::HTTPMethod::PATCH
        )
        ([&router](const crow::request& req, const std::string&) {
            return router.handleRequest(req);
        });

        // ================= WebSocket =================
        CROW_WEBSOCKET_ROUTE(app, "/api/ws/notifications")
        .onopen([&](crow::websocket::connection& conn) {
            std::cout << "[WS] Handshake in progress...\n";
        })
        .onmessage([&](crow::websocket::connection& conn, const std::string& data, bool is_binary) {
            // First message must be "Bearer <token>"
            if (data.substr(0, 7) == "Bearer ") {
                std::string token = data.substr(7);
                std::string projectId = env::get("FIREBASE_PROJECT_ID", "");
                
                // Simplified Auth Check (matches FirebaseAuth logic)
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
        .onclose([&](crow::websocket::connection& conn, const std::string& reason) {
            notificationGateway.unregisterConnection(&conn);
        });

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