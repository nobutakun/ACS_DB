cmake_minimum_required(VERSION 3.10)
project(accessd_tests)

set(CMAKE_CXX_STANDARD 17)

# Find GTest (includes GMock)
find_package(GTest REQUIRED)

# Get the test directory (where this CMakeLists.txt lives)
set(TEST_DIR ${CMAKE_CURRENT_LIST_DIR})
set(PROJECT_ROOT ${TEST_DIR}/..)

# Include parent project headers
include_directories(${PROJECT_ROOT}/include)

# Include SQLite
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(SQLITE3 REQUIRED sqlite3)
else()
    set(SQLITE3_INCLUDE_DIRS "/usr/include/sqlite3")
    set(SQLITE3_LIBRARIES "sqlite3")
endif()

include_directories(${SQLITE3_INCLUDE_DIRS})

# Test executables with source files
add_executable(test_config
    ${TEST_DIR}/test_config.cpp
    ${PROJECT_ROOT}/src/core/config_loader.cpp
)

target_link_libraries(test_config
    GTest::GTest
    GTest::Main
    ${SQLITE3_LIBRARIES}
    pthread
)

add_executable(test_db_connection
    ${TEST_DIR}/db/test_db_connection.cpp
    ${PROJECT_ROOT}/src/db/db_connection.cpp
)

target_link_libraries(test_db_connection
    GTest::GTest
    GTest::Main
    ${SQLITE3_LIBRARIES}
    pthread
)

add_executable(test_credential_resolver
    ${TEST_DIR}/access/test_credential_resolver.cpp
    ${PROJECT_ROOT}/src/db/db_connection.cpp
    ${PROJECT_ROOT}/src/access/credential_resolver.cpp
)

target_link_libraries(test_credential_resolver
    GTest::GTest
    GTest::Main
    ${SQLITE3_LIBRARIES}
    pthread
)

add_executable(test_access_rule_evaluator
    ${TEST_DIR}/access/test_access_rule_evaluator.cpp
    ${PROJECT_ROOT}/src/db/db_connection.cpp
    ${PROJECT_ROOT}/src/access/access_rule_evaluator.cpp
)

target_link_libraries(test_access_rule_evaluator
    GTest::GTest
    GTest::Main
    ${SQLITE3_LIBRARIES}
    pthread
)

add_executable(test_event_logger
    ${TEST_DIR}/event/test_event_logger.cpp
    ${PROJECT_ROOT}/src/db/db_connection.cpp
    ${PROJECT_ROOT}/src/event/event_logger.cpp
)

target_link_libraries(test_event_logger
    GTest::GTest
    GTest::Main
    ${SQLITE3_LIBRARIES}
    pthread
)

add_executable(test_access_logger
    ${TEST_DIR}/event/test_access_logger.cpp
    ${PROJECT_ROOT}/src/db/db_connection.cpp
    ${PROJECT_ROOT}/src/event/access_logger.cpp
)

target_link_libraries(test_access_logger
    GTest::GTest
    GTest::Main
    ${SQLITE3_LIBRARIES}
    pthread
)

# Decision Response Test
add_executable(test_decision_response
    ${TEST_DIR}/event/test_decision_response.cpp
    ${PROJECT_ROOT}/src/db/db_connection.cpp
    ${PROJECT_ROOT}/src/event/decision_response.cpp
    ${PROJECT_ROOT}/src/access/access_rule_evaluator.cpp
    ${PROJECT_ROOT}/src/core/config_loader.cpp
    ${PROJECT_ROOT}/src/reader/osdp_connection.cpp
    ${PROJECT_ROOT}/src/reader/rs485.cpp
)

target_link_libraries(test_decision_response
    GTest::GTest
    GTest::Main
    GTest::gmock
    ${SQLITE3_LIBRARIES}
    pthread
)

# Door Controller Test
add_executable(test_door_controller
    ${TEST_DIR}/event/test_door_controller.cpp
    ${PROJECT_ROOT}/src/db/db_connection.cpp
    ${PROJECT_ROOT}/src/event/door_controller.cpp
    ${PROJECT_ROOT}/src/core/config_loader.cpp
)

target_link_libraries(test_door_controller
    GTest::GTest
    GTest::Main
    GTest::gmock
    ${SQLITE3_LIBRARIES}
    pthread
)

# RS485 Test
add_executable(test_rs485
    ${TEST_DIR}/reader/test_rs485.cpp
    ${PROJECT_ROOT}/src/reader/rs485.cpp
)

target_link_libraries(test_rs485
    GTest::GTest
    GTest::Main
    pthread
)

# OSDP Connection Test
find_package(LibOSDP CONFIG QUIET)
add_executable(test_osdp_connection
    ${TEST_DIR}/reader/test_osdp_connection.cpp
    ${PROJECT_ROOT}/src/reader/osdp_connection.cpp
    ${PROJECT_ROOT}/src/reader/rs485.cpp
)

target_link_libraries(test_osdp_connection
    PRIVATE
        GTest::GTest
        GTest::Main
        GTest::gmock
        pthread
)

if(TARGET libosdp::osdp)
    target_link_libraries(test_osdp_connection PRIVATE libosdp::osdp)
endif()

# Enable testing
enable_testing()

# Add tests
add_test(NAME ConfigTest COMMAND test_config)
add_test(NAME DBConnectionTest COMMAND test_db_connection)
add_test(NAME CredentialResolverTest COMMAND test_credential_resolver)
add_test(NAME AccessRuleEvaluatorTest COMMAND test_access_rule_evaluator)
add_test(NAME EventLoggerTest COMMAND test_event_logger)
add_test(NAME AccessLoggerTest COMMAND test_access_logger)
add_test(NAME DecisionResponseTest COMMAND test_decision_response)
add_test(NAME DoorControllerTest COMMAND test_door_controller)
add_test(NAME RS485Test COMMAND test_rs485)
add_test(NAME OSDPConnectionTest COMMAND test_osdp_connection)
