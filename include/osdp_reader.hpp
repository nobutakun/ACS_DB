#ifndef ACCESSD_OSDP_READER_HPP
#define ACCESSD_OSDP_READER_HPP

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <cstdint>

namespace accessd {

// OSDP Event types (from libosdp)
enum osdp_event_type {
    OSDP_EVENT_CARDREAD = 1,
    OSDP_EVENT_KEYPRESS,
    OSDP_EVENT_MFGREP,
    OSDP_EVENT_STATUS,
    OSDP_EVENT_NOTIFICATION,
    OSDP_EVENT_SENTINEL
};

// Forward declarations for libosdp types
struct osdp_event {
    enum osdp_event_type type;
    uint32_t flags;
    union {
        struct {
            int reader_no;
            int length;
            uint8_t data[64];
        } keypress;
        struct {
            int reader_no;
            int format;
            int direction;
            int length;
            uint8_t data[64];
        } cardread;
    };
};

// OSDP PD (Peripheral Device) - Reader info
struct OSDPReader {
    // Database fields
    int reader_id;              // Database ID (autoincrement)
    int device_id;              // Device ID reference
    int door_id;                // Associated door ID
    std::string device_uid;     // Unique identifier
    std::string name;
    std::string reader_type;    // 'card', 'fingerprint', 'face', 'qr', 'multi'
    std::string direction;      // 'entry', 'exit', 'both'

    // Connection type
    std::string connection_type; // 'tcp' or 'osdp'

    // TCP/IP connection fields
    std::string ip_address;
    int port;

    // OSDP connection fields
    int osdp_address;           // PD address (0-127)
    int baudrate;               // 9600, 19200, 38400, 57600, 115200
    bool use_encryption;        // AES-128
    std::vector<uint8_t> master_key;  // SCBK: 16 bytes
    std::string rs485_port;     // /dev/ttyUSB0, /dev/ttyAMA0

    // Status
    std::string status;         // 'online', 'offline', 'error'

    // Reader capabilities (from PD discovery)
    bool supports_card;
    bool supports_pin;
    bool supports_biometric;
    bool supports_led;
    bool supports_buzzer;
    bool supports_display;

    // PD identification (from OSDP CMD_ID response)
    uint32_t vendor_code;
    uint32_t firmware_version;
    std::string serial_number;

    // Check if this is an OSDP reader
    bool is_osdp() const { return connection_type == "osdp"; }

    // Check if this is a TCP reader
    bool is_tcp() const { return connection_type == "tcp"; }
};

// Access Request from Reader
struct AccessRequest {
    int reader_id;              // OSDP PD ID
    std::string device_uid;     // Reader unique ID
    std::string credential;     // Card number, PIN, etc.
    std::string credential_type;// card, pin, fingerprint, face
    std::string door_id;        // Associated door
    uint64_t timestamp;         // Unix timestamp

    // OSDP specific
    int osdp_pd_id;
    std::string format;         // Wiegand format, OSDP format
};

// OSDP Event Callback
using AccessRequestCallback = std::function<void(const AccessRequest&)>;

class OSDPConnection {
public:
    OSDPConnection();
    ~OSDPConnection();

    // Initialize RS485 and OSDP
    bool init(const std::string& serial_port, int baudrate = 9600);

    // Add/Remove reader (PD)
    bool add_reader(const OSDPReader& reader);
    bool remove_reader(int reader_id);
    bool remove_reader_by_address(int osdp_address);

    // Start/Stop OSDP loop
    bool start();
    void stop();

    // Register callback for access requests
    void set_access_callback(AccessRequestCallback callback);

    // Get connected readers
    std::vector<OSDPReader> get_readers() const;

    // Send commands to reader
    bool led(int pd_id, int led_number, bool on);
    bool buzzer(int pd_id, int on_time_ms, int off_time_ms, int count);
    bool display_text(int pd_id, const std::string& text, int duration_sec);

    // Event handlers (called from libosdp callback)
    void handle_osdp_event(int pd_offset, struct osdp_event& event);
    void handle_card_read(int pd_id, const std::string& card_data);
    void handle_keypress(int pd_id, uint8_t* keys, int length);

private:
    std::string serial_port_;
    int baudrate_;
    bool running_;

    std::vector<OSDPReader> readers_;
    AccessRequestCallback access_callback_;

    // OSDP context (libosdp)
    void* osdp_ctx_;

    // Private methods
    bool setup_rs485();
    void osdp_loop();
};

} // namespace accessd

#endif // ACCESSD_OSDP_READER_HPP
