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

# echo ""
# echo "Running DecisionResponseTest..."
# ./test_decision_response

# echo ""
# echo "Running DoorControllerTest..."
# ./test_door_controller

# echo ""
# echo "Running RS485Test..."
# ./test_rs485

# echo ""
# echo "Running OSDPConnectionTest..."
# ./test_osdp_connection

echo ""
echo "=== Test Summary ==="
echo "Total tests run: $(ctest --print-count | grep 'Total Tests:' | cut -d: -f2)"
echo "See results above for details."
