#!/bin/bash
# ============================================
# Deploy Accessd Service to Pi4 (Complete)
# ============================================

PI_USER="nobuta"
PI_HOST="Cv.local"
PI_DIR="/home/nobuta/accessd"
PROJECT_ROOT="/c/Users/CAS/Work-Project/ACS_DB"

echo "=== Deploying Accessd Service to Pi4 ==="

# Create directory structure
echo "Creating directory structure..."
ssh "$PI_USER@$PI_HOST" "mkdir -p $PI_DIR/src/core $PI_DIR/src/db $PI_DIR/src/access $PI_DIR/src/event $PI_DIR/src/reader $PI_DIR/src/api"
ssh "$PI_USER@$PI_HOST" "mkdir -p $PI_DIR/include $PI_DIR/config $PI_DIR/scripts $PI_DIR/test $PI_DIR/docs $PI_DIR/build"
ssh "$PI_USER@$PI_HOST" "mkdir -p $PI_DIR/database/schema $PI_DIR/database/migrations $PI_DIR/database/seed"
ssh "$PI_USER@$PI_HOST" "mkdir -p $PI_DIR/logs"

# Copy source files
echo ""
echo "Copying source files..."
# Core
scp "$PROJECT_ROOT/src/core/"*.cpp "$PI_USER@$PI_HOST:$PI_DIR/src/core/"

# Database
scp "$PROJECT_ROOT/src/db/"*.cpp "$PI_USER@$PI_HOST:$PI_DIR/src/db/"

# Access
scp "$PROJECT_ROOT/src/access/"*.cpp "$PI_USER@$PI_HOST:$PI_DIR/src/access/"

# Event
scp "$PROJECT_ROOT/src/event/"*.cpp "$PI_USER@$PI_HOST:$PI_DIR/src/event/"

# Reader
scp "$PROJECT_ROOT/src/reader/"*.cpp "$PI_USER@$PI_HOST:$PI_DIR/src/reader/"

# Copy headers
echo "Copying headers..."
scp "$PROJECT_ROOT/include/"*.hpp "$PI_USER@$PI_HOST:$PI_DIR/include/"

# Copy config
echo "Copying config..."
scp "$PROJECT_ROOT/config/accessd.conf" "$PI_USER@$PI_HOST:$PI_DIR/config/"

# Copy build files
echo "Copying build files..."
scp "$PROJECT_ROOT/CMakeLists.txt" "$PI_USER@$PI_HOST:$PI_DIR/"

# Copy database
echo "Copying database..."
scp "$PROJECT_ROOT/database/schema/"*.sql "$PI_USER@$PI_HOST:$PI_DIR/database/schema/"
scp "$PROJECT_ROOT/database/init_db.sh" "$PI_USER@$PI_HOST:$PI_DIR/database/"
scp "$PROJECT_ROOT/database/init_db.sql" "$PI_USER@$PI_HOST:$PI_DIR/database/"

# Copy scripts
echo "Copying scripts..."
scp "$PROJECT_ROOT/scripts/build_on_pi.sh" "$PI_USER@$PI_HOST:$PI_DIR/scripts/"

echo ""
echo "=== Deployment complete ==="
echo ""
echo "On Pi4, run:"
echo "  cd $PI_DIR"
echo "  mkdir -p build && cd build"
echo "  cmake .."
echo "  make"
echo "  ./accessd"
