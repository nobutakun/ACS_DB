#include "event_logger.hpp"
#include "db_connection.hpp"
#include <sqlite3.h>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace accessd {

EventLogger::EventLogger(std::shared_ptr<DatabaseConnection> db)
    : db_(db)
{
}

EventLogger::~EventLogger() = default;

bool EventLogger::write_event(const Event& event) {
    return insert_event(
        event.event_code,
        event.event_category,
        event.severity,
        event.rule_id,
        event.user_id,
        event.door_id,
        event.reader_id,
        event.device_id,
        event.schedule_id,
        event.allow,
        event.event_data
    );
}

bool EventLogger::log_access_granted(int user_id, int door_id, int reader_id,
                                     int rule_id, const std::string& credential_type) {
    // Build event data
    std::stringstream event_data;
    event_data << "{\"credential_type\":\"" << credential_type << "\"}";

    return insert_event(
        "access_granted",
        "access",
        "info",
        rule_id,
        user_id,
        door_id,
        reader_id,
        0,      // device_id
        0,      // schedule_id
        true,   // allow
        event_data.str()
    );
}

bool EventLogger::log_access_denied(int user_id, int door_id, int reader_id,
                                   const std::string& reason, int rule_id,
                                   const std::string& credential_type) {
    // Build event data
    std::stringstream event_data;
    event_data << "{\"reason\":\"" << reason
               << "\",\"credential_type\":\"" << credential_type << "\"}";

    return insert_event(
        "access_denied",
        "access",
        "warning",
        rule_id,
        user_id,
        door_id,
        reader_id,
        0,      // device_id
        0,      // schedule_id
        false,  // allow
        event_data.str()
    );
}

bool EventLogger::log_door_open(int door_id, int reader_id) {
    return insert_event(
        "door_open",
        "door",
        "info",
        0,      // rule_id
        0,      // user_id
        door_id,
        reader_id,
        0,      // device_id
        0,      // schedule_id
        false,  // allow (not applicable)
        "{}"
    );
}

bool EventLogger::log_door_closed(int door_id) {
    return insert_event(
        "door_closed",
        "door",
        "info",
        0, 0, door_id, 0, 0, 0, false, "{}"
    );
}

bool EventLogger::log_door_forced(int door_id) {
    return insert_event(
        "door_forced",
        "security",
        "error",
        0, 0, door_id, 0, 0, 0, false, "{}"
    );
}

bool EventLogger::log_door_held(int door_id) {
    return insert_event(
        "door_held",
        "security",
        "warning",
        0, 0, door_id, 0, 0, 0, false, "{}"
    );
}

bool EventLogger::log_reader_online(int reader_id, const std::string& device_uid) {
    std::stringstream event_data;
    event_data << "{\"device_uid\":\"" << device_uid << "\"}";

    return insert_event(
        "reader_online",
        "reader",
        "info",
        0, 0, 0, reader_id, 0, 0, false, event_data.str()
    );
}

bool EventLogger::log_reader_offline(int reader_id, const std::string& device_uid) {
    std::stringstream event_data;
    event_data << "{\"device_uid\":\"" << device_uid << "\"}";

    return insert_event(
        "reader_offline",
        "reader",
        "warning",
        0, 0, 0, reader_id, 0, 0, false, event_data.str()
    );
}

bool EventLogger::log_reader_error(int reader_id, const std::string& error) {
    std::stringstream event_data;
    event_data << "{\"error\":\"" << error << "\"}";

    return insert_event(
        "reader_error",
        "reader",
        "error",
        0, 0, 0, reader_id, 0, 0, false, event_data.str()
    );
}

bool EventLogger::log_system_start() {
    return insert_event(
        "system_start",
        "system",
        "info",
        0, 0, 0, 0, 0, 0, false, "{}"
    );
}

bool EventLogger::log_system_stop() {
    return insert_event(
        "system_stop",
        "system",
        "info",
        0, 0, 0, 0, 0, 0, false, "{}"
    );
}

bool EventLogger::log_system_error(const std::string& error) {
    std::stringstream event_data;
    event_data << "{\"error\":\"" << error << "\"}";

    return insert_event(
        "system_error",
        "system",
        "error",
        0, 0, 0, 0, 0, 0, false, event_data.str()
    );
}

bool EventLogger::insert_event(const std::string& event_code,
                              const std::string& event_category,
                              const std::string& severity,
                              int rule_id, int user_id, int door_id,
                              int reader_id, int device_id, int schedule_id,
                              bool allow, const std::string& event_data) {
    if (!db_ || !db_->is_open()) {
        std::cerr << "Database not connected" << std::endl;
        return false;
    }

    std::string query =
        "INSERT INTO events "
        "(event_code, event_category, severity, rule_id, user_id, door_id, "
        "reader_id, device_id, schedule_id, allow, event_data) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    auto stmt = db_->prepare(query);
    if (!stmt) {
        std::cerr << "Failed to prepare event query" << std::endl;
        return false;
    }

    stmt->bind_text(1, event_code);
    stmt->bind_text(2, event_category);
    stmt->bind_text(3, severity);

    if (rule_id > 0) {
        stmt->bind_int(4, rule_id);
    } else {
        stmt->bind_null(4);
    }

    if (user_id > 0) {
        stmt->bind_int(5, user_id);
    } else {
        stmt->bind_null(5);
    }

    if (door_id > 0) {
        stmt->bind_int(6, door_id);
    } else {
        stmt->bind_null(6);
    }

    if (reader_id > 0) {
        stmt->bind_int(7, reader_id);
    } else {
        stmt->bind_null(7);
    }

    if (device_id > 0) {
        stmt->bind_int(8, device_id);
    } else {
        stmt->bind_null(8);
    }

    if (schedule_id > 0) {
        stmt->bind_int(9, schedule_id);
    } else {
        stmt->bind_null(9);
    }

    stmt->bind_int(10, allow ? 1 : 0);
    stmt->bind_text(11, event_data);

    int result = stmt->step();
    if (result != SQLITE_DONE) {
        std::cerr << "Failed to insert event: " << db_->last_error() << std::endl;
        return false;
    }

    std::cout << "Event logged: " << event_code << std::endl;
    return true;
}

} // namespace accessd
