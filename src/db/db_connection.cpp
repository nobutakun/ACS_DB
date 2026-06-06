#include "db_connection.hpp"
#include <sqlite3.h>
#include <stdexcept>
#include <iostream>

namespace accessd {

// Statement implementation
Statement::Statement(sqlite3_stmt* stmt) : stmt_(stmt) {}

Statement::~Statement() {
    if (stmt_) {
        sqlite3_finalize(stmt_);
    }
}

void Statement::bind_int(int index, int value) {
    if (stmt_) {
        sqlite3_bind_int(stmt_, index, value);
    }
}

void Statement::bind_int64(int index, int64_t value) {
    if (stmt_) {
        sqlite3_bind_int64(stmt_, index, value);
    }
}

void Statement::bind_text(int index, const std::string& value) {
    if (stmt_) {
        sqlite3_bind_text(stmt_, index, value.c_str(), value.length(),
                         SQLITE_TRANSIENT);
    }
}

void Statement::bind_null(int index) {
    if (stmt_) {
        sqlite3_bind_null(stmt_, index);
    }
}

int Statement::step() {
    if (stmt_) {
        return sqlite3_step(stmt_);
    }
    return SQLITE_ERROR;
}

int Statement::column_int(int index) {
    if (stmt_) {
        return sqlite3_column_int(stmt_, index);
    }
    return 0;
}

int64_t Statement::column_int64(int index) {
    if (stmt_) {
        return sqlite3_column_int64(stmt_, index);
    }
    return 0;
}

std::string Statement::column_text(int index) {
    if (stmt_) {
        const char* text = reinterpret_cast<const char*>(
            sqlite3_column_text(stmt_, index));
        return text ? text : "";
    }
    return "";
}

double Statement::column_double(int index) {
    if (stmt_) {
        return sqlite3_column_double(stmt_, index);
    }
    return 0.0;
}

bool Statement::column_is_null(int index) {
    if (stmt_) {
        return sqlite3_column_type(stmt_, index) == SQLITE_NULL;
    }
    return true;
}

void Statement::reset() {
    if (stmt_) {
        sqlite3_reset(stmt_);
    }
}

// DatabaseConnection implementation
DatabaseConnection::DatabaseConnection() : db_(nullptr) {}

DatabaseConnection::~DatabaseConnection() {
    close();
}

bool DatabaseConnection::open(const std::string& db_path) {
    db_path_ = db_path;

    int result = sqlite3_open_v2(
        db_path.c_str(),
        &db_,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
        nullptr
    );

    if (result != SQLITE_OK) {
        std::cerr << "Failed to open database: " << db_path << std::endl;
        std::cerr << "Error: " << sqlite3_errmsg(db_) << std::endl;
        db_ = nullptr;
        return false;
    }

    // Enable foreign keys
    execute("PRAGMA foreign_keys = ON");

    std::cout << "Database opened: " << db_path << std::endl;
    return true;
}

void DatabaseConnection::close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
        std::cout << "Database closed" << std::endl;
    }
}

bool DatabaseConnection::execute(const std::string& sql) {
    if (!db_) return false;

    char* err_msg = nullptr;
    int result = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err_msg);

    if (result != SQLITE_OK) {
        std::cerr << "SQL error: " << (err_msg ? err_msg : "unknown") << std::endl;
        std::cerr << "SQL: " << sql << std::endl;
        if (err_msg) {
            sqlite3_free(err_msg);
        }
        return false;
    }

    return true;
}

std::unique_ptr<Statement> DatabaseConnection::prepare(const std::string& sql) {
    if (!db_) return nullptr;

    sqlite3_stmt* stmt = nullptr;
    const char* remaining = nullptr;

    int result = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, &remaining);

    if (result != SQLITE_OK) {
        std::cerr << "Prepare error: " << sqlite3_errmsg(db_) << std::endl;
        std::cerr << "SQL: " << sql << std::endl;
        return nullptr;
    }

    return std::make_unique<Statement>(stmt);
}

bool DatabaseConnection::begin_transaction() {
    return execute("BEGIN TRANSACTION");
}

bool DatabaseConnection::commit() {
    return execute("COMMIT");
}

bool DatabaseConnection::rollback() {
    return execute("ROLLBACK");
}

std::string DatabaseConnection::last_error() const {
    if (db_) {
        return sqlite3_errmsg(db_);
    }
    return "No database connection";
}

int64_t DatabaseConnection::last_insert_rowid() const {
    if (db_) {
        return sqlite3_last_insert_rowid(db_);
    }
    return 0;
}

} // namespace accessd
