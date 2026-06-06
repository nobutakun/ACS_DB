-- ============================================
-- Module 9: Access Logs
-- Sprint 1: Enhanced with credential_type for reporting
-- Output: allow, deny
-- ============================================

CREATE TABLE IF NOT EXISTS access_logs (
    log_id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    user_id INTEGER NOT NULL,
    door_id INTEGER NOT NULL,
    reader_id INTEGER,
    credential_id INTEGER,
    credential_type TEXT NOT NULL CHECK(credential_type IN ('card', 'fingerprint', 'face', 'qr', 'pin')),
    result TEXT NOT NULL CHECK(result IN ('allow', 'deny')),
    reason TEXT,
    rule_id INTEGER,
    schedule_id INTEGER,
    attempted_at DATETIME,
    decision_latency_ms INTEGER,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE SET NULL,
    FOREIGN KEY (door_id) REFERENCES doors(door_id) ON DELETE SET NULL,
    FOREIGN KEY (reader_id) REFERENCES readers(reader_id) ON DELETE SET NULL,
    FOREIGN KEY (credential_id) REFERENCES credentials(credential_id) ON DELETE SET NULL,
    FOREIGN KEY (rule_id) REFERENCES access_rules(rule_id) ON DELETE SET NULL,
    FOREIGN KEY (schedule_id) REFERENCES schedules(schedule_id) ON DELETE SET NULL
);

CREATE INDEX idx_access_logs_timestamp ON access_logs(timestamp DESC);
CREATE INDEX idx_access_logs_user ON access_logs(user_id, timestamp DESC);
CREATE INDEX idx_access_logs_door ON access_logs(door_id, timestamp DESC);
CREATE INDEX idx_access_logs_result ON access_logs(result, timestamp DESC);
CREATE INDEX idx_access_logs_credential_type ON access_logs(credential_type, timestamp DESC);
