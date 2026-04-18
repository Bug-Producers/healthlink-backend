#pragma once

#include <string>
#include <mutex>
#include <iostream>

#include <mongocxx/instance.hpp>
#include <mongocxx/pool.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/options/client.hpp>
#include <mongocxx/options/server_api.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>

#include "../utils/env_loader.h"

/**
 * @brief Singleton that manages the MongoDB driver and connection pool.
 */
class MongoConnection {
private:
    mongocxx::instance instance_;
    mongocxx::pool* pool_;

    /**
     * @brief Sets up the driver and opens a connection pool to MongoDB.
     */
    MongoConnection() {
        pool_ = new mongocxx::pool{
            mongocxx::uri{env::get("MONGO_URI", "mongodb://localhost:27017")},
            buildPoolOptions()
        };
        // Try a quick ping to make sure the connection actually works.
        // If the URI is wrong or the cluster is down, we'll know right away.
        try {
            auto client = pool_->acquire();
            auto db = (*client)["admin"];
            auto ping = bsoncxx::builder::basic::make_document(
                bsoncxx::builder::basic::kvp("ping", 1)
            );
            db.run_command(ping.view());
            std::cout << "Connected to MongoDB successfully!" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "MongoDB connection failed: " << e.what() << std::endl;
        }
    }

    /**
     * @brief Builds the connection pool options.
     */
    static mongocxx::options::pool buildPoolOptions() {
        mongocxx::options::client clientOpts{};

        // Stable API v1 tells the server to only use behavior guaranteed
        // to be stable across major versions. Keeps things predictable.
        auto api = mongocxx::options::server_api{
            mongocxx::options::server_api::version::k_version_1
        };
        clientOpts.server_api_opts(api);

        return mongocxx::options::pool{clientOpts};
    }

    // No copies, no moves — just the one singleton.
    MongoConnection(const MongoConnection&) = delete;
    MongoConnection& operator=(const MongoConnection&) = delete;

public:
    /**
     * @brief Returns the single MongoConnection for the application.
     */
    static MongoConnection& getInstance() {
        static MongoConnection instance;
        return instance;
    }

    /**
     * @brief Hands out a reference to the connection pool.
     */
    mongocxx::pool& getPool() {
        return *pool_;
    }
};
