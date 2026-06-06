cmake_minimum_required(VERSION 3.14)
project(accessd VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# Find libosdp (system or FetchContent)
# ============================================================================
# Try to find system-installed libosdp first (recommended for production)
find_package(LibOSDP CONFIG QUIET)

if(NOT LibOSDP_FOUND)
    message(STATUS "LibOSDP not found in system, fetching from source...")
    include(FetchContent)

    FetchContent_Declare(
        libosdp
        GIT_REPOSITORY https://github.com/goToMain/libosdp.git
        GIT_TAG        master
        GIT_SHALLOW    TRUE
    )

    # Set libosdp build options
    set(OSDP_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(OSDP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(OSDP_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(OSDP_BUILD_PYTHON OFF CACHE BOOL "" FORCE)
    set(OSDP_BUILD_RUST OFF CACHE BOOL "" FORCE)
    set(OPT_BUILD_SHARED ON CACHE BOOL "" FORCE)
    set(OPT_BUILD_STATIC ON CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(libosdp)
    set(LibOSDP_FOUND TRUE)
else()
    message(STATUS "Found system LibOSDP: ${LibOSDP_VERSION}")
endif()

# ============================================================================
# Find SQLite3
# ============================================================================
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(SQLITE3 REQUIRED sqlite3)
else()
    # Fallback for systems without pkg-config
    set(SQLITE3_INCLUDE_DIRS "/usr/include/sqlite3")
    set(SQLITE3_LIBRARIES "sqlite3")
endif()

# ============================================================================
# Include directories
# ============================================================================
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${SQLITE3_INCLUDE_DIRS}
)

# ============================================================================
# Source files
# ============================================================================
# Note: Old OSDP implementation files (src/osdp/*.cpp) are deprecated and not compiled.
# We now use libosdp for all OSDP protocol handling.
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
    # Add more sources as we implement them
)

# ============================================================================
# Create executable
# ============================================================================
add_executable(accessd ${SOURCES})

# ============================================================================
# Link libraries
# ============================================================================
target_link_libraries(accessd
    PRIVATE
    ${SQLITE3_LIBRARIES}
    pthread
    rt
)

# ============================================================================
# Link against libosdp
# ============================================================================
# Use the official LibOSDP target (preferred: libosdp::osdp, fallback to osdpstatic)
if(TARGET libosdp::osdp)
    target_link_libraries(accessd PRIVATE libosdp::osdp)
    message(STATUS "Linking with libosdp::osdp (shared library)")
elseif(TARGET libosdp::osdpstatic)
    target_link_libraries(accessd PRIVATE libosdp::osdpstatic)
    message(STATUS "Linking with libosdp::osdpstatic (static library)")
elseif(TARGET osdp)
    target_link_libraries(accessd PRIVATE osdp)
    message(STATUS "Linking with osdp (legacy target)")
elseif(TARGET osdpstatic)
    target_link_libraries(accessd PRIVATE osdpstatic)
    message(STATUS "Linking with osdpstatic (legacy static target)")
else()
    message(FATAL_ERROR "Could not find libosdp target. Please install libosdp first.")
endif()

# ============================================================================
# Installation
# ============================================================================
install(TARGETS accessd DESTINATION /usr/local/bin)
install(FILES config/accessd.conf DESTINATION /etc/accessd)

# ============================================================================
# Print configuration summary
# ============================================================================
message(STATUS "===============================================")
message(STATUS "Access Control System Configuration")
message(STATUS "===============================================")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "SQLite3: ${SQLITE3_LIBRARIES}")
message(STATUS "libosdp: " ${libosdp_FOUND})
message(STATUS "===============================================")
