# Accessd Service - Resolve Credential Diagram

## Block Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         Access Request (Input)                              │
└─────────────────────────────────────────────────────────────────────────────┘
                                     │
                                     ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                         CredentialResolver                                   │
│                                                                              │
│  resolve(credential, credential_type) → UserContext                          │
└─────────────────────────────────────────────────────────────────────────────┘
                                     │
                ┌────────────────────┼────────────────────┐
                ▼                    ▼                    ▼
    ┌───────────────────┐  ┌───────────────────┐  ┌───────────────────┐
    │   Step 1          │  │   Step 2          │  │   Step 3          │
    │ Find Credential   │  │ Find User         │  │ Validate User     │
    └───────────────────┘  └───────────────────┘  └───────────────────┘
                │                    │                    │
                ▼                    ▼                    ▼
    ┌───────────────────┐  ┌───────────────────┐  ┌───────────────────┐
    │  Database Query   │  │  Database Query   │  │  Check Conditions │
    │  SELECT * FROM    │  │  SELECT * FROM    │  │                  │
    │  credentials      │  │  users            │  │  - user_status    │
    │  WHERE           │  │  WHERE user_id =  │  │  - valid_from     │
    │  credential_value │  └───────────────────┘  │  - valid_until    │
    │  = ? AND          │                         │  - is_expired?    │
    │  credential_type  │                         └───────────────────┘
    │  = ? AND status   │                                    │
    │  = 'active'       │                                    ▼
    └───────────────────┘                         ┌───────────────────┐
                                                   │ ResolutionResult │
                                                   │ - SUCCESS         │
                                                   │ - NOT_FOUND      │
                                                   │ - INVALID_...    │
                                                   │ - EXPIRED_USER   │
                                                   └───────────────────┘
```

## Database Queries

### Step 1: Find Credential
```sql
SELECT credential_id, user_id, credential_type,
       credential_value, status
FROM credentials
WHERE credential_value = ?           -- Input: "1234567890"
  AND credential_type = ?             -- Input: "card"
  AND status = 'active';
```

### Step 2: Find User
```sql
SELECT user_id, name, employee_no,
       status, valid_from, valid_until
FROM users
WHERE user_id = ?;                    -- From credential query
```

## Resolution Decision Tree

```
┌─────────────────┐
│ Input:          │
│ credential      │
│ credential_type │
└────────┬────────┘
         │
         ▼
┌─────────────────────────┐
│ Credential exists?      │
└────────┬────────────────┘
         │
    ┌────┴────┐
    │         │
   NO        YES
    │         │
    ▼         ▼
┌─────────┐ ┌──────────────────┐
│NOT_FOUND│ │Credential Active?│
└─────────┘ └───┬──────────────┘
               │
          ┌────┴────┐
          │         │
         NO        YES
          │         │
          ▼         ▼
    ┌──────────┐ ┌──────────────────┐
    │INVALID   │ │   User exists?   │
    │CREDENTIAL│ └───┬──────────────┘
    └──────────┘     │
                   ┌┴┐
                  NO YES
                   │  │
                   ▼  ▼
            ┌─────────┐ ┌──────────────────┐
            │DB_ERROR │ │User Active?      │
            └─────────┘ └───┬──────────────┘
                            │
                       ┌────┴────┐
                       │         │
                      NO        YES
                       │         │
                       ▼         ▼
                 ┌──────────┐ ┌──────────────────┐
                 │INVALID   │ │Within Valid      │
                 │USER      │ │Period?           │
                 └──────────┘ └───┬──────────────┘
                                     │
                                ┌────┴────┐
                                │         │
                               NO        YES
                                │         │
                                ▼         ▼
                          ┌──────────┐ ┌─────────┐
                          │EXPIRED   │ │ SUCCESS │
                          │USER      │ └─────────┘
                          └──────────┘     │
                                            ▼
                                     ┌──────────────┐
                                     │ UserContext  │
                                     │ valid=true   │
                                     └──────────────┘
```

## Data Flow

```
┌─────────────┐
│ OSDP Reader│
│  Card Read │
└──────┬──────┘
       │
       ▼
┌─────────────────┐
│ AccessRequest   │
│ credential:     │
│   "1234567890"  │
│ type: "card"    │
└────────┬────────┘
         │
         ▼
┌─────────────────────────────────────────┐
│           Database                      │
│  ┌─────────────────────────────────┐   │
│  │ credentials table               │   │
│  │ ┌─────────────────────────┐     │   │
│  │ │ credential_id: 100      │     │   │
│  │ │ user_id: 5               │     │   │
│  │ │ credential_type: card   │     │   │
│  │ │ credential_value: 123... │     │   │
│  │ │ status: active           │     │   │
│  │ └─────────────────────────┘     │   │
│  └─────────────────────────────────┘   │
│             │                           │
│             ▼                           │
│  ┌─────────────────────────────────┐   │
│  │ users table                     │   │
│  │ ┌─────────────────────────┐     │   │
│  │ │ user_id: 5              │     │   │
│  │ │ name: "Nguyen Van A"    │     │   │
│  │ │ employee_no: "EMP001"   │     │   │
│  │ │ status: active           │     │   │
│  │ │ valid_from: 2024-01-01   │     │   │
│  │ │ valid_until: 2026-12-31  │     │   │
│  │ └─────────────────────────┘     │   │
│  └─────────────────────────────────┘   │
└─────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────┐
│           UserContext                    │
│ ┌─────────────────────────────────┐     │
│ │ user_id: 5                      │     │
│ │ name: "Nguyen Van A"            │     │
│ │ employee_no: "EMP001"           │     │
│ │ user_status: active             │     │
│ │ credential_type: card           │     │
│ │ credential_value: 1234567890    │     │
│ │ valid_from: 2024-01-01         │     │
│ │ valid_until: 2026-12-31        │     │
│ │ found: true                     │     │
│ │ valid: true                    │     │
│ └─────────────────────────────────┘     │
└─────────────────────────────────────────┘
                         │
                         ▼
            ┌─────────────────────────┐
            │ ResolutionResult::      │
            │ SUCCESS                 │
            └─────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────┐
│     Next: Evaluate Permission            │
│     (Check Access Rules)                 │
└─────────────────────────────────────────┘
```

## UserContext Structure

```
┌──────────────────────────────────────────┐
│ UserContext                               │
├──────────────────────────────────────────┤
│ ┌────────────────────────────────────────┐│
│ │ User Information                        ││
│ │ • user_id        : int                 ││
│ │ • name           : string              ││
│ │ • employee_no    : string              ││
│ │ • user_status    : string              ││
│ └────────────────────────────────────────┘│
│ ┌────────────────────────────────────────┐│
│ │ Credential Information                  ││
│ │ • credential_id  : int                  ││
│ │ • credential_type: string              ││
│ │ • credential_value: string             ││
│ └────────────────────────────────────────┘│
│ ┌────────────────────────────────────────┐│
│ │ Validity                                ││
│ │ • valid_from     : string              ││
│ │ • valid_until    : string              ││
│ └────────────────────────────────────────┘│
│ ┌────────────────────────────────────────┐│
│ │ Resolution Status                       ││
│ │ • found          : bool                ││
│ │ • valid          : bool                ││
│ │ • reason         : string              ││
│ └────────────────────────────────────────┘│
└──────────────────────────────────────────┘
```

## ResolutionResult Enum

```
┌──────────────────────────────────────────┐
│ ResolutionResult                          │
├──────────────────────────────────────────┤
│ SUCCESS           → User found & valid   │
│ NOT_FOUND         → Credential not in DB │
│ INVALID_CREDENTIAL → Credential inactive │
│ INVALID_USER      → User locked/disabled │
│ EXPIRED_USER      → User past valid_until│
│ DATABASE_ERROR    → DB query failed      │
└──────────────────────────────────────────┘
```

## Module Structure

```
include/
├── db_connection.hpp         - SQLite wrapper
└── credential_resolver.hpp   - Credential to user resolution

src/db/
└── db_connection.cpp          - Database implementation

src/access/
└── credential_resolver.cpp   - Resolution logic
```

## Validation Checks

| Check | Description | Result if Fail |
|-------|-------------|----------------|
| Credential exists | Credential in DB | NOT_FOUND |
| Credential active | status = 'active' | INVALID_CREDENTIAL |
| User exists | User in DB | DATABASE_ERROR |
| User active | user_status = 'active' | INVALID_USER |
| Within validity period | now < valid_until | EXPIRED_USER |
