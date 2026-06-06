#include <gtest/gtest.h>
#include "db_connection.hpp"
#include <sqlite3.h>
#include <fstream>

class DBConnectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test database
        db_path_ = "/tmp/test_accessd.db";
        // Remove if exists
        std::remove(db_path_.c_str());

        db_ = std::make_shared<accessd::DatabaseConnection>();
        ASSERT_TRUE(db_->open(db_path_));

        // Create test tables
        setup_test_tables();
    }

    void TearDown() override {
        db_->close();
        std::remove(db_path_.c_str());
    }

    void setup_test_tables() {
        db_->execute("CREATE TABLE IF NOT EXISTS test_users ("
                    "user_id INTEGER PRIMARY KEY,"
                    "name TEXT NOT NULL,"
                    "status TEXT NOT NULL)");
    }

    std::string db_path_;
    std::shared_ptr<accessd::DatabaseConnection> db_;
};

TEST_F(DBConnectionTest, OpenDatabase) {
    auto db2 = std::make_shared<accessd::DatabaseConnection>();
    EXPECT_TRUE(db2->open(db_path_));
    EXPECT_TRUE(db2->is_open());
    db2->close();
}

TEST_F(DBConnectionTest, ExecuteSQL) {
    EXPECT_TRUE(db_->execute("INSERT INTO test_users (name, status) VALUES ('Test', 'active')"));

    auto stmt = db_->prepare("SELECT COUNT(*) FROM test_users");
    ASSERT_NE(stmt, nullptr);
    ASSERT_EQ(stmt->step(), SQLITE_ROW);
    EXPECT_EQ(stmt->column_int(0), 1);
}

TEST_F(DBConnectionTest, PrepareStatement) {
    auto stmt = db_->prepare("INSERT INTO test_users (name, status) VALUES (?, ?)");
    ASSERT_NE(stmt, nullptr);

    stmt->bind_text(1, "User1");
    stmt->bind_text(2, "active");
    EXPECT_EQ(stmt->step(), SQLITE_DONE);
}

TEST_F(DBConnectionTest, Transaction) {
    EXPECT_TRUE(db_->begin_transaction());

    db_->execute("INSERT INTO test_users (name, status) VALUES ('User1', 'active')");
    db_->execute("INSERT INTO test_users (name, status) VALUES ('User2', 'active')");

    EXPECT_TRUE(db_->commit());

    auto stmt = db_->prepare("SELECT COUNT(*) FROM test_users");
    stmt->step();
    EXPECT_EQ(stmt->column_int(0), 2);
}

TEST_F(DBConnectionTest, TransactionRollback) {
    EXPECT_TRUE(db_->begin_transaction());

    db_->execute("INSERT INTO test_users (name, status) VALUES ('User1', 'active')");

    EXPECT_TRUE(db_->rollback());

    auto stmt = db_->prepare("SELECT COUNT(*) FROM test_users");
    stmt->step();
    EXPECT_EQ(stmt->column_int(0), 0);
}

TEST_F(DBConnectionTest, LastInsertRowId) {
    db_->execute("INSERT INTO test_users (name, status) VALUES ('User1', 'active')");
    int64_t rowid = db_->last_insert_rowid();
    EXPECT_GT(rowid, 0);
}

TEST_F(DBConnectionTest, ColumnTypes) {
    auto stmt = db_->prepare("INSERT INTO test_users (name, status) VALUES (?, ?)");
    stmt->bind_text(1, "TestUser");
    stmt->bind_text(2, "active");
    stmt->step();

    auto select_stmt = db_->prepare("SELECT name, status FROM test_users WHERE user_id = ?");
    select_stmt->bind_int(1, 1);
    ASSERT_EQ(select_stmt->step(), SQLITE_ROW);

    EXPECT_EQ(select_stmt->column_text(0), "TestUser");
    EXPECT_EQ(select_stmt->column_text(1), "active");
}

TEST_F(DBConnectionTest, NullHandling) {
    // Add nullable column
    db_->execute("ALTER TABLE test_users ADD COLUMN email TEXT");

    auto stmt = db_->prepare("INSERT INTO test_users (name, status, email) VALUES (?, ?, ?)");
    stmt->bind_text(1, "NoEmailUser");
    stmt->bind_text(2, "active");
    stmt->bind_null(3);  // Null email
    stmt->step();

    auto select_stmt = db_->prepare("SELECT email FROM test_users WHERE name = ?");
    select_stmt->bind_text(1, "NoEmailUser");
    ASSERT_EQ(select_stmt->step(), SQLITE_ROW);
    EXPECT_TRUE(select_stmt->column_is_null(0));
}
