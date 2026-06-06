#ifndef ACCESSD_ACCESS_LOGGER_HPP
#define ACCESSD_ACCESS_LOGGER_HPP

#include <string>
#include <memory>
#include <vector>

namespace accessd {

// Forward declaration
class DatabaseConnection;

// Access log entry
struct AccessLogEntry {
    int64_t timestamp;         // Unix timestamp (milliseconds)
    int user_id;
    int door_id;
    int reader_id;
    int credential_id;
    std::string credential_type;
    std::string result;        // "allow" or "deny"
    std::string reason;
    int rule_id;
    int schedule_id;
    int64_t attempted_at;
    int decision_latency_ms;   // Time taken to make decision
};

class AccessLogger {
public:
    AccessLogger(std::shared_ptr<DatabaseConnection> db);
    ~AccessLogger();

    // Write access log
    bool write_log(const AccessLogEntry& entry);

    // Convenience methods
    bool log_allow(int user_id, int door_id, int reader_id,
                  int credential_id, const std::string& credential_type,
                  int rule_id, int schedule_id, int latency_ms);

    bool log_deny(int user_id, int door_id, int reader_id,
                 const std::string& credential_type,
                 const std::string& reason, int rule_id,
                 int schedule_id, int latency_ms);

    // Query access logs
    std::vector<AccessLogEntry> get_logs(int user_id, int limit = 100);
    std::vector<AccessLogEntry> get_logs_by_door(int door_id, int limit = 100);
    std::vector<AccessLogEntry> get_recent_logs(int limit = 100);

    // Statistics
    int get_today_allow_count(int door_id = 0);
    int get_today_deny_count(int door_id = 0);

private:
    std::shared_ptr<DatabaseConnection> db_;

    // Get current timestamp as int64
    int64_t current_timestamp_ms() const;
};

} // namespace accessd

#endif // ACCESSD_ACCESS_LOGGER_HPP
