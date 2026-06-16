#include <gtest/gtest.h>
#include "rs485.hpp"
#include <memory>

namespace accessd {

class RS485Test : public ::testing::Test {
protected:
    void SetUp() override {
        rs485 = std::make_unique<RS485>();
    }

    void TearDown() override {
        if (rs485->is_open()) {
            rs485->close();
        }
    }

    std::unique_ptr<RS485> rs485;
};

// =============================================================================
// Test: Default Configuration
// =============================================================================

TEST_F(RS485Test, DefaultConfigValues) {
    RS485Config config;

    EXPECT_EQ(config.port, "/dev/ttyUSB0");
    EXPECT_EQ(config.baud_rate, 9600);
    EXPECT_EQ(config.data_bits, 8);
    EXPECT_EQ(config.stop_bits, 1);
    EXPECT_EQ(config.parity, 'N');
    EXPECT_FALSE(config.rtscts);
}

// =============================================================================
// Test: Configuration Validation
// =============================================================================

TEST_F(RS485Test, Config_CustomPort) {
    RS485Config config;
    config.port = "/dev/ttyUSB1";
    config.baud_rate = 115200;

    EXPECT_EQ(config.port, "/dev/ttyUSB1");
    EXPECT_EQ(config.baud_rate, 115200);
}

TEST_F(RS485Test, Config_CommonBaudRates) {
    RS485Config config;

    // Test common baud rates
    std::vector<int> valid_bauds = {9600, 19200, 38400, 57600, 115200};
    for (int baud : valid_bauds) {
        config.baud_rate = baud;
        EXPECT_EQ(config.baud_rate, baud);
    }
}

TEST_F(RS485Test, Config_ParityOptions) {
    RS485Config config;

    config.parity = 'N';  // None
    EXPECT_EQ(config.parity, 'N');

    config.parity = 'E';  // Even
    EXPECT_EQ(config.parity, 'E');

    config.parity = 'O';  // Odd
    EXPECT_EQ(config.parity, 'O');
}

TEST_F(RS485Test, Config_StopBits) {
    RS485Config config;

    config.stop_bits = 1;
    EXPECT_EQ(config.stop_bits, 1);

    config.stop_bits = 2;
    EXPECT_EQ(config.stop_bits, 2);
}

TEST_F(RS485Test, Config_DataBits) {
    RS485Config config;

    for (int bits = 5; bits <= 8; bits++) {
        config.data_bits = bits;
        EXPECT_EQ(config.data_bits, bits);
    }
}

TEST_F(RS485Test, Config_RtsCts) {
    RS485Config config;

    config.rtscts = false;
    EXPECT_FALSE(config.rtscts);

    config.rtscts = true;
    EXPECT_TRUE(config.rtscts);
}

// =============================================================================
// Test: Port Name
// =============================================================================

TEST_F(RS485Test, PortGetter) {
    RS485Config config;
    config.port = "/dev/ttyACM0";

    rs485->open(config);
    // Even if open fails, port should be stored
    EXPECT_EQ(rs485->port(), "/dev/ttyACM0");

    if (rs485->is_open()) {
        rs485->close();
    }
}

// =============================================================================
// Test: Open/Close State
// =============================================================================

TEST_F(RS485Test, IsNotOpenInitially) {
    EXPECT_FALSE(rs485->is_open());
}

TEST_F(RS485Test, OpenFailsWithInvalidPort) {
    RS485Config config;
    config.port = "/nonexistent/port";
    config.baud_rate = 9600;

    bool result = rs485->open(config);
    // Should fail because port doesn't exist
    EXPECT_FALSE(result);
    EXPECT_FALSE(rs485->is_open());
}

// =============================================================================
// Test: Read/Write on Closed Port
// =============================================================================

TEST_F(RS485Test, ReadOnClosedPort) {
    uint8_t buffer[100];
    ssize_t result = rs485->read(buffer, sizeof(buffer));

    EXPECT_EQ(result, -1);
}

TEST_F(RS485Test, WriteOnClosedPort) {
    uint8_t data[] = {0x01, 0x02, 0x03};
    ssize_t result = rs485->write(data, sizeof(data));

    EXPECT_EQ(result, -1);
}

// =============================================================================
// Test: Flush Operations on Closed Port
// =============================================================================

TEST_F(RS485Test, FlushInputOnClosedPort) {
    // Should not crash on closed port
    rs485->flush_input();
    SUCCEED();
}

TEST_F(RS485Test, FlushOutputOnClosedPort) {
    // Should not crash on closed port
    rs485->flush_output();
    SUCCEED();
}

TEST_F(RS485Test, FlushOnClosedPort) {
    // Should not crash on closed port
    rs485->flush();
    SUCCEED();
}

// =============================================================================
// Test: Multiple Close Calls
// =============================================================================

TEST_F(RS485Test, MultipleCloseCalls) {
    rs485->close();  // First close on already closed port
    rs485->close();  // Second close
    rs485->close();  // Third close

    EXPECT_FALSE(rs485->is_open());
}

// =============================================================================
// Test: Config Persistence
// =============================================================================

TEST_F(RS485Test, ConfigPersistedAfterOpen) {
    RS485Config config;
    config.port = "/dev/ttyUSB0";
    config.baud_rate = 115200;
    config.data_bits = 8;
    config.stop_bits = 1;
    config.parity = 'N';

    rs485->open(config);

    // Port name should be preserved even if open failed
    EXPECT_EQ(rs485->port(), "/dev/ttyUSB0");

    if (rs485->is_open()) {
        rs485->close();
    }
}

// =============================================================================
// Test: Platform Specific Port Names
// =============================================================================

TEST_F(RS485Test, LinuxPortNames) {
    std::vector<std::string> linux_ports = {
        "/dev/ttyUSB0", "/dev/ttyUSB1", "/dev/ttyACM0",
        "/dev/ttyACM1", "/dev/ttyS0", "/dev/ttyS1"
    };

    for (const auto& port : linux_ports) {
        RS485Config config;
        config.port = port;
        EXPECT_EQ(config.port, port);
    }
}

TEST_F(RS485Test, WindowsPortNames) {
    std::vector<std::string> win_ports = {
        "COM1", "COM10", "com1", "com10"
    };

    for (const auto& port : win_ports) {
        RS485Config config;
        config.port = port;
        EXPECT_EQ(config.port, port);
    }
}

// =============================================================================
// Test: Baud Rate Constants
// =============================================================================

TEST_F(RS485Test, BaudRateConstants) {
    // Verify common baud rates are distinct
    std::vector<int> bauds = {9600, 19200, 38400, 57600, 115200};

    for (size_t i = 0; i < bauds.size(); i++) {
        for (size_t j = i + 1; j < bauds.size(); j++) {
            EXPECT_NE(bauds[i], bauds[j]);
        }
    }
}

// =============================================================================
// Test: Invalid Baud Rates (should default to 9600)
// =============================================================================

TEST_F(RS485Test, InvalidBaudRate) {
    RS485Config config;
    config.baud_rate = 12345;  // Not a standard baud rate

    // Config accepts any value, validation happens during open
    EXPECT_EQ(config.baud_rate, 12345);
}

} // namespace accessd
