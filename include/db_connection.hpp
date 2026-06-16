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
    virtual ~Statement();

    // Bind parameters
    virtual void bind_int(int index, int value);
    virtual void bind_int64(int index, int64_t value);
    virtual void bind_text(int index, const std::string& value);
    virtual void bind_null(int index);

    // Step execution
    virtual int step();

    // Get columns
    virtual int column_int(int index) const;
    virtual int64_t column_int64(int index) const;
    virtual std::string column_text(int index) const;
    virtual double column_double(int index) const;

    // Check if column is NULL
    virtual bool column_is_null(int index) const;

    // Reset
    virtual void reset();

    // Check if valid
    virtual bool is_valid() const { return stmt_ != nullptr; }

private:
    sqlite3_stmt* stmt_;
};

// Database connection
class DatabaseConnection {
public:
    DatabaseConnection();
    virtual ~DatabaseConnection();

    // Open/close database
    virtual bool open(const std::string& db_path);
    virtual void close();

    // Check if open
    virtual bool is_open() const { return db_ != nullptr; }

    // Execute SQL statement
    virtual bool execute(const std::string& sql);

    // Prepare statement
    virtual std::unique_ptr<Statement> prepare(const std::string& sql);

    // Begin transaction
    virtual bool begin_transaction();
    virtual bool commit();
    virtual bool rollback();

    // Get last error
    virtual std::string last_error() const;

    // Get last insert rowid
    virtual int64_t last_insert_rowid() const;

private:
    sqlite3* db_;
    std::string db_path_;
};

} // namespace accessd

#endif // ACCESSD_DB_CONNECTION_HPP
