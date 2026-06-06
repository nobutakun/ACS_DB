#include "osdp/osdp_commands.hpp"
#include <iostream>
#include <cstring>

namespace accessd {
namespace osdp {

// ============================================================================
// OSDPCommands Implementation
// ============================================================================

OSDPCommands::OSDPCommands()
    : pd_address_(0)
    , last_command_(static_cast<Command>(0))
{
}

OSDPCommands::~OSDPCommands() {
    clear_callback();
}

void OSDPCommands::set_pd_address(uint8_t address) {
    if (OSDPCommandFactory::validate_address(address)) {
        pd_address_ = address;
    }
}

bool OSDPCommands::poll(CommandCallback callback) {
    pending_callback_ = callback;
    last_command_ = CMD_POLL;
    // Packet will be built and sent by OSDPChannel
    return true;
}

bool OSDPCommands::cmd_id(CommandCallback callback) {
    pending_callback_ = callback;
    last_command_ = CMD_ID;
    return true;
}

bool OSDPCommands::cmd_cap(CommandCallback callback) {
    pending_callback_ = callback;
    last_command_ = CMD_CAP;
    return true;
}

bool OSDPCommands::cmd_comset(uint32_t baudrate, CommandCallback callback) {
    pending_callback_ = callback;
    last_command_ = CMD_COMSET;

    // Build COMSET data
    std::vector<uint8_t> data;
    // baudrate encoding (1 = 9600, 2 = 19200, 3 = 38400, 4 = 57600, 5 = 115200)
    uint8_t baud_code = 1;  // Default 9600
    switch (baudrate) {
        case 9600:   baud_code = 1; break;
        case 19200:  baud_code = 2; break;
        case 38400:  baud_code = 3; break;
        case 57600:  baud_code = 4; break;
        case 115200: baud_code = 5; break;
        default:    baud_code = 1; break;
    }
    data.push_back(baud_code);
    // TODO: Store data for packet building

    return true;
}

bool OSDPCommands::cmd_led(const LedParams& params, CommandCallback callback) {
    if (!OSDPCommandFactory::validate_led_params(params)) {
        std::cerr << "OSDPCommands: Invalid LED parameters" << std::endl;
        return false;
    }

    pending_callback_ = callback;
    last_command_ = CMD_LED;
    return true;
}

bool OSDPCommands::cmd_buzzer(const BuzzerParams& params, CommandCallback callback) {
    if (!OSDPCommandFactory::validate_buzzer_params(params)) {
        std::cerr << "OSDPCommands: Invalid buzzer parameters" << std::endl;
        return false;
    }

    pending_callback_ = callback;
    last_command_ = CMD_BUZ;
    return true;
}

bool OSDPCommands::cmd_output(const OutputControlParams& params, CommandCallback callback) {
    if (!OSDPCommandFactory::validate_output_params(params)) {
        std::cerr << "OSDPCommands: Invalid output parameters" << std::endl;
        return false;
    }

    pending_callback_ = callback;
    last_command_ = CMD_OUT;
    return true;
}

bool OSDPCommands::cmd_text(const TextParams& params, CommandCallback callback) {
    if (!OSDPCommandFactory::validate_text_params(params)) {
        std::cerr << "OSDPCommands: Invalid text parameters" << std::endl;
        return false;
    }

    pending_callback_ = callback;
    last_command_ = CMD_TEXT;
    return true;
}

bool OSDPCommands::cmd_keyset(const std::vector<uint8_t>& scbk, CommandCallback callback) {
    if (!OSDPCommandFactory::validate_scbk(scbk)) {
        std::cerr << "OSDPCommands: Invalid SCBK" << std::endl;
        return false;
    }

    pending_callback_ = callback;
    last_command_ = CMD_KEYSET;
    return true;
}

bool OSDPCommands::cmd_challenge(const std::vector<uint8_t>& random, CommandCallback callback) {
    pending_callback_ = callback;
    last_command_ = CMD_CHLNG;
    return true;
}

bool OSDPCommands::cmd_scrypt(const std::vector<uint8_t>& cryptogram, CommandCallback callback) {
    pending_callback_ = callback;
    last_command_ = CMD_SCRYPT;
    return true;
}

bool OSDPCommands::cmd_file_transfer(uint8_t file_id, uint32_t offset,
                                      const std::vector<uint8_t>& data,
                                      CommandCallback callback) {
    pending_callback_ = callback;
    last_command_ = CMD_DATA;
    return true;
}

bool OSDPCommands::cmd_abort(CommandCallback callback) {
    pending_callback_ = callback;
    last_command_ = CMD_ABORT;
    return true;
}

bool OSDPCommands::cmd_mfg(const uint8_t mfg_code[3], uint8_t mfg_command,
                            const std::vector<uint8_t>& data,
                            CommandCallback callback) {
    pending_callback_ = callback;
    last_command_ = CMD_MFG;
    return true;
}

bool OSDPCommands::process_reply(const Packet& reply) {
    // Store last reply
    last_reply_ = std::make_unique<Packet>(reply);

    // Check reply type
    Reply reply_type = static_cast<Reply>(reply.command);

    bool success = true;
    switch (reply_type) {
        case REPLY_ACK:
            success = true;
            break;

        case REPLY_NAK:
            success = false;
            break;

        case REPLY_PDID:
        case REPLY_PDCAP:
        case REPLY_RAW:
        case REPLY_KEYPAD:
        case REPLY_COM:
            success = true;
            break;

        default:
            if (reply_type >= 0x40 && reply_type < 0x50) {
                // It's a valid reply
                success = true;
            } else {
                success = false;
            }
            break;
    }

    // Invoke callback if set
    if (pending_callback_) {
        pending_callback_(success, reply);
        clear_callback();
    }

    return success;
}

void OSDPCommands::handle_reply(const Packet& reply) {
    process_reply(reply);
}

void OSDPCommands::clear_callback() {
    pending_callback_ = nullptr;
}

// ============================================================================
// OSDPReplies Implementation
// ============================================================================

bool OSDPReplies::extract_pdid(const Packet& reply, PDId& pdid) {
    return PacketParser::parse_pdid(reply, pdid);
}

bool OSDPReplies::extract_pdcap(const Packet& reply, PDCapabilities& caps) {
    return PacketParser::parse_pdcap(reply, caps);
}

bool OSDPReplies::extract_card_data(const Packet& reply, CardData& data) {
    return PacketParser::parse_card_data(reply, data);
}

bool OSDPReplies::extract_card_formatted(const Packet& reply, CardDataFormatted& data) {
    if (reply.command != REPLY_FMT || reply.data.size() < 7) {
        return false;
    }

    data.reader_number = reply.data[0];
    data.direction = reply.data[1];
    data.format = reply.data[2];

    // Extract card number (32-bit)
    data.card_number = reply.data[3] | (reply.data[4] << 8) |
                       (reply.data[5] << 16) | (reply.data[6] << 24);

    // Extract facility code if present
    if (reply.data.size() >= 9) {
        data.facility_code = reply.data[7] | (reply.data[8] << 8);
    }

    return true;
}

bool OSDPReplies::extract_keypad_data(const Packet& reply, KeypadData& data) {
    return PacketParser::parse_keypad_data(reply, data);
}

bool OSDPReplies::extract_biometric_data(const Packet& reply, BiometricData& data) {
    if (reply.command != REPLY_BIOREADR && reply.command != REPLY_BIOMATCHR) {
        return false;
    }

    if (reply.data.size() < 4) {
        return false;
    }

    data.reader_number = reply.data[0];
    data.biometric_type = reply.data[1];
    data.biometric_format = reply.data[2];
    data.quality = reply.data[3];

    // Extract template ID for match result
    if (reply.command == REPLY_BIOMATCHR && reply.data.size() >= 5) {
        data.template_id = reply.data[4];
        if (reply.data.size() >= 6) {
            data.match_result = reply.data[5];
        }
    }

    // Extract biometric data
    if (reply.data.size() > 6) {
        data.data.assign(reply.data.begin() + 6, reply.data.end());
    }

    return true;
}

bool OSDPReplies::extract_lstat(const Packet& reply, PDLocalStatus& status) {
    if (reply.command != REPLY_LSTATR || reply.data.size() < 4) {
        return false;
    }

    status.tamper_status = reply.data[0];
    status.power_status = reply.data[1];
    status.led_status = reply.data[2];
    status.buzzer_status = reply.data[3];

    return true;
}

bool OSDPReplies::extract_istat(const Packet& reply, PDInputStatus& status) {
    if (reply.command != REPLY_IASTR || reply.data.size() < 4) {
        return false;
    }

    status.input_mask = reply.data[0] | (reply.data[1] << 8) |
                      (reply.data[2] << 16) | (reply.data[3] << 24);

    return true;
}

bool OSDPReplies::extract_ostat(const Packet& reply, PDOutputStatus& status) {
    if (reply.command != REPLY_OSTATR || reply.data.size() < 4) {
        return false;
    }

    status.output_mask = reply.data[0] | (reply.data[1] << 8) |
                       (reply.data[2] << 16) | (reply.data[3] << 24);

    return true;
}

bool OSDPReplies::extract_rstat(const Packet& reply, PDReaderStatus& status) {
    if (reply.command != REPLY_RSTATR || reply.data.size() < 4) {
        return false;
    }

    status.reader_status = reply.data[0];
    status.tamper_status = reply.data[1];
    status.power_status = reply.data[2];

    return true;
}

bool OSDPReplies::extract_nak_error(const Packet& reply, NakError& error) {
    return PacketParser::parse_nak(reply, error);
}

const char* OSDPReplies::get_nak_error_string(NakError error) {
    switch (error) {
        case NAK_ERR_BAD_CRC:       return "Bad CRC";
        case NAK_ERR_BAD_LEN:       return "Bad length";
        case NAK_ERR_BAD_CMD:       return "Unknown command";
        case NAK_ERR_BAD_SEQ:       return "Bad sequence number";
        case NAK_ERR_UNSUPPORTED_SEC: return "Unsupported security block";
        case NAK_ERR_UNMET_SECURITY: return "Unmet security conditions";
        case NAK_ERR_UNSUPPORTED_BIO_TYPE: return "Unsupported biometric type";
        case NAK_ERR_UNSUPPORTED_BIO_FORMAT: return "Unsupported biometric format";
        case NAK_ERR_UNKNOWN:       return "Unknown error";
        default:                     return "Invalid error code";
    }
}

// ============================================================================
// OSDPCommandFactory Implementation
// ============================================================================

bool OSDPCommandFactory::validate_led_params(const LedParams& params) {
    // LED number typically 0 or 1
    if (params.led_number > 1) {
        return false;
    }

    // Validate color codes (simple RGB)
    if (params.temporary_on_color > 0xFF ||
        params.temporary_off_color > 0xFF ||
        params.permanent_on_color > 0xFF ||
        params.permanent_off_color > 0xFF) {
        return false;
    }

    return true;
}

bool OSDPCommandFactory::validate_buzzer_params(const BuzzerParams& params) {
    // Buzzer number typically 0 or 1
    if (params.buzzer_number > 1) {
        return false;
    }

    // Validate counts (0-255)
    if (params.on_count > 255 || params.off_count > 255 ||
        params.repeat_count > 255) {
        return false;
    }

    return true;
}

bool OSDPCommandFactory::validate_output_params(const OutputControlParams& params) {
    // Output number typically 0-7
    if (params.output_number > 7) {
        return false;
    }

    // Validate control code
    if (params.control_code != OUTPUT_OFF &&
        params.control_code != OUTPUT_ON &&
        params.control_code != OUTPUT_PULSE) {
        return false;
    }

    return true;
}

bool OSDPCommandFactory::validate_text_params(const TextParams& params) {
    // Reader number typically 0 or 1
    if (params.reader_number > 1) {
        return false;
    }

    // Row and column limits (typical LCD: 2-4 rows x 16-20 cols)
    if (params.row_number > 3 || params.col_number > 19) {
        return false;
    }

    // Text length limit (OSDP max is typically 32 bytes)
    if (params.text.size() > 32) {
        return false;
    }

    return true;
}

bool OSDPCommandFactory::validate_address(uint8_t address) {
    return address <= OSDP_ADDRESS_MAX;
}

bool OSDPCommandFactory::validate_scbk(const std::vector<uint8_t>& scbk) {
    return scbk.size() == OSDP_KEY_SIZE_SCBK;
}

LedParams OSDPCommandFactory::default_led_green() {
    LedParams params;
    params.led_number = 0;
    params.temporary_mode = LED_TEMP_SET;
    params.temporary_on_count = 10;   // 1 second
    params.temporary_off_count = 0;
    params.temporary_on_color = 0x20;  // Green
    params.temporary_off_color = 0;
    params.permanent_mode = 0;
    params.permanent_on_count = 0;
    params.permanent_off_count = 0;
    params.permanent_on_color = 0;
    params.permanent_off_color = 0;
    return params;
}

LedParams OSDPCommandFactory::default_led_red() {
    LedParams params;
    params.led_number = 0;
    params.temporary_mode = LED_TEMP_SET;
    params.temporary_on_count = 10;   // 1 second
    params.temporary_off_count = 0;
    params.temporary_on_color = 0x04;  // Red
    params.temporary_off_color = 0;
    params.permanent_mode = 0;
    params.permanent_on_count = 0;
    params.permanent_off_count = 0;
    params.permanent_on_color = 0;
    params.permanent_off_color = 0;
    return params;
}

BuzzerParams OSDPCommandFactory::default_buzzer_success() {
    BuzzerParams params;
    params.buzzer_number = 0;
    params.on_count = 2;      // 200ms on
    params.off_count = 2;     // 200ms off
    params.repeat_count = 2;   // 2 beeps
    return params;
}

BuzzerParams OSDPCommandFactory::default_buzzer_error() {
    BuzzerParams params;
    params.buzzer_number = 0;
    params.on_count = 1;      // 100ms on
    params.off_count = 1;     // 100ms off
    params.repeat_count = 10;  // Long error beep
    return params;
}

OutputControlParams OSDPCommandFactory::default_unlock(uint8_t output_number) {
    OutputControlParams params;
    params.output_number = output_number;
    params.control_code = OUTPUT_PULSE;  // Pulse
    params.on_time = 50;      // 5 seconds (in 100ms units)
    params.off_time = 0;
    return params;
}

OutputControlParams OSDPCommandFactory::default_lock(uint8_t output_number) {
    OutputControlParams params;
    params.output_number = output_number;
    params.control_code = OUTPUT_OFF;  // Turn off (lock)
    params.on_time = 0;
    params.off_time = 0;
    return params;
}

} // namespace osdp
} // namespace accessd
