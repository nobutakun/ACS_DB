#include "credential_resolver.hpp"
#include "db_connection.hpp"
#include <sqlite3.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>

namespace accessd {

CredentialResolver::CredentialResolver(std::shared_ptr<DatabaseConnection> db)
    : db_(db)
{
}

CredentialResolver::~CredentialResolver() = default;

ResolutionResult CredentialResolver::resolve(
    const std::string& credential,
    const std::string& credential_type,
    UserContext& context)
{
    // Initialize context as invalid
    context = UserContext();
    context.credential_value = credential;
    context.credential_type = credential_type;

    // Step 1: Find credential in database
    if (!find_credential(credential, credential_type, context)) {
        context.reason = "Credential not found";
        return ResolutionResult::NOT_FOUND;
    }

    // Credential found - set found flag
    context.found = true;

    // Step 2: Check credential status (stored in user_status field temporarily)
    if (!check_credential_status(context.user_status)) {
        context.reason = "Credential invalid status: " + context.user_status;
        return ResolutionResult::INVALID_CREDENTIAL;
    }

    // Step 3: Find user
    if (!find_user(context.user_id, context)) {
        context.reason = "User not found for credential";
        return ResolutionResult::DATABASE_ERROR;
    }

    // Step 4: Check user status
    if (!check_user_status(context.user_status)) {
        context.reason = "User invalid status: " + context.user_status;
        return ResolutionResult::INVALID_USER;
    }

    // Step 5: Check validity period
    if (!is_valid_now(context)) {
        context.reason = "User expired (valid_until: " + context.valid_until + ")";
        return ResolutionResult::EXPIRED_USER;
    }

    // All checks passed
    context.valid = true;
    return ResolutionResult::SUCCESS;
}

bool CredentialResolver::validate_user(const UserContext& context) const {
    return context.valid &&
           check_user_status(context.user_status) &&
           is_valid_now(context);
}

bool CredentialResolver::is_valid_now(const UserContext& context) const {
    return check_validity_period(context.valid_from, context.valid_until);
}

bool CredentialResolver::find_credential(
    const std::string& credential_value,
    const std::string& credential_type,
    UserContext& context)
{
    if (!db_ || !db_->is_open()) {
        std::cerr << "Database not connected" << std::endl;
        return false;
    }

    std::string query =
        "SELECT credential_id, user_id, credential_type, credential_value, status "
        "FROM credentials "
        "WHERE credential_value = ? AND credential_type = ? AND status = 'active'";

    auto stmt = db_->prepare(query);
    if (!stmt) {
        std::cerr << "Failed to prepare query" << std::endl;
        return false;
    }

    stmt->bind_text(1, credential_value);
    stmt->bind_text(2, credential_type);

    if (stmt->step() == SQLITE_ROW) {
        context.credential_id = stmt->column_int(0);
        context.user_id = stmt->column_int(1);
        context.credential_type = stmt->column_text(2);
        context.credential_value = stmt->column_text(3);
        context.user_status = stmt->column_text(4);  // This is credential status
        return true;
    }

    return false;
}

bool CredentialResolver::find_user(int user_id, UserContext& context) {
    if (!db_ || !db_->is_open()) {
        return false;
    }

    std::string query =
        "SELECT user_id, name, employee_no, status, valid_from, valid_until "
        "FROM users WHERE user_id = ?";

    auto stmt = db_->prepare(query);
    if (!stmt) return false;

    stmt->bind_int(1, user_id);

    if (stmt->step() == SQLITE_ROW) {
        context.user_id = stmt->column_int(0);
        context.name = stmt->column_text(1);
        context.employee_no = stmt->column_text(2);
        context.user_status = stmt->column_text(3);  // This is user status
        context.valid_from = stmt->column_text(4);
        context.valid_until = stmt->column_text(5);
        return true;
    }

    return false;
}

bool CredentialResolver::check_credential_status(const std::string& status) const {
    return status == "active";
}

bool CredentialResolver::check_user_status(const std::string& status) const {
    return status == "active";
}

bool CredentialResolver::check_validity_period(
    const std::string& valid_from,
    const std::string& valid_until) const
{
    // If valid_until is NULL or empty, user has no expiration
    if (valid_until.empty() || valid_until == "NULL") {
        return true;
    }

    // Parse current time and valid_until
    auto now = std::chrono::system_clock::now();
    auto now_ts = std::chrono::system_clock::to_time_t(now);

    // Parse valid_until (format: YYYY-MM-DD HH:MM:SS)
    std::tm tm = {};
    std::istringstream ss(valid_until);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    if (ss.fail()) {
        // Try date only
        ss.clear();
        ss.str(valid_until);
        ss >> std::get_time(&tm, "%Y-%m-%d");
        if (ss.fail()) {
            std::cerr << "Failed to parse valid_until: " << valid_until << std::endl;
            return true;  // Default to valid if parse fails
        }
    }

    auto until_ts = std::mktime(&tm);

    // Check if current time is before valid_until
    return now_ts < until_ts;
}

} // namespace accessd
