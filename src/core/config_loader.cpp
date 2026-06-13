#include "config.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace accessd {

Config& Config::instance() {
    static Config instance;
    return instance;
}   

bool Config::load(const std::string& config_file) {
    std::ifstream file(config_file);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    std::string current_section;

    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }

        std::string section, key, value;
        if (parse_line(line, section, key, value)) {
            current_section = section.empty() ? current_section : section;

            // Trim whitespace
            auto trim = [](std::string& s) {
                s.erase(0, s.find_first_not_of(" \t\r\n"));
                s.erase(s.find_last_not_of(" \t\r\n") + 1);
            };
            trim(key);
            trim(value);

            // Parse values
            if (current_section == "database") {
                if (key == "path") db_.path = value;
                else if (key == "timeout") db_.timeout = std::stoi(value);
            }
            else if (current_section == "accessd") {
                if (key == "default_policy") accessd_.default_policy = value;
                else if (key == "decision_timeout") accessd_.decision_timeout = std::stoi(value);
                else if (key == "enable_cache") accessd_.enable_cache = (value == "true");
                else if (key == "cache_ttl") accessd_.cache_ttl = std::stoi(value);
            }
            else if (current_section == "log") {
                if (key == "file") log_.file = value;
                else if (key == "level") log_.level = value;
                else if (key == "console") log_.console = (value == "true");
                else if (key == "retention_days") log_.retention_days = std::stoi(value);
            }
            else if (current_section == "network") {
                if (key == "bind_address") network_.bind_address = value;
                else if (key == "port") network_.port = std::stoi(value);
                else if (key == "enable_api") network_.enable_api = (value == "true");
            }
            else if (current_section == "hardware") {
                if (key == "relay_default_ms") hardware_.relay_default_ms = std::stoi(value);
                else if (key == "door_timeout_sec") hardware_.door_timeout_sec = std::stoi(value);
                else if (key == "door_held_timeout_sec") hardware_.door_held_timeout_sec = std::stoi(value);
            }
            else if (current_section == "security") {
                if (key == "auto_expire_users") security_.auto_expire_users = (value == "true");
                else if (key == "enable_holiday_check") security_.enable_holiday_check = (value == "true");
                else if (key == "timezone") security_.timezone = value;
            }
        }
    }

    return true;
}

bool Config::parse_line(const std::string& line, std::string& section, std::string& key, std::string& value) {
    section.clear();
    key.clear();
    value.clear();

    // Check for section [section]
    if (line[0] == '[' && line.back() == ']') {
        section = line.substr(1, line.size() - 2);
        return true;
    }

    // Parse key = value
    size_t pos = line.find('=');
    if (pos != std::string::npos) {
        key = line.substr(0, pos);
        value = line.substr(pos + 1);
        return true;
    }

    return false;
}

} // namespace accessd
