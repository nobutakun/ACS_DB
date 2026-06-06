#include <gtest/gtest.h>
#include "access_rule_evaluator.hpp"
#include "db_connection.hpp"
#include <sqlite3.h>

class AccessRuleEvaluatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = "/tmp/test_rules.db";
        std::remove(db_path_.c_str());

        db_ = std::make_shared<accessd::DatabaseConnection>();
        ASSERT_TRUE(db_->open(db_path_));
        setup_test_data();
    }

    void TearDown() override {
        db_->close();
        std::remove(db_path_.c_str());
    }

    void setup_test_data() {
        // Create tables
        db_->execute("CREATE TABLE users ("
                    "user_id INTEGER PRIMARY KEY,"
                    "name TEXT NOT NULL,"
                    "status TEXT NOT NULL)");

        db_->execute("CREATE TABLE doors ("
                    "door_id INTEGER PRIMARY KEY,"
                    "name TEXT NOT NULL)");

        db_->execute("CREATE TABLE schedules ("
                    "schedule_id INTEGER PRIMARY KEY,"
                    "name TEXT NOT NULL,"
                    "schedule_type TEXT NOT NULL,"
                    "time_ranges TEXT,"
                    "status TEXT NOT NULL)");

        db_->execute("CREATE TABLE access_rules ("
                    "rule_id INTEGER PRIMARY KEY,"
                    "rule_name TEXT NOT NULL,"
                    "user_id INTEGER,"
                    "door_id INTEGER NOT NULL,"
                    "schedule_id INTEGER,"
                    "credential_type TEXT,"
                    "allow BOOLEAN NOT NULL,"
                    "priority INTEGER DEFAULT 0,"
                    "valid_from DATETIME DEFAULT CURRENT_TIMESTAMP,"
                    "valid_until DATETIME,"
                    "status TEXT NOT NULL)");

        // Insert test data
        db_->execute("INSERT INTO users (user_id, name, status) VALUES "
                    "(1, 'John Doe', 'active'),"
                    "(2, 'Jane Smith', 'active')");

        db_->execute("INSERT INTO doors (door_id, name) VALUES "
                    "(1, 'Main Door'),"
                    "(2, 'Back Door')");

        db_->execute("INSERT INTO schedules (schedule_id, name, schedule_type, status) VALUES "
                    "(1, 'Always', 'always', 'active'),"
                    "(2, 'Office Hours', 'office_hours', 'active')");

        // Insert rules:
        // Rule 1: All users, Main Door, always ALLOW (priority 100)
        // Rule 2: User 1, Main Door, always ALLOW (priority 90)
        // Rule 3: User 2, Main Door, always DENY (priority 110) - higher priority to override Rule 1
        // Rule 4: All users, Back Door, always DENY (priority 70)
        db_->execute("INSERT INTO access_rules (rule_name, user_id, door_id, schedule_id, allow, priority, status) VALUES "
                    "('All Users - Main Door', NULL, 1, 1, 1, 100, 'active'),"
                    "('User 1 - Main Door', 1, 1, 1, 1, 90, 'active'),"
                    "('User 2 - Main Door', 2, 1, 1, 0, 110, 'active'),"
                    "('All Users - Back Door', NULL, 2, 1, 0, 70, 'active')");
    }

    std::string db_path_;
    std::shared_ptr<accessd::DatabaseConnection> db_;
};

TEST_F(AccessRuleEvaluatorTest, EvaluateAllowRule) {
    accessd::AccessRuleEvaluator evaluator(db_);

    accessd::EvaluationContext context;
    context.user.user_id = 2;
    context.user.valid = true;
    context.door_id = "1";  // Main Door
    context.credential_type = "card";

    accessd::AccessDecision decision = evaluator.evaluate(context);

    // User 2 should be DENIED by Rule 3
    EXPECT_FALSE(decision.allow);
    EXPECT_EQ(decision.rule_id, 3);
}

TEST_F(AccessRuleEvaluatorTest, EvaluateDefaultAllow) {
    accessd::AccessRuleEvaluator evaluator(db_);
    evaluator.set_default_policy(true);  // Default ALLOW

    accessd::EvaluationContext context;
    context.user.user_id = 1;
    context.user.valid = true;
    context.door_id = "99";  // Door with no rules
    context.credential_type = "card";

    accessd::AccessDecision decision = evaluator.evaluate(context);

    // No rules - use default
    EXPECT_TRUE(decision.allow);
    EXPECT_EQ(decision.reason, "No rules - default ALLOW");
}

TEST_F(AccessRuleEvaluatorTest, EvaluateDefaultDeny) {
    accessd::AccessRuleEvaluator evaluator(db_);
    evaluator.set_default_policy(false);  // Default DENY

    accessd::EvaluationContext context;
    context.user.user_id = 1;
    context.user.valid = true;
    context.door_id = "99";  // Door with no rules
    context.credential_type = "card";

    accessd::AccessDecision decision = evaluator.evaluate(context);

    EXPECT_FALSE(decision.allow);
    EXPECT_EQ(decision.reason, "No rules - default DENY");
}

TEST_F(AccessRuleEvaluatorTest, EvaluateInvalidUser) {
    accessd::AccessRuleEvaluator evaluator(db_);

    accessd::EvaluationContext context;
    context.user.valid = false;  // Invalid user
    context.door_id = "1";
    context.credential_type = "card";

    accessd::AccessDecision decision = evaluator.evaluate(context);

    EXPECT_FALSE(decision.allow);
    EXPECT_EQ(decision.reason, "Invalid user context");
}

TEST_F(AccessRuleEvaluatorTest, GetRulesForUserDoor) {
    accessd::AccessRuleEvaluator evaluator(db_);

    auto rules = evaluator.get_rules(1, "1", "card");

    // Should have rules for user 1, door 1
    EXPECT_GT(rules.size(), 0);

    // Rules should be sorted by priority DESC
    if (rules.size() >= 2) {
        EXPECT_GE(rules[0].priority, rules[1].priority);
    }
}

TEST_F(AccessRuleEvaluatorTest, CheckScheduleAlways) {
    accessd::AccessRuleEvaluator evaluator(db_);

    // Schedule ID 1 is "always" type
    bool result = evaluator.check_schedule(1, time(nullptr) * 1000);
    EXPECT_TRUE(result);
}

TEST_F(AccessRuleEvaluatorTest, CheckScheduleOfficeHours) {
    accessd::AccessRuleEvaluator evaluator(db_);

    // Schedule ID 2 is "office_hours" type
    // For now, this returns true (TODO: implement time check)
    bool result = evaluator.check_schedule(2, time(nullptr) * 1000);
    EXPECT_TRUE(result);  // Will change when time check is implemented
}
