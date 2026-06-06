-- ============================================
-- Module 7: Access Rule Engine
-- Sprint 1: Enhanced with credential_type filtering
-- ============================================

CREATE TABLE IF NOT EXISTS access_rules (
    rule_id INTEGER PRIMARY KEY AUTOINCREMENT,
    rule_name TEXT NOT NULL,
    user_id INTEGER,
    door_id INTEGER,
    schedule_id INTEGER,
    credential_type TEXT CHECK(credential_type IN ('card', 'fingerprint', 'face', 'qr', 'pin', NULL)),
    allow BOOLEAN NOT NULL DEFAULT 1,
    priority INTEGER DEFAULT 0,
    valid_from DATETIME DEFAULT CURRENT_TIMESTAMP,
    valid_until DATETIME,
    status TEXT NOT NULL CHECK(status IN ('active', 'inactive')),
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,
    FOREIGN KEY (door_id) REFERENCES doors(door_id) ON DELETE CASCADE,
    FOREIGN KEY (schedule_id) REFERENCES schedules(schedule_id) ON DELETE SET NULL
);

CREATE INDEX idx_access_rules_user ON access_rules(user_id, status);
CREATE INDEX idx_access_rules_door ON access_rules(door_id, status);
CREATE INDEX idx_access_rules_priority ON access_rules(priority DESC, status);
CREATE INDEX idx_access_rules_credential_type ON access_rules(credential_type, status);
