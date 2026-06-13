#include "decision_response.hpp"
#include "db_connection.hpp"
#include "osdp_reader.hpp"
#include "config.hpp"
#include <sqlite3.h>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <iostream>

namespace accessd {

DecisionResponse::DecisionResponse(std::shared_ptr<DatabaseConnection> db,
                                  std::shared_ptr<OSDPConnection> osdp)
    : db_(db)
    , osdp_(osdp)
{
}

DecisionResponse::~DecisionResponse() = default;

bool DecisionResponse::send_response(const EvaluationContext& context,
                                    const AccessDecision& decision,
                                    int reader_id) {
    // 1. Determine response action
    ResponseAction action = determine_action(decision);

    // 2. Send OSDP commands to reader
    if (!send_osdp_commands(reader_id, action)) {
        std::cerr << "Failed to send OSDP commands" << std::endl;
    }

    // 3. Write access log
    if (!write_access_log(context, decision, reader_id)) {
        std::cerr << "Failed to write access log" << std::endl;
    }

    // 4. Write event
    if (!write_event(context, decision, reader_id)) {
        std::cerr << "Failed to write event" << std::endl;
    }

    return true;
}

ResponseAction DecisionResponse::determine_action(const AccessDecision& decision) const {
    ResponseAction action;

    if (decision.allow) {
        // ALLOW response
        action.led_green = true;
        action.buzzer = true;
        action.buzzer_duration_ms = 100;  // Short beep
        action.door_relay = true;
        action.relay_time_ms = get_default_relay_time_ms();
        action.display_message = "ACCESS GRANTED";
    } else {
        // DENY response
        action.led_red = true;
        action.buzzer = true;
        action.buzzer_duration_ms = 500;  // Longer beep
        action.door_relay = false;
        action.display_message = "ACCESS DENIED";
    }

    return action;
}

bool DecisionResponse::send_osdp_commands(int reader_id,
                                          const ResponseAction& action) {
    if (!osdp_) {
        std::cerr << "OSDP connection not available" << std::endl;
        return false;
    }

    // Send LED command
    if (action.led_green) {
        osdp_->led(reader_id, 1, true);  // LED 1 (green)
    } else if (action.led_red) {
        osdp_->led(reader_id, 2, true);  // LED 2 (red)
    }

    // Send buzzer command
    if (action.buzzer) {
        osdp_->buzzer(reader_id, action.buzzer_duration_ms,
                     action.buzzer_duration_ms, 1);
    }

    // Send display message
    if (!action.display_message.empty()) {
        osdp_->display_text(reader_id, action.display_message, 3);
    }

    // Trigger door relay via OSDP or GPIO
    if (action.door_relay) {
        // TODO: Trigger relay (may need separate GPIO control)
        std::cout << "Door relay triggered for " << action.relay_time_ms
                  << "ms" << std::endl;

        // In production, this would:
        // 1. Set GPIO pin high
        // 2. Wait for relay_time_ms
        // 3. Set GPIO pin low
        // Or send OSDP command to door controller
    }

    return true;
}

bool DecisionResponse::write_access_log(const EvaluationContext& context,
                                        const AccessDecision& decision,
                                        int reader_id) {
    if (!db_ || !db_->is_open()) {
        return false;
    }

    // Get current timestamp
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();

    // Get door_id from context (convert to string if needed)
    std::string door_id = context.door_id;

    // Convert result to string
    std::string result = decision.allow ? "allow" : "deny";

    // Build query
    std::string query =
        "INSERT INTO access_logs "
        "(timestamp, user_id, door_id, reader_id, credential_type, "
        "result, reason, rule_id, schedule_id, attempted_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    auto stmt = db_->prepare(query);
    if (!stmt) {
        std::cerr << "Failed to prepare access log query" << std::endl;
        return false;
    }

    // Note: SQLite doesn't have native DATETIME, we store timestamps as strings or integers
    // For simplicity, storing timestamp as integer (milliseconds since epoch)
    std::string timestamp_str = std::to_string(now_ms);

    stmt->bind_text(1, timestamp_str);
    stmt->bind_int(2, context.user.user_id);
    stmt->bind_int(3, std::stoi(door_id));
    stmt->bind_int(4, reader_id);
    stmt->bind_text(5, context.credential_type);
    stmt->bind_text(6, result);
    stmt->bind_text(7, decision.reason);

    if (decision.rule_id > 0) {
        stmt->bind_int(8, decision.rule_id);
    } else {
        stmt->bind_null(8);
    }

    if (decision.schedule_id > 0) {
        stmt->bind_int(9, decision.schedule_id);
    } else {
        stmt->bind_null(9);
    }

    stmt->bind_text(10, timestamp_str);  // attempted_at same as timestamp

    int ret = stmt->step();
    if (ret != SQLITE_DONE) {
        std::cerr << "Failed to insert access log: " << db_->last_error() << std::endl;
        return false;
    }

    std::cout << "Access log written: user=" << context.user.user_id
              << ", result=" << result << std::endl;

    return true;
}

bool DecisionResponse::write_event(const EvaluationContext& context,
                                   const AccessDecision& decision,
                                   int reader_id) {
    if (!db_ || !db_->is_open()) {
        return false;
    }

    // Determine event code
    std::string event_code;
    std::string event_category = "access";
    std::string severity = "info";

    if (decision.allow) {
        event_code = "access_granted";
    } else {
        event_code = "access_denied";
        severity = "warning";
    }

    // Build query
    std::string query =
        "INSERT INTO events "
        "(event_code, event_category, severity, rule_id, user_id, door_id, "
        "reader_id, schedule_id, allow, event_data) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    auto stmt = db_->prepare(query);
    if (!stmt) {
        std::cerr << "Failed to prepare event query" << std::endl;
        return false;
    }

    stmt->bind_text(1, event_code);
    stmt->bind_text(2, event_category);
    stmt->bind_text(3, severity);

    if (decision.rule_id > 0) {
        stmt->bind_int(4, decision.rule_id);
    } else {
        stmt->bind_null(4);
    }

    stmt->bind_int(5, context.user.user_id);
    stmt->bind_int(6, std::stoi(context.door_id));
    stmt->bind_int(7, reader_id);
    stmt->bind_null(8);  // schedule_id

    // Convert bool to int for SQLite
    stmt->bind_int(9, decision.allow ? 1 : 0);

    // Event data as JSON string
    std::stringstream event_data;
    event_data << "{\"reason\":\"" << decision.reason
               << "\",\"credential_type\":\"" << context.credential_type
               << "\",\"rule_name\":\"" << decision.rule_name << "\"}";
    stmt->bind_text(10, event_data.str());

    int ret = stmt->step();
    if (ret != SQLITE_DONE) {
        std::cerr << "Failed to insert event: " << db_->last_error() << std::endl;
        return false;
    }

    std::cout << "Event written: " << event_code << std::endl;

    return true;
}

int DecisionResponse::get_default_relay_time_ms() const {
    // Get from config
    auto& config = Config::instance();
    return config.hardware().relay_default_ms;
}

} // namespace accessd
