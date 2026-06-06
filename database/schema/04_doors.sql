-- ============================================
-- Module 5: Door Management
-- Sprint 1: Enhanced with relay and timeout settings
-- ============================================

CREATE TABLE IF NOT EXISTS doors (
    door_id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE,
    location TEXT,
    relay_time_ms INTEGER DEFAULT 3000,
    open_timeout_sec INTEGER DEFAULT 30,
    door_held_timeout_sec INTEGER DEFAULT 60,
    status TEXT NOT NULL CHECK(status IN ('online', 'offline', 'maintenance', 'held_open', 'forced_open')),
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_doors_status ON doors(status);
