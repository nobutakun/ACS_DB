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
#include <algorithm>
#include <cstdint>

// Include libosdp C API
#include <osdp.h>

// Static callback wrapper for libosdp event callback
static int osdp_event_callback_wrapper(void *arg, int pd, struct osdp_event *ev) {
    accessd::OSDPConnection* conn = static_cast<accessd::OSDPConnection*>(arg);
    // Cast from osdp_event to accessd::osdp_event (same layout)
    conn->handle_osdp_event(pd, *reinterpret_cast<accessd::osdp_event*>(ev));
    return 0;
}

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
    auto it = std::remove_if(readers_.begin(), readers_.end(),
                            [reader_id](const OSDPReader& r) { return r.reader_id == reader_id; });
    if (it != readers_.end()) {
        readers_.erase(it);
        std::cout << "Removed reader ID: " << reader_id << std::endl;
        return true;
    }
    return false;
}

bool OSDPConnection::remove_reader_by_address(int osdp_address) {
    auto it = std::remove_if(readers_.begin(), readers_.end(),
                            [osdp_address](const OSDPReader& r) { return r.osdp_address == osdp_address; });
    if (it != readers_.end()) {
        readers_.erase(it);
        std::cout << "Removed reader with OSDP address: " << osdp_address << std::endl;
        return true;
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

    // Build PD info array using libosdp's osdp_pd_info_t
    std::vector<osdp_pd_info_t> pd_infos;
    pd_address_to_offset.clear();
    pd_address_to_reader.clear();

    // RS485 instances - one per reader
    static RS485 rs485_instances[8];
    static int rs485_index = 0;
    rs485_index = 0;  // Reset on start

    int offset = 0;
    for (const auto& reader : readers_) {
        if (rs485_index < 8) {
            RS485Config config;
            config.port = reader.rs485_port;
            config.baud_rate = reader.baudrate;
            rs485_instances[rs485_index].open(config);
        }

        osdp_pd_info_t info = {0};
        info.baud_rate = reader.baudrate;
        info.address = reader.osdp_address;
        info.flags = 0;

        // Store the RS485 instance pointer for this PD
        pd_address_to_offset[reader.osdp_address] = rs485_index;

        pd_infos.push_back(info);
        pd_address_to_offset[reader.osdp_address] = offset;
        pd_address_to_reader[reader.osdp_address] = reader;
        offset++;
        rs485_index++;
    }

    // Setup channel - shared by all PDs
    // The channel callbacks will use the PD address to select the correct RS485 instance
    struct osdp_channel channel = {0};
    channel.data = this;

    // Store RS485 instances pointer for channel callbacks
    static std::vector<RS485*>* rs485_vector_ptr = nullptr;
    static std::vector<RS485*> rs485_vector;

    rs485_vector.clear();
    for (int i = 0; i < rs485_index && i < 8; i++) {
        rs485_vector.push_back(&rs485_instances[i]);
    }
    rs485_vector_ptr = &rs485_vector;

    channel.send = [](void *data, uint8_t *buf, int len) -> int {
        // In multi-PD setup, libosdp calls this with proper PD context
        // For now, use the first available RS485
        if (rs485_vector_ptr && !rs485_vector_ptr->empty()) {
            RS485* rs485 = (*rs485_vector_ptr)[0];
            if (rs485 && rs485->is_open()) {
                return static_cast<int>(rs485->write(buf, len));
            }
        }
        return -1;
    };

    channel.recv = [](void *data, uint8_t *buf, int maxlen) -> int {
        if (rs485_vector_ptr && !rs485_vector_ptr->empty()) {
            RS485* rs485 = (*rs485_vector_ptr)[0];
            if (rs485 && rs485->is_open()) {
                return static_cast<int>(rs485->read(buf, maxlen));
            }
        }
        return -1;
    };

    // Setup libosdp CP context
    // API: osdp_cp_setup(const osdp_channel *channel, int num_pd, const osdp_pd_info_t *pd_info)
    osdp_ctx_ = osdp_cp_setup(&channel, static_cast<int>(pd_infos.size()), pd_infos.data());
    if (!osdp_ctx_) {
        std::cerr << "Failed to setup OSDP CP" << std::endl;
        return false;
    }

    // Set event callback
    osdp_cp_set_event_callback(osdp_ctx_, osdp_event_callback_wrapper, this);

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

    struct osdp_cmd cmd = {};
    cmd.id = OSDP_CMD_LED;
    cmd.flags = 0;

    cmd.led.reader = 0;
    cmd.led.led_number = led_number;

    // Permanent mode: set LED on or off
    cmd.led.permanent.control_code = 1;
    cmd.led.permanent.on_count = on ? 255 : 0;
    cmd.led.permanent.off_count = on ? 0 : 255;
    cmd.led.permanent.on_color = on ? OSDP_LED_COLOR_GREEN : OSDP_LED_COLOR_NONE;
    cmd.led.permanent.off_color = OSDP_LED_COLOR_NONE;
    cmd.led.permanent.timer_count = 0;

    // Temporary mode: NOP
    cmd.led.temporary.control_code = 0;
    cmd.led.temporary.on_count = 0;
    cmd.led.temporary.off_count = 0;
    cmd.led.temporary.on_color = OSDP_LED_COLOR_NONE;
    cmd.led.temporary.off_color = OSDP_LED_COLOR_NONE;
    cmd.led.temporary.timer_count = 0;

    // Use osdp_cp_submit_command (new API)
    if (osdp_cp_submit_command(osdp_ctx_, it->second, &cmd) != 0) {
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

    struct osdp_cmd cmd = {};
    cmd.id = OSDP_CMD_BUZZER;
    cmd.flags = 0;

    cmd.buzzer.reader = 0;
    cmd.buzzer.control_code = 2;
    cmd.buzzer.on_count = static_cast<uint8_t>(on_time_ms / 100);
    cmd.buzzer.off_count = static_cast<uint8_t>(off_time_ms / 100);
    cmd.buzzer.rep_count = static_cast<uint8_t>(count);

    if (osdp_cp_submit_command(osdp_ctx_, it->second, &cmd) != 0) {
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

    struct osdp_cmd cmd = {};
    cmd.id = OSDP_CMD_TEXT;
    cmd.flags = 0;

    cmd.text.reader = 0;
    cmd.text.control_code = (duration_sec > 0) ? 3 : 1;
    cmd.text.temp_time = static_cast<uint8_t>(duration_sec);
    cmd.text.offset_row = 1;
    cmd.text.offset_col = 1;
    cmd.text.length = std::min(text.size(), sizeof(cmd.text.data) - 1);
    std::strncpy(reinterpret_cast<char*>(cmd.text.data), text.c_str(), sizeof(cmd.text.data) - 1);

    if (osdp_cp_submit_command(osdp_ctx_, it->second, &cmd) != 0) {
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
