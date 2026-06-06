-- ============================================
-- SQLite Database Initialization (All-in-one)
-- Access Control System - Sprint 1
-- Run: sqlite3 accessd.db < init_db.sql
-- ============================================

-- Read all schema files in order
.read schema/01_users.sql
.read schema/02_credentials.sql
.read schema/03_devices.sql
.read schema/04_doors.sql
.read schema/05_readers.sql
.read schema/06_schedules.sql
.read schema/07_holidays.sql
.read schema/08_access_rules.sql
.read schema/09_events.sql
.read schema/10_access_logs.sql
.read schema/11_system_config.sql
.read schema/12_system_state.sql

-- Show created tables
.tables

-- Verify counts
SELECT '=== Initialization Complete ===' AS status;
SELECT 'Users:' || COUNT(*) FROM users;
SELECT 'Credentials:' || COUNT(*) FROM credentials;
SELECT 'Devices:' || COUNT(*) FROM devices;
SELECT 'Doors:' || COUNT(*) FROM doors;
SELECT 'Readers:' || COUNT(*) FROM readers;
SELECT 'Schedules:' || COUNT(*) FROM schedules;
SELECT 'Holidays:' || COUNT(*) FROM holidays;
SELECT 'Access Rules:' || COUNT(*) FROM access_rules;
SELECT 'System Config:' || COUNT(*) FROM system_config;
SELECT 'System State:' || COUNT(*) FROM system_state;
