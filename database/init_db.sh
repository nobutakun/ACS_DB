#!/bin/bash
# ============================================
# SQLite Database Initialization Script
# Access Control System
# ============================================

DB_DIR="/home/nobuta/accessd/database"
DB_FILE="$DB_DIR/accessd.db"
SCHEMA_DIR="$DB_DIR/schema"

echo "=== Access Control Database Initialization ==="
echo "Database: $DB_FILE"

# Create database directory
mkdir -p "$DB_DIR"

# Remove existing database if present (optional)
if [ -f "$DB_FILE" ]; then
    read -p "Database exists. Overwrite? (y/N): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        rm "$DB_FILE"
        echo "Old database removed."
    else
        echo "Aborted."
        exit 1
    fi
fi

# Apply schema files in order
echo "Applying schemas..."
for schema in "$SCHEMA_DIR"/*.sql; do
    if [ -f "$schema" ]; then
        echo "  - $(basename "$schema")"
        sqlite3 "$DB_FILE" < "$schema"
        if [ $? -eq 0 ]; then
            echo "    OK"
        else
            echo "    FAILED!"
            exit 1
        fi
    fi
done

echo ""
echo "=== Database created successfully ==="
sqlite3 "$DB_FILE" ".tables"
echo ""
echo "Database location: $DB_FILE"
