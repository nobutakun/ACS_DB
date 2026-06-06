#!/bin/bash
# ============================================
# Build and Run accessd on Pi4
# Run this script ON Pi4
# ============================================

ACCESSD_HOME="/home/nobuta/accessd"
BUILD_DIR="$ACCESSD_HOME/build"

echo "=== Building and Running Accessd ==="

# Install dependencies if needed
if ! command -v cmake &> /dev/null; then
    echo "Installing cmake..."
    sudo apt update
    sudo apt install -y cmake build-essential
fi

if ! pkg-config --exists sqlite3; then
    echo "Installing sqlite3 dev..."
    sudo apt install -y libsqlite3-dev
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
echo "Configuring..."
cmake ..

# Build
echo "Building..."
make

# Check if build succeeded
if [ ! -f "./accessd" ]; then
    echo ""
    echo "=== Build failed! ==="
    exit 1
fi

echo ""
echo "=== Build successful! ==="

# Initialize database if not exists
if [ ! -f "$ACCESSD_HOME/database/accessd.db" ]; then
    echo ""
    echo "Initializing database..."
    cd "$ACCESSD_HOME/database"
    chmod +x init_db.sh
    ./init_db.sh
fi

# Run accessd
echo ""
echo "Starting accessd..."
cd "$BUILD_DIR"
./accessd
