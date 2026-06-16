#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <sqlite3.h>
#include "door_controller.hpp"
#include "db_connection.hpp"
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

class DoorControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        mock_db = std::make_shared<MockDatabaseConnection>();
        controller = std::make_unique<DoorController>(mock_db);

        // Setup default config for testing
        auto& config = Config::instance();
        config.set_hardware_defaults(2000, 30, 60);
    }

    void TearDown() override {
        controller.reset();
    }

    std::shared_ptr<MockDatabaseConnection> mock_db;
    std::unique_ptr<DoorController> controller;
};

// =============================================================================
// Test: unlock_door()
// =============================================================================

TEST_F(DoorControllerTest, UnlockDoor_DatabaseNotConnected) {
    EXPECT_CALL(*mock_db, is_open())
        .WillOnce(::testing::Return(false));

    bool result = controller->unlock_door(1, 2000, "Access granted");
    EXPECT_FALSE(result);
}

TEST_F(DoorControllerTest, UnlockDoor_Success) {
    auto* mock_stmt = new MockStatement();
    auto* mock_update_stmt = new MockStatement();

    EXPECT_CALL(*mock_db, is_open())
        .WillOnce(::testing::Return(true));

    EXPECT_CALL(*mock_stmt, bind_int(1, 1))
        .Times(1);
    EXPECT_CALL(*mock_stmt, step())
        .WillOnce(::testing::Return(SQLITE_ROW));
    EXPECT_CALL(*mock_stmt, column_int(1))
        .WillOnce(::testing::Return(3000));  // relay_time_ms from DB

    EXPECT_CALL(*mock_db, prepare(::testing::StartsWith("SELECT")))
        .WillOnce(::testing::Return(
            ::testing::ByMove(std::unique_ptr<Statement>(mock_stmt))));

    // Update door status
    EXPECT_CALL(*mock_update_stmt, bind_int(1, 1))
        .Times(1);
    EXPECT_CALL(*mock_update_stmt, step())
        .WillOnce(::testing::Return(SQLITE_DONE));

    EXPECT_CALL(*mock_db, prepare(::testing::StartsWith("UPDATE")))
        .WillOnce(::testing::Return(
            ::testing::ByMove(std::unique_ptr<Statement>(mock_update_stmt))));

    bool result = controller->unlock_door(1, 2000, "Access granted");
    EXPECT_TRUE(result);
}

TEST_F(DoorControllerTest, UnlockDoor_DoorNotFound) {
    auto* mock_stmt = new MockStatement();

    EXPECT_CALL(*mock_db, is_open())
        .WillOnce(::testing::Return(true));

    EXPECT_CALL(*mock_stmt, bind_int(1, 999))
        .Times(1);
    EXPECT_CALL(*mock_stmt, step())
        .WillOnce(::testing::Return(SQLITE_DONE));  // No row found

    EXPECT_CALL(*mock_db, prepare(::testing::_))
        .WillOnce(::testing::Return(
            ::testing::ByMove(std::unique_ptr<Statement>(mock_stmt))));

    bool result = controller->unlock_door(999, 2000, "Test");
    // Should still succeed with default relay time
    EXPECT_TRUE(result);
}

TEST_F(DoorControllerTest, UnlockDoor_UseDefaultRelayTime) {
    auto* mock_stmt = new MockStatement();
    auto* mock_update_stmt = new MockStatement();

    EXPECT_CALL(*mock_db, is_open())
        .WillOnce(::testing::Return(true));

    EXPECT_CALL(*mock_stmt, step())
        .WillOnce(::testing::Return(SQLITE_ROW));
    EXPECT_CALL(*mock_stmt, column_int(1))
        .WillOnce(::testing::Return(3000));

    EXPECT_CALL(*mock_db, prepare(::testing::_))
        .WillOnce(::testing::Return(
            ::testing::ByMove(std::unique_ptr<Statement>(mock_stmt))));

    EXPECT_CALL(*mock_update_stmt, step())
        .WillOnce(::testing::Return(SQLITE_DONE));

    EXPECT_CALL(*mock_db, prepare(::testing::StartsWith("UPDATE")))
        .WillOnce(::testing::Return(
            ::testing::ByMove(std::unique_ptr<Statement>(mock_update_stmt))));

    // Pass 0 to use database default
    bool result = controller->unlock_door(1, 0, "Test");
    EXPECT_TRUE(result);
}

// =============================================================================
// Test: lock_door()
// =============================================================================

TEST_F(DoorControllerTest, LockDoor_DatabaseNotConnected) {
    EXPECT_CALL(*mock_db, is_open())
        .WillOnce(::testing::Return(false));

    bool result = controller->lock_door(1, "Manual lock");
    EXPECT_FALSE(result);
}

TEST_F(DoorControllerTest, LockDoor_Success) {
    EXPECT_CALL(*mock_db, is_open())
        .WillOnce(::testing::Return(true));

    auto* mock_stmt = new MockStatement();
    EXPECT_CALL(*mock_stmt, bind_int(1, 1))
        .Times(1);
    EXPECT_CALL(*mock_stmt, step())
        .WillOnce(::testing::Return(SQLITE_DONE));

    EXPECT_CALL(*mock_db, prepare(::testing::_))
        .WillOnce(::testing::Return(
            ::testing::ByMove(std::unique_ptr<Statement>(mock_stmt))));

    bool result = controller->lock_door(1, "Manual lock");
    EXPECT_TRUE(result);
}

// =============================================================================
// Test: get_door_state()
// =============================================================================

TEST_F(DoorControllerTest, GetDoorState_DefaultClosed) {
    // Default implementation returns CLOSED
    DoorState state = controller->get_door_state(1);
    EXPECT_EQ(state, DoorState::CLOSED);
}

// =============================================================================
// Test: check_door_timeouts()
// =============================================================================

TEST_F(DoorControllerTest, CheckDoorTimeouts_NoDatabase) {
    EXPECT_CALL(*mock_db, is_open())
        .WillOnce(::testing::Return(false));

    bool result = controller->check_door_timeouts();
    EXPECT_FALSE(result);
}

TEST_F(DoorControllerTest, CheckDoorTimeouts_NoHeldDoors) {
    auto* mock_stmt = new MockStatement();

    EXPECT_CALL(*mock_db, is_open())
        .WillOnce(::testing::Return(true));

    EXPECT_CALL(*mock_stmt, step())
        .WillOnce(::testing::Return(SQLITE_DONE));  // No rows

    EXPECT_CALL(*mock_db, prepare(::testing::_))
        .WillOnce(::testing::Return(
            ::testing::ByMove(std::unique_ptr<Statement>(mock_stmt))));

    bool result = controller->check_door_timeouts();
    EXPECT_TRUE(result);
}

TEST_F(DoorControllerTest, CheckDoorTimeouts_HasHeldDoors) {
    auto* mock_stmt = new MockStatement();

    EXPECT_CALL(*mock_db, is_open())
        .WillOnce(::testing::Return(true));

    EXPECT_CALL(*mock_stmt, step())
        .WillOnce(::testing::Return(SQLITE_ROW))
        .WillRepeatedly(::testing::Return(SQLITE_DONE));

    EXPECT_CALL(*mock_stmt, column_int(0))
        .WillOnce(::testing::Return(1));  // door_id
    EXPECT_CALL(*mock_stmt, column_text(1))
        .WillOnce(::testing::Return("Main Door"));
    EXPECT_CALL(*mock_stmt, column_int(2))
        .WillOnce(::testing::Return(30));  // timeout_sec

    EXPECT_CALL(*mock_db, prepare(::testing::_))
        .WillOnce(::testing::Return(
            ::testing::ByMove(std::unique_ptr<Statement>(mock_stmt))));

    bool result = controller->check_door_timeouts();
    EXPECT_TRUE(result);
}

// =============================================================================
// Test: trigger_relay()
// =============================================================================

TEST_F(DoorControllerTest, TriggerRelay_ValidDoorId) {
    // Door ID 1 maps to GPIO pin 0
    bool result = controller->trigger_relay(1, 2000);
    EXPECT_TRUE(result);
}

TEST_F(DoorControllerTest, TriggerRelay_InvalidDoorId) {
    // Invalid door ID returns -1 for GPIO pin
    bool result = controller->trigger_relay(999, 2000);
    EXPECT_FALSE(result);
}

// =============================================================================
// Test: get_door_gpio_pin()
// =============================================================================

TEST_F(DoorControllerTest, GetDoorGpioPin_ValidDoors) {
    // Test door_id to GPIO pin mapping
    EXPECT_EQ(controller->get_door_gpio_pin(1), 0);  // PA0
    EXPECT_EQ(controller->get_door_gpio_pin(2), 1);  // PA1
    EXPECT_EQ(controller->get_door_gpio_pin(3), 2);  // PA2
    EXPECT_EQ(controller->get_door_gpio_pin(4), 3);  // PA3
}

TEST_F(DoorControllerTest, GetDoorGpioPin_InvalidDoor) {
    EXPECT_EQ(controller->get_door_gpio_pin(999), -1);
    EXPECT_EQ(controller->get_door_gpio_pin(0), -1);
    EXPECT_EQ(controller->get_door_gpio_pin(-1), -1);
}

// =============================================================================
// Test: Config-based timeouts
// =============================================================================

TEST_F(DoorControllerTest, GetRelayTime_FromConfig) {
    // This tests that the controller reads from config
    // Actual value depends on config file
    int relay_time = controller->get_relay_time_ms(1);
    EXPECT_GT(relay_time, 0);  // Should be positive
}

TEST_F(DoorControllerTest, GetOpenTimeout_FromConfig) {
    int timeout = controller->get_open_timeout_sec(1);
    EXPECT_GT(timeout, 0);  // Should be positive
}

TEST_F(DoorControllerTest, GetHeldTimeout_FromConfig) {
    int timeout = controller->get_held_timeout_sec(1);
    EXPECT_GT(timeout, 0);  // Should be positive
}

// =============================================================================
// Test: DoorState enum values
// =============================================================================

TEST_F(DoorControllerTest, DoorStateValues) {
    // Just verify enum values are as expected
    EXPECT_EQ(static_cast<int>(DoorState::CLOSED), 0);
    EXPECT_EQ(static_cast<int>(DoorState::OPEN), 1);
    EXPECT_EQ(static_cast<int>(DoorState::HELD_OPEN), 2);
    EXPECT_EQ(static_cast<int>(DoorState::FORCED_OPEN), 3);
    EXPECT_EQ(static_cast<int>(DoorState::ERROR), 4);
}

} // namespace accessd
