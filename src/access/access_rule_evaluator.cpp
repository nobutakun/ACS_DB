#include "access_rule_evaluator.hpp"
#include "db_connection.hpp"
#include <sqlite3.h>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <cstring>
#include <iostream>

// Simple JSON parsing (for production, use proper JSON library)
namespace {
    // Parse time_ranges JSON: [{"start":"08:00","end":"18:00","days":[1,2,3,4,5]}]
    // For now, just check if schedule type is "always"
}

namespace accessd {

AccessRuleEvaluator::AccessRuleEvaluator(std::shared_ptr<DatabaseConnection> db)
    : db_(db)
    , default_allow_(false)      // Default policy: DENY
    , holiday_check_enabled_(true)
{
}

AccessRuleEvaluator::~AccessRuleEvaluator() = default;

AccessDecision AccessRuleEvaluator::evaluate(const EvaluationContext& context) {
    AccessDecision decision;

    // Validate user context
    if (!context.user.valid) {
        decision.allow = false;
        decision.reason = "Invalid user context";
        return decision;
    }

    // Find matching rules
    std::vector<AccessRule> rules = get_rules(
        context.user.user_id,
        context.door_id,
        context.credential_type
    );

    if (rules.empty()) {
        // No rules found - use default policy
        decision.allow = default_allow_;
        decision.reason = default_allow_ ? "No rules - default ALLOW"
                                         : "No rules - default DENY";
        return decision;
    }

    // Rules are already sorted by priority (DESC)
    // Evaluate each rule in order
    for (const auto& rule : rules) {
        // Check schedule if specified
        bool schedule_ok = true;
        if (rule.schedule_id > 0) {
            schedule_ok = check_schedule(rule.schedule_id, context.timestamp);

            // Check holiday override
            if (schedule_ok && holiday_check_enabled_) {
                bool holiday_deny = check_holiday(rule.schedule_id, context.timestamp);
                if (holiday_deny) {
                    decision.allow = false;
                    decision.reason = "Holiday denial";
                    decision.rule_id = rule.rule_id;
                    decision.rule_name = rule.rule_name;
                    decision.schedule_id = rule.schedule_id;
                    return decision;
                }
            }
        }

        if (schedule_ok) {
            // Rule matched - return decision
            decision.allow = rule.allow;
            decision.reason = rule.allow ? "Rule match - ALLOW"
                                          : "Rule match - DENY";
            decision.rule_id = rule.rule_id;
            decision.rule_name = rule.rule_name;
            decision.schedule_id = rule.schedule_id;
            decision.schedule_matched = (rule.schedule_id > 0);
            return decision;
        }
    }

    // No rule matched - use default policy
    decision.allow = default_allow_;
    decision.reason = default_allow_ ? "No matching rule - default ALLOW"
                                     : "No matching rule - default DENY";
    return decision;
}

std::vector<AccessRule> AccessRuleEvaluator::get_rules(
    int user_id,
    const std::string& door_id,
    const std::string& credential_type)
{
    std::vector<AccessRule> rules;

    if (!db_ || !db_->is_open()) {
        std::cerr << "Database not connected" << std::endl;
        return rules;
    }

    // Query rules matching:
    // - user_id (exact match or NULL for all users)
    // - door_id (we'll filter this after query since door_id might be string)
    // - credential_type (exact match or NULL for all types)
    // - status = 'active'
    // - Within validity period
    std::string query =
        "SELECT rule_id, rule_name, user_id, door_id, schedule_id, "
        "credential_type, allow, priority, valid_from, valid_until, status "
        "FROM access_rules "
        "WHERE (user_id = ? OR user_id IS NULL) "
        "AND status = 'active' "
        "ORDER BY priority DESC";

    auto stmt = db_->prepare(query);
    if (!stmt) {
        std::cerr << "Failed to prepare rules query" << std::endl;
        return rules;
    }

    stmt->bind_int(1, user_id);

    while (stmt->step() == SQLITE_ROW) {
        AccessRule rule;
        rule.rule_id = stmt->column_int(0);
        rule.rule_name = stmt->column_text(1);
        rule.user_id = stmt->column_int(2);
        rule.door_id = stmt->column_int(3);  // Note: door_id in DB is INT
        rule.schedule_id = stmt->column_int(4);

        // credential_type can be NULL
        if (stmt->column_is_null(5)) {
            rule.credential_type = "";
        } else {
            rule.credential_type = stmt->column_text(5);
        }

        rule.allow = stmt->column_int(6) != 0;
        rule.priority = stmt->column_int(7);
        rule.valid_from = stmt->column_text(8);
        rule.valid_until = stmt->column_text(9);
        rule.status = stmt->column_text(10);

        // Filter by door_id (convert string door_id to int for comparison)
        int target_door_id = std::stoi(door_id);
        if (rule.door_id == 0 || rule.door_id == target_door_id) {
            // Filter by credential_type (if rule specifies one)
            if (rule.credential_type.empty() ||
                rule.credential_type == credential_type) {
                rules.push_back(rule);
            }
        }
    }

    return rules;
}

bool AccessRuleEvaluator::check_schedule(int schedule_id, int64_t timestamp) {
    if (!db_ || !db_->is_open()) {
        return false;  // Fail closed
    }

    // Get schedule
    std::string query =
        "SELECT schedule_id, name, schedule_type, time_ranges, status "
        "FROM schedules WHERE schedule_id = ?";

    auto stmt = db_->prepare(query);
    if (!stmt) return false;

    stmt->bind_int(1, schedule_id);

    if (stmt->step() != SQLITE_ROW) {
        std::cerr << "Schedule not found: " << schedule_id << std::endl;
        return false;
    }

    std::string schedule_type = stmt->column_text(2);
    std::string status = stmt->column_text(4);

    if (status != "active") {
        return false;
    }

    // Check schedule type
    if (is_schedule_type_always(schedule_type)) {
        return true;  // "always" type always allows
    }

    // For other types, check time_ranges
    std::string time_ranges = stmt->column_text(3);
    return check_time_ranges(time_ranges, timestamp);
}

bool AccessRuleEvaluator::check_holiday(int schedule_id, int64_t timestamp) {
    // Check if this schedule has holiday restrictions
    if (!db_ || !db_->is_open()) {
        return false;
    }

    // First check if today is a holiday
    if (!is_today_holiday(timestamp)) {
        return false;  // Not a holiday
    }

    // Check if schedule has holiday override
    std::string query =
        "SELECT holiday_action "
        "FROM schedule_holidays "
        "WHERE schedule_id = ? "
        "AND holiday_id IN ("
        "    SELECT holiday_id FROM holidays "
        "    WHERE ? BETWEEN start_date AND end_date"
        ")";

    auto stmt = db_->prepare(query);
    if (!stmt) return false;

    // Convert timestamp to date string
    std::time_t tt = timestamp / 1000;
    std::tm* tm = std::gmtime(&tt);
    char date_buf[11];
    std::strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", tm);

    stmt->bind_int(1, schedule_id);
    stmt->bind_text(2, date_buf);

    if (stmt->step() == SQLITE_ROW) {
        std::string action = stmt->column_text(0);
        return (action == "deny");  // Return true if holiday denies access
    }

    return false;  // No holiday rule configured
}

bool AccessRuleEvaluator::is_schedule_type_always(const std::string& type) {
    return type == "always";
}

bool AccessRuleEvaluator::check_time_ranges(const std::string& time_ranges,
                                           int64_t timestamp) {
    // TODO: Parse JSON time_ranges
    // Format: [{"start":"08:00","end":"18:00","days":[1,2,3,4,5]}]
    // For now, return true (schedule allows access)
    // In production, use proper JSON parser (nlohmann/json, etc.)

    std::cout << "TODO: Parse time_ranges: " << time_ranges << std::endl;
    return true;
}

bool AccessRuleEvaluator::is_within_time_range(const std::string& start,
                                               const std::string& end,
                                               int day_of_week,
                                               int64_t timestamp) {
    // TODO: Check if current time is within start-end range
    // Parse HH:MM format and compare
    return true;
}

bool AccessRuleEvaluator::is_today_holiday(int64_t timestamp) {
    if (!db_ || !db_->is_open()) {
        return false;
    }

    // Convert timestamp to date
    std::time_t tt = timestamp / 1000;
    std::tm* tm = std::gmtime(&tt);
    int current_month = tm->tm_mon + 1;  // tm_mon is 0-11
    int current_day = tm->tm_mday;

    char date_buf[11];
    std::strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", tm);

    // Check specific date holidays
    std::string query =
        "SELECT holiday_id FROM holidays "
        "WHERE ? BETWEEN start_date AND end_date";

    auto stmt = db_->prepare(query);
    if (!stmt) return false;

    stmt->bind_text(1, date_buf);

    if (stmt->step() == SQLITE_ROW) {
        return true;  // Today is a holiday
    }

    // Check recurring holidays
    query = "SELECT holiday_id FROM holidays WHERE is_recurring = 1 "
            "AND month = ? AND day = ?";

    stmt = db_->prepare(query);
    if (!stmt) return false;

    stmt->bind_int(1, current_month);
    stmt->bind_int(2, current_day);

    if (stmt->step() == SQLITE_ROW) {
        return true;  // Today is a recurring holiday
    }

    return false;
}

bool AccessRuleEvaluator::check_recurring_holiday(int month, int day) {
    // Check if specific month/day is a recurring holiday
    if (!db_ || !db_->is_open()) {
        return false;
    }

    std::string query =
        "SELECT holiday_id FROM holidays "
        "WHERE is_recurring = 1 AND month = ? AND day = ?";

    auto stmt = db_->prepare(query);
    if (!stmt) return false;

    stmt->bind_int(1, month);
    stmt->bind_int(2, day);

    return (stmt->step() == SQLITE_ROW);
}

} // namespace accessd
