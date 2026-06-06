#include <gtest/gtest.h>
#include "config.hpp"

class ConfigTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ConfigTest, SingletonInstance) {
    auto& config1 = accessd::Config::instance();
    auto& config2 = accessd::Config::instance();
    EXPECT_EQ(&config1, &config2);
}

TEST_F(ConfigTest, LoadConfig) {
    auto& config = accessd::Config::instance();
    // Test with actual config file
    bool result = config.load("/home/nobuta/accessd/config/accessd.conf");
    // EXPECT_TRUE(result);  // Uncomment on Pi4

    // If loaded, verify values
    if (result) {
        EXPECT_EQ(config.accessd().default_policy, "deny");
        EXPECT_EQ(config.accessd().decision_timeout, 5000);
        EXPECT_TRUE(config.accessd().enable_cache);
    }
}

TEST_F(ConfigTest, DatabaseConfig) {
    auto& config = accessd::Config::instance();
    bool result = config.load("/home/nobuta/accessd/config/accessd.conf");

    if (result) {
        EXPECT_FALSE(config.database().path.empty());
        EXPECT_GT(config.database().timeout, 0);
    }
}

TEST_F(ConfigTest, HardwareConfig) {
    auto& config = accessd::Config::instance();
    bool result = config.load("/home/nobuta/accessd/config/accessd.conf");

    if (result) {
        EXPECT_GT(config.hardware().relay_default_ms, 0);
        EXPECT_GT(config.hardware().door_timeout_sec, 0);
        EXPECT_GT(config.hardware().door_held_timeout_sec, 0);
    }
}
