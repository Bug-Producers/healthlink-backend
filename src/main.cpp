#include <crow.h>
#include <mongocxx/instance.hpp>
#include <mongocxx/pool.hpp>
#include <mongocxx/uri.hpp>

#include "env_loader.h"

#include <iostream>

int main() {
    try {
        env::load();

        mongocxx::instance instance{};

        std::string mongo_uri = env::get("MONGO_URI", "mongodb://localhost:27017");
        int port = std::stoi(env::get("PORT", "18080"));

        std::cout << "Mongo URI: " << mongo_uri << "\n";

        mongocxx::pool db_pool{mongocxx::uri{mongo_uri}};

        crow::SimpleApp app;

        CROW_ROUTE(app, "/")([] {
            return "HealthLink API is running";
        });

        std::cout << "Server running on port " << port << "\n";

        app.port(port).multithreaded().run();

    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}