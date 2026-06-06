#ifndef ACCESSD_EVENT_LOGGER_HPP
#define ACCESSD_EVENT_LOGGER_HPP

#include <string>
#include <memory>

namespace accessd {

// Forward declaration
class DatabaseConnection;

// Event data
struct Event {
    std::string event_code;
    std::string event_category;
    std::string severity;        // debug, info, warning, error, critical
    int rule_id;
    int user_id;
    int door_id;
    int reader_id;
    int device_id;
    int schedule_id;
    bool allow;
    std::string event_data;     // JSON string
};

class EventLogger {
public:
    EventLogger(std::shared_ptr<DatabaseConnection> db);
    ~EventLogger();

    // Write event to database
    bool write_event(const Event& event);

    // Convenience methods for common events
    bool log_access_granted(int user_id, int door_id, int reader_id,
                           int rule_id, const std::string& credential_type);

    bool log_access_denied(int user_id, int door_id, int reader_id,
                          const std::string& reason, int rule_id,
                          const std::string& credential_type);

    bool log_door_open(int door_id, int reader_id);
    bool log_door_closed(int door_id);
    bool log_door_forced(int door_id);
    bool log_door_held(int door_id);

    bool log_reader_online(int reader_id, const std::string& device_uid);
    bool log_reader_offline(int reader_id, const std::string& device_uid);
    bool log_reader_error(int reader_id, const std::string& error);

    bool log_system_start();
    bool log_system_stop();
    bool log_system_error(const std::string& error);

private:
    std::shared_ptr<DatabaseConnection> db_;

    // Helper to insert event
    bool insert_event(const std::string& event_code,
                     const std::string& event_category,
                     const std::string& severity,
                     int rule_id, int user_id, int door_id,
                     int reader_id, int device_id, int schedule_id,
                     bool allow, const std::string& event_data);
};

} // namespace accessd

#endif // ACCESSD_EVENT_LOGGER_HPP
