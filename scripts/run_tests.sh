#!/bin/bash
# ============================================
# Run Unit Tests on Pi4
# ============================================

ACCESSD_HOME="/home/nobuta/accessd"
TEST_DIR="$ACCESSD_HOME/test/build"

echo "=== Building and Running Unit Tests ==="

# Install Google Test if needed
if ! pkg-config --exists gtest; then
    echo "Installing Google Test..."
    sudo apt update
    sudo apt install -y libgtest-dev cmake
fi

# Create test build directory
mkdir -p "$TEST_DIR"
cd "$TEST_DIR"

# Configure tests
echo "Configuring tests..."
cmake ..

# Build tests
echo "Building tests..."
make

# Run tests
echo ""
echo "=== Running Tests ==="
ctest --output-on-failure

# Or run individually:
# echo ""
# echo "Running ConfigTest..."
# ./test_config

# echo ""
# echo "Running DBConnectionTest..."
# ./test_db_connection

# echo ""
# echo "Running CredentialResolverTest..."
# ./test_credential_resolver

# echo ""
# echo "Running AccessRuleEvaluatorTest..."
# ./test_access_rule_evaluator

# echo ""
# echo "Running EventLoggerTest..."
# ./test_event_logger

# echo ""
# echo "Running AccessLoggerTest..."
# ./test_access_logger
