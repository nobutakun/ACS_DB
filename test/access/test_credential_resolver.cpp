#include <gtest/gtest.h>
#include "credential_resolver.hpp"
#include "db_connection.hpp"
#include <sqlite3.h>

class CredentialResolverTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = "/tmp/test_credentials.db";
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
                    "employee_no TEXT UNIQUE,"
                    "status TEXT NOT NULL CHECK(status IN ('active', 'inactive', 'locked')),"
                    "valid_from DATETIME DEFAULT CURRENT_TIMESTAMP,"
                    "valid_until DATETIME)");

        db_->execute("CREATE TABLE credentials ("
                    "credential_id INTEGER PRIMARY KEY,"
                    "user_id INTEGER NOT NULL,"
                    "credential_type TEXT NOT NULL,"
                    "credential_value TEXT,"
                    "status TEXT NOT NULL CHECK(status IN ('active', 'inactive', 'lost', 'expired')),"
                    "FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE)");

        // Insert test users
        db_->execute("INSERT INTO users (name, employee_no, status, valid_until) VALUES "
                    "('John Doe', 'EMP001', 'active', '2026-12-31'),"
                    "('Jane Smith', 'EMP002', 'inactive', '2026-12-31'),"
                    "('Bob Johnson', 'EMP003', 'active', '2024-01-01')");  // Expired

        // Insert test credentials
        db_->execute("INSERT INTO credentials (user_id, credential_type, credential_value, status) VALUES "
                    "(1, 'card', '1234567890', 'active'),"      // John - active
                    "(2, 'card', '0987654321', 'active'),"      // Jane - inactive user
                    "(3, 'card', '5555555555', 'active'),"      // Bob - expired user
                    "(1, 'pin', '1234', 'active')");            // John - PIN
    }

    std::string db_path_;
    std::shared_ptr<accessd::DatabaseConnection> db_;
};

TEST_F(CredentialResolverTest, ResolveValidCredential) {
    accessd::CredentialResolver resolver(db_);
    accessd::UserContext context;

    auto result = resolver.resolve("1234567890", "card", context);

    EXPECT_EQ(result, accessd::ResolutionResult::SUCCESS);
    EXPECT_TRUE(context.found);
    EXPECT_TRUE(context.valid);
    EXPECT_EQ(context.user_id, 1);
    EXPECT_EQ(context.name, "John Doe");
    EXPECT_EQ(context.user_status, "active");
}

TEST_F(CredentialResolverTest, ResolveCredentialNotFound) {
    accessd::CredentialResolver resolver(db_);
    accessd::UserContext context;

    auto result = resolver.resolve("9999999999", "card", context);

    EXPECT_EQ(result, accessd::ResolutionResult::NOT_FOUND);
    EXPECT_FALSE(context.found);
    EXPECT_FALSE(context.valid);
}

TEST_F(CredentialResolverTest, ResolveInactiveUser) {
    accessd::CredentialResolver resolver(db_);
    accessd::UserContext context;

    auto result = resolver.resolve("0987654321", "card", context);

    EXPECT_EQ(result, accessd::ResolutionResult::INVALID_USER);
    EXPECT_TRUE(context.found);
    EXPECT_FALSE(context.valid);
    EXPECT_EQ(context.user_status, "inactive");
}

TEST_F(CredentialResolverTest, ResolveExpiredUser) {
    accessd::CredentialResolver resolver(db_);
    accessd::UserContext context;

    auto result = resolver.resolve("5555555555", "card", context);

    EXPECT_EQ(result, accessd::ResolutionResult::EXPIRED_USER);
    EXPECT_TRUE(context.found);
    EXPECT_FALSE(context.valid);
}

TEST_F(CredentialResolverTest, ResolvePIN) {
    accessd::CredentialResolver resolver(db_);
    accessd::UserContext context;

    auto result = resolver.resolve("1234", "pin", context);

    EXPECT_EQ(result, accessd::ResolutionResult::SUCCESS);
    EXPECT_EQ(context.user_id, 1);
    EXPECT_EQ(context.credential_type, "pin");
}

TEST_F(CredentialResolverTest, ValidateUser) {
    accessd::CredentialResolver resolver(db_);
    accessd::UserContext context;

    resolver.resolve("1234567890", "card", context);
    EXPECT_TRUE(resolver.validate_user(context));

    // Test invalid user
    accessd::UserContext invalid_context;
    resolver.resolve("0987654321", "card", invalid_context);
    EXPECT_FALSE(resolver.validate_user(invalid_context));
}
