-- ============================================
-- NEW: Devices Table
-- Sprint 1: Device management for Orange Pi
-- ============================================

CREATE TABLE IF NOT EXISTS devices (
    device_id INTEGER PRIMARY KEY AUTOINCREMENT,
    device_uid TEXT NOT NULL UNIQUE,
    device_name TEXT NOT NULL,
    device_type TEXT NOT NULL CHECK(device_type IN ('face', 'card', 'fingerprint', 'controller', 'reader')),
    firmware_version TEXT,
    status TEXT NOT NULL CHECK(status IN ('online', 'offline', 'error', 'maintenance')),
    ip_address TEXT,
    port INTEGER,
    last_seen DATETIME,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_devices_uid ON devices(device_uid);
CREATE INDEX idx_devices_status ON devices(device_type, status);
CREATE INDEX idx_devices_last_seen ON devices(last_seen);
