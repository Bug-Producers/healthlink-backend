#pragma once

#include <string>
#include <vector>
#include <optional>
#include <future>
#include <mutex>

#include <mongocxx/pool.hpp>
#include <mongocxx/client.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/json.hpp>

#include "MongoConnection.h"

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

/**
 * @brief A wrapper around MongoDB that hides driver-specific code.
 */
class MongoService {
private:
    std::string dbName_;   // Which database to talk to (e.g. "healthlink")

public:
    /**
     * @brief Creates a service instance.
     */
    explicit MongoService(const std::string& dbName = "healthlink") {
        dbName_ = dbName;
    }

    /**
     * @brief Finds a single document that matches the filter.
     */
    std::optional<bsoncxx::document::value> findOne(
        const std::string& collectionName,
        bsoncxx::document::view filter
    ) {
        auto client = MongoConnection::getInstance().getPool().acquire();
        auto collection = (*client)[dbName_][collectionName];

        auto result = collection.find_one(filter);
        if (result) {
            return bsoncxx::document::value{result->view()};
        }
        return std::nullopt;
    }

    /**
     * @brief Fetches all documents matching a filter.
     */
    std::vector<bsoncxx::document::value> findMany(
        const std::string& collectionName,
        bsoncxx::document::view filter
    ) {
        auto client = MongoConnection::getInstance().getPool().acquire();
        auto collection = (*client)[dbName_][collectionName];

        std::vector<bsoncxx::document::value> results;
        auto cursor = collection.find(filter);

        for (auto& doc : cursor) {
            results.emplace_back(bsoncxx::document::value{doc});
        }
        return results;
    }

    /**
     * @brief Fetches all documents in a collection (no filter).
     */
    std::vector<bsoncxx::document::value> findAll(const std::string& collectionName) {
        return findMany(collectionName, make_document().view());
    }

    /**
     * @brief Inserts a document into a collection.
     */
    bool insertOne(
        const std::string& collectionName,
        bsoncxx::document::view document
    ) {
        auto client = MongoConnection::getInstance().getPool().acquire();
        auto collection = (*client)[dbName_][collectionName];

        auto result = collection.insert_one(document);
        return result.has_value();
    }

    /**
     * @brief Replaces a single document that matches the filter.
     */
    bool replaceOne(
        const std::string& collectionName,
        bsoncxx::document::view filter,
        bsoncxx::document::view replacement,
        bool upsert = false
    ) {
        auto client = MongoConnection::getInstance().getPool().acquire();
        auto collection = (*client)[dbName_][collectionName];

        mongocxx::options::replace opts{};
        if (upsert) opts.upsert(true);

        auto result = collection.replace_one(filter, replacement, opts);
        return result && (result->modified_count() > 0 || result->upserted_id().has_value());
    }

    /**
     * @brief Updates specific fields in a document.
     */
    bool updateOne(
        const std::string& collectionName,
        bsoncxx::document::view filter,
        bsoncxx::document::view update
    ) {
        auto client = MongoConnection::getInstance().getPool().acquire();
        auto collection = (*client)[dbName_][collectionName];

        auto result = collection.update_one(filter, update);
        return result && result->modified_count() > 0;
    }

    /**
     * @brief Appends a value to an array field.
     */
    bool pushToArray(
        const std::string& collectionName,
        bsoncxx::document::view filter,
        const std::string& field,
        const std::string& value
    ) {
        auto client = MongoConnection::getInstance().getPool().acquire();
        auto collection = (*client)[dbName_][collectionName];

        auto update = make_document(
            kvp("$push", make_document(kvp(field, value)))
        );

        mongocxx::options::update opts{};
        opts.upsert(true);

        auto result = collection.update_one(filter, update.view(), opts);
        return result.has_value();
    }
};
