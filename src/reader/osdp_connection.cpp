/**
 * @file osdp_connection.cpp
 * @brief OSDP connection implementation using libosdp
 *
 * This file implements OSDP communication using the libosdp library.
 * libosdp is a well-tested, production-ready implementation of the OSDP protocol.
 *
 * Reference: https://github.com/goToMain/libosdp
 */

#include "osdp_reader.hpp"
#include "rs485.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <vector>
#include <map>

// Include libosdp C API
#ifdef HAVE_LIBOSDP
#include <osdp.h>
#else
// If libosdp is not available, we'll use stub declarations for now
// This allows the project to compile even without libosdp installed
#warning "libosdp not found! OSDP functionality will be limited."

// Forward declarations for libosdp types (for compilation without libosdp)
typedef void osdp_t;

// OSDP Event types
enum osdp_event_type {
    OSDP_EVENT_CARDREAD = 1,
    OSDP_EVENT_KEYPRESS,
    OSDP_EVENT_MFGREP,
    OSDP_EVENT_STATUS,
    OSDP_EVENT_NOTIFICATION,
    OSDP_EVENT_SENTINEL
};

// OSDP Command types
enum osdp_cmd_e {
    OSDP_CMD_OUTPUT = 1,
    OSDP_CMD_LED,
    OSDP_CMD_BUZZER,
    OSDP_CMD_TEXT,
    OSDP_CMD_KEYSET,
    OSDP_CMD_COMSET,
    OSDP_CMD_MFG,
    OSDP_CMD_FILE_TX,
    OSDP_CMD_STATUS,
    OSDP_CMD_COMSET_DONE,
    OSDP_CMD_NOTIFICATION,
    OSDP_CMD_SENTINEL
};

// LED Color enum
enum osdp_led_color_e {
    OSDP_LED_COLOR_NONE = 0,
    OSDP_LED_COLOR_BLACK,
    OSDP_LED_COLOR_RED,
    OSDP_LED_COLOR_GREEN,
    OSDP_LED_COLOR_AMBER,
    OSDP_LED_COLOR_BLUE,
    OSDP_LED_COLOR_MAGENTA,
    OSDP_LED_COLOR_CYAN,
    OSDP_LED_COLOR_WHITE
};

// OSDP Event structures
struct osdp_event_cardread {
    int reader_no;
    int format;
    int direction;
    int length;
    uint8_t data[64];
};

struct osdp_event_keypress {
    int reader_no;
    int length;
    uint8_t data[64];
};

struct osdp_event {
    enum osdp_event_type type;
    uint32_t flags;
    union {
        struct osdp_event_keypress keypress;
        struct osdp_event_cardread cardread;
    };
};

// OSDP Command structures
struct osdp_cmd_led_params {
    uint8_t control_code;
    uint8_t on_count;
    uint8_t off_count;
    uint8_t on_color;
    uint8_t off_color;
    uint16_t timer_count;
};

struct osdp_cmd_led {
    uint8_t reader;
    uint8_t led_number;
    struct osdp_cmd_led_params temporary;
    struct osdp_cmd_led_params permanent;
};

struct osdp_cmd_buzzer {
    uint8_t reader;
    uint8_t control_code;
    uint8_t on_count;
    uint8_t off_count;
    uint8_t rep_count;
};

struct osdp_cmd_text {
    uint8_t reader;
    uint8_t control_code;
    uint8_t temp_time;
    uint8_t offset_row;
    uint8_t offset_col;
    uint8_t length;
    uint8_t data[32];
};

struct osdp_cmd_output {
    uint8_t output_no;
    uint8_t control_code;
    uint16_t timer_count;
};

struct osdp_cmd {
    enum osdp_cmd_e id;
    uint32_t flags;
    union {
        struct osdp_cmd_led led;
        struct osdp_cmd_buzzer buzzer;
        struct osdp_cmd_text text;
        struct osdp_cmd_output output;
    };
};

// OSDP Channel and PD Info
struct osdp_channel {
    void *data;
    int (*send)(void *data, uint8_t *buf, int len);
    int (*recv)(void *data, uint8_t *buf, int maxlen);
};

struct osdp_pd_info {
    int baud_rate;
    int address;
    const char *device_path;
    struct osdp_channel channel;
    const void *scbk;
    int flags;
};

// libosdp CP functions
osdp_t *osdp_cp_setup(const struct osdp_channel *channel, int num_pd,
                      const struct osdp_pd_info *pd_info);
void osdp_cp_refresh(osdp_t *ctx);
void osdp_cp_teardown(osdp_t *ctx);
int osdp_cp_send_command(osdp_t *ctx, int pd, const struct osdp_cmd *cmd);
void osdp_cp_set_event_callback(osdp_t *ctx, void (*cb)(void *arg, int pd, struct osdp_event *ev), void *arg);

#endif

namespace accessd {

// Per-PD context mapping: OSDP address -> pd_offset
static std::map<int, int> pd_address_to_offset;
static std::map<int, OSDPReader> pd_address_to_reader;

// ============================================================================
// OSDPConnection Implementation with libosdp
// ============================================================================

OSDPConnection::OSDPConnection()
    : baudrate_(9600)
    , running_(false)
    , osdp_ctx_(nullptr)
{
}

OSDPConnection::~OSDPConnection() {
    stop();
}

bool OSDPConnection::init(const std::string& serial_port, int baudrate) {
    serial_port_ = serial_port;
    baudrate_ = baudrate;

    // Setup RS485
    RS485Config config;
    config.port = serial_port;
    config.baud_rate = baudrate;

    RS485 rs485;
    if (!rs485.open(config)) {
        std::cerr << "Failed to open RS485: " << serial_port << std::endl;
        return false;
    }

    std::cout << "OSDP initialized on " << serial_port
              << " at " << baudrate_ << " baud" << std::endl;

    return true;
}

bool OSDPConnection::setup_rs485() {
    RS485Config config;
    config.port = serial_port_;
    config.baud_rate = baudrate_;

    RS485 rs485;
    return rs485.open(config);
}

/**
 * @brief Add a PD (reader) to the OSDP bus
 */
bool OSDPConnection::add_reader(const OSDPReader& reader) {
    // Check if reader already exists
    for (const auto& r : readers_) {
        if (r.osdp_address == reader.osdp_address) {
            std::cerr << "Reader OSDP address " << reader.osdp_address << " already exists" << std::endl;
            return false;
        }
    }

    readers_.push_back(reader);
    std::cout << "Added reader: " << reader.name
              << " (PD ID: " << reader.osdp_address << ", port: " << reader.rs485_port << ")" << std::endl;

    return true;
}

bool OSDPConnection::remove_reader(int reader_id) {
    for (auto it = readers_.begin(); it != readers_.end(); ++it) {
        if (it->reader_id == reader_id) {
            std::cout << "Removed reader ID: " << reader_id << std::endl;
            readers_.erase(it);
            return true;
        }
    }
    return false;
}

bool OSDPConnection::remove_reader_by_address(int osdp_address) {
    for (auto it = readers_.begin(); it != readers_.end(); ++it) {
        if (it->osdp_address == osdp_address) {
            std::cout << "Removed reader with OSDP address: " << osdp_address << std::endl;
            readers_.erase(it);
            return true;
        }
    }
    return false;
}

bool OSDPConnection::start() {
    if (running_) {
        std::cerr << "OSDP already running" << std::endl;
        return false;
    }

    if (readers_.empty()) {
        std::cerr << "No readers added. Cannot start OSDP." << std::endl;
        return false;
    }

    // Build PD info array
    std::vector<struct osdp_pd_info> pd_infos;
    pd_address_to_offset.clear();
    pd_address_to_reader.clear();

    int offset = 0;
    for (const auto& reader : readers_) {
        struct osdp_pd_info info = {0};
        info.baud_rate = reader.baudrate;
        info.address = reader.osdp_address;
        info.device_path = reader.rs485_port.c_str();
        info.scbk = nullptr;
        info.flags = 0;

        pd_infos.push_back(info);
        pd_address_to_offset[reader.osdp_address] = offset;
        pd_address_to_reader[reader.osdp_address] = reader;
        offset++;
    }

    // Setup channel
    struct osdp_channel channel = {0};
    channel.send = [](void *data, uint8_t *buf, int len) -> int {
        std::cout << "OSDP TX: " << len << " bytes" << std::endl;
        // TODO: Implement RS485 send
        return len;
    };

    channel.recv = [](void *data, uint8_t *buf, int maxlen) -> int {
        // TODO: Implement RS485 recv
        return 0;
    };

    // Setup libosdp CP context
    osdp_ctx_ = osdp_cp_setup(&channel, static_cast<int>(pd_infos.size()), pd_infos.data());
    if (!osdp_ctx_) {
        std::cerr << "Failed to setup OSDP CP" << std::endl;
        return false;
    }

    // Set event callback
    osdp_cp_set_event_callback(osdp_ctx_, [](void *arg, int pd, struct osdp_event *ev) {
        OSDPConnection* conn = static_cast<OSDPConnection*>(arg);
        conn->handle_osdp_event(pd, *ev);
    }, this);

    running_ = true;

    // Start OSDP refresh loop in background thread
    std::thread([this]() {
        this->osdp_loop();
    }).detach();

    std::cout << "OSDP connection started" << std::endl;
    return true;
}

void OSDPConnection::stop() {
    if (!running_) return;

    running_ = false;

    // Teardown OSDP context
    if (osdp_ctx_) {
        osdp_cp_teardown(osdp_ctx_);
        osdp_ctx_ = nullptr;
    }

    readers_.clear();
    pd_address_to_offset.clear();
    pd_address_to_reader.clear();

    std::cout << "OSDP connection stopped" << std::endl;
}

void OSDPConnection::set_access_callback(AccessRequestCallback callback) {
    access_callback_ = callback;
}

std::vector<OSDPReader> OSDPConnection::get_readers() const {
    return readers_;
}

bool OSDPConnection::led(int pd_id, int led_number, bool on) {
    auto it = pd_address_to_offset.find(pd_id);
    if (it == pd_address_to_offset.end()) {
        std::cerr << "Unknown PD ID: " << pd_id << std::endl;
        return false;
    }

    struct osdp_cmd cmd = {0};
    cmd.id = OSDP_CMD_LED;
    cmd.flags = 0;

    cmd.led.reader = 0;
    cmd.led.led_number = led_number;

    // Permanent mode: set LED on or off
    cmd.led.permanent.control_code = 1;
    cmd.led.permanent.on_count = on ? 255 : 0;
    cmd.led.permanent.off_count = on ? 0 : 255;
    cmd.led.permanent.on_color = on ? OSDP_LED_COLOR_GREEN : OSDP_LED_COLOR_BLACK;
    cmd.led.permanent.off_color = OSDP_LED_COLOR_BLACK;
    cmd.led.permanent.timer_count = 0;

    // Temporary mode: NOP
    cmd.led.temporary.control_code = 0;
    cmd.led.temporary.on_count = 0;
    cmd.led.temporary.off_count = 0;
    cmd.led.temporary.on_color = OSDP_LED_COLOR_NONE;
    cmd.led.temporary.off_color = OSDP_LED_COLOR_NONE;
    cmd.led.temporary.timer_count = 0;

    if (osdp_cp_send_command(osdp_ctx_, it->second, &cmd) != 0) {
        std::cerr << "Failed to send LED command" << std::endl;
        return false;
    }

    std::cout << "LED " << led_number << " (PD " << pd_id << "): "
              << (on ? "ON" : "OFF") << std::endl;
    return true;
}

bool OSDPConnection::buzzer(int pd_id, int on_time_ms, int off_time_ms, int count) {
    auto it = pd_address_to_offset.find(pd_id);
    if (it == pd_address_to_offset.end()) {
        std::cerr << "Unknown PD ID: " << pd_id << std::endl;
        return false;
    }

    struct osdp_cmd cmd = {0};
    cmd.id = OSDP_CMD_BUZZER;
    cmd.flags = 0;

    cmd.buzzer.reader = 0;
    cmd.buzzer.control_code = 2;
    cmd.buzzer.on_count = static_cast<uint8_t>(on_time_ms / 100);
    cmd.buzzer.off_count = static_cast<uint8_t>(off_time_ms / 100);
    cmd.buzzer.rep_count = static_cast<uint8_t>(count);

    if (osdp_cp_send_command(osdp_ctx_, it->second, &cmd) != 0) {
        std::cerr << "Failed to send buzzer command" << std::endl;
        return false;
    }

    std::cout << "Buzzer (PD " << pd_id << "): " << count
              << " beeps (on=" << on_time_ms << "ms, off=" << off_time_ms << "ms)" << std::endl;
    return true;
}

bool OSDPConnection::display_text(int pd_id, const std::string& text, int duration_sec) {
    auto it = pd_address_to_offset.find(pd_id);
    if (it == pd_address_to_offset.end()) {
        std::cerr << "Unknown PD ID: " << pd_id << std::endl;
        return false;
    }

    struct osdp_cmd cmd = {0};
    cmd.id = OSDP_CMD_TEXT;
    cmd.flags = 0;

    cmd.text.reader = 0;
    cmd.text.control_code = (duration_sec > 0) ? 3 : 1;
    cmd.text.temp_time = static_cast<uint8_t>(duration_sec);
    cmd.text.offset_row = 1;
    cmd.text.offset_col = 1;
    cmd.text.length = std::min(text.size(), sizeof(cmd.text.data) - 1);
    std::strncpy(reinterpret_cast<char*>(cmd.text.data), text.c_str(), sizeof(cmd.text.data) - 1);

    if (osdp_cp_send_command(osdp_ctx_, it->second, &cmd) != 0) {
        std::cerr << "Failed to send text command" << std::endl;
        return false;
    }

    std::cout << "Display (PD " << pd_id << "): " << text
              << " (" << duration_sec << "s)" << std::endl;
    return true;
}

void OSDPConnection::osdp_loop() {
    std::cout << "OSDP refresh loop started" << std::endl;

    while (running_) {
        if (osdp_ctx_) {
            osdp_cp_refresh(osdp_ctx_);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::cout << "OSDP refresh loop stopped" << std::endl;
}

void OSDPConnection::handle_osdp_event(int pd_offset, struct osdp_event& event) {
    // Find OSDP address by offset
    int osdp_address = -1;
    for (const auto& pair : pd_address_to_offset) {
        if (pair.second == pd_offset) {
            osdp_address = pair.first;
            break;
        }
    }

    if (osdp_address < 0) {
        std::cerr << "Unknown PD offset: " << pd_offset << std::endl;
        return;
    }

    switch (event.type) {
        case OSDP_EVENT_CARDREAD: {
            std::string card_str;
            char buf[4];
            for (int i = 0; i < event.cardread.length; i++) {
                snprintf(buf, sizeof(buf), "%02X", event.cardread.data[i]);
                card_str += buf;
            }
            handle_card_read(osdp_address, card_str);
            break;
        }
        case OSDP_EVENT_KEYPRESS:
            handle_keypress(osdp_address, event.keypress.data, event.keypress.length);
            break;
        default:
            std::cout << "Unhandled OSDP event type: " << event.type << std::endl;
            break;
    }
}

void OSDPConnection::handle_card_read(int pd_id, const std::string& card_data) {
    if (!access_callback_) {
        std::cerr << "No access callback registered" << std::endl;
        return;
    }

    auto it = pd_address_to_reader.find(pd_id);
    if (it == pd_address_to_reader.end()) {
        std::cerr << "Unknown PD ID: " << pd_id << std::endl;
        return;
    }

    OSDPReader& reader = it->second;

    AccessRequest req;
    req.reader_id = reader.reader_id;
    req.device_uid = reader.device_uid;
    req.credential = card_data;
    req.credential_type = reader.reader_type;
    req.door_id = std::to_string(reader.door_id);
    req.timestamp = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
    req.osdp_pd_id = pd_id;
    req.format = "wiegand";

    access_callback_(req);
    std::cout << "Card read from PD " << pd_id << ": " << card_data << std::endl;
}

void OSDPConnection::handle_keypress(int pd_id, uint8_t* keys, int length) {
    if (!access_callback_) {
        return;
    }

    auto it = pd_address_to_reader.find(pd_id);
    if (it == pd_address_to_reader.end()) {
        return;
    }

    OSDPReader& reader = it->second;

    std::string pin_str;
    for (int i = 0; i < length; i++) {
        pin_str += static_cast<char>(keys[i]);
    }

    AccessRequest req;
    req.reader_id = reader.reader_id;
    req.device_uid = reader.device_uid;
    req.credential = pin_str;
    req.credential_type = "pin";
    req.door_id = std::to_string(reader.door_id);
    req.timestamp = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
    req.osdp_pd_id = pd_id;
    req.format = "pin";

    access_callback_(req);
    std::cout << "PIN entered on PD " << pd_id << ": " << pin_str << std::endl;
}

} // namespace accessd
