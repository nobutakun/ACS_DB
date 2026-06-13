cmake_minimum_required(VERSION 3.14)

project(accessd
    VERSION 1.0.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# Dependencies
# ============================================================================

find_package(LibOSDP CONFIG REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

# ============================================================================
# Sources
# ============================================================================

set(SOURCES
    src/core/config_loader.cpp
    src/core/main.cpp

    src/db/db_connection.cpp

    src/access/credential_resolver.cpp
    src/access/access_rule_evaluator.cpp

    src/event/decision_response.cpp
    src/event/door_controller.cpp
    src/event/event_logger.cpp
    src/event/access_logger.cpp

    src/reader/rs485.cpp
    src/reader/osdp_connection.cpp
)

# ============================================================================
# Executable
# ============================================================================

add_executable(accessd ${SOURCES})

# ============================================================================
# Includes
# ============================================================================

target_include_directories(accessd
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${SQLITE3_INCLUDE_DIRS}

        # Workaround for current libosdp header layout
        /usr/local/include/libosdp
)

# ============================================================================
# Libraries
# ============================================================================

target_link_libraries(accessd
    PRIVATE
        ${SQLITE3_LIBRARIES}
        pthread
        rt
        libosdp::osdp
)

target_compile_definitions(accessd
    PRIVATE
    HAVE_LIBOSDP=1
)

# ============================================================================
# Install
# ============================================================================

install(
    TARGETS accessd
    DESTINATION /usr/local/bin
)

install(
    FILES config/accessd.conf
    DESTINATION /etc/accessd
)

# ============================================================================
# Summary
# ============================================================================

message(STATUS "")
message(STATUS "===============================================")
message(STATUS "Access Control System Configuration")
message(STATUS "===============================================")
message(STATUS "Version        : ${PROJECT_VERSION}")
message(STATUS "Build Type     : ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard   : ${CMAKE_CXX_STANDARD}")
message(STATUS "SQLite3        : ${SQLITE3_LIBRARIES}")
message(STATUS "LibOSDP        : FOUND")
message(STATUS "LibOSDP Target : libosdp::osdp")
message(STATUS "===============================================")
message(STATUS "")