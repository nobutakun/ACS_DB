-- ============================================
-- Module 8: Events
-- Sprint 1: Changed to event_code for flexibility
-- ============================================

CREATE TABLE IF NOT EXISTS events (
    event_id INTEGER PRIMARY KEY AUTOINCREMENT,
    event_code TEXT NOT NULL,
    event_category TEXT CHECK(event_category IN ('access', 'door', 'reader', 'device', 'user', 'credential', 'rule', 'system', 'security')),
    severity TEXT CHECK(severity IN ('debug', 'info', 'warning', 'error', 'critical')),
    rule_id INTEGER,
    user_id INTEGER,
    door_id INTEGER,
    reader_id INTEGER,
    device_id INTEGER,
    schedule_id INTEGER,
    allow BOOLEAN,
    event_data TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (rule_id) REFERENCES access_rules(rule_id) ON DELETE SET NULL,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE SET NULL,
    FOREIGN KEY (door_id) REFERENCES doors(door_id) ON DELETE SET NULL,
    FOREIGN KEY (reader_id) REFERENCES readers(reader_id) ON DELETE SET NULL,
    FOREIGN KEY (device_id) REFERENCES devices(device_id) ON DELETE SET NULL,
    FOREIGN KEY (schedule_id) REFERENCES schedules(schedule_id) ON DELETE SET NULL
);

CREATE INDEX idx_events_code ON events(event_code);
CREATE INDEX idx_events_category ON events(event_category);
CREATE INDEX idx_events_severity ON events(severity, created_at DESC);
CREATE INDEX idx_events_user ON events(user_id);
CREATE INDEX idx_events_door ON events(door_id);
CREATE INDEX idx_events_device ON events(device_id);
CREATE INDEX idx_events_created ON events(created_at DESC);
