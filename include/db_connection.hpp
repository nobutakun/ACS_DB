#ifndef ACCESSD_DB_CONNECTION_HPP
#define ACCESSD_DB_CONNECTION_HPP

#include <string>
#include <memory>
#include <vector>

// Forward declaration for sqlite3
struct sqlite3;
struct sqlite3_stmt;

namespace accessd {

// SQLite statement wrapper
class Statement {
public:
    Statement(sqlite3_stmt* stmt);
    ~Statement();

    // Bind parameters
    void bind_int(int index, int value);
    void bind_int64(int index, int64_t value);
    void bind_text(int index, const std::string& value);
    void bind_null(int index);

    // Step execution
    int step();

    // Get columns
    int column_int(int index);
    int64_t column_int64(int index);
    std::string column_text(int index);
    double column_double(int index);

    // Check if column is NULL
    bool column_is_null(int index);

    // Reset
    void reset();

    // Check if valid
    bool is_valid() const { return stmt_ != nullptr; }

private:
    sqlite3_stmt* stmt_;
};

// Database connection
class DatabaseConnection {
public:
    DatabaseConnection();
    ~DatabaseConnection();

    // Open/close database
    bool open(const std::string& db_path);
    void close();

    // Check if open
    bool is_open() const { return db_ != nullptr; }

    // Execute SQL statement
    bool execute(const std::string& sql);

    // Prepare statement
    std::unique_ptr<Statement> prepare(const std::string& sql);

    // Begin transaction
    bool begin_transaction();
    bool commit();
    bool rollback();

    // Get last error
    std::string last_error() const;

    // Get last insert rowid
    int64_t last_insert_rowid() const;

private:
    sqlite3* db_;
    std::string db_path_;
};

} // namespace accessd

#endif // ACCESSD_DB_CONNECTION_HPP
