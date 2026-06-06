#!/bin/bash
# ============================================
# Install Script - Access Control System
# Run on Raspberry Pi
# ============================================

ACCESSD_HOME="/home/nobuta/accessd"

echo "=== Access Control System Installation ==="

# Create directory structure
echo "Creating directory structure..."
mkdir -p "$ACCESSD_HOME/database/schema"
mkdir -p "$ACCESSD_HOME/database/migrations"
mkdir -p "$ACCESSD_HOME/database/seeds"
mkdir -p "$ACCESSD_HOME/services/accessd"
mkdir -p "$ACCESSD_HOME/management/user_mgmt"
mkdir -p "$ACCESSD_HOME/management/credential_mgmt"
mkdir -p "$ACCESSD_HOME/management/door_reader_mgmt"
mkdir -p "$ACCESSD_HOME/scripts"
mkdir -p "$ACCESSD_HOME/logs"
mkdir -p "$ACCESSD_HOME/config"

echo "Directory structure created."
echo "Location: $ACCESSD_HOME"
echo ""
echo "Next steps:"
echo "1. Copy schema files to database/schema/"
echo "2. Run: cd $ACCESSD_HOME/database && bash init_db.sh"
