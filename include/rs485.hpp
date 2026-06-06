#ifndef ACCESSD_RS485_HPP
#define ACCESSD_RS485_HPP

#include <string>
#include <cstdint>

// Platform-specific includes
#ifdef _WIN32
    // Windows
    #define WINDOWS_PLATFORM
    typedef int ssize_t;
#else
    // Linux/Unix
    #define POSIX_PLATFORM
    #include <termios.h>
    #include <unistd.h>
    #include <fcntl.h>
#endif

namespace accessd {

struct RS485Config {
    std::string port = "/dev/ttyUSB0";
    int baud_rate = 9600;
    int data_bits = 8;
    int stop_bits = 1;
    char parity = 'N';  // N=None, E=Even, O=Odd
    bool rtscts = false;
};

class RS485 {
public:
    RS485();
    ~RS485();

    // Open and configure serial port
    bool open(const RS485Config& config);
    void close();

    // Check if open
    bool is_open() const;

    // Read/Write
    ssize_t read(uint8_t* buffer, size_t size);
    ssize_t write(const uint8_t* buffer, size_t size);

    // Flush buffers
    void flush_input();
    void flush_output();
    void flush();

    // Get port name
    const std::string& port() const { return config_.port; }

private:
    RS485Config config_;

#ifdef WINDOWS_PLATFORM
    // Windows handle
    void* handle_;  // HANDLE is void*

    bool set_windows_comm_config();
    DWORD baud_to_windows_baud(int baud);
#else
    // POSIX file descriptor
    int fd_;
    struct termios orig_termios_;

    bool set_termios();
    speed_t baud_to_speed(int baud);
#endif
};

} // namespace accessd

#endif // ACCESSD_RS485_HPP
