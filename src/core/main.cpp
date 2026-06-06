#include "config.hpp"
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {
    std::cout << "Access Control Daemon v1.0" << std::endl;
    std::cout << "=========================" << std::endl;

    // Default config path
    std::string config_file = "/home/nobuta/accessd/config/accessd.conf";

    // Allow custom config path via argument
    if (argc > 1) {
        config_file = argv[1];
    }

    // Load configuration
    std::cout << "Loading config from: " << config_file << std::endl;

    if (!accessd::Config::instance().load(config_file)) {
        std::cerr << "Failed to load configuration!" << std::endl;
        return 1;
    }

    std::cout << "Configuration loaded successfully!" << std::endl;

    // Print loaded config for verification
    const auto& db = accessd::Config::instance().database();
    std::cout << "\nDatabase:" << std::endl;
    std::cout << "  Path: " << db.path << std::endl;
    std::cout << "  Timeout: " << db.timeout << "s" << std::endl;

    const auto& accessd = accessd::Config::instance().accessd();
    std::cout << "\nAccessd:" << std::endl;
    std::cout << "  Default Policy: " << accessd.default_policy << std::endl;
    std::cout << "  Decision Timeout: " << accessd.decision_timeout << "ms" << std::endl;
    std::cout << "  Cache Enabled: " << (accessd.enable_cache ? "Yes" : "No") << std::endl;

    const auto& log = accessd::Config::instance().log();
    std::cout << "\nLog:" << std::endl;
    std::cout << "  File: " << log.file << std::endl;
    std::cout << "  Level: " << log.level << std::endl;

    // TODO: Initialize database connection
    // TODO: Start access decision engine
    // TODO: Start API server if enabled

    return 0;
}
