#include "osdp/osdp_packet.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>

namespace accessd {
namespace osdp {

// CRC-16 table (OSDP polynomial 0x8005)
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

// ============================================================================
// PacketBuilder Implementation
// ============================================================================

Packet PacketBuilder::build_command(uint8_t address, Command cmd,
                                     const std::vector<uint8_t>& data) {
    Packet packet;
    packet.som = MARKER_SOM;
    packet.address = address;
    packet.command = static_cast<uint8_t>(cmd);
    packet.data = data;

    // Calculate length: 1 (SOM) + 1 (ADDR) + 2 (LEN) + 1 (CMD) + N (DATA) + 2 (CRC)
    uint16_t len = 5 + data.size();
    packet.set_length(len);

    // Calculate CRC over ADDR + LEN + CMD + DATA
    std::vector<uint8_t> crc_data;
    crc_data.push_back(packet.address);
    crc_data.push_back(packet.length_lsb);
    crc_data.push_back(packet.length_msb);
    crc_data.push_back(packet.command);
    crc_data.insert(crc_data.end(), data.begin(), data.end());
    packet.crc = calculate_crc(crc_data.data(), crc_data.size());

    return packet;
}

Packet PacketBuilder::build_poll(uint8_t address) {
    return build_command(address, CMD_POLL, {});
}

Packet PacketBuilder::build_id(uint8_t address) {
    return build_command(address, CMD_ID, {});
}

Packet PacketBuilder::build_cap(uint8_t address) {
    return build_command(address, CMD_CAP, {});
}

Packet PacketBuilder::build_led(uint8_t address, const LedParams& params) {
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
    return build_command(address, CMD_LED, data);
}

Packet PacketBuilder::build_buzzer(uint8_t address, const BuzzerParams& params) {
    std::vector<uint8_t> data;
    data.push_back(params.buzzer_number);
    data.push_back(params.on_count);
    data.push_back(params.off_count);
    data.push_back(params.repeat_count);
    return build_command(address, CMD_BUZ, data);
}

Packet PacketBuilder::build_output(uint8_t address, const OutputControlParams& params) {
    std::vector<uint8_t> data;
    data.push_back(params.output_number);
    data.push_back(params.control_code);
    data.push_back(params.on_time & 0xFF);
    data.push_back((params.on_time >> 8) & 0xFF);
    data.push_back(params.off_time & 0xFF);
    data.push_back((params.off_time >> 8) & 0xFF);
    return build_command(address, CMD_OUT, data);
}

Packet PacketBuilder::build_text(uint8_t address, const TextParams& params) {
    std::vector<uint8_t> data;
    data.push_back(params.reader_number);
    data.push_back(params.row_number);
    data.push_back(params.col_number);
    data.push_back(params.command);
    data.push_back(params.timeout);
    // Limit text to OSDP max (usually 32 bytes)
    size_t max_text = 32;
    size_t text_len = std::min(params.text.size(), max_text);
    data.insert(data.end(), params.text.begin(), params.text.begin() + text_len);
    return build_command(address, CMD_TEXT, data);
}

Packet PacketBuilder::build_keyset(uint8_t address, const std::vector<uint8_t>& scbk) {
    std::vector<uint8_t> data;
    data.insert(data.end(), scbk.begin(), scbk.end());
    return build_command(address, CMD_KEYSET, data);
}

Packet PacketBuilder::build_challenge(uint8_t address, const std::vector<uint8_t>& random) {
    std::vector<uint8_t> data;
    data.insert(data.end(), random.begin(), random.end());
    return build_command(address, CMD_CHLNG, data);
}

Packet PacketBuilder::build_scrypt(uint8_t address, const std::vector<uint8_t>& cryptogram) {
    std::vector<uint8_t> data;
    data.insert(data.end(), cryptogram.begin(), cryptogram.end());
    return build_command(address, CMD_SCRYPT, data);
}

std::vector<uint8_t> PacketBuilder::serialize(const Packet& packet) {
    std::vector<uint8_t> buffer;

    buffer.push_back(packet.som);
    buffer.push_back(packet.address);
    buffer.push_back(packet.length_lsb);
    buffer.push_back(packet.length_msb);
    buffer.push_back(packet.command);
    buffer.insert(buffer.end(), packet.data.begin(), packet.data.end());
    buffer.push_back(packet.crc & 0xFF);
    buffer.push_back((packet.crc >> 8) & 0xFF);

    return buffer;
}

uint16_t PacketBuilder::calculate_crc(const uint8_t* data, size_t length) {
    uint16_t crc = 0;
    for (size_t i = 0; i < length; i++) {
        uint8_t index = (crc ^ data[i]) & 0xFF;
        crc = (crc >> 8) ^ CRC16_TABLE[index];
    }
    return crc;
}

uint16_t PacketBuilder::calculate_crc(const std::vector<uint8_t>& data) {
    return calculate_crc(data.data(), data.size());
}

// ============================================================================
// PacketParser Implementation
// ============================================================================

bool PacketParser::parse(const std::vector<uint8_t>& buffer, Packet& packet) {
    if (buffer.size() < 6) {
        return false;  // Minimum packet size
    }

    // Parse header
    packet.som = buffer[0];
    packet.address = buffer[1];
    packet.length_lsb = buffer[2];
    packet.length_msb = buffer[3];
    packet.command = buffer[4];

    // Validate basic format
    if (packet.som != MARKER_SOM) {
        return false;
    }

    uint16_t pkt_length = packet.get_length();
    if (pkt_length < 6 || pkt_length > 128) {
        return false;
    }

    // Extract data
    uint16_t data_len = pkt_length - 6;  // Subtract header and CRC
    if (data_len > 0 && buffer.size() >= pkt_length) {
        packet.data.assign(buffer.begin() + 5, buffer.begin() + 5 + data_len);
    }

    // Extract CRC (last 2 bytes)
    if (buffer.size() >= pkt_length) {
        packet.crc = buffer[pkt_length - 2] | (buffer[pkt_length - 1] << 8);
    }

    // Validate CRC
    return validate_crc(packet);
}

int PacketParser::parse_stream(const uint8_t* data_ptr, size_t length, Packet& packet) {
    if (length < 6) {
        return 0;  // Need more data
    }

    // Find SOM
    size_t som_offset = 0;
    while (som_offset < length && data_ptr[som_offset] != MARKER_SOM) {
        som_offset++;
    }

    if (som_offset >= length) {
        return -1;  // No SOM found
    }

    const uint8_t* pkt_start = data_ptr + som_offset;
    size_t remaining = length - som_offset;

    // Check if we have enough data
    if (remaining < 5) {
        return 0;  // Need more data for header
    }

    // Get length
    uint16_t pkt_length = pkt_start[2] | (pkt_start[3] << 8);
    if (pkt_length < 6 || pkt_length > 128) {
        return -1;  // Invalid length
    }

    if (remaining < pkt_length) {
        return 0;  // Need more data
    }

    // Parse packet
    std::vector<uint8_t> buffer(pkt_start, pkt_start + pkt_length);
    if (parse(buffer, packet)) {
        return som_offset + pkt_length;  // Bytes consumed
    }

    return -1;  // Parse failed
}

bool PacketParser::validate(const Packet& packet) {
    return packet.is_valid();
}

bool PacketParser::validate_crc(const Packet& packet) {
    // Rebuild CRC data
    std::vector<uint8_t> crc_data;
    crc_data.push_back(packet.address);
    crc_data.push_back(packet.length_lsb);
    crc_data.push_back(packet.length_msb);
    crc_data.push_back(packet.command);
    crc_data.insert(crc_data.end(), packet.data.begin(), packet.data.end());

    uint16_t calc_crc = PacketBuilder::calculate_crc(crc_data);
    return (calc_crc == packet.crc);
}

const char* PacketParser::get_reply_name(uint8_t reply_code) {
    switch (reply_code) {
        case REPLY_ACK:        return "ACK";
        case REPLY_NAK:        return "NAK";
        case REPLY_PDID:       return "PDID";
        case REPLY_PDCAP:      return "PDCAP";
        case REPLY_LSTATR:     return "LSTATR";
        case REPLY_IASTR:      return "IASTR";
        case REPLY_OSTATR:     return "OSTATR";
        case REPLY_RSTATR:     return "RSTATR";
        case REPLY_RAW:        return "RAW";
        case REPLY_FMT:        return "FMT";
        case REPLY_KEYPAD:     return "KEYPAD";
        case REPLY_COM:        return "COM";
        case REPLY_BIOREADR:   return "BIOREADR";
        case REPLY_BIOMATCHR:  return "BIOMATCHR";
        case REPLY_CCRYPT:     return "CCRYPT";
        case REPLY_RMAC_I:     return "RMAC_I";
        case REPLY_MFGREP:     return "MFGREP";
        case REPLY_BUSY:       return "BUSY";
        case REPLY_XRD:        return "XRD";
        default:               return "UNKNOWN";
    }
}

const char* PacketParser::get_command_name(uint8_t cmd_code) {
    switch (cmd_code) {
        case CMD_POLL:         return "POLL";
        case CMD_ID:           return "ID";
        case CMD_CAP:          return "CAP";
        case CMD_DIAG:         return "DIAG";
        case CMD_LSTAT:        return "LSTAT";
        case CMD_ISTAT:        return "ISTAT";
        case CMD_OSTAT:        return "OSTAT";
        case CMD_RSTAT:        return "RSTAT";
        case CMD_OUT:          return "OUT";
        case CMD_LED:          return "LED";
        case CMD_BUZ:          return "BUZ";
        case CMD_TEXT:         return "TEXT";
        case CMD_COMSET:       return "COMSET";
        case CMD_DATA:         return "DATA";
        case CMD_PROMPT:       return "PROMPT";
        case CMD_BIOREAD:      return "BIOREAD";
        case CMD_BIOMATCH:     return "BIOMATCH";
        case CMD_KEYSET:       return "KEYSET";
        case CMD_CHLNG:        return "CHLNG";
        case CMD_SCRYPT:       return "SCRYPT";
        case CMD_ABORT:        return "ABORT";
        case CMD_MAXREPLY:     return "MAXREPLY";
        case CMD_MFG:          return "MFG";
        default:               return "UNKNOWN";
    }
}

bool PacketParser::parse_pdid(const Packet& packet, PDId& pdid) {
    if (packet.command != REPLY_PDID || packet.data.size() < 12) {
        return false;
    }

    std::memcpy(pdid.vendor_code, &packet.data[0], 3);
    pdid.model = packet.data[3];
    pdid.version = packet.data[4] | (packet.data[5] << 8) |
                   (packet.data[6] << 16) | (packet.data[7] << 24);
    std::memcpy(pdid.serial_number, &packet.data[8], 4);
    pdid.firmware_version = packet.data[12];

    return true;
}

bool PacketParser::parse_pdcap(const Packet& packet, PDCapabilities& caps) {
    if (packet.command != REPLY_PDCAP || packet.data.size() < 2) {
        return false;
    }

    caps.max_command_code = packet.data[0];
    caps.max_reply_code = packet.data[1];
    caps.caps.clear();

    // Parse capability entries (3 bytes each)
    for (size_t i = 2; i + 2 < packet.data.size(); i += 3) {
        PDCapability cap;
        cap.function_code = packet.data[i];
        cap.compliance_level = packet.data[i + 1];
        cap.num_items = packet.data[i + 2];
        caps.caps.push_back(cap);
    }

    return true;
}

bool PacketParser::parse_card_data(const Packet& packet, CardData& data) {
    if (packet.command != REPLY_RAW && packet.command != REPLY_FMT) {
        return false;
    }

    if (packet.data.size() < 2) {
        return false;
    }

    data.reader_number = packet.data[0];
    data.format = packet.data[1];

    // Extract raw card data
    if (packet.data.size() > 2) {
        data.raw_data.assign(packet.data.begin() + 2, packet.data.end());
        data.length = data.raw_data.size() * 8;  // bits
    }

    return true;
}

bool PacketParser::parse_keypad_data(const Packet& packet, KeypadData& data) {
    if (packet.command != REPLY_KEYPAD) {
        return false;
    }

    if (packet.data.size() < 2) {
        return false;
    }

    data.reader_number = packet.data[0];
    data.length = packet.data[1];

    // Extract key presses
    if (packet.data.size() > 2) {
        data.keys.assign(packet.data.begin() + 2, packet.data.end());
    }

    return true;
}

bool PacketParser::parse_nak(const Packet& packet, NakError& error) {
    if (packet.command != REPLY_NAK || packet.data.empty()) {
        return false;
    }

    error = static_cast<NakError>(packet.data[0]);
    return true;
}

// ============================================================================
// PacketHelper Implementation
// ============================================================================

std::string PacketHelper::to_hex(const std::vector<uint8_t>& data) {
    std::ostringstream oss;
    for (uint8_t b : data) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)b;
    }
    return oss.str();
}

void PacketHelper::print_packet(const Packet& packet) {
    std::cout << "Packet: SOM=0x" << std::hex << (int)packet.som
              << ", ADDR=0x" << (int)packet.address
              << ", LEN=" << std::dec << packet.get_length()
              << ", CMD/REPLY=0x" << std::hex << (int)packet.command;

    if (!packet.data.empty()) {
        std::cout << ", DATA=" << to_hex(packet.data);
    }

    std::cout << ", CRC=0x" << packet.crc << std::dec << std::endl;
}

size_t PacketHelper::get_packet_length(const Packet& packet) {
    return packet.get_length();
}

uint8_t PacketHelper::extract_command_id(const std::vector<uint8_t>& buffer) {
    if (buffer.size() >= 5) {
        return buffer[4];  // CMD/REPLY is at offset 4
    }
    return 0;
}

bool PacketHelper::has_som(const std::vector<uint8_t>& buffer) {
    for (uint8_t b : buffer) {
        if (b == MARKER_SOM) {
            return true;
        }
    }
    return false;
}

} // namespace osdp
} // namespace accessd
