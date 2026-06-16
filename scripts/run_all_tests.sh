#!/bin/bash
# ============================================
# Run All Tests with Detailed Output
# ============================================

ACCESSD_HOME="/home/nobuta/accessd"
TEST_DIR="$ACCESSD_HOME/test/build"
FAILED_TESTS=()
PASSED_TESTS=()

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "============================================"
echo "  ACS-DB Unit Tests"
echo "============================================"
echo ""

# Check if build directory exists
if [ ! -d "$TEST_DIR" ]; then
    echo "Build directory not found. Creating..."
    mkdir -p "$TEST_DIR"
fi

cd "$TEST_DIR"

# Build tests first
echo "Building tests..."
cmake .. >/dev/null 2>&1
if ! make -j$(nproc) >/dev/null 2>&1; then
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi
echo -e "${GREEN}Build successful${NC}"
echo ""

# Array of test executables
TESTS=(
    "test_config:Config Tests"
    "test_db_connection:Database Connection Tests"
    "test_credential_resolver:Credential Resolver Tests"
    "test_access_rule_evaluator:Access Rule Evaluator Tests"
    "test_event_logger:Event Logger Tests"
    "test_access_logger:Access Logger Tests"
    "test_decision_response:Decision Response Tests"
    "test_door_controller:Door Controller Tests"
    "test_rs485:RS485 Tests"
    "test_osdp_connection:OSDP Connection Tests"
)

TOTAL_TESTS=${#TESTS[@]}

for test_entry in "${TESTS[@]}"; do
    IFS=':' read -r test_name test_desc <<< "$test_entry"

    echo "Running: $test_desc"
    echo "Command: ./$test_name"

    if ./"$test_name" >/dev/null 2>&1; then
        echo -e "${GREEN}✓ PASSED${NC} - $test_desc"
        PASSED_TESTS+=("$test_name")
    else
        echo -e "${RED}✗ FAILED${NC} - $test_desc"
        FAILED_TESTS+=("$test_name")
    fi
    echo ""
done

# Summary
echo "============================================"
echo "  Test Summary"
echo "============================================"
echo -e "Total Tests: $TOTAL_TESTS"
echo -e "${GREEN}Passed: ${#PASSED_TESTS[@]}${NC}"
echo -e "${RED}Failed: ${#FAILED_TESTS[@]}${NC}"
echo ""

if [ ${#FAILED_TESTS[@]} -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Failed tests:${NC}"
    for test in "${FAILED_TESTS[@]}"; do
        echo "  - $test"
    done
    exit 1
fi
