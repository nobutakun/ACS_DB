-- ============================================
-- Module 5: Reader Management
-- Sprint 1: Enhanced with device_uid and updated types
-- ============================================

CREATE TABLE IF NOT EXISTS readers (
    reader_id INTEGER PRIMARY KEY AUTOINCREMENT,
    device_id INTEGER,
    door_id INTEGER NOT NULL,
    device_uid TEXT NOT NULL UNIQUE,
    name TEXT NOT NULL,
    reader_type TEXT NOT NULL CHECK(reader_type IN ('card', 'fingerprint', 'face', 'qr', 'multi')),
    direction TEXT NOT NULL CHECK(direction IN ('entry', 'exit', 'both')),

    -- Connection type: tcp (network) or osdp (RS-485)
    connection_type TEXT NOT NULL CHECK(connection_type IN ('tcp', 'osdp')) DEFAULT 'tcp',

    -- TCP/IP connection fields (for network readers)
    ip_address TEXT,
    port INTEGER,

    -- OSDP connection fields (for RS-485 readers)
    osdp_address INTEGER CHECK(osdp_address BETWEEN 0 AND 127),
    baudrate INTEGER DEFAULT 9600 CHECK(baudrate IN (9600, 19200, 38400, 57600, 115200)),
    use_encryption BOOLEAN DEFAULT 0,
    master_key BLOB,              -- SCBK: 16 bytes for AES-128
    rs485_port TEXT,              -- Serial port: /dev/ttyUSB0, /dev/ttyAMA0

    status TEXT NOT NULL CHECK(status IN ('online', 'offline', 'error')),
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (device_id) REFERENCES devices(device_id) ON DELETE SET NULL,
    FOREIGN KEY (door_id) REFERENCES doors(door_id) ON DELETE CASCADE
);

CREATE INDEX idx_readers_door ON readers(door_id);
CREATE INDEX idx_readers_device ON readers(device_id);
CREATE INDEX idx_readers_status ON readers(reader_type, status);

-- OSDP-specific indexes
CREATE INDEX idx_readers_osdp_address ON readers(osdp_address) WHERE osdp_address IS NOT NULL;
CREATE INDEX idx_readers_connection_type ON readers(connection_type, status);

-- ============================================
-- Sample Data
-- ============================================

-- TCP/IP Network Reader (traditional)
INSERT INTO readers (
    device_id, door_id, device_uid, name,
    reader_type, direction, connection_type,
    ip_address, port, status
) VALUES (
    1, 1, 'TCP-READER-001', 'Main Entry TCP Reader',
    'card', 'entry', 'tcp',
    '192.168.1.100', 8080, 'online'
);

-- OSDP Card Reader (RS-485)
INSERT INTO readers (
    device_id, door_id, device_uid, name,
    reader_type, direction, connection_type,
    osdp_address, baudrate, use_encryption, rs485_port, status
) VALUES (
    1, 1, 'OSDP-READER-001', 'Main Entry OSDP Reader',
    'card', 'entry', 'osdp',
    1, 9600, 1, '/dev/ttyUSB0', 'online'
);

-- OSDP Fingerprint Reader (RS-485)
INSERT INTO readers (
    device_id, door_id, device_uid, name,
    reader_type, direction, connection_type,
    osdp_address, baudrate, use_encryption, rs485_port, status
) VALUES (
    2, 2, 'OSDP-READER-002', 'Back Door Biometric Reader',
    'fingerprint', 'entry', 'osdp',
    2, 9600, 1, '/dev/ttyUSB0', 'online'
);

-- ============================================
-- Views
-- ============================================

-- View: All OSDP readers only
CREATE VIEW IF NOT EXISTS view_osdp_readers AS
SELECT
    reader_id, device_id, door_id, device_uid, name,
    reader_type, direction,
    osdp_address, baudrate, use_encryption, rs485_port,
    status, created_at, updated_at
FROM readers
WHERE connection_type = 'osdp';

-- View: All TCP/IP readers only
CREATE VIEW IF NOT EXISTS view_tcp_readers AS
SELECT
    reader_id, device_id, door_id, device_uid, name,
    reader_type, direction,
    ip_address, port,
    status, created_at, updated_at
FROM readers
WHERE connection_type = 'tcp';
