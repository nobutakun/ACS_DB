#include <gtest/gtest.h>
#include "access_logger.hpp"
#include "db_connection.hpp"
#include <sqlite3.h>

class AccessLoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = "/tmp/test_access_logs.db";
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
        db_->execute("CREATE TABLE access_logs ("
                    "log_id INTEGER PRIMARY KEY,"
                    "timestamp INTEGER,"
                    "user_id INTEGER NOT NULL,"
                    "door_id INTEGER NOT NULL,"
                    "reader_id INTEGER,"
                    "credential_id INTEGER,"
                    "credential_type TEXT NOT NULL,"
                    "result TEXT NOT NULL,"
                    "reason TEXT,"
                    "rule_id INTEGER,"
                    "schedule_id INTEGER,"
                    "attempted_at INTEGER,"
                    "decision_latency_ms INTEGER)");
    }

    std::string db_path_;
    std::shared_ptr<accessd::DatabaseConnection> db_;
};

TEST_F(AccessLoggerTest, LogAllow) {
    accessd::AccessLogger logger(db_);

    bool result = logger.log_allow(1, 1, 1, 100, "card", 10, 2, 50);

    EXPECT_TRUE(result);

    auto stmt = db_->prepare("SELECT user_id, door_id, credential_type, result FROM access_logs WHERE log_id = 1");
    ASSERT_EQ(stmt->step(), SQLITE_ROW);

    EXPECT_EQ(stmt->column_int(0), 1);
    EXPECT_EQ(stmt->column_int(1), 1);
    EXPECT_EQ(stmt->column_text(2), "card");
    EXPECT_EQ(stmt->column_text(3), "allow");
}

TEST_F(AccessLoggerTest, LogDeny) {
    accessd::AccessLogger logger(db_);

    bool result = logger.log_deny(1, 1, 1, "card", "Invalid credential", 0, 0, 30);

    EXPECT_TRUE(result);

    auto stmt = db_->prepare("SELECT result, reason FROM access_logs WHERE log_id = 1");
    ASSERT_EQ(stmt->step(), SQLITE_ROW);

    EXPECT_EQ(stmt->column_text(0), "deny");
    EXPECT_EQ(stmt->column_text(1), "Invalid credential");
}

TEST_F(AccessLoggerTest, WriteCustomLog) {
    accessd::AccessLogger logger(db_);

    accessd::AccessLogEntry entry;
    entry.timestamp = 1700000000000LL;
    entry.user_id = 5;
    entry.door_id = 2;
    entry.reader_id = 3;
    entry.credential_id = 200;
    entry.credential_type = "fingerprint";
    entry.result = "allow";
    entry.reason = "Access granted";
    entry.rule_id = 15;
    entry.schedule_id = 3;
    entry.attempted_at = 1700000000000LL;
    entry.decision_latency_ms = 75;

    bool result = logger.write_log(entry);

    EXPECT_TRUE(result);

    auto stmt = db_->prepare("SELECT user_id, door_id, credential_type, decision_latency_ms FROM access_logs WHERE log_id = 1");
    ASSERT_EQ(stmt->step(), SQLITE_ROW);

    EXPECT_EQ(stmt->column_int(0), 5);
    EXPECT_EQ(stmt->column_int(1), 2);
    EXPECT_EQ(stmt->column_text(2), "fingerprint");
    EXPECT_EQ(stmt->column_int(3), 75);
}

TEST_F(AccessLoggerTest, GetLogsByUser) {
    accessd::AccessLogger logger(db_);

    // Log multiple entries for different users
    logger.log_allow(1, 1, 1, 100, "card", 10, 2, 50);
    logger.log_allow(2, 1, 1, 101, "card", 11, 2, 60);
    logger.log_allow(1, 2, 1, 100, "card", 12, 2, 55);

    auto logs = logger.get_logs(1, 100);

    // Should have 2 logs for user 1
    EXPECT_EQ(logs.size(), 2);

    // Should be ordered by timestamp DESC
    // (second entry should come first)
    EXPECT_EQ(logs[0].door_id, 2);
    EXPECT_EQ(logs[1].door_id, 1);
}

TEST_F(AccessLoggerTest, GetLogsByDoor) {
    accessd::AccessLogger logger(db_);

    logger.log_allow(1, 1, 1, 100, "card", 10, 2, 50);
    logger.log_allow(2, 1, 1, 101, "card", 11, 2, 60);
    logger.log_allow(3, 2, 1, 102, "card", 12, 2, 55);

    auto logs = logger.get_logs_by_door(1, 100);

    EXPECT_EQ(logs.size(), 2);
}

TEST_F(AccessLoggerTest, GetRecentLogs) {
    accessd::AccessLogger logger(db_);

    logger.log_allow(1, 1, 1, 100, "card", 10, 2, 50);
    logger.log_allow(2, 2, 1, 101, "card", 11, 2, 60);
    logger.log_allow(3, 1, 1, 102, "card", 12, 2, 55);

    auto logs = logger.get_recent_logs(10);

    EXPECT_EQ(logs.size(), 3);
}

TEST_F(AccessLoggerTest, Statistics) {
    accessd::AccessLogger logger(db_);

    // Add some logs for today
    logger.log_allow(1, 1, 1, 100, "card", 10, 2, 50);
    logger.log_allow(2, 1, 1, 101, "card", 11, 2, 60);
    logger.log_deny(3, 1, 1, "card", "Invalid", 0, 0, 30);

    int allow_count = logger.get_today_allow_count(1);
    int deny_count = logger.get_today_deny_count(1);

    EXPECT_EQ(allow_count, 2);
    EXPECT_EQ(deny_count, 1);
}
