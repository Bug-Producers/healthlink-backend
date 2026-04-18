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
        NotificationRepository notificationRepo{};
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

        TestController testController{};

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
            crow::HTTPMethod::PUT
        )
        ([&router](const crow::request& req, const std::string&) {
            return router.handleRequest(req);
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