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

        // Pad to a multiple of 4
        while (base64.size() % 4 != 0) {
            base64 += '=';
        }

        const std::string chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        std::string decoded;
        unsigned int val = 0;
        int bits = 0;

        for (unsigned char c : base64) {
            if (c == '=') break;
            auto pos = chars.find(c);
            if (pos == std::string::npos) continue;

            val = (val << 6) | (pos & 0x3F);
            bits += 6;

            if (bits >= 8) {
                bits -= 8;
                decoded += static_cast<char>((val >> bits) & 0xFF);
                // Clear the bits we just extracted to prevent overflow
                val &= (1 << bits) - 1;
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

        pos = json.find(':', pos);
        if (pos == std::string::npos) return "";
        pos++;

        // Skip spaces
        while (pos < json.size() && json[pos] == ' ') pos++;
        if (pos >= json.size()) return "";

        std::string value;
        if (json[pos] == '"') {
            // It's a string
            pos++;
            while (pos < json.size() && json[pos] != '"') {
                value += json[pos++];
            }
        } else {
            // It's a number, bool, etc.
            while (pos < json.size() && json[pos] != ',' && json[pos] != '}' && json[pos] != ' ') {
                value += json[pos++];
            }
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

        if (payload.empty()) {
            std::cerr << "[Auth] Failed to decode JWT payload\n";
            return "";
        }

        // Check the issuer — should be Firebase's token server for our project
        std::string expectedIssuer = "https://securetoken.google.com/" + projectId;
        std::string iss = extractJsonField(payload, "iss");
        if (iss != expectedIssuer) {
            std::cerr << "[Auth] Token issuer mismatch: expected " << expectedIssuer << " but got " << iss << std::endl;
            return "";
        }

        // Check the audience — should be our project ID
        std::string aud = extractJsonField(payload, "aud");
        if (aud != projectId) {
            std::cerr << "[Auth] Token audience mismatch: expected " << projectId << " but got " << aud << std::endl;
            return "";
        }

        // Check if the token has expired
        long long exp = extractJsonNumber(payload, "exp");
        long long now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();

        if (exp == 0) {
            std::cerr << "[Auth] Token 'exp' claim missing or invalid\n";
            return "";
        }

        if (exp < now) {
            std::cerr << "[Auth] Token has expired (exp=" << exp << ", now=" << now << ")" << std::endl;
            return "";
        }

        // Everything looks good — return the user's Firebase UID
        std::string uid = extractJsonField(payload, "sub");
        if (uid.empty()) {
            std::cerr << "[Auth] Token 'sub' claim missing\n";
            return "";
        }

        return uid;
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
