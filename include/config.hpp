#ifndef ACCESSD_CONFIG_HPP
#define ACCESSD_CONFIG_HPP

#include <string>
#include <unordered_map>

namespace accessd {

struct DatabaseConfig {
    std::string path;
    int timeout;
};

struct AccessdConfig {
    std::string default_policy;
    int decision_timeout;
    bool enable_cache;
    int cache_ttl;
};

struct LogConfig {
    std::string file;
    std::string level;
    bool console;
    int retention_days;
};

struct NetworkConfig {
    std::string bind_address;
    int port;
    bool enable_api;
};

struct HardwareConfig {
    int relay_default_ms;
    int door_timeout_sec;
    int door_held_timeout_sec;
};

struct SecurityConfig {
    bool auto_expire_users;
    bool enable_holiday_check;
    std::string timezone;
};

class Config {
public:
    static Config& instance();

    bool load(const std::string& config_file);

    // Getters
    const DatabaseConfig& database() const { return db_; }
    const AccessdConfig& accessd() const { return accessd_; }
    const LogConfig& log() const { return log_; }
    const NetworkConfig& network() const { return network_; }
    const HardwareConfig& hardware() const { return hardware_; }
    const SecurityConfig& security() const { return security_; }

private:
    Config() = default;
    bool parse_line(const std::string& line, std::string& section, std::string& key, std::string& value);

    DatabaseConfig db_;
    AccessdConfig accessd_;
    LogConfig log_;
    NetworkConfig network_;
    HardwareConfig hardware_;
    SecurityConfig security_;
};

} // namespace accessd

#endif // ACCESSD_CONFIG_HPP
