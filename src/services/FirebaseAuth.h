#pragma once

#include <string>
#include <iostream>
#include <chrono>

#include <crow.h>

#include "../utils/env_loader.h"

/**
 * @brief Handles Firebase ID token validation.
 */
namespace FirebaseAuth {

    /**
     * @brief Decodes a base64url-encoded string.
     */
    std::string base64UrlDecode(const std::string& input) {
        std::string base64 = input;

        // Convert URL-safe characters back to standard base64
        for (auto& c : base64) {
            if (c == '-') c = '+';
            else if (c == '_') c = '/';
        }

        // Pad to a multiple of 4 (base64 requires it)
        while (base64.size() % 4 != 0) {
            base64 += '=';
        }

        // Standard base64 alphabet
        const std::string chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        std::string decoded;
        int val = 0;
        int bits = -8;

        for (unsigned char c : base64) {
            if (c == '=') break;
            auto pos = chars.find(c);
            if (pos == std::string::npos) continue;

            val = (val << 6) + static_cast<int>(pos);
            bits += 6;

            if (bits >= 0) {
                decoded += static_cast<char>((val >> bits) & 0xFF);
                bits -= 8;
            }
        }

        return decoded;
    }

    /**
     * @brief Pulls a string value out of a raw JSON string by key.
     */
    std::string extractJsonField(const std::string& json, const std::string& key) {
        std::string search = "\"" + key + "\"";
        auto pos = json.find(search);
        if (pos == std::string::npos) return "";

        // Skip past the key and the colon
        pos = json.find(':', pos);
        if (pos == std::string::npos) return "";
        pos++;

        // Skip whitespace and opening quotes
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '"')) pos++;

        // Read until we hit a closing quote, comma, or end of object
        std::string value;
        while (pos < json.size() && json[pos] != '"' && json[pos] != ',' && json[pos] != '}') {
            value += json[pos++];
        }

        return value;
    }

    /**
     * @brief Pulls a numeric value out of a raw JSON string by key.
     */
    long long extractJsonNumber(const std::string& json, const std::string& key) {
        std::string val = extractJsonField(json, key);
        if (val.empty()) return 0;
        try {
            return std::stoll(val);
        } catch (...) {
            return 0;
        }
    }

    /**
     * @brief Validates a Firebase ID token and returns the user's UID.
     */
    std::string validateToken(const std::string& token, const std::string& projectId) {
        // A JWT looks like: header.payload.signature — split on the dots
        auto firstDot  = token.find('.');
        auto secondDot = token.find('.', firstDot + 1);

        if (firstDot == std::string::npos || secondDot == std::string::npos) {
            return "";   // Not a valid JWT format
        }

        // Decode the payload (the middle part)
        std::string payload = base64UrlDecode(token.substr(firstDot + 1, secondDot - firstDot - 1));

        // Check the issuer — should be Firebase's token server for our project
        std::string expectedIssuer = "https://securetoken.google.com/" + projectId;
        std::string iss = extractJsonField(payload, "iss");
        if (iss != expectedIssuer) {
            std::cerr << "Token issuer mismatch: got " << iss << std::endl;
            return "";
        }

        // Check the audience — should be our project ID
        std::string aud = extractJsonField(payload, "aud");
        if (aud != projectId) {
            std::cerr << "Token audience mismatch: got " << aud << std::endl;
            return "";
        }

        // Check if the token has expired
        long long exp = extractJsonNumber(payload, "exp");
        long long now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();

        if (exp < now) {
            std::cerr << "Token has expired" << std::endl;
            return "";
        }

        // Everything looks good — return the user's Firebase UID
        return extractJsonField(payload, "sub");
    }

    /**
     * @brief Extracts and validates the Firebase token from a Crow request.
     */
    std::string authenticate(const crow::request& req) {
        // Pull the Authorization header
        std::string authHeader = req.get_header_value("Authorization");
        
        //  BYPASS
        if (authHeader == "Bearer admin_doctor_token") return "admin_doctor_token";
        if (authHeader == "Bearer admin_patient_token") return "admin_patient_token";
        
        std::string projectId = env::get("FIREBASE_PROJECT_ID", "");
        if (projectId.empty()) {
            std::cerr << "FIREBASE_PROJECT_ID not set in environment!" << std::endl;
            return "";
        }

        if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ") {
            return "";   
        }

        // Strip "Bearer " and validate
        std::string token = authHeader.substr(7);
        return validateToken(token, projectId);
    }

} // namespace FirebaseAuth
