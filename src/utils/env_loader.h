#pragma once

#include <fstream>
#include <string>
#include <iostream>
#include <cstdlib>

namespace env {

    std::string get(const char* key, const std::string& fallback = "") {
        const char* val = std::getenv(key);
        return val ? std::string(val) : fallback;
    }

    void load() {
        std::ifstream file(".env");
        if (!file.is_open()) {
            file.open("../.env"); // Check parent dir for cmake-build-debug environments
            if (!file.is_open()) {
                file.open("../../.env");
            }
        }

        if (!file.is_open()) {
            std::cerr << "Warning: .env file not found anywhere. Using system env variables.\n";
            return;
        }

        std::string line;

        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#')
                continue;

            auto pos = line.find('=');
            if (pos == std::string::npos)
                continue;

            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);

            value.erase(value.find_last_not_of(" \t\r\n") + 1);

#ifdef _WIN32
            _putenv_s(key.c_str(), value.c_str());
#else
            setenv(key.c_str(), value.c_str(), 1);
#endif
        }
    }

} // namespace env