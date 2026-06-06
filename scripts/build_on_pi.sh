#!/bin/bash
# ============================================
# Build Accessd on Pi4 (run this ON Pi4)
# ============================================

ACCESSD_HOME="/home/nobuta/accessd"

echo "=== Building Accessd on Pi4 ==="

# Install cmake if needed
if ! command -v cmake &> /dev/null; then
    echo "Installing cmake..."
    sudo apt update
    sudo apt install -y cmake build-essential
fi

# Create build directory
cd "$ACCESSD_HOME"
mkdir -p build && cd build

# Configure
echo "Configuring..."
cmake ..

# Build
echo "Building..."
make

# Check if build succeeded
if [ -f "./accessd" ]; then
    echo ""
    echo "=== Build successful! ==="
    echo "Run: ./accessd"
    echo "Or: sudo ./accessd"
else
    echo ""
    echo "=== Build failed! ==="
fi
