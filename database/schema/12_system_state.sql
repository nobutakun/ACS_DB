-- ============================================
-- Module 1: System State
-- Sprint 1: Reduced scope - only DB version tracking
-- ============================================
    
CREATE TABLE IF NOT EXISTS system_state (
    state_id INTEGER PRIMARY KEY AUTOINCREMENT,
    state_key TEXT NOT NULL UNIQUE,
    state_value TEXT,
    last_updated DATETIME DEFAULT CURRENT_TIMESTAMP
);

INSERT INTO system_state (state_key, state_value) VALUES
('database.version', '1.0'),
('database.migration_version', '001'),
('database.last_migration', 'sprint1_initial');
