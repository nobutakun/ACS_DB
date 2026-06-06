-- ============================================
-- Module 6: Schedule Management
-- Sprint 1: JSON time_ranges format
-- ============================================

CREATE TABLE IF NOT EXISTS schedules (
    schedule_id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE,
    schedule_type TEXT NOT NULL CHECK(schedule_type IN ('always', 'office_hours', 'custom', 'holiday')),
    description TEXT,
    time_ranges TEXT,
    timezone TEXT DEFAULT 'Asia/Ho_Chi_Minh',
    status TEXT NOT NULL CHECK(status IN ('active', 'inactive')),
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_schedules_status ON schedules(schedule_type, status);

INSERT INTO schedules (name, schedule_type, description, time_ranges, status) VALUES
('Always', 'always', '24/7 Access', NULL, 'active'),
('Office Hours', 'office_hours', 'Mon-Fri 08:00-18:00',
 '[{"start":"08:00","end":"18:00","days":[1,2,3,4,5]}]', 'active');
