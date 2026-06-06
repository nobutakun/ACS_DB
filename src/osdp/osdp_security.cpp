#include "osdp/osdp_security.hpp"
#include <iostream>
#include <cstring>
#include <random>

// For AES encryption (simplified - in production use OpenSSL/mbedtls)
// This is a placeholder implementation showing the structure
// Real implementation should use OpenSSL EVP or mbedTLS

namespace accessd {
namespace osdp {

// Default SCBK-D (for provisioning mode)
// WARNING: Only use during controlled provisioning!
const std::vector<uint8_t> DEFAULT_SCBK_D = {
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
    0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F
};

// Simple AES S-Box (for demonstration - use real AES in production)
static const uint8_t AES_SBOX[256] = {
    0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
    0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
    0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
    0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
    0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
    0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
    0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
    0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
    0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
    0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
    0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
    0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
    0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
    0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
    0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
    0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16
};

// Simple AES-128 ECB (for demonstration - NOT SECURE for production!)
// In production, use OpenSSL EVP or mbedTLS
static void aes_128_ecb_encrypt_block(const uint8_t* input, uint8_t* output, const uint8_t* key) {
    // Simplified AES for demonstration
    // Real implementation should use proper AES

    // Apply S-Box (substitute bytes)
    for (int i = 0; i < 16; i++) {
        output[i] = AES_SBOX[input[i] ^ key[i]];
    }

    // In production, use:
    // OpenSSL: EVP_EncryptInit_ex, EVP_EncryptUpdate, EVP_EncryptFinal_ex
    // mbedTLS: mbedtls_aes_crypt_ecb
}

// ============================================================================
// OSDPSecurity Implementation
// ============================================================================

OSDPSecurity::OSDPSecurity()
    : security_level_(SECURITY_NONE)
    , sc_state_(SC_STATE_NONE)
{
}

OSDPSecurity::~OSDPSecurity() {
    clear();
}

bool OSDPSecurity::init(const std::vector<uint8_t>& scbk, bool use_default_key) {
    if (use_default_key) {
        scbk_ = get_scbk_d();
        std::cout << "OSDPSecurity: Using SCBK-D (default key) - PROVISIONING MODE" << std::endl;
    } else {
        if (scbk.size() != SCBK_SIZE) {
            std::cerr << "OSDPSecurity: Invalid SCBK size" << std::endl;
            return false;
        }
        scbk_ = scbk;
    }

    // Derive MAC keys
    derive_mac_keys();

    security_level_ = SECURITY_SC_ENCRYPT;
    return true;
}

void OSDPSecurity::set_security_level(SecurityLevel level) {
    security_level_ = level;
    if (level == SECURITY_NONE) {
        sc_state_ = SC_STATE_NONE;
    }
}

std::vector<uint8_t> OSDPSecurity::encrypt(const std::vector<uint8_t>& data) {
    if (!is_encryption_enabled() || !is_secure_channel_established()) {
        return data;  // No encryption
    }

    std::vector<uint8_t> encrypted(data);

    // Pad to block size (PKCS#7)
    size_t padding = AES_BLOCK_SIZE - (data.size() % AES_BLOCK_SIZE);
    encrypted.insert(encrypted.end(), padding, static_cast<uint8_t>(padding));

    // Encrypt each block (ECB mode - for OSDP)
    for (size_t i = 0; i < encrypted.size(); i += AES_BLOCK_SIZE) {
        std::vector<uint8_t> block(encrypted.begin() + i,
                                   encrypted.begin() + i + AES_BLOCK_SIZE);
        std::vector<uint8_t> enc_block = aes_encrypt_block(block);
        std::copy(enc_block.begin(), enc_block.end(), encrypted.begin() + i);
    }

    return encrypted;
}

std::vector<uint8_t> OSDPSecurity::decrypt(const std::vector<uint8_t>& data) {
    if (!is_encryption_enabled() || !is_secure_channel_established()) {
        return data;  // No encryption
    }

    std::vector<uint8_t> decrypted(data);

    // Decrypt each block (ECB mode)
    for (size_t i = 0; i < decrypted.size(); i += AES_BLOCK_SIZE) {
        std::vector<uint8_t> block(decrypted.begin() + i,
                                   decrypted.begin() + i + AES_BLOCK_SIZE);
        std::vector<uint8_t> dec_block = aes_decrypt_block(block);
        std::copy(dec_block.begin(), dec_block.end(), decrypted.begin() + i);
    }

    // Remove PKCS#7 padding
    if (!decrypted.empty()) {
        uint8_t padding = decrypted.back();
        if (padding > 0 && padding <= AES_BLOCK_SIZE) {
            decrypted.resize(decrypted.size() - padding);
        }
    }

    return decrypted;
}

std::vector<uint8_t> OSDPSecurity::generate_challenge(size_t size) {
    return OSDPRandom::generate(size);
}

std::vector<uint8_t> OSDPSecurity::calculate_ccrypt(const std::vector<uint8_t>& challenge,
                                                       const std::vector<uint8_t>& cp_random) {
    if (challenge.size() < 8 || cp_random.size() < 8) {
        std::cerr << "OSDPSecurity: Invalid challenge/random size" << std::endl;
        return {};
    }

    // Calculate client cryptogram (8 bytes)
    // CCrypt = AES-128(SCBK, Challenge[0-8] || CP_Random[0-8])[0-8]
    std::vector<uint8_t> input;
    input.insert(input.end(), challenge.begin(), challenge.begin() + 8);
    input.insert(input.end(), cp_random.begin(), cp_random.begin() + 8);

    std::vector<uint8_t> output = aes_encrypt_block(input);

    // Return first 8 bytes
    return std::vector<uint8_t>(output.begin(), output.begin() + 8);
}

bool OSDPSecurity::validate_scrypt(const std::vector<uint8_t>& scrypt,
                                  const std::vector<uint8_t>& challenge,
                                  const std::vector<uint8_t>& pd_random) {
    if (scrypt.size() < 8 || challenge.size() < 8 || pd_random.size() < 8) {
        return false;
    }

    // Calculate expected server cryptogram
    // SCrypt = AES-128(SCBK, Challenge[0-8] || PD_Random[0-8])[0-8]
    std::vector<uint8_t> input;
    input.insert(input.end(), challenge.begin(), challenge.begin() + 8);
    input.insert(input.end(), pd_random.begin(), pd_random.begin() + 8);

    std::vector<uint8_t> expected = aes_encrypt_block(input);
    std::vector<uint8_t> expected_crypt(expected.begin(), expected.begin() + 8);

    // Compare
    return std::equal(scrypt.begin(), scrypt.begin() + 8, expected_crypt.begin());
}

std::vector<uint8_t> OSDPSecurity::calculate_rmac(const uint8_t* data, size_t length) {
    if (length < 4) {
        return std::vector<uint8_t>(4, 0);
    }

    // Simple RMAC calculation (OSDP uses CMAC with AES-128)
    // For now, simplified version
    std::vector<uint8_t> mac_data(data, data + length - 4);  // Exclude existing MAC

    // XOR with CP MAC key
    std::vector<uint8_t> mac(4, 0);
    for (size_t i = 0; i < mac_data.size() && i < 8; i++) {
        mac[i % 4] ^= mac_data[i] ^ cp_mac_key_[i];
    }

    return mac;
}

std::vector<uint8_t> OSDPSecurity::calculate_rmac(const std::vector<uint8_t>& data) {
    return calculate_rmac(data.data(), data.size());
}

bool OSDPSecurity::validate_rmac(const uint8_t* data, size_t length) {
    if (length < 4) {
        return false;
    }

    // Extract received MAC (last 4 bytes)
    std::vector<uint8_t> received_mac(data + length - 4, data + length);

    // Calculate expected MAC
    std::vector<uint8_t> expected_mac = calculate_rmac(data, length - 4);

    return std::equal(received_mac.begin(), received_mac.end(), expected_mac.begin());
}

const std::vector<uint8_t>& OSDPSecurity::get_scbk_d() {
    return DEFAULT_SCBK_D;
}

void OSDPSecurity::set_sc_state(SecureChannelState state) {
    if (sc_state_ != state) {
        std::cout << "OSDPSecurity: SC State " << sc_state_ << " -> " << state << std::endl;
        sc_state_ = state;
    }
}

void OSDPSecurity::reset_secure_channel() {
    sc_state_ = SC_STATE_NONE;
    cp_random_.clear();
    pd_random_.clear();
    std::cout << "OSDPSecurity: Secure channel reset" << std::endl;
}

void OSDPSecurity::clear() {
    security_level_ = SECURITY_NONE;
    sc_state_ = SC_STATE_NONE;
    scbk_.clear();
    scbk_.shrink_to_fit();
    cp_random_.clear();
    pd_random_.clear();
    cp_mac_key_.fill(0);
    pd_mac_key_.fill(0);
}

void OSDPSecurity::derive_mac_keys() {
    if (scbk_.size() != SCBK_SIZE) {
        return;
    }

    // Derive CP MAC key (first 8 bytes of SCBK XOR with pattern)
    for (size_t i = 0; i < 8; i++) {
        cp_mac_key_[i] = scbk_[i] ^ 0x11;  // Simple XOR derivation
    }

    // Derive PD MAC key (last 8 bytes of SCBK XOR with pattern)
    for (size_t i = 0; i < 8; i++) {
        pd_mac_key_[i] = scbk_[i + 8] ^ 0x22;  // Simple XOR derivation
    }
}

std::vector<uint8_t> OSDPSecurity::xor_bytes(const std::vector<uint8_t>& a,
                                                const std::vector<uint8_t>& b) {
    std::vector<uint8_t> result;
    size_t min_len = std::min(a.size(), b.size());

    for (size_t i = 0; i < min_len; i++) {
        result.push_back(a[i] ^ b[i]);
    }

    return result;
}

std::vector<uint8_t> OSDPSecurity::aes_encrypt_block(const std::vector<uint8_t>& input) {
    std::vector<uint8_t> output(16);
    aes_128_ecb_encrypt_block(input.data(), output.data(), scbk_.data());
    return output;
}

std::vector<uint8_t> OSDPSecurity::aes_decrypt_block(const std::vector<uint8_t>& input) {
    std::vector<uint8_t> output(16);

    // For AES in ECB mode, decrypt is same as encrypt (with modified key schedule)
    // Simplified: use same function (NOT SECURE!)
    aes_128_ecb_encrypt_block(input.data(), output.data(), scbk_.data());

    return output;
}

// ============================================================================
// OSDPKeyDerivation Implementation
// ============================================================================

std::vector<uint8_t> OSDPKeyDerivation::derive_key(const std::vector<uint8_t>& scbk,
                                                      const std::string& derivation_string) {
    // In production, use proper KDF (HKDF, PBKDF2, etc.)
    // Simplified: XOR SCBK with derivation string

    std::vector<uint8_t> derived_key(16, 0);

    for (size_t i = 0; i < 16 && i < derivation_string.size(); i++) {
        derived_key[i] = scbk_[i] ^ derivation_string[i];
    }

    return derived_key;
}

void OSDPKeyDerivation::derive_mac_keys(const std::vector<uint8_t>& scbk,
                                         std::array<uint8_t, 8>& cp_mac_key,
                                         std::array<uint8_t, 8>& pd_mac_key) {
    // CP MAC key from first 8 bytes
    for (size_t i = 0; i < 8; i++) {
        cp_mac_key[i] = scbk[i] ^ 0x11;
    }

    // PD MAC key from last 8 bytes
    for (size_t i = 0; i < 8; i++) {
        pd_mac_key[i] = scbk[i + 8] ^ 0x22;
    }
}

std::vector<uint8_t> OSDPKeyDerivation::derive_session_key(const std::vector<uint8_t>& scbk,
                                                             const std::vector<uint8_t>& cp_random,
                                                             const std::vector<uint8_t>& pd_random) {
    // Derive session key from SCBK and both random values
    std::vector<uint8_t> session_key(16, 0);

    // XOR SCBK with CP random
    for (size_t i = 0; i < 16 && i < cp_random.size(); i++) {
        session_key[i] = scbk[i] ^ cp_random[i];
    }

    // XOR with PD random
    for (size_t i = 0; i < 16 && i < pd_random.size(); i++) {
        session_key[i] ^= pd_random[i];
    }

    return session_key;
}

// ============================================================================
// OSDPCryptogramValidator Implementation
// ============================================================================

bool OSDPCryptogramValidator::validate_ccrypt(const std::vector<uint8_t>& ccrypt,
                                                const std::vector<uint8_t>& challenge,
                                                const std::vector<uint8_t>& random,
                                                const std::array<uint8_t, 8>& mac_key) {
    // Calculate expected cryptogram
    std::vector<uint8_t> expected = calculate_ccrypt(challenge, random, mac_key);

    // Compare
    return std::equal(ccrypt.begin(), ccrypt.begin() + std::min(ccrypt.size(), expected.size()),
                      expected.begin());
}

bool OSDPCryptogramValidator::validate_scrypt(const std::vector<uint8_t>& scrypt,
                                                const std::vector<uint8_t>& challenge,
                                                const std::vector<uint8_t>& random,
                                                const std::array<uint8_t, 8>& mac_key) {
    // Calculate expected cryptogram
    std::vector<uint8_t> expected = calculate_scrypt(challenge, random, mac_key);

    // Compare
    return std::equal(scrypt.begin(), scrypt.begin() + std::min(scrypt.size(), expected.size()),
                      expected.begin());
}

std::vector<uint8_t> OSDPCryptogramValidator::calculate_ccrypt(const std::vector<uint8_t>& challenge,
                                                                   const std::vector<uint8_t>& random,
                                                                   const std::array<uint8_t, 8>& mac_key) {
    // CCrypt = AES-128(Key, Challenge || Random)[0-8]
    std::vector<uint8_t> input;
    input.insert(input.end(), challenge.begin(), challenge.begin() + 8);
    input.insert(input.end(), random.begin(), random.begin() + 8);

    std::vector<uint8_t> output(16);
    // In production, use real AES-128 here
    std::vector<uint8_t> key(mac_key.begin(), mac_key.end());
    aes_128_ecb_encrypt_block(input.data(), output.data(), key.data());

    return std::vector<uint8_t>(output.begin(), output.begin() + 8);
}

std::vector<uint8_t> OSDPCryptogramValidator::calculate_scrypt(const std::vector<uint8_t>& challenge,
                                                                   const std::vector<uint8_t>& random,
                                                                   const std::array<uint8_t, 8>& mac_key) {
    // SCrypt = AES-128(Key, Challenge || Random)[0-8]
    std::vector<uint8_t> input;
    input.insert(input.end(), challenge.begin(), challenge.begin() + 8);
    input.insert(input.end(), random.begin(), random.begin() + 8);

    std::vector<uint8_t> output(16);
    // In production, use real AES-128 here
    std::vector<uint8_t> key(mac_key.begin(), mac_key.end());
    aes_128_ecb_encrypt_block(input.data(), output.data(), key.data());

    return std::vector<uint8_t>(output.begin(), output.begin() + 8);
}

// ============================================================================
// OSDPRandom Implementation
// ============================================================================

std::vector<uint8_t> OSDPRandom::generate(size_t size) {
    std::vector<uint8_t> random(size);

    // Use C++ random_device (cryptographically secure on most platforms)
    std::random_device rd;
    std::uniform_int_distribution<int> dist(0, 255);

    for (size_t i = 0; i < size; i++) {
        random[i] = static_cast<uint8_t>(dist(rd));
    }

    return random;
}

uint8_t OSDPRandom::generate_byte() {
    std::vector<uint8_t> bytes = generate(1);
    return bytes[0];
}

uint16_t OSDPRandom::generate_uint16() {
    std::vector<uint8_t> bytes = generate(2);
    return static_cast<uint16_t>(bytes[0]) | (static_cast<uint16_t>(bytes[1]) << 8);
}

uint32_t OSDPRandom::generate_uint32() {
    std::vector<uint8_t> bytes = generate(4);
    return static_cast<uint32_t>(bytes[0]) |
           (static_cast<uint32_t>(bytes[1]) << 8) |
           (static_cast<uint32_t>(bytes[2]) << 16) |
           (static_cast<uint32_t>(bytes[3]) << 24);
}

} // namespace osdp
} // namespace accessd
