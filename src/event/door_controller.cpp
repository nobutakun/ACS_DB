#include "door_controller.hpp"
#include "db_connection.hpp"
#include "config.hpp"
#include <chrono>
#include <thread>
#include <iostream>
#include <fstream>
#include <sqlite3.h>

namespace accessd {

// GPIO path for Orange Pi
#define GPIO_PATH "/sys/class/gpio"

DoorController::DoorController(std::shared_ptr<DatabaseConnection> db)
    : db_(db)
{
}

DoorController::~DoorController() = default;

bool DoorController::unlock_door(int door_id, int relay_time_ms,
                                 const std::string& reason) {
    if (!db_ || !db_->is_open()) {
        std::cerr << "Database not connected" << std::endl;
        return false;
    }

    std::cout << "Unlocking door " << door_id
              << " for " << relay_time_ms << "ms"
              << " (reason: " << reason << ")" << std::endl;

    // 1. Get door info
    std::string query =
        "SELECT name, relay_time_ms, open_timeout_sec, door_held_timeout_sec "
        "FROM doors WHERE door_id = ?";

    auto stmt = db_->prepare(query);
    if (!stmt) {
        std::cerr << "Failed to prepare door query" << std::endl;
        return false;
    }

    stmt->bind_int(1, door_id);

    int db_relay_time = relay_time_ms;
    if (stmt->step() == SQLITE_ROW) {
        std::string name = stmt->column_text(0);
        db_relay_time = stmt->column_int(1);
        std::cout << "Door: " << name << std::endl;
    }

    // Use provided relay time or database default
    if (relay_time_ms <= 0) {
        relay_time_ms = db_relay_time;
    }

    // 2. Trigger relay
    if (!trigger_relay(door_id, relay_time_ms)) {
        std::cerr << "Failed to trigger relay for door " << door_id << std::endl;
        return false;
    }

    // 3. Update door status
    std::string update_query =
        "UPDATE doors SET status = 'online' WHERE door_id = ?";
    auto update_stmt = db_->prepare(update_query);
    if (update_stmt) {
        update_stmt->bind_int(1, door_id);
        update_stmt->step();
    }

    return true;
}

bool DoorController::lock_door(int door_id, const std::string& reason) {
    std::cout << "Locking door " << door_id
              << " (reason: " << reason << ")" << std::endl;

    // For magnetic lock, this means power OFF
    // For electric strike, this means power ON
    // Implementation depends on hardware

    // Update door status
    if (!db_ || !db_->is_open()) {
        return false;
    }

    std::string query =
        "UPDATE doors SET status = 'online' WHERE door_id = ?";
    auto stmt = db_->prepare(query);
    if (!stmt) return false;

    stmt->bind_int(1, door_id);
    return stmt->step() == SQLITE_DONE;
}

DoorState DoorController::get_door_state(int door_id) {
    // TODO: Read from door sensor or database
    return DoorState::CLOSED;
}

bool DoorController::check_door_timeouts() {
    // TODO: Check for doors held open or forced open
    // This would be run periodically

    if (!db_ || !db_->is_open()) {
        return false;
    }

    // Query doors with status 'held_open' or check sensor input
    std::string query =
        "SELECT door_id, name, door_held_timeout_sec "
        "FROM doors WHERE status = 'held_open'";

    auto stmt = db_->prepare(query);
    if (!stmt) return false;

    while (stmt->step() == SQLITE_ROW) {
        int door_id = stmt->column_int(0);
        std::string name = stmt->column_text(1);
        int timeout = stmt->column_int(2);

        // TODO: Check how long door has been held open
        // If timeout exceeded, trigger alarm and write event
    }

    return true;
}

bool DoorController::trigger_relay(int door_id, int duration_ms) {
    // Get GPIO pin for this door
    int gpio_pin = get_door_gpio_pin(door_id);
    if (gpio_pin < 0) {
        std::cerr << "Invalid GPIO pin for door " << door_id << std::endl;
        return false;
    }

    std::cout << "Triggering relay: GPIO " << gpio_pin
              << " for " << duration_ms << "ms" << std::endl;

    // TODO: Implement proper GPIO control
    // For now, this is a simulation

    // Steps:
    // 1. Export GPIO if not already exported
    // 2. Set direction to output
    // 3. Set GPIO high (unlock)
    // 4. Wait for duration_ms
    // 5. Set GPIO low (lock)

    // Example GPIO control (would need root access):
    /*
    std::ofstream export_file(GPIO_PATH "/export");
    export_file << gpio_pin;
    export_file.close();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::ofstream direction_file(GPIO_PATH "/gpio" + std::to_string(gpio_pin) + "/direction");
    direction_file << "out";
    direction_file.close();

    std::ofstream value_file(GPIO_PATH "/gpio" + std::to_string(gpio_pin) + "/value");
    value_file << "1";  // High = unlock
    value_file.close();

    std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));

    value_file.open(GPIO_PATH "/gpio" + std::to_string(gpio_pin) + "/value");
    value_file << "0";  // Low = lock
    value_file.close();
    */

    return true;
}

int DoorController::get_relay_time_ms(int door_id) const {
    // Get from config
    auto& config = Config::instance();
    return config.hardware().relay_default_ms;
}

int DoorController::get_open_timeout_sec(int door_id) const {
    auto& config = Config::instance();
    return config.hardware().door_timeout_sec;
}

int DoorController::get_held_timeout_sec(int door_id) const {
    auto& config = Config::instance();
    return config.hardware().door_held_timeout_sec;
}

bool DoorController::set_gpio(int gpio_pin, bool state) {
    // TODO: Implement GPIO control
    return true;
}

int DoorController::get_door_gpio_pin(int door_id) const {
    // TODO: Map door_id to GPIO pin
    // For now, return example pins
    switch (door_id) {
        case 1: return 0;   // PA0
        case 2: return 1;   // PA1
        case 3: return 2;   // PA2
        case 4: return 3;   // PA3
        default: return -1;
    }
}

} // namespace accessd
