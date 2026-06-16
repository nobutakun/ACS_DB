#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "osdp_reader.hpp"
#include <memory>
#include <vector>

namespace accessd {

// Mock OSDPConnection for testing
class MockOSDPConnection : public OSDPConnection {
public:
    MOCK_METHOD(bool, add_reader, (const OSDPReader& reader), (override));
    MOCK_METHOD(bool, remove_reader, (int reader_id), (override));
    MOCK_METHOD(bool, remove_reader_by_address, (int osdp_address), (override));
    MOCK_METHOD(bool, start, (), (override));
    MOCK_METHOD(void, stop, (), (override));
    MOCK_METHOD(void, set_access_callback, (AccessRequestCallback callback), (override));
    MOCK_METHOD(std::vector<OSDPReader>, get_readers, (), (const, override));
    MOCK_METHOD(bool, led, (int pd_id, int led_number, bool on), (override));
    MOCK_METHOD(bool, buzzer, (int pd_id, int on_time_ms, int off_time_ms, int count), (override));
    MOCK_METHOD(bool, display_text, (int pd_id, const std::string& text, int duration_sec), (override));
};

class OSDPConnectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        conn = std::make_unique<MockOSDPConnection>();

        // Setup test reader
        test_reader.reader_id = 1;
        test_reader.device_id = 101;
        test_reader.door_id = 1;
        test_reader.device_uid = "READER-001";
        test_reader.name = "Main Door Reader";
        test_reader.reader_type = "card";
        test_reader.direction = "entry";
        test_reader.connection_type = "osdp";
        test_reader.osdp_address = 1;
        test_reader.baudrate = 9600;
        test_reader.use_encryption = false;
        test_reader.rs485_port = "/dev/ttyUSB0";
        test_reader.status = "online";
    }

    void TearDown() override {
        conn.reset();
    }

    std::unique_ptr<MockOSDPConnection> conn;
    OSDPReader test_reader;
};

// =============================================================================
// Test: OSDPReader Structure
// =============================================================================

TEST_F(OSDPConnectionTest, OSDPReader_DefaultValues) {
    OSDPReader reader;

    EXPECT_EQ(reader.reader_id, 0);
    EXPECT_EQ(reader.device_id, 0);
    EXPECT_EQ(reader.door_id, 0);
    EXPECT_TRUE(reader.device_uid.empty());
    EXPECT_TRUE(reader.name.empty());
    EXPECT_TRUE(reader.reader_type.empty());
}

TEST_F(OSDPConnectionTest, OSDPReader_Fields) {
    EXPECT_EQ(test_reader.reader_id, 1);
    EXPECT_EQ(test_reader.device_id, 101);
    EXPECT_EQ(test_reader.door_id, 1);
    EXPECT_EQ(test_reader.device_uid, "READER-001");
    EXPECT_EQ(test_reader.name, "Main Door Reader");
    EXPECT_EQ(test_reader.reader_type, "card");
    EXPECT_EQ(test_reader.direction, "entry");
}

TEST_F(OSDPConnectionTest, OSDPReader_IsOsdp) {
    OSDPReader osdp_reader;
    osdp_reader.connection_type = "osdp";
    EXPECT_TRUE(osdp_reader.is_osdp());
    EXPECT_FALSE(osdp_reader.is_tcp());
}

TEST_F(OSDPConnectionTest, OSDPReader_IsTcp) {
    OSDPReader tcp_reader;
    tcp_reader.connection_type = "tcp";
    EXPECT_FALSE(tcp_reader.is_osdp());
    EXPECT_TRUE(tcp_reader.is_tcp());
}

TEST_F(OSDPConnectionTest, OSDPReader_ConnectionTypes) {
    OSDPReader reader;

    reader.connection_type = "osdp";
    EXPECT_TRUE(reader.is_osdp());
    EXPECT_FALSE(reader.is_tcp());

    reader.connection_type = "tcp";
    EXPECT_FALSE(reader.is_osdp());
    EXPECT_TRUE(reader.is_tcp());
}

// =============================================================================
// Test: OSDP Address Range
// =============================================================================

TEST_F(OSDPConnectionTest, OSDPAddress_ValidRange) {
    OSDPReader reader;

    // Valid OSDP addresses: 0-127
    for (int addr = 0; addr <= 127; addr++) {
        reader.osdp_address = addr;
        EXPECT_GE(reader.osdp_address, 0);
        EXPECT_LE(reader.osdp_address, 127);
    }
}

TEST_F(OSDPConnectionTest, OSDPAddress_InvalidRange) {
    OSDPReader reader;

    // These are technically invalid but struct allows any value
    reader.osdp_address = 128;
    EXPECT_EQ(reader.osdp_address, 128);

    reader.osdp_address = 255;
    EXPECT_EQ(reader.osdp_address, 255);

    reader.osdp_address = -1;
    EXPECT_EQ(reader.osdp_address, -1);
}

// =============================================================================
// Test: Baud Rate Values
// =============================================================================

TEST_F(OSDPConnectionTest, BaudRate_CommonValues) {
    OSDPReader reader;

    std::vector<int> valid_bauds = {9600, 19200, 38400, 57600, 115200};
    for (int baud : valid_bauds) {
        reader.baudrate = baud;
        EXPECT_EQ(reader.baudrate, baud);
    }
}

// =============================================================================
// Test: Encryption Key
// =============================================================================

TEST_F(OSDPConnectionTest, Encryption_EmptyKey) {
    OSDPReader reader;

    EXPECT_TRUE(reader.master_key.empty());
    EXPECT_FALSE(reader.use_encryption);
}

TEST_F(OSDPConnectionTest, Encryption_WithKey) {
    OSDPReader reader;

    reader.use_encryption = true;
    reader.master_key = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                          0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};

    EXPECT_TRUE(reader.use_encryption);
    EXPECT_EQ(reader.master_key.size(), 16);
}

// =============================================================================
// Test: Reader Capabilities
// =============================================================================

TEST_F(OSDPConnectionTest, ReaderCapabilities_Defaults) {
    OSDPReader reader;

    EXPECT_FALSE(reader.supports_card);
    EXPECT_FALSE(reader.supports_pin);
    EXPECT_FALSE(reader.supports_biometric);
    EXPECT_FALSE(reader.supports_led);
    EXPECT_FALSE(reader.supports_buzzer);
    EXPECT_FALSE(reader.supports_display);
}

TEST_F(OSDPConnectionTest, ReaderCapabilities_AllEnabled) {
    OSDPReader reader;

    reader.supports_card = true;
    reader.supports_pin = true;
    reader.supports_biometric = true;
    reader.supports_led = true;
    reader.supports_buzzer = true;
    reader.supports_display = true;

    EXPECT_TRUE(reader.supports_card);
    EXPECT_TRUE(reader.supports_pin);
    EXPECT_TRUE(reader.supports_biometric);
    EXPECT_TRUE(reader.supports_led);
    EXPECT_TRUE(reader.supports_buzzer);
    EXPECT_TRUE(reader.supports_display);
}

// =============================================================================
// Test: PD Identification
// =============================================================================

TEST_F(OSDPConnectionTest, PDIdentification_Defaults) {
    OSDPReader reader;

    EXPECT_EQ(reader.vendor_code, 0);
    EXPECT_EQ(reader.firmware_version, 0);
    EXPECT_TRUE(reader.serial_number.empty());
}

TEST_F(OSDPConnectionTest, PDIdentification_WithValues) {
    OSDPReader reader;

    reader.vendor_code = 0x12345678;
    reader.firmware_version = 0x0102;  // v1.2
    reader.serial_number = "SN12345";

    EXPECT_EQ(reader.vendor_code, 0x12345678);
    EXPECT_EQ(reader.firmware_version, 0x0102);
    EXPECT_EQ(reader.serial_number, "SN12345");
}

// =============================================================================
// Test: Status Values
// =============================================================================

TEST_F(OSDPConnectionTest, Status_ValidValues) {
    OSDPReader reader;

    std::vector<std::string> valid_statuses = {
        "online", "offline", "error", "initializing", "busy"
    };

    for (const auto& status : valid_statuses) {
        reader.status = status;
        EXPECT_EQ(reader.status, status);
    }
}

// =============================================================================
// Test: AccessRequest Structure
// =============================================================================

TEST_F(OSDPConnectionTest, AccessRequest_Defaults) {
    AccessRequest req;

    EXPECT_EQ(req.reader_id, 0);
    EXPECT_TRUE(req.device_uid.empty());
    EXPECT_TRUE(req.credential.empty());
    EXPECT_TRUE(req.credential_type.empty());
    EXPECT_TRUE(req.door_id.empty());
    EXPECT_EQ(req.timestamp, 0);
    EXPECT_EQ(req.osdp_pd_id, 0);
    EXPECT_TRUE(req.format.empty());
}

TEST_F(OSDPConnectionTest, AccessRequest_WithValues) {
    AccessRequest req;

    req.reader_id = 1;
    req.device_uid = "READER-001";
    req.credential = "1234567890AB";
    req.credential_type = "card";
    req.door_id = "1";
    req.timestamp = 1234567890000;
    req.osdp_pd_id = 1;
    req.format = "wiegand";

    EXPECT_EQ(req.reader_id, 1);
    EXPECT_EQ(req.device_uid, "READER-001");
    EXPECT_EQ(req.credential, "1234567890AB");
    EXPECT_EQ(req.credential_type, "card");
    EXPECT_EQ(req.door_id, "1");
    EXPECT_EQ(req.timestamp, 1234567890000);
    EXPECT_EQ(req.osdp_pd_id, 1);
    EXPECT_EQ(req.format, "wiegand");
}

// =============================================================================
// Test: Credential Types
// =============================================================================

TEST_F(OSDPConnectionTest, CredentialTypes) {
    OSDPReader reader;

    std::vector<std::string> valid_types = {
        "card", "pin", "fingerprint", "face", "qr", "multi"
    };

    for (const auto& type : valid_types) {
        reader.reader_type = type;
        EXPECT_EQ(reader.reader_type, type);
    }
}

// =============================================================================
// Test: Direction Values
// =============================================================================

TEST_F(OSDPConnectionTest, DirectionValues) {
    OSDPReader reader;

    reader.direction = "entry";
    EXPECT_EQ(reader.direction, "entry");

    reader.direction = "exit";
    EXPECT_EQ(reader.direction, "exit");

    reader.direction = "both";
    EXPECT_EQ(reader.direction, "both");
}

// =============================================================================
// Test: TCP Connection Fields
// =============================================================================

TEST_F(OSDPConnectionTest, TcpConnectionFields) {
    OSDPReader reader;

    reader.connection_type = "tcp";
    reader.ip_address = "192.168.1.100";
    reader.port = 8080;

    EXPECT_EQ(reader.connection_type, "tcp");
    EXPECT_EQ(reader.ip_address, "192.168.1.100");
    EXPECT_EQ(reader.port, 8080);
}

// =============================================================================
// Test: Mock Method Calls
// =============================================================================

TEST_F(OSDPConnectionTest, MockAddReader) {
    EXPECT_CALL(*conn, add_reader(::testing::_))
        .WillOnce(::testing::Return(true));

    bool result = conn->add_reader(test_reader);
    EXPECT_TRUE(result);
}

TEST_F(OSDPConnectionTest, MockRemoveReader) {
    EXPECT_CALL(*conn, remove_reader(1))
        .WillOnce(::testing::Return(true));

    bool result = conn->remove_reader(1);
    EXPECT_TRUE(result);
}

TEST_F(OSDPConnectionTest, MockStart) {
    EXPECT_CALL(*conn, start())
        .WillOnce(::testing::Return(true));

    bool result = conn->start();
    EXPECT_TRUE(result);
}

TEST_F(OSDPConnectionTest, MockStop) {
    EXPECT_CALL(*conn, stop())
        .Times(1);

    conn->stop();
}

TEST_F(OSDPConnectionTest, MockLedCommand) {
    EXPECT_CALL(*conn, led(1, 1, true))
        .WillOnce(::testing::Return(true));

    bool result = conn->led(1, 1, true);
    EXPECT_TRUE(result);
}

TEST_F(OSDPConnectionTest, MockBuzzerCommand) {
    EXPECT_CALL(*conn, buzzer(1, 100, 100, 1))
        .WillOnce(::testing::Return(true));

    bool result = conn->buzzer(1, 100, 100, 1);
    EXPECT_TRUE(result);
}

TEST_F(OSDPConnectionTest, MockDisplayText) {
    EXPECT_CALL(*conn, display_text(1, "Hello", 3))
        .WillOnce(::testing::Return(true));

    bool result = conn->display_text(1, "Hello", 3);
    EXPECT_TRUE(result);
}

TEST_F(OSDPConnectionTest, MockGetReaders) {
    std::vector<OSDPReader> readers = {test_reader};

    EXPECT_CALL(*conn, get_readers())
        .WillOnce(::testing::Return(readers));

    auto result = conn->get_readers();
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].reader_id, 1);
}

// =============================================================================
// Test: Access Callback
// =============================================================================

TEST_F(OSDPConnectionTest, AccessCallback) {
    bool callback_called = false;

    EXPECT_CALL(*conn, set_access_callback(::testing::_))
        .WillOnce(::testing::Invoke([&](AccessRequestCallback cb) {
            callback_called = true;
        }));

    conn->set_access_callback([](const AccessRequest& req) {
        // Callback body
    });

    EXPECT_TRUE(callback_called);
}

} // namespace accessd
