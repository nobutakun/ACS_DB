-- ============================================
-- Module 1: System Configuration
-- Sprint 1: Access Control System config
-- ============================================

CREATE TABLE IF NOT EXISTS system_config (
    config_id INTEGER PRIMARY KEY AUTOINCREMENT,
    config_key TEXT NOT NULL UNIQUE,
    config_value TEXT,
    description TEXT,
    config_type TEXT NOT NULL CHECK(config_type IN ('string', 'integer', 'boolean', 'json')),
    is_editable BOOLEAN DEFAULT 1,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

INSERT INTO system_config (config_key, config_value, description, config_type) VALUES
-- Access Decision Engine
('accessd.default_policy', 'deny', 'Default access policy when no rules match', 'string'),
('accessd.decision_timeout', '5000', 'Decision timeout in milliseconds', 'integer'),
('accessd.enable_cache', 'true', 'Enable rule caching for performance', 'boolean'),
('accessd.cache_ttl', '300', 'Cache TTL in seconds', 'integer'),
-- Log Management
('accessd.log_retention_days', '90', 'Number of days to retain access logs', 'integer'),
('accessd.event_retention_days', '30', 'Number of days to retain events', 'integer'),
-- System
('system.timezone', 'Asia/Ho_Chi_Minh', 'System timezone', 'string'),
('system.version', '1.0.0', 'System version', 'string'),
-- Hardware
('hardware.relay_default_ms', '3000', 'Default relay activation time', 'integer'),
('hardware.door_timeout_default_sec', '30', 'Default door open timeout', 'integer'),
('hardware.door_held_timeout_sec', '60', 'Door held open detection timeout', 'integer'),
-- Security
('security.auto_expire_users', 'true', 'Auto-deny users past valid_until', 'boolean'),
('security.enable_holiday_check', 'true', 'Check holidays in access decision', 'boolean');
