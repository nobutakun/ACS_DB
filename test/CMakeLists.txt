cmake_minimum_required(VERSION 3.10)
project(accessd_tests)

set(CMAKE_CXX_STANDARD 17)

# Find GTest
find_package(GTest REQUIRED)

# Include parent project headers
include_directories(${CMAKE_SOURCE_DIR}/../include)

# Include SQLite
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(SQLITE3 REQUIRED sqlite3)
else()
    set(SQLITE3_INCLUDE_DIRS "/usr/include/sqlite3")
    set(SQLITE3_LIBRARIES "sqlite3")
endif()

include_directories(${SQLITE3_INCLUDE_DIRS})

# Add parent source directories
set(PARENT_SRC_DIR ${CMAKE_SOURCE_DIR}/..)

# Test executables with source files
add_executable(test_config
    test_config.cpp
    ${PARENT_SRC_DIR}/src/core/config_loader.cpp
)

target_link_libraries(test_config
    GTest::GTest
    GTest::Main
    ${SQLITE3_LIBRARIES}
    pthread
)

add_executable(test_db_connection
    db/test_db_connection.cpp
    ${PARENT_SRC_DIR}/src/db/db_connection.cpp
)

target_link_libraries(test_db_connection
    GTest::GTest
    GTest::Main
    ${SQLITE3_LIBRARIES}
    pthread
)

add_executable(test_credential_resolver
    access/test_credential_resolver.cpp
    ${PARENT_SRC_DIR}/src/db/db_connection.cpp
    ${PARENT_SRC_DIR}/src/access/credential_resolver.cpp
)

target_link_libraries(test_credential_resolver
    GTest::GTest
    GTest::Main
    ${SQLITE3_LIBRARIES}
    pthread
)

add_executable(test_access_rule_evaluator
    access/test_access_rule_evaluator.cpp
    ${PARENT_SRC_DIR}/src/db/db_connection.cpp
    ${PARENT_SRC_DIR}/src/access/access_rule_evaluator.cpp
)

target_link_libraries(test_access_rule_evaluator
    GTest::GTest
    GTest::Main
    ${SQLITE3_LIBRARIES}
    pthread
)

add_executable(test_event_logger
    event/test_event_logger.cpp
    ${PARENT_SRC_DIR}/src/db/db_connection.cpp
    ${PARENT_SRC_DIR}/src/event/event_logger.cpp
)

target_link_libraries(test_event_logger
    GTest::GTest
    GTest::Main
    ${SQLITE3_LIBRARIES}
    pthread
)

add_executable(test_access_logger
    event/test_access_logger.cpp
    ${PARENT_SRC_DIR}/src/db/db_connection.cpp
    ${PARENT_SRC_DIR}/src/event/access_logger.cpp
)

target_link_libraries(test_access_logger
    GTest::GTest
    GTest::Main
    ${SQLITE3_LIBRARIES}
    pthread
)

# Enable testing
enable_testing()

# Add tests
add_test(NAME ConfigTest COMMAND test_config)
add_test(NAME DBConnectionTest COMMAND test_db_connection)
add_test(NAME CredentialResolverTest COMMAND test_credential_resolver)
add_test(NAME AccessRuleEvaluatorTest COMMAND test_access_rule_evaluator)
add_test(NAME EventLoggerTest COMMAND test_event_logger)
add_test(NAME AccessLoggerTest COMMAND test_access_logger)
