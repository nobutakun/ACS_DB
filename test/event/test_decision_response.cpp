#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <sqlite3.h>
#include "decision_response.hpp"
#include "access_rule_evaluator.hpp"
#include "db_connection.hpp"
#include "osdp_reader.hpp"
#include "config.hpp"
#include <memory>

namespace accessd {

// Mock DatabaseConnection
class MockDatabaseConnection : public DatabaseConnection {
public:
    MOCK_METHOD(bool, open, (const std::string& db_path), (override));
    MOCK_METHOD(void, close, (), (override));
    MOCK_METHOD(bool, is_open, (), (const, override));
    MOCK_METHOD(bool, execute, (const std::string& sql), (override));
    MOCK_METHOD(std::unique_ptr<Statement>, prepare, (const std::string& sql), (override));
    MOCK_METHOD(bool, begin_transaction, (), (override));
    MOCK_METHOD(bool, commit, (), (override));
    MOCK_METHOD(bool, rollback, (), (override));
    MOCK_METHOD(std::string, last_error, (), (const, override));
    MOCK_METHOD(int64_t, last_insert_rowid, (), (const, override));
};

// Mock OSDPConnection
class MockOSDPConnection : public OSDPConnection {
public:
    MOCK_METHOD(bool, add_reader, (const OSDPReader& reader), (override));
    MOCK_METHOD(bool, remove_reader, (int reader_id), (override));
    MOCK_METHOD(bool, remove_reader_by_address, (int osdp_address), (override));
    MOCK_METHOD(bool, start, (), (override));
    MOCK_METHOD(void, stop, (), (override));
    MOCK_METHOD(void, set_access_callback, (AccessRequestCallback callback), (override));
    MOCK_METHOD(std::vector<OSDPReader>, get_readers, (), (const, override));
    MOCK_METHOD(bool, led, (int pd_id, int led_number, bool on), (override));
    MOCK_METHOD(bool, buzzer, (int pd_id, int on_time_ms, int off_time_ms, int count), (override));
    MOCK_METHOD(bool, display_text, (int pd_id, const std::string& text, int duration_sec), (override));
};

// Mock Statement
class MockStatement : public Statement {
public:
    MockStatement() : Statement(nullptr) {}
    MOCK_METHOD(void, bind_int, (int index, int value), (override));
    MOCK_METHOD(void, bind_int64, (int index, int64_t value), (override));
    MOCK_METHOD(void, bind_text, (int index, const std::string& value), (override));
    MOCK_METHOD(void, bind_null, (int index), (override));
    MOCK_METHOD(int, step, (), (override));
    MOCK_METHOD(int, column_int, (int index), (const, override));
    MOCK_METHOD(int64_t, column_int64, (int index), (const, override));
    MOCK_METHOD(std::string, column_text, (int index), (const, override));
    MOCK_METHOD(double, column_double, (int index), (const, override));
    MOCK_METHOD(bool, column_is_null, (int index), (const, override));
    MOCK_METHOD(void, reset, (), (override));
    MOCK_METHOD(bool, is_valid, (), (const, override));
};

class DecisionResponseTest : public ::testing::Test {
protected:
    void SetUp() override {
        mock_db = std::make_shared<MockDatabaseConnection>();
        mock_osdp = std::make_shared<MockOSDPConnection>();

        // Setup default config for testing
        auto& config = Config::instance();
        config.set_hardware_defaults(2000, 30, 60);
    }

    void TearDown() override {}

    std::shared_ptr<MockDatabaseConnection> mock_db;
    std::shared_ptr<MockOSDPConnection> mock_osdp;
};

// =============================================================================
// Test: determine_action()
// =============================================================================

TEST_F(DecisionResponseTest, DetermineAction_AllowDecision) {
    DecisionResponse responder(mock_db, mock_osdp);

    AccessDecision decision;
    decision.allow = true;
    decision.reason = "Rule matched";

    ResponseAction action = responder.determine_action(decision);

    EXPECT_TRUE(action.led_green);
    EXPECT_FALSE(action.led_red);
    EXPECT_TRUE(action.buzzer);
    EXPECT_EQ(action.buzzer_duration_ms, 100);
    EXPECT_TRUE(action.door_relay);
    EXPECT_GT(action.relay_time_ms, 0);
    EXPECT_EQ(action.display_message, "ACCESS GRANTED");
}

TEST_F(DecisionResponseTest, DetermineAction_DenyDecision) {
    DecisionResponse responder(mock_db, mock_osdp);

    AccessDecision decision;
    decision.allow = false;
    decision.reason = "No matching rule";

    ResponseAction action = responder.determine_action(decision);

    EXPECT_FALSE(action.led_green);
    EXPECT_TRUE(action.led_red);
    EXPECT_TRUE(action.buzzer);
    EXPECT_EQ(action.buzzer_duration_ms, 500);  // Longer beep for deny
    EXPECT_FALSE(action.door_relay);
    EXPECT_EQ(action.display_message, "ACCESS DENIED");
}

// =============================================================================
// Test: send_osdp_commands()
// =============================================================================

TEST_F(DecisionResponseTest, SendOsdpCommands_AllowAction) {
    EXPECT_CALL(*mock_osdp, led(1, 1, true))  // Green LED
        .WillOnce(::testing::Return(true));
    EXPECT_CALL(*mock_osdp, buzzer(1, 100, 100, 1))  // Short beep
        .WillOnce(::testing::Return(true));
    EXPECT_CALL(*mock_osdp, display_text(1, "ACCESS GRANTED", 3))
        .WillOnce(::testing::Return(true));

    DecisionResponse responder(mock_db, mock_osdp);

    ResponseAction action;
    action.led_green = true;
    action.buzzer = true;
    action.buzzer_duration_ms = 100;
    action.door_relay = true;
    action.relay_time_ms = 2000;
    action.display_message = "ACCESS GRANTED";

    bool result = responder.send_osdp_commands(1, action);
    EXPECT_TRUE(result);
}

TEST_F(DecisionResponseTest, SendOsdpCommands_DenyAction) {
    EXPECT_CALL(*mock_osdp, led(1, 2, true))  // Red LED
        .WillOnce(::testing::Return(true));
    EXPECT_CALL(*mock_osdp, buzzer(1, 500, 500, 1))  // Longer beep
        .WillOnce(::testing::Return(true));
    EXPECT_CALL(*mock_osdp, display_text(1, "ACCESS DENIED", 3))
        .WillOnce(::testing::Return(true));

    DecisionResponse responder(mock_db, mock_osdp);

    ResponseAction action;
    action.led_red = true;
    action.buzzer = true;
    action.buzzer_duration_ms = 500;
    action.door_relay = false;
    action.display_message = "ACCESS DENIED";

    bool result = responder.send_osdp_commands(1, action);
    EXPECT_TRUE(result);
}

TEST_F(DecisionResponseTest, SendOsdpCommands_NullOsdpConnection) {
    DecisionResponse responder(mock_db, nullptr);  // No OSDP connection

    ResponseAction action;
    action.led_green = true;

    bool result = responder.send_osdp_commands(1, action);
    EXPECT_FALSE(result);
}

// =============================================================================
// Test: write_access_log()
// =============================================================================

TEST_F(DecisionResponseTest, WriteAccessLog_AllowDecision) {
    auto* mock_stmt = new MockStatement();

    EXPECT_CALL(*mock_db, is_open())
        .WillOnce(::testing::Return(true));

    EXPECT_CALL(*mock_stmt, bind_text(::testing::_, ::testing::_))
        .Times(5);  // timestamp, credential_type, result, reason, attempted_at
    EXPECT_CALL(*mock_stmt, bind_int(::testing::_, ::testing::_))
        .Times(4);  // user_id, door_id, reader_id, rule_id
    EXPECT_CALL(*mock_stmt, bind_null(::testing::_))
        .Times(1);  // schedule_id (null)
    EXPECT_CALL(*mock_stmt, step())
        .WillOnce(::testing::Return(SQLITE_DONE));

    EXPECT_CALL(*mock_db, prepare(::testing::_))
        .WillOnce(::testing::Return(
            ::testing::ByMove(std::unique_ptr<Statement>(mock_stmt))));

    DecisionResponse responder(mock_db, mock_osdp);

    EvaluationContext context;
    context.user.user_id = 123;
    context.user.name = "John Doe";
    context.door_id = "1";
    context.credential_type = "card";
    context.timestamp = 1234567890000;

    AccessDecision decision;
    decision.allow = true;
    decision.reason = "Valid credential";
    decision.rule_id = 10;
    decision.schedule_id = 0;

    bool result = responder.write_access_log(context, decision, 1);
    EXPECT_TRUE(result);
}

TEST_F(DecisionResponseTest, WriteAccessLog_DatabaseNotOpen) {
    EXPECT_CALL(*mock_db, is_open())
        .WillOnce(::testing::Return(false));

    DecisionResponse responder(mock_db, mock_osdp);

    EvaluationContext context;
    AccessDecision decision;

    bool result = responder.write_access_log(context, decision, 1);
    EXPECT_FALSE(result);
}

// =============================================================================
// Test: write_event()
// =============================================================================

TEST_F(DecisionResponseTest, WriteEvent_AllowEvent) {
    auto* mock_stmt = new MockStatement();

    EXPECT_CALL(*mock_db, is_open())
        .WillOnce(::testing::Return(true));

    EXPECT_CALL(*mock_stmt, bind_text(::testing::_, ::testing::_))
        .Times(4);  // event_code, category, severity, event_data
    EXPECT_CALL(*mock_stmt, bind_int(::testing::_, ::testing::_))
        .Times(5);  // rule_id, user_id, door_id, reader_id, allow
    EXPECT_CALL(*mock_stmt, bind_null(::testing::_))
        .Times(1);  // schedule_id
    EXPECT_CALL(*mock_stmt, step())
        .WillOnce(::testing::Return(SQLITE_DONE));

    EXPECT_CALL(*mock_db, prepare(::testing::_))
        .WillOnce(::testing::Return(
            ::testing::ByMove(std::unique_ptr<Statement>(mock_stmt))));

    DecisionResponse responder(mock_db, mock_osdp);

    EvaluationContext context;
    context.user.user_id = 123;
    context.door_id = "1";
    context.credential_type = "card";

    AccessDecision decision;
    decision.allow = true;
    decision.reason = "Access granted";
    decision.rule_id = 10;
    decision.rule_name = "Office Hours Rule";

    bool result = responder.write_event(context, decision, 1);
    EXPECT_TRUE(result);
}

TEST_F(DecisionResponseTest, WriteEvent_DenyEvent) {
    auto* mock_stmt = new MockStatement();

    EXPECT_CALL(*mock_db, is_open())
        .WillOnce(::testing::Return(true));

    EXPECT_CALL(*mock_stmt, step())
        .WillOnce(::testing::Return(SQLITE_DONE));

    EXPECT_CALL(*mock_db, prepare(::testing::_))
        .WillOnce(::testing::Return(
            ::testing::ByMove(std::unique_ptr<Statement>(mock_stmt))));

    DecisionResponse responder(mock_db, mock_osdp);

    EvaluationContext context;
    context.user.user_id = 123;
    context.door_id = "1";
    context.credential_type = "card";

    AccessDecision decision;
    decision.allow = false;
    decision.reason = "Invalid schedule";
    decision.rule_id = 0;
    decision.rule_name = "";

    bool result = responder.write_event(context, decision, 1);
    EXPECT_TRUE(result);
}

// =============================================================================
// Test: send_response() - Integration test
// =============================================================================

TEST_F(DecisionResponseTest, SendResponse_AllowFlow) {
    // Database is open
    EXPECT_CALL(*mock_db, is_open())
        .WillRepeatedly(::testing::Return(true));

    // OSDP commands
    EXPECT_CALL(*mock_osdp, led(1, 1, true))
        .WillOnce(::testing::Return(true));
    EXPECT_CALL(*mock_osdp, buzzer(1, 100, 100, 1))
        .WillOnce(::testing::Return(true));
    EXPECT_CALL(*mock_osdp, display_text(1, "ACCESS GRANTED", 3))
        .WillOnce(::testing::Return(true));

    // Access log write - return a new fake statement each time
    EXPECT_CALL(*mock_db, prepare(::testing::_))
        .WillRepeatedly(::testing::Invoke([](const std::string&) {
            struct FakeStatement : public Statement {
                FakeStatement() : Statement(nullptr) {}
                MOCK_METHOD(int, step, (), (override));
            };
            auto* s = new FakeStatement();
            EXPECT_CALL(*s, step())
                .WillRepeatedly(::testing::Return(SQLITE_DONE));
            return std::unique_ptr<Statement>(s);
        }));

    DecisionResponse responder(mock_db, mock_osdp);

    EvaluationContext context;
    context.user.user_id = 123;
    context.door_id = "1";
    context.credential_type = "card";

    AccessDecision decision;
    decision.allow = true;
    decision.reason = "Valid credential";
    decision.rule_id = 10;

    bool result = responder.send_response(context, decision, 1);
    EXPECT_TRUE(result);
}

} // namespace accessd
