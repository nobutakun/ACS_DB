-- ============================================
-- Module 3: User Management
-- Sprint 1: Enhanced with employee_no and validity period
-- ============================================

CREATE TABLE IF NOT EXISTS users (
    user_id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    employee_no TEXT UNIQUE,
    status TEXT NOT NULL CHECK(status IN ('active', 'inactive', 'locked')),
    valid_from DATETIME DEFAULT CURRENT_TIMESTAMP,
    valid_until DATETIME,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_users_status ON users(status);
CREATE INDEX idx_users_name ON users(name);
CREATE INDEX idx_users_employee_no ON users(employee_no);
CREATE INDEX idx_users_valid_until ON users(valid_until);
