#ifndef ACCESSD_CREDENTIAL_RESOLVER_HPP
#define ACCESSD_CREDENTIAL_RESOLVER_HPP

#include <string>
#include <memory>

namespace accessd {

// Forward declarations
class DatabaseConnection;

// User context after credential resolution
struct UserContext {
    int user_id;
    std::string name;
    std::string employee_no;
    std::string user_status;        // active, inactive, locked

    // Credential info
    int credential_id;
    std::string credential_type;
    std::string credential_value;

    // Validity
    std::string valid_from;
    std::string valid_until;

    // Resolution status
    bool found;
    bool valid;
    std::string reason;  // Why invalid (expired, locked, etc.)

    UserContext() : user_id(0), credential_id(0), found(false), valid(false) {}
};

// Resolution result
enum class ResolutionResult {
    SUCCESS,           // Credential resolved to valid user
    NOT_FOUND,         // Credential not in database
    INVALID_CREDENTIAL, // Credential inactive/expired/lost
    INVALID_USER,      // User inactive/locked
    EXPIRED_USER,      // User past valid_until date
    DATABASE_ERROR
};

class CredentialResolver {
public:
    CredentialResolver(std::shared_ptr<DatabaseConnection> db);
    ~CredentialResolver();

    // Main resolve function
    ResolutionResult resolve(const std::string& credential,
                           const std::string& credential_type,
                           UserContext& context);

    // Validate user context
    bool validate_user(const UserContext& context) const;

    // Check if user is currently valid (within validity period)
    bool is_valid_now(const UserContext& context) const;

private:
    std::shared_ptr<DatabaseConnection> db_;

    // Database queries
    bool find_credential(const std::string& credential_value,
                        const std::string& credential_type,
                        UserContext& context);

    bool find_user(int user_id, UserContext& context);

    // Validation helpers
    bool check_credential_status(const std::string& status) const;
    bool check_user_status(const std::string& status) const;
    bool check_validity_period(const std::string& valid_from,
                              const std::string& valid_until) const;
};

} // namespace accessd

#endif // ACCESSD_CREDENTIAL_RESOLVER_HPP
