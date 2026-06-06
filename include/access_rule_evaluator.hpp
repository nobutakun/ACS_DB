#ifndef ACCESSD_ACCESS_RULE_EVALUATOR_HPP
#define ACCESSD_ACCESS_RULE_EVALUATOR_HPP

#include "credential_resolver.hpp"
#include <string>
#include <vector>
#include <memory>

namespace accessd {

// Forward declaration
class DatabaseConnection;

// Access rule from database
struct AccessRule {
    int rule_id;
    std::string rule_name;
    int user_id;
    int door_id;
    int schedule_id;
    std::string credential_type;  // Can be NULL (match all)
    bool allow;
    int priority;
    std::string valid_from;
    std::string valid_until;
    std::string status;
};

// Schedule from database
struct Schedule {
    int schedule_id;
    std::string name;
    std::string schedule_type;  // always, office_hours, custom, holiday
    std::string time_ranges;    // JSON: [{"start":"08:00","end":"18:00","days":[1,2,3,4,5]}]
    std::string timezone;
    std::string status;
};

// Holiday from database
struct Holiday {
    int holiday_id;
    std::string name;
    std::string start_date;
    std::string end_date;
    bool is_recurring;
    int month;  // For recurring
    int day;
};

// Access decision
struct AccessDecision {
    bool allow;                    // ALLOW or DENY
    std::string reason;             // Why this decision
    int rule_id;                   // Matching rule ID
    std::string rule_name;         // Matching rule name
    int schedule_id;               // Schedule used (if any)
    bool schedule_matched;          // Whether schedule was checked/matched

    AccessDecision() : allow(false), rule_id(0), schedule_id(0),
                       schedule_matched(false) {}
};

// Evaluation context
struct EvaluationContext {
    UserContext user;
    std::string door_id;
    std::string credential_type;
    int64_t timestamp;             // Unix timestamp (milliseconds)

    EvaluationContext() : timestamp(0) {}
};

class AccessRuleEvaluator {
public:
    AccessRuleEvaluator(std::shared_ptr<DatabaseConnection> db);
    ~AccessRuleEvaluator();

    // Main evaluation function
    AccessDecision evaluate(const EvaluationContext& context);

    // Get all rules for a user-door pair
    std::vector<AccessRule> get_rules(int user_id, const std::string& door_id,
                                      const std::string& credential_type);

    // Check if schedule allows access at current time
    bool check_schedule(int schedule_id, int64_t timestamp);

    // Check if today is a holiday (and if holiday denies access)
    bool check_holiday(int schedule_id, int64_t timestamp);

    // Get default policy from config
    void set_default_policy(bool default_allow) {
        default_allow_ = default_allow;
    }

    // Enable/disable holiday checking
    void set_holiday_check_enabled(bool enabled) {
        holiday_check_enabled_ = enabled;
    }

private:
    std::shared_ptr<DatabaseConnection> db_;
    bool default_allow_;            // Default: deny
    bool holiday_check_enabled_;

    // Helper functions
    AccessRule find_matching_rule(const EvaluationContext& context);
    bool is_schedule_type_always(const std::string& type);
    bool check_time_ranges(const std::string& time_ranges, int64_t timestamp);
    bool is_within_time_range(const std::string& start, const std::string& end,
                              int day_of_week, int64_t timestamp);
    bool is_today_holiday(int64_t timestamp);
    bool check_recurring_holiday(int month, int day);
};

} // namespace accessd

#endif // ACCESSD_ACCESS_RULE_EVALUATOR_HPP
