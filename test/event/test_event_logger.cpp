#include <gtest/gtest.h>
#include "event_logger.hpp"
#include "db_connection.hpp"
#include <sqlite3.h>

class EventLoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = "/tmp/test_events.db";
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
        db_->execute("CREATE TABLE events ("
                    "event_id INTEGER PRIMARY KEY,"
                    "event_code TEXT NOT NULL,"
                    "event_category TEXT,"
                    "severity TEXT,"
                    "rule_id INTEGER,"
                    "user_id INTEGER,"
                    "door_id INTEGER,"
                    "reader_id INTEGER,"
                    "device_id INTEGER,"
                    "schedule_id INTEGER,"
                    "allow BOOLEAN,"
                    "event_data TEXT)");
    }

    std::string db_path_;
    std::shared_ptr<accessd::DatabaseConnection> db_;
};

TEST_F(EventLoggerTest, LogAccessGranted) {
    accessd::EventLogger logger(db_);

    bool result = logger.log_access_granted(1, 1, 1, 10, "card");

    EXPECT_TRUE(result);

    // Verify event was written
    auto stmt = db_->prepare("SELECT event_code, event_category, severity FROM events WHERE event_id = 1");
    ASSERT_EQ(stmt->step(), SQLITE_ROW);

    EXPECT_EQ(stmt->column_text(0), "access_granted");
    EXPECT_EQ(stmt->column_text(1), "access");
    EXPECT_EQ(stmt->column_text(2), "info");
}

TEST_F(EventLoggerTest, LogAccessDenied) {
    accessd::EventLogger logger(db_);

    bool result = logger.log_access_denied(1, 1, 1, "Invalid credential", 0, "card");

    EXPECT_TRUE(result);

    auto stmt = db_->prepare("SELECT event_code, severity FROM events WHERE event_id = 1");
    ASSERT_EQ(stmt->step(), SQLITE_ROW);

    EXPECT_EQ(stmt->column_text(0), "access_denied");
    EXPECT_EQ(stmt->column_text(1), "warning");
}

TEST_F(EventLoggerTest, LogDoorEvents) {
    accessd::EventLogger logger(db_);

    EXPECT_TRUE(logger.log_door_open(1, 1));
    EXPECT_TRUE(logger.log_door_closed(1));
    EXPECT_TRUE(logger.log_door_forced(1));
    EXPECT_TRUE(logger.log_door_held(1));

    auto stmt = db_->prepare("SELECT COUNT(*) FROM events");
    stmt->step();
    EXPECT_EQ(stmt->column_int(0), 4);
}

TEST_F(EventLoggerTest, LogReaderEvents) {
    accessd::EventLogger logger(db_);

    EXPECT_TRUE(logger.log_reader_online(1, "CARD001"));
    EXPECT_TRUE(logger.log_reader_offline(1, "CARD001"));
    EXPECT_TRUE(logger.log_reader_error(1, "Communication failed"));

    auto stmt = db_->prepare("SELECT event_code FROM events ORDER BY event_id");
    ASSERT_EQ(stmt->step(), SQLITE_ROW);
    EXPECT_EQ(stmt->column_text(0), "reader_online");
    ASSERT_EQ(stmt->step(), SQLITE_ROW);
    EXPECT_EQ(stmt->column_text(0), "reader_offline");
    ASSERT_EQ(stmt->step(), SQLITE_ROW);
    EXPECT_EQ(stmt->column_text(0), "reader_error");
}

TEST_F(EventLoggerTest, LogSystemEvents) {
    accessd::EventLogger logger(db_);

    EXPECT_TRUE(logger.log_system_start());
    EXPECT_TRUE(logger.log_system_stop());
    EXPECT_TRUE(logger.log_system_error("Test error"));

    auto stmt = db_->prepare("SELECT event_code FROM events ORDER BY event_id");
    ASSERT_EQ(stmt->step(), SQLITE_ROW);
    EXPECT_EQ(stmt->column_text(0), "system_start");
    ASSERT_EQ(stmt->step(), SQLITE_ROW);
    EXPECT_EQ(stmt->column_text(0), "system_stop");
    ASSERT_EQ(stmt->step(), SQLITE_ROW);
    EXPECT_EQ(stmt->column_text(0), "system_error");
}

TEST_F(EventLoggerTest, CustomEvent) {
    accessd::EventLogger logger(db_);

    accessd::Event event;
    event.event_code = "custom_event";
    event.event_category = "test";
    event.severity = "debug";
    event.event_data = "{\"test\":\"data\"}";

    bool result = logger.write_event(event);

    EXPECT_TRUE(result);

    auto stmt = db_->prepare("SELECT event_code, event_category, severity, event_data FROM events WHERE event_id = 1");
    ASSERT_EQ(stmt->step(), SQLITE_ROW);

    EXPECT_EQ(stmt->column_text(0), "custom_event");
    EXPECT_EQ(stmt->column_text(1), "test");
    EXPECT_EQ(stmt->column_text(2), "debug");
    EXPECT_EQ(stmt->column_text(3), "{\"test\":\"data\"}");
}
