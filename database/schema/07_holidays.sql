-- ============================================
-- NEW: Holidays Table
-- Sprint 1: Holiday schedule support
-- ============================================

CREATE TABLE IF NOT EXISTS holidays (
    holiday_id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    start_date DATE NOT NULL,
    end_date DATE NOT NULL,
    is_recurring BOOLEAN DEFAULT 0,
    month INTEGER,
    day INTEGER,
    description TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS schedule_holidays (
    schedule_holiday_id INTEGER PRIMARY KEY AUTOINCREMENT,
    schedule_id INTEGER NOT NULL,
    holiday_id INTEGER NOT NULL,
    holiday_action TEXT NOT NULL CHECK(holiday_action IN ('deny', 'allow')),
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (schedule_id) REFERENCES schedules(schedule_id) ON DELETE CASCADE,
    FOREIGN KEY (holiday_id) REFERENCES holidays(holiday_id) ON DELETE CASCADE,
    UNIQUE(schedule_id, holiday_id)
);

CREATE INDEX idx_holidays_date ON holidays(start_date, end_date);
CREATE INDEX idx_holidays_recurring ON holidays(is_recurring, month, day);
CREATE INDEX idx_schedule_holidays ON schedule_holidays(schedule_id);
