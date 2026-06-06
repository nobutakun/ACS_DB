-- ============================================
-- Module 4: Credential Management
-- Sprint 1: Support Card, Face, Fingerprint with BLOB data
-- ============================================

CREATE TABLE IF NOT EXISTS credentials (
    credential_id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    credential_type TEXT NOT NULL CHECK(credential_type IN ('card', 'fingerprint', 'face', 'qr', 'pin')),
    credential_data BLOB,
    credential_value TEXT,
    issuer TEXT,
    status TEXT NOT NULL CHECK(status IN ('active', 'inactive', 'lost', 'expired')),
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    expires_at DATETIME,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,
    UNIQUE(credential_type, credential_value)
);

CREATE INDEX idx_credentials_user ON credentials(user_id);
CREATE INDEX idx_credentials_status ON credentials(credential_type, status);
