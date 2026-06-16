#!/bin/bash
# ============================================
# Run New Tests Only
# ============================================

ACCESSD_HOME="/home/nobuta/accessd"
TEST_DIR="$ACCESSD_HOME/test/build"

cd "$TEST_DIR" || exit 1

echo "============================================"
echo "  New Unit Tests"
echo "============================================"
echo ""

# New tests
NEW_TESTS=(
    "test_decision_response:Decision Response Tests"
    "test_door_controller:Door Controller Tests"
    "test_rs485:RS485 Tests"
    "test_osdp_connection:OSDP Connection Tests"
)

for test_entry in "${NEW_TESTS[@]}"; do
    IFS=':' read -r test_name test_desc <<< "$test_entry"

    echo "Running: $test_desc"
    ./"$test_name" --gtest_color=yes
    RESULT=$?

    if [ $RESULT -eq 0 ]; then
        echo -e "✓ PASSED\n"
    else
        echo -e "✗ FAILED\n"
    fi
done

echo "============================================"
echo "Run with verbose output:"
echo "  ./test_decision_response --gtest_filter=*.TestName"
