#!/bin/bash
# ============================================
# Deploy to Raspberry Pi 4
# Run from Git Bash / WSL / Linux
# ============================================

PI_USER="nobuta"
PI_HOST="Cv.local"
PI_DIR="/home/nobuta/accessd"
PROJECT_ROOT="/c/Users/CAS/Work-Project/ACS_DB"

echo "=== Deploying Access Control Database to Pi4 ==="

# Create directory structure on Pi4
echo "Creating directory structure..."
ssh "$PI_USER@$PI_HOST" "mkdir -p $PI_DIR/database/schema"
ssh "$PI_USER@$PI_HOST" "mkdir -p $PI_DIR/database/migrations"
ssh "$PI_USER@$PI_HOST" "mkdir -p $PI_DIR/database/seeds"
ssh "$PI_USER@$PI_HOST" "mkdir -p $PI_DIR/management/user_mgmt"
ssh "$PI_USER@$PI_HOST" "mkdir -p $PI_DIR/management/credential_mgmt"
ssh "$PI_USER@$PI_HOST" "mkdir -p $PI_DIR/management/door_reader_mgmt"
ssh "$PI_USER@$PI_HOST" "mkdir -p $PI_DIR/services/accessd"
ssh "$PI_USER@$PI_HOST" "mkdir -p $PI_DIR/scripts"
ssh "$PI_USER@$PI_HOST" "mkdir -p $PI_DIR/logs"
ssh "$PI_USER@$PI_HOST" "mkdir -p $PI_DIR/config"

# Copy files
echo ""
echo "Copying schema files..."
scp "$PROJECT_ROOT/database/schema"/*.sql "$PI_USER@$PI_HOST:$PI_DIR/database/schema/"

echo "Copying init scripts..."
scp "$PROJECT_ROOT/database/init_db.sh" "$PI_USER@$PI_HOST:$PI_DIR/database/"
scp "$PROJECT_ROOT/database/init_db.sql" "$PI_USER@$PI_HOST:$PI_DIR/database/"
scp "$PROJECT_ROOT/scripts/install.sh" "$PI_USER@$PI_HOST:$PI_DIR/scripts/"

echo ""
echo "=== Deployment complete ==="
echo ""
echo "On Pi4, run:"
echo "  cd $PI_DIR/database"
echo "  bash init_db.sh"
