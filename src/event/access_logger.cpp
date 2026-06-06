#include "access_logger.hpp"
#include "db_connection.hpp"
#include <sqlite3.h>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace accessd {

AccessLogger::AccessLogger(std::shared_ptr<DatabaseConnection> db)
    : db_(db)
{
}

AccessLogger::~AccessLogger() = default;

bool AccessLogger::write_log(const AccessLogEntry& entry) {
    if (!db_ || !db_->is_open()) {
        std::cerr << "Database not connected" << std::endl;
        return false;
    }

    std::string query =
        "INSERT INTO access_logs "
        "(timestamp, user_id, door_id, reader_id, credential_id, "
        "credential_type, result, reason, rule_id, schedule_id, "
        "attempted_at, decision_latency_ms) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    auto stmt = db_->prepare(query);
    if (!stmt) {
        std::cerr << "Failed to prepare access log query" << std::endl;
        return false;
    }

    // Convert timestamp to string for storage (or store as int64)
    stmt->bind_int64(1, entry.timestamp);
    stmt->bind_int(2, entry.user_id);
    stmt->bind_int(3, entry.door_id);
    stmt->bind_int(4, entry.reader_id);

    if (entry.credential_id > 0) {
        stmt->bind_int(5, entry.credential_id);
    } else {
        stmt->bind_null(5);
    }

    stmt->bind_text(6, entry.credential_type);
    stmt->bind_text(7, entry.result);
    stmt->bind_text(8, entry.reason);

    if (entry.rule_id > 0) {
        stmt->bind_int(9, entry.rule_id);
    } else {
        stmt->bind_null(9);
    }

    if (entry.schedule_id > 0) {
        stmt->bind_int(10, entry.schedule_id);
    } else {
        stmt->bind_null(10);
    }

    stmt->bind_int64(11, entry.attempted_at);
    stmt->bind_int(12, entry.decision_latency_ms);

    int result = stmt->step();
    if (result != SQLITE_DONE) {
        std::cerr << "Failed to insert access log: " << db_->last_error() << std::endl;
        return false;
    }

    std::cout << "Access log written: user=" << entry.user_id
              << ", result=" << entry.result << std::endl;

    return true;
}

bool AccessLogger::log_allow(int user_id, int door_id, int reader_id,
                             int credential_id, const std::string& credential_type,
                             int rule_id, int schedule_id, int latency_ms) {
    AccessLogEntry entry;
    entry.timestamp = current_timestamp_ms();
    entry.user_id = user_id;
    entry.door_id = door_id;
    entry.reader_id = reader_id;
    entry.credential_id = credential_id;
    entry.credential_type = credential_type;
    entry.result = "allow";
    entry.reason = "Access granted";
    entry.rule_id = rule_id;
    entry.schedule_id = schedule_id;
    entry.attempted_at = entry.timestamp;
    entry.decision_latency_ms = latency_ms;

    return write_log(entry);
}

bool AccessLogger::log_deny(int user_id, int door_id, int reader_id,
                            const std::string& credential_type,
                            const std::string& reason, int rule_id,
                            int schedule_id, int latency_ms) {
    AccessLogEntry entry;
    entry.timestamp = current_timestamp_ms();
    entry.user_id = user_id;
    entry.door_id = door_id;
    entry.reader_id = reader_id;
    entry.credential_id = 0;  // May not have credential_id for deny
    entry.credential_type = credential_type;
    entry.result = "deny";
    entry.reason = reason;
    entry.rule_id = rule_id;
    entry.schedule_id = schedule_id;
    entry.attempted_at = entry.timestamp;
    entry.decision_latency_ms = latency_ms;

    return write_log(entry);
}

std::vector<AccessLogEntry> AccessLogger::get_logs(int user_id, int limit) {
    std::vector<AccessLogEntry> logs;

    if (!db_ || !db_->is_open()) {
        return logs;
    }

    std::string query =
        "SELECT log_id, timestamp, user_id, door_id, reader_id, "
        "credential_id, credential_type, result, reason, rule_id, schedule_id, "
        "attempted_at, decision_latency_ms "
        "FROM access_logs "
        "WHERE user_id = ? "
        "ORDER BY timestamp DESC, log_id DESC "
        "LIMIT ?";

    auto stmt = db_->prepare(query);
    if (!stmt) return logs;

    stmt->bind_int(1, user_id);
    stmt->bind_int(2, limit);

    while (stmt->step() == SQLITE_ROW) {
        AccessLogEntry entry;
        // Note: skip log_id (index 0)
        entry.timestamp = stmt->column_int64(1);
        entry.user_id = stmt->column_int(2);
        entry.door_id = stmt->column_int(3);
        entry.reader_id = stmt->column_int(4);
        entry.credential_id = stmt->column_int(5);
        entry.credential_type = stmt->column_text(6);
        entry.result = stmt->column_text(7);
        entry.reason = stmt->column_text(8);
        entry.rule_id = stmt->column_int(9);
        entry.schedule_id = stmt->column_int(10);
        entry.attempted_at = stmt->column_int64(11);
        entry.decision_latency_ms = stmt->column_int(12);

        logs.push_back(entry);
    }

    return logs;
}

std::vector<AccessLogEntry> AccessLogger::get_logs_by_door(int door_id, int limit) {
    std::vector<AccessLogEntry> logs;

    if (!db_ || !db_->is_open()) {
        return logs;
    }

    std::string query =
        "SELECT log_id, timestamp, user_id, door_id, reader_id, "
        "credential_id, credential_type, result, reason, rule_id, schedule_id, "
        "attempted_at, decision_latency_ms "
        "FROM access_logs "
        "WHERE door_id = ? "
        "ORDER BY timestamp DESC, log_id DESC "
        "LIMIT ?";

    auto stmt = db_->prepare(query);
    if (!stmt) return logs;

    stmt->bind_int(1, door_id);
    stmt->bind_int(2, limit);

    while (stmt->step() == SQLITE_ROW) {
        AccessLogEntry entry;
        entry.timestamp = stmt->column_int64(1);
        entry.user_id = stmt->column_int(2);
        entry.door_id = stmt->column_int(3);
        entry.reader_id = stmt->column_int(4);
        entry.credential_id = stmt->column_int(5);
        entry.credential_type = stmt->column_text(6);
        entry.result = stmt->column_text(7);
        entry.reason = stmt->column_text(8);
        entry.rule_id = stmt->column_int(9);
        entry.schedule_id = stmt->column_int(10);
        entry.attempted_at = stmt->column_int64(11);
        entry.decision_latency_ms = stmt->column_int(12);

        logs.push_back(entry);
    }

    return logs;
}

std::vector<AccessLogEntry> AccessLogger::get_recent_logs(int limit) {
    std::vector<AccessLogEntry> logs;

    if (!db_ || !db_->is_open()) {
        return logs;
    }

    std::string query =
        "SELECT log_id, timestamp, user_id, door_id, reader_id, "
        "credential_id, credential_type, result, reason, rule_id, schedule_id, "
        "attempted_at, decision_latency_ms "
        "FROM access_logs "
        "ORDER BY timestamp DESC, log_id DESC "
        "LIMIT ?";

    auto stmt = db_->prepare(query);
    if (!stmt) return logs;

    stmt->bind_int(1, limit);

    while (stmt->step() == SQLITE_ROW) {
        AccessLogEntry entry;
        entry.timestamp = stmt->column_int64(1);
        entry.user_id = stmt->column_int(2);
        entry.door_id = stmt->column_int(3);
        entry.reader_id = stmt->column_int(4);
        entry.credential_id = stmt->column_int(5);
        entry.credential_type = stmt->column_text(6);
        entry.result = stmt->column_text(7);
        entry.reason = stmt->column_text(8);
        entry.rule_id = stmt->column_int(9);
        entry.schedule_id = stmt->column_int(10);
        entry.attempted_at = stmt->column_int64(11);
        entry.decision_latency_ms = stmt->column_int(12);

        logs.push_back(entry);
    }

    return logs;
}

int AccessLogger::get_today_allow_count(int door_id) {
    if (!db_ || !db_->is_open()) {
        return 0;
    }

    // Get today's date in YYYY-MM-DD format
    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&tt);
    char date_buf[11];
    std::strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", &tm);

    std::string query =
        "SELECT COUNT(*) FROM access_logs "
        "WHERE result = 'allow' "
        "AND date(timestamp/1000, 'unixepoch') = ?";

    if (door_id > 0) {
        query += " AND door_id = ?";
    }

    auto stmt = db_->prepare(query);
    if (!stmt) return 0;

    stmt->bind_text(1, date_buf);
    if (door_id > 0) {
        stmt->bind_int(2, door_id);
    }

    if (stmt->step() == SQLITE_ROW) {
        return stmt->column_int(0);
    }

    return 0;
}

int AccessLogger::get_today_deny_count(int door_id) {
    if (!db_ || !db_->is_open()) {
        return 0;
    }

    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&tt);
    char date_buf[11];
    std::strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", &tm);

    std::string query =
        "SELECT COUNT(*) FROM access_logs "
        "WHERE result = 'deny' "
        "AND date(timestamp/1000, 'unixepoch') = ?";

    if (door_id > 0) {
        query += " AND door_id = ?";
    }

    auto stmt = db_->prepare(query);
    if (!stmt) return 0;

    stmt->bind_text(1, date_buf);
    if (door_id > 0) {
        stmt->bind_int(2, door_id);
    }

    if (stmt->step() == SQLITE_ROW) {
        return stmt->column_int(0);
    }

    return 0;
}

int64_t AccessLogger::current_timestamp_ms() const {
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
}

} // namespace accessd
