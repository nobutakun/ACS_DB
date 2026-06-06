#include "osdp/osdp_channel.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>

namespace accessd {
namespace osdp {

// CRC-16 table for OSDP (polynomial 0x8005)
static const uint16_t CRC16_TABLE[256] = {
    0x0000, 0x8005, 0x800F, 0x000A, 0x801B, 0x001E, 0x0014, 0x8011,
    0x8033, 0x0036, 0x003C, 0x8039, 0x0028, 0x802D, 0x8027, 0x0022,
    0x8063, 0x0066, 0x006C, 0x8069, 0x0078, 0x807D, 0x8077, 0x0072,
    0x0050, 0x8055, 0x805F, 0x005A, 0x804B, 0x004E, 0x0044, 0x8041,
    0x80C3, 0x00C6, 0x00CC, 0x80C9, 0x00D8, 0x80DD, 0x80D7, 0x00D2,
    0x00F0, 0x80F5, 0x80FF, 0x00FA, 0x80EB, 0x00EE, 0x00E4, 0x80E1,
    0x00A0, 0x80A5, 0x80AF, 0x00AA, 0x80BB, 0x00BE, 0x00B4, 0x80B1,
    0x8093, 0x0096, 0x009C, 0x8099, 0x0088, 0x808D, 0x8087, 0x0082,
    0x8183, 0x0186, 0x018C, 0x8189, 0x0198, 0x819D, 0x8197, 0x0192,
    0x01B0, 0x81B5, 0x81BF, 0x01BA, 0x81AB, 0x01AE, 0x01A4, 0x81A1,
    0x01E0, 0x81E5, 0x81EF, 0x01EA, 0x81FB, 0x01FE, 0x01F4, 0x81F1,
    0x81D3, 0x01D6, 0x01DC, 0x81D9, 0x01C8, 0x81CD, 0x81C7, 0x01C2,
    0x0140, 0x8145, 0x814F, 0x014A, 0x815B, 0x015E, 0x0154, 0x8151,
    0x8173, 0x0176, 0x017C, 0x8179, 0x0168, 0x816D, 0x8167, 0x0162,
    0x8123, 0x0126, 0x012C, 0x8129, 0x0138, 0x813D, 0x8137, 0x0132,
    0x0110, 0x8115, 0x811F, 0x011A, 0x810B, 0x010E, 0x0104, 0x8101,
    0x8303, 0x0306, 0x030C, 0x8309, 0x0318, 0x831D, 0x8317, 0x0312,
    0x0330, 0x8335, 0x833F, 0x033A, 0x832B, 0x032E, 0x0324, 0x8321,
    0x0360, 0x8365, 0x836F, 0x036A, 0x837B, 0x037E, 0x0374, 0x8371,
    0x8353, 0x0356, 0x035C, 0x8359, 0x0348, 0x834D, 0x8347, 0x0342,
    0x03C0, 0x83C5, 0x83CF, 0x03CA, 0x83DB, 0x03DE, 0x03D4, 0x83D1,
    0x83F3, 0x03F6, 0x03FC, 0x83F9, 0x03E8, 0x83ED, 0x83E7, 0x03E2,
    0x83A3, 0x03A6, 0x03AC, 0x83A9, 0x03B8, 0x83BD, 0x83B7, 0x03B2,
    0x0390, 0x8395, 0x839F, 0x039A, 0x838B, 0x038E, 0x0384, 0x8381,
    0x0280, 0x8285, 0x828F, 0x028A, 0x829B, 0x029E, 0x0294, 0x8291,
    0x82B3, 0x02B6, 0x02BC, 0x82B9, 0x02A8, 0x82AD, 0x82A7, 0x02A2,
    0x82E3, 0x02E6, 0x02EC, 0x82E9, 0x02F8, 0x82FD, 0x82F7, 0x02F2,
    0x02D0, 0x82D5, 0x82DF, 0x02DA, 0x82CB, 0x02CE, 0x02C4, 0x82C1,
    0x8243, 0x0246, 0x024C, 0x8249, 0x0258, 0x825D, 0x8257, 0x0252,
    0x0270, 0x8275, 0x827F, 0x027A, 0x826B, 0x026E, 0x0264, 0x8261,
    0x0220, 0x8225, 0x822F, 0x022A, 0x823B, 0x023E, 0x0234, 0x8231,
    0x8203, 0x0206, 0x020C, 0x8209, 0x0218, 0x821D, 0x8217, 0x0212,
    0x8403, 0x0406, 0x040C, 0x8409, 0x0418, 0x841D, 0x8417, 0x0412,
    0x0430, 0x8435, 0x843F, 0x043A, 0x842B, 0x042E, 0x0424, 0x8421,
    0x0460, 0x8465, 0x846F, 0x046A, 0x847B, 0x047E, 0x0474, 0x8471,
    0x8453, 0x0456, 0x045C, 0x8459, 0x0448, 0x844D, 0x8447, 0x0442,
    0x84C3, 0x04C6, 0x04CC, 0x84C9, 0x04D8, 0x84DD, 0x84D7, 0x04D2,
    0x04F0, 0x84F5, 0x84FF, 0x04FA, 0x84EB, 0x04EE, 0x04E4, 0x84E1,
    0x84A3, 0x04A6, 0x04AC, 0x84A9, 0x04B8, 0x84BD, 0x84B7, 0x04B2,
    0x0490, 0x8495, 0x849F, 0x049A, 0x848B, 0x048E, 0x0484, 0x8481,
    0x0580, 0x8585, 0x858F, 0x058A, 0x859B, 0x059E, 0x0594, 0x8591,
    0x85B3, 0x05B6, 0x05BC, 0x85B9, 0x05A8, 0x85AD, 0x85A7, 0x05A2,
    0x85E3, 0x05E6, 0x05EC, 0x85E9, 0x05F8, 0x85FD, 0x85F7, 0x05F2,
    0x05D0, 0x85D5, 0x85DF, 0x05DA, 0x85CB, 0x05CE, 0x05C4, 0x85C1,
    0x8543, 0x0546, 0x054C, 0x8549, 0x0558, 0x855D, 0x8557, 0x0552,
    0x0570, 0x8575, 0x857F, 0x057A, 0x856B, 0x056E, 0x0564, 0x8561,
    0x0520, 0x8525, 0x852F, 0x052A, 0x853B, 0x053E, 0x0534, 0x8531,
    0x8503, 0x0506, 0x050C, 0x8509, 0x0518, 0x851D, 0x8517, 0x0512,
    0x8703, 0x0706, 0x070C, 0x8709, 0x0718, 0x871D, 0x8717, 0x0712,
    0x0730, 0x8735, 0x873F, 0x073A, 0x872B, 0x072E, 0x0724, 0x8721,
    0x0760, 0x8765, 0x876F, 0x076A, 0x877B, 0x077E, 0x0774, 0x8771,
    0x8753, 0x0756, 0x075C, 0x8759, 0x0748, 0x874D, 0x8747, 0x0742,
    0x07C0, 0x87C5, 0x87CF, 0x07CA, 0x87DB, 0x07DE, 0x07D4, 0x87D1,
    0x87F3, 0x07F6, 0x07FC, 0x87F9, 0x07E8, 0x87ED, 0x87E7, 0x07E2,
    0x87A3, 0x07A6, 0x07AC, 0x87A9, 0x07B8, 0x87BD, 0x87B7, 0x07B2,
    0x0790, 0x8795, 0x879F, 0x079A, 0x878B, 0x078E, 0x0784, 0x8781,
    0x0680, 0x8685, 0x868F, 0x068A, 0x869B, 0x069E, 0x0694, 0x8691,
    0x86B3, 0x06B6, 0x06BC, 0x86B9, 0x06A8, 0x86AD, 0x86A7, 0x06A2,
    0x86E3, 0x06E6, 0x06EC, 0x86E9, 0x06F8, 0x86FD, 0x86F7, 0x06F2,
    0x06D0, 0x86D5, 0x86DF, 0x06DA, 0x86CB, 0x06CE, 0x06C4, 0x86C1,
    0x8643, 0x0646, 0x064C, 0x8649, 0x0658, 0x865D, 0x8657, 0x0652,
    0x0670, 0x8675, 0x867F, 0x067A, 0x866B, 0x066E, 0x0664, 0x8661,
    0x0620, 0x8625, 0x862F, 0x062A, 0x863B, 0x063E, 0x0634, 0x8631,
    0x8603, 0x0606, 0x060C, 0x8609, 0x0618, 0x861D, 0x8617, 0x0612
};

OSDPChannel::OSDPChannel()
    : state_(STATE_IDLE)
    , running_(false)
    , tx_seq_(0)
    , rx_seq_(0)
    , retry_count_(0)
    , last_poll_time_(0)
    , last_reply_time_(0)
    , last_activity_time_(0)
{
    std::memset(&stats_, 0, sizeof(stats_));
}

OSDPChannel::~OSDPChannel() {
    stop();
}

bool OSDPChannel::init(const std::string& port, const ChannelConfig& config) {
    config_ = config;

    // Create and open serial port
    serial_ = std::make_unique<SerialPort>();
    SerialConfig serial_cfg(port, config.baudrate);
    serial_cfg.rs485 = true;  // Enable RS-485 mode

    if (!serial_->open(serial_cfg)) {
        std::cerr << "OSDPChannel: Failed to open serial port: " << port << std::endl;
        return false;
    }

    set_state(STATE_CONNECTING);
    std::cout << "OSDPChannel: Initialized for PD " << (int)config.pd_address
              << " on " << port << std::endl;

    return true;
}

void OSDPChannel::close() {
    stop();
    if (serial_) {
        serial_->close();
    }
    set_state(STATE_IDLE);
}

bool OSDPChannel::is_connected() const {
    return state_ == STATE_CONNECTED || state_ == STATE_WAIT_REPLY;
}

void OSDPChannel::set_event_callback(ChannelEventCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    event_callback_ = callback;
}

bool OSDPChannel::start() {
    if (running_) {
        std::cerr << "OSDPChannel: Already running" << std::endl;
        return false;
    }

    running_ = true;

    // Start processing thread
    std::thread processing_thread([this]() {
        this->processing_loop();
    });
    processing_thread.detach();

    std::cout << "OSDPChannel: Started for PD " << (int)config_.pd_address << std::endl;
    return true;
}

void OSDPChannel::stop() {
    if (!running_) return;

    running_ = false;
    set_state(STATE_IDLE);
    std::cout << "OSDPChannel: Stopped for PD " << (int)config_.pd_address << std::endl;
}

// ========== OSDP Commands ==========

bool OSDPChannel::cmd_poll() {
    return send_command(CMD_POLL, {});
}

bool OSDPChannel::cmd_id() {
    return send_command(CMD_ID, {});
}

bool OSDPChannel::cmd_cap() {
    return send_command(CMD_CAP, {});
}

bool OSDPChannel::cmd_led(const LedParams& params) {
    std::vector<uint8_t> data;
    data.push_back(params.reader_number);
    data.push_back(params.led_number);
    data.push_back(params.temporary_mode);
    data.push_back(params.temporary_on_count);
    data.push_back(params.temporary_off_count);
    data.push_back(params.temporary_on_color);
    data.push_back(params.temporary_off_color);
    data.push_back(params.permanent_mode);
    data.push_back(params.permanent_on_count);
    data.push_back(params.permanent_off_count);
    data.push_back(params.permanent_on_color);
    data.push_back(params.permanent_off_color);
    return send_command(CMD_LED, data);
}

bool OSDPChannel::cmd_buzzer(const BuzzerParams& params) {
    std::vector<uint8_t> data;
    data.push_back(params.buzzer_number);
    data.push_back(params.on_count);
    data.push_back(params.off_count);
    data.push_back(params.repeat_count);
    return send_command(CMD_BUZ, data);
}

bool OSDPChannel::cmd_output(const OutputControlParams& params) {
    std::vector<uint8_t> data;
    data.push_back(params.output_number);
    data.push_back(params.control_code);
    data.push_back(params.on_time & 0xFF);
    data.push_back((params.on_time >> 8) & 0xFF);
    data.push_back(params.off_time & 0xFF);
    data.push_back((params.off_time >> 8) & 0xFF);
    return send_command(CMD_OUT, data);
}

bool OSDPChannel::cmd_text(const TextParams& params) {
    std::vector<uint8_t> data;
    data.push_back(params.reader_number);
    data.push_back(params.row_number);
    data.push_back(params.col_number);
    data.push_back(params.command);
    data.push_back(params.timeout);
    data.insert(data.end(), params.text.begin(), params.text.end());
    return send_command(CMD_TEXT, data);
}

// ========== Internal Methods ==========

bool OSDPChannel::send_command(Command cmd, const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (state_ == STATE_OFFLINE || state_ == STATE_ERROR) {
        std::cerr << "OSDPChannel: Cannot send in current state" << std::endl;
        return false;
    }

    // Build packet
    Packet packet = build_packet(cmd, data);

    // Encrypt if secure channel enabled
    if (config_.use_encryption) {
        if (!encrypt_packet(packet)) {
            std::cerr << "OSDPChannel: Encryption failed" << std::endl;
            return false;
        }
    }

    // Serialize packet
    std::vector<uint8_t> buffer;
    buffer.push_back(packet.som);
    buffer.push_back(packet.address);
    buffer.push_back(packet.length_lsb);
    buffer.push_back(packet.length_msb);
    buffer.push_back(packet.command);
    buffer.insert(buffer.end(), packet.data.begin(), packet.data.end());
    buffer.push_back(packet.crc & 0xFF);
    buffer.push_back((packet.crc >> 8) & 0xFF);

    // Send to serial
    int written = serial_->write(buffer);
    if (written < 0) {
        std::cerr << "OSDPChannel: Write failed" << std::endl;
        stats_.crc_errors++;  // Count as error
        return false;
    }

    stats_.bytes_sent += buffer.size();
    stats_.polls_sent++;

    // Store pending command for retry
    pending_cmd_ = packet;
    retry_count_ = 0;
    set_state(STATE_WAIT_REPLY);

    return true;
}

bool OSDPChannel::receive_reply(Packet& packet) {
    uint8_t buffer[128];
    int idx = 0;

    // Wait for SOM (0xFF)
    int bytes_read = serial_->read(&buffer[0], 1, config_.reply_timeout_ms);
    if (bytes_read <= 0) {
        return false;  // Timeout or error
    }

    if (buffer[0] != MARKER_SOM) {
        std::cerr << "OSDPChannel: Invalid SOM" << std::endl;
        return false;
    }

    // Read remaining bytes
    buffer[idx++] = buffer[0];

    // Read address
    bytes_read = serial_->read(&buffer[idx], 1, config_.reply_timeout_ms);
    if (bytes_read <= 0) return false;
    idx++;

    // Read length
    bytes_read = serial_->read(&buffer[idx], 2, config_.reply_timeout_ms);
    if (bytes_read <= 0) return false;
    idx += 2;

    uint16_t length = buffer[2] | (buffer[3] << 8);
    if (length < 5 || length > 128) {
        std::cerr << "OSDPChannel: Invalid length" << std::endl;
        return false;
    }

    // Read remaining bytes (including CRC)
    int remaining = length - 4;  // Subtract SOM, ADDR, LEN_L, LEN_H
    bytes_read = serial_->read(&buffer[idx], remaining, config_.reply_timeout_ms);
    if (bytes_read != remaining) {
        std::cerr << "OSDPChannel: Incomplete packet" << std::endl;
        return false;
    }
    idx += remaining;

    // Parse packet
    return parse_packet(std::vector<uint8_t>(buffer, buffer + idx), packet);
}

void OSDPChannel::process_packet(const Packet& packet) {
    last_activity_time_ = get_current_time_ms();
    stats_.replies_received++;
    stats_.bytes_received += packet.get_length();

    // Update state
    if (state_ == STATE_WAIT_REPLY) {
        set_state(STATE_CONNECTED);
    }

    // Check reply type
    Reply reply_type = static_cast<Reply>(packet.command);

    switch (reply_type) {
        case REPLY_ACK:
            // Command acknowledged
            break;

        case REPLY_NAK:
            handle_nak(packet);
            break;

        case REPLY_PDID:
            // Parse PD identification
            if (packet.data.size() >= 12) {
                pd_id_ = std::make_unique<PDId>();
                std::memcpy(pd_id_->vendor_code, &packet.data[0], 3);
                pd_id_->model = packet.data[3];
                pd_id_->version = packet.data[4] | (packet.data[5] << 8) |
                                  (packet.data[6] << 16) | (packet.data[7] << 24);
                std::memcpy(pd_id_->serial_number, &packet.data[8], 4);
                pd_id_->firmware_version = packet.data[12];

                std::cout << "OSDPChannel: PD ID received - Vendor: "
                          << pd_id_->get_vendor_code() << std::endl;

                // Emit PD online event
                Event event;
                event.type = EVENT_PD_ONLINE;
                event.pd_address = config_.pd_address;
                event.timestamp = last_activity_time_;
                emit_event(event);
            }
            break;

        case REPLY_PDCAP:
            // Parse PD capabilities
            pd_capabilities_ = std::make_unique<PDCapabilities>();
            if (packet.data.size() >= 2) {
                pd_capabilities_->max_command_code = packet.data[0];
                pd_capabilities_->max_reply_code = packet.data[1];

                // Parse capability entries
                for (size_t i = 2; i < packet.data.size(); i += 3) {
                    PDCapability cap;
                    cap.function_code = packet.data[i];
                    cap.compliance_level = packet.data[i + 1];
                    cap.num_items = packet.data[i + 2];
                    pd_capabilities_->caps.push_back(cap);
                }

                std::cout << "OSDPChannel: PD capabilities - "
                          << pd_capabilities_->caps.size() << " capabilities" << std::endl;
            }
            break;

        case REPLY_RAW:
            handle_card_data(packet);
            break;

        case REPLY_KEYPAD:
            handle_keypad_data(packet);
            break;

        default:
            std::cout << "OSDPChannel: Unknown reply 0x"
                      << std::hex << (int)reply_type << std::dec << std::endl;
            break;
    }

    // Clear pending command on success
    retry_count_ = 0;
    pending_cmd_ = Packet();
}

Packet OSDPChannel::build_packet(Command cmd, const std::vector<uint8_t>& data) {
    Packet packet;
    packet.som = MARKER_SOM;
    packet.address = config_.pd_address;
    packet.command = static_cast<uint8_t>(cmd);
    packet.data = data;

    // Calculate length
    uint16_t length = 5 + data.size();  // SOM + ADDR + LEN + CMD + DATA + CRC
    packet.set_length(length);

    // Calculate CRC
    std::vector<uint8_t> crc_data;
    crc_data.push_back(packet.address);
    crc_data.push_back(packet.length_lsb);
    crc_data.push_back(packet.length_msb);
    crc_data.push_back(packet.command);
    crc_data.insert(crc_data.end(), data.begin(), data.end());
    packet.crc = calculate_crc(crc_data.data(), crc_data.size());

    return packet;
}

bool OSDPChannel::parse_packet(const std::vector<uint8_t>& buffer, Packet& packet) {
    if (buffer.size() < 6) {
        return false;  // Minimum packet size
    }

    packet.som = buffer[0];
    packet.address = buffer[1];
    packet.length_lsb = buffer[2];
    packet.length_msb = buffer[3];
    packet.command = buffer[4];

    uint16_t data_len = packet.get_length() - 6;  // Subtract header and CRC
    if (data_len > 0 && buffer.size() >= 6 + data_len) {
        packet.data.assign(buffer.begin() + 5, buffer.begin() + 5 + data_len);
    }

    // Extract CRC
    packet.crc = buffer[buffer.size() - 2] | (buffer[buffer.size() - 1] << 8);

    // Validate CRC
    std::vector<uint8_t> crc_data(buffer.begin() + 1, buffer.end() - 2);
    uint16_t calc_crc = calculate_crc(crc_data.data(), crc_data.size());

    if (calc_crc != packet.crc) {
        std::cerr << "OSDPChannel: CRC mismatch - expected 0x"
                  << std::hex << calc_crc << ", got 0x" << packet.crc << std::dec << std::endl;
        stats_.crc_errors++;
        return false;
    }

    return packet.is_valid();
}

uint16_t OSDPChannel::calculate_crc(const uint8_t* data, size_t length) {
    uint16_t crc = 0;
    for (size_t i = 0; i < length; i++) {
        uint8_t index = (crc ^ data[i]) & 0xFF;
        crc = (crc >> 8) ^ CRC16_TABLE[index];
    }
    return crc;
}

bool OSDPChannel::encrypt_packet(Packet& packet) {
    // TODO: Implement AES-128 encryption for OSDP secure channel
    // For now, just mark as encrypted
    return true;
}

bool OSDPChannel::decrypt_packet(Packet& packet) {
    // TODO: Implement AES-128 decryption
    return true;
}

void OSDPChannel::emit_event(Event& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (event_callback_) {
        event_callback_(event);
    }
}

void OSDPChannel::processing_loop() {
    std::cout << "OSDPChannel: Processing loop started for PD "
              << (int)config_.pd_address << std::endl;

    set_state(STATE_CONNECTED);

    // Initial PD discovery
    cmd_id();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    cmd_cap();

    uint64_t last_poll = 0;

    while (running_) {
        uint64_t now = get_current_time_ms();

        // Poll PD at regular interval
        if (now - last_poll >= config_.poll_timeout_ms) {
            if (!cmd_poll()) {
                // Poll failed - check if offline
                if (check_offline()) {
                    set_state(STATE_OFFLINE);
                }
            }
            last_poll = now;
        }

        // Check for incoming data
        if (serial_->available() > 0) {
            Packet packet;
            if (receive_reply(packet)) {
                process_packet(packet);
            }
        }

        // Small sleep to prevent CPU spin
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << "OSDPChannel: Processing loop stopped" << std::endl;
}

void OSDPChannel::set_state(ChannelState state) {
    if (state_ != state) {
        std::cout << "OSDPChannel: State " << state_ << " -> " << state << std::endl;
        state_ = state;
    }
}

bool OSDPChannel::check_offline() {
    uint64_t now = get_current_time_ms();
    if (now - last_activity_time_ > config_.offline_threshold_ms) {
        // Emit offline event
        Event event;
        event.type = EVENT_PD_OFFLINE;
        event.pd_address = config_.pd_address;
        event.timestamp = now;
        event.message = "PD offline - no response";
        emit_event(event);
        return true;
    }
    return false;
}

void OSDPChannel::handle_card_data(const Packet& packet) {
    if (packet.data.size() < 2) {
        return;
    }

    Event event;
    event.type = EVENT_CARD_DATA;
    event.pd_address = config_.pd_address;
    event.timestamp = get_current_time_ms();

    event.card_data.reader_number = packet.data[0];
    event.card_data.format = packet.data[1];

    // Extract card data (remaining bytes)
    if (packet.data.size() > 2) {
        event.card_data.raw_data.assign(packet.data.begin() + 2, packet.data.end());
        event.card_data.length = event.card_data.raw_data.size() * 8;  // bits
    }

    event.message = "Card read: " + event.card_data.to_hex_string();
    emit_event(event);
}

void OSDPChannel::handle_keypad_data(const Packet& packet) {
    if (packet.data.size() < 2) {
        return;
    }

    Event event;
    event.type = EVENT_KEYPAD_DATA;
    event.pd_address = config_.pd_address;
    event.timestamp = get_current_time_ms();

    event.keypad_data.reader_number = packet.data[0];
    event.keypad_data.length = packet.data[1];

    // Extract key presses
    if (packet.data.size() > 2) {
        event.keypad_data.keys.assign(packet.data.begin() + 2, packet.data.end());
    }

    emit_event(event);
}

void OSDPChannel::handle_nak(const Packet& packet) {
    if (packet.data.empty()) {
        return;
    }

    NakError error = static_cast<NakError>(packet.data[0]);

    Event event;
    event.type = EVENT_NAK;
    event.pd_address = config_.pd_address;
    event.timestamp = get_current_time_ms();
    event.nak_error = error;

    switch (error) {
        case NAK_ERR_BAD_CRC:
            event.message = "NAK: Bad CRC";
            stats_.crc_errors++;
            break;
        case NAK_ERR_BAD_CMD:
            event.message = "NAK: Unknown command";
            break;
        case NAK_ERR_BAD_SEQ:
            event.message = "NAK: Bad sequence number";
            break;
        default:
            event.message = "NAK: Unknown error";
            break;
    }

    stats_.nak_received++;
    emit_event(event);
}

bool OSDPChannel::retry_pending() {
    if (retry_count_ >= config_.retry_count) {
        return false;
    }

    retry_count_++;

    // Re-send pending command
    std::vector<uint8_t> buffer;
    buffer.push_back(pending_cmd_.som);
    buffer.push_back(pending_cmd_.address);
    buffer.push_back(pending_cmd_.length_lsb);
    buffer.push_back(pending_cmd_.length_msb);
    buffer.push_back(pending_cmd_.command);
    buffer.insert(buffer.end(), pending_cmd_.data.begin(), pending_cmd_.data.end());
    buffer.push_back(pending_cmd_.crc & 0xFF);
    buffer.push_back((pending_cmd_.crc >> 8) & 0xFF);

    return serial_->write(buffer) > 0;
}

uint64_t OSDPChannel::get_current_time_ms() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

} // namespace osdp
} // namespace accessd
