#pragma once

#include <string>
#include <vector>
#include <sstream>

/**
 * @brief Provides safe sequence manipulation algorithms natively.
 * 
 * Custom lists can sometimes memory-leak during string tokenization loops.
 * This utility acts as a secure processing bridge leveraging standard native vectors.
 *
 */
class StringUtils {
public:
    /**
     * @brief Splits a string by a delimiter while automatically stripping query string parameters natively.
     * 
     * @param path The raw string sequence to tokenize.
     * @param delimiter The character natively dividing the components (default is '/').
     * @return std::vector<std::string> A contiguous, safely allocated array holding each stripped segment natively.
     */
    static std::vector<std::string> split(std::string path, char delimiter = '/') {
        auto qMark = path.find('?');
        if (qMark != std::string::npos) path = path.substr(0, qMark);
        
        std::vector<std::string> segments;
        std::stringstream ss(path);
        std::string item;
        while (std::getline(ss, item, delimiter)) {
            if (!item.empty()) segments.push_back(std::move(item));
        }
        return segments;
    }
};
