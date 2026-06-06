#ifndef ACCESSD_DECISION_RESPONSE_HPP
#define ACCESSD_DECISION_RESPONSE_HPP

#include "access_rule_evaluator.hpp"
#include "osdp_reader.hpp"
#include <memory>

namespace accessd {

// Forward declarations
class DatabaseConnection;
class OSDPConnection;

// Response action
struct ResponseAction {
    bool led_green;              // Green LED on
    bool led_red;                // Red LED on
    bool buzzer;                 // Buzzer beep
    int buzzer_duration_ms;      // Buzzer on time
    bool door_relay;             // Trigger door relay
    int relay_time_ms;           // Relay activation time
    std::string display_message; // LCD message (optional)

    ResponseAction() : led_green(false), led_red(false), buzzer(false),
                       buzzer_duration_ms(0), door_relay(false),
                       relay_time_ms(0) {}
};

class DecisionResponse {
public:
    DecisionResponse(std::shared_ptr<DatabaseConnection> db,
                    std::shared_ptr<OSDPConnection> osdp);

    ~DecisionResponse();

    // Main response function
    bool send_response(const EvaluationContext& context,
                      const AccessDecision& decision,
                      int reader_id);

    // Write access log
    bool write_access_log(const EvaluationContext& context,
                         const AccessDecision& decision,
                         int reader_id);

    // Write event
    bool write_event(const EvaluationContext& context,
                    const AccessDecision& decision,
                    int reader_id);

    // Determine response action based on decision
    ResponseAction determine_action(const AccessDecision& decision) const;

    // Send OSDP commands
    bool send_osdp_commands(int reader_id, const ResponseAction& action);

private:
    std::shared_ptr<DatabaseConnection> db_;
    std::shared_ptr<OSDPConnection> osdp_;

    // Get relay time from config
    int get_default_relay_time_ms() const;
};

} // namespace accessd

#endif // ACCESSD_DECISION_RESPONSE_HPP
