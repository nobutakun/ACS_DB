#ifndef ACCESSD_DOOR_CONTROLLER_HPP
#define ACCESSD_DOOR_CONTROLLER_HPP

#include <string>
#include <memory>

namespace accessd {

// Forward declaration
class DatabaseConnection;

// Door state
enum class DoorState {
    CLOSED,
    OPEN,
    HELD_OPEN,
    FORCED_OPEN,
    ERROR
};

// Door action
struct DoorAction {
    int door_id;
    bool unlock;              // true = unlock, false = lock
    int relay_time_ms;        // How long to keep relay active
    std::string reason;       // Why this action
};

class DoorController {
public:
    DoorController(std::shared_ptr<DatabaseConnection> db);
    ~DoorController();

    // Unlock door (trigger relay)
    bool unlock_door(int door_id, int relay_time_ms, const std::string& reason);

    // Lock door
    bool lock_door(int door_id, const std::string& reason);

    // Get door state
    DoorState get_door_state(int door_id);

    // Check door timeout (held open, forced)
    bool check_door_timeouts();

    // Trigger relay via GPIO
    bool trigger_relay(int door_id, int duration_ms);

private:
    std::shared_ptr<DatabaseConnection> db_;

    // Get relay time for door from config or database
    int get_relay_time_ms(int door_id) const;

    // Get door timeout settings
    int get_open_timeout_sec(int door_id) const;
    int get_held_timeout_sec(int door_id) const;

    // GPIO control (implementation depends on hardware)
    bool set_gpio(int gpio_pin, bool state);
    int get_door_gpio_pin(int door_id) const;
};

} // namespace accessd

#endif // ACCESSD_DOOR_CONTROLLER_HPP
