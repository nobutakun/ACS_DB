#include "osdp/serial_port.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <dirent.h>
#include <termios.h>

#ifdef __linux__
#include <linux/serial.h>
#endif

namespace accessd {
namespace osdp {

SerialPort::SerialPort()
    : fd_(-1)
{
    std::memset(&orig_termios_, 0, sizeof(orig_termios_));
}

SerialPort::~SerialPort() {
    close();
}

bool SerialPort::open(const SerialConfig& config) {
    config_ = config;

    // Open port
    fd_ = ::open(config.port.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd_ < 0) {
        error_msg_ = "Failed to open port: ";
        error_msg_ += config.port;
        error_msg_ += " (";
        error_msg_ += strerror(errno);
        error_msg_ += ")";
        std::cerr << "SerialPort: " << error_msg_ << std::endl;
        return false;
    }

    // Save original settings
    if (tcgetattr(fd_, &orig_termios_) < 0) {
        error_msg_ = "tcgetattr failed";
        std::cerr << "SerialPort: " << error_msg_ << std::endl;
        ::close(fd_);
        fd_ = -1;
        return false;
    }

    // Configure port
    if (!setup_termios()) {
        restore_termios();
        ::close(fd_);
        fd_ = -1;
        return false;
    }

    // Flush buffers
    flush(true, true);

    std::cout << "SerialPort: Opened " << config.port
              << " at " << config.baudrate << " baud" << std::endl;

    return true;
}

void SerialPort::close() {
    if (fd_ >= 0) {
        restore_termios();
        ::close(fd_);
        fd_ = -1;
        std::cout << "SerialPort: Closed " << config_.port << std::endl;
    }
}

bool SerialPort::setup_termios() {
    struct termios options;

    // Get current settings
    if (tcgetattr(fd_, &options) < 0) {
        error_msg_ = "tcgetattr failed";
        return false;
    }

    // Set input/output baud rates
    speed_t baud;
    switch (config_.baudrate) {
        case 9600:   baud = B9600; break;
        case 19200:  baud = B19200; break;
        case 38400:  baud = B38400; break;
        case 57600:  baud = B57600; break;
        case 115200: baud = B115200; break;
        default:
            error_msg_ = "Invalid baudrate";
            return false;
    }

    if (cfsetispeed(&options, baud) < 0 || cfsetospeed(&options, baud) < 0) {
        error_msg_ = "cfsetispeed/cfsetospeed failed";
        return false;
    }

    // Control flags
    options.c_cflag &= ~PARENB;  // No parity
    if (config_.parity == 'E' || config_.parity == 'e') {
        options.c_cflag |= PARENB;  // Enable parity
        options.c_cflag &= ~PARODD;  // Even parity
    } else if (config_.parity == 'O' || config_.parity == 'o') {
        options.c_cflag |= PARENB;  // Enable parity
        options.c_cflag |= PARODD;  // Odd parity
    }

    options.c_cflag &= ~CSTOPB;  // 1 stop bit
    if (config_.stop_bits == 2) {
        options.c_cflag |= CSTOPB;  // 2 stop bits
    }

    options.c_cflag &= ~CSIZE;  // Clear data size
    switch (config_.data_bits) {
        case 7:
            options.c_cflag |= CS7;
            break;
        case 8:
        default:
            options.c_cflag |= CS8;
            break;
    }

    options.c_cflag |= CREAD | CLOCAL;  // Enable receiver, ignore modem control lines

    // Enable hardware flow control (RTS/CTS) for RS-485
    if (config_.rs485) {
        options.c_cflag |= CRTSCTS;
    } else {
        options.c_cflag &= ~CRTSCTS;
    }

    // Input flags
    options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
    options.c_iflag |= IGNPAR;  // Ignore parity errors
    options.c_iflag &= ~(IXON | IXOFF | IXANY);  // No software flow control

    // Output flags
    options.c_oflag &= ~OPOST;  // No output processing

    // Local flags
    options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);

    // Set read timeout (inter-character timer)
    options.c_cc[VMIN] = 0;   // Non-blocking read
    options.c_cc[VTIME] = 1;  // 0.1 second timeout

    // Apply settings
    if (tcsetattr(fd_, TCSANOW, &options) < 0) {
        error_msg_ = "tcsetattr failed";
        return false;
    }

    // Configure RS-485 if requested (Linux specific)
#ifdef __linux__
    if (config_.rs485) {
        struct serial_rs485 rs485_conf;
        std::memset(&rs485_conf, 0, sizeof(rs485_conf));

        rs485_conf.flags |= SER_RS485_ENABLED;
        rs485_conf.flags |= SER_RS485_RTS_ON_SEND;  // RTS high during send
        rs485_conf.flags |= SER_RS485_RTS_AFTER_SEND;  // RTS low after send

        if (ioctl(fd_, TIOCSRS485, &rs485_conf) < 0) {
            std::cerr << "SerialPort: Failed to enable RS-485 mode ("
                      << strerror(errno) << ")" << std::endl;
            // Not fatal - continue without RS-485 mode
        }
    }
#endif

    return true;
}

void SerialPort::restore_termios() {
    if (fd_ >= 0) {
        tcsetattr(fd_, TCSANOW, &orig_termios_);
    }
}

int SerialPort::write(const uint8_t* data, size_t length) {
    if (fd_ < 0) {
        error_msg_ = "Port not open";
        return -1;
    }

    ssize_t written = ::write(fd_, data, length);
    if (written < 0) {
        error_msg_ = "write failed: ";
        error_msg_ += strerror(errno);
        return -1;
    }

    // Wait for data to be transmitted
    tcdrain(fd_);

    return static_cast<int>(written);
}

int SerialPort::write(const std::vector<uint8_t>& data) {
    return write(data.data(), data.size());
}

int SerialPort::read(uint8_t* buffer, size_t length, int timeout_ms) {
    if (fd_ < 0) {
        error_msg_ = "Port not open";
        return -1;
    }

    if (timeout_ms > 0) {
        // Use select for timeout
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(fd_, &read_fds);

        struct timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;

        int ret = select(fd_ + 1, &read_fds, nullptr, nullptr, &tv);
        if (ret < 0) {
            error_msg_ = "select failed: ";
            error_msg_ += strerror(errno);
            return -1;
        } else if (ret == 0) {
            return 0;  // Timeout
        }
    }

    ssize_t n = ::read(fd_, buffer, length);
    if (n < 0) {
        error_msg_ = "read failed: ";
        error_msg_ += strerror(errno);
        return -1;
    }

    return static_cast<int>(n);
}

std::vector<uint8_t> SerialPort::read_available() {
    std::vector<uint8_t> data;

    if (fd_ < 0) {
        return data;
    }

    int avail = available();
    if (avail <= 0) {
        return data;
    }

    data.resize(avail);
    int n = read(data.data(), avail);
    if (n > 0) {
        data.resize(n);
    } else {
        data.clear();
    }

    return data;
}

void SerialPort::flush(bool flush_tx, bool flush_rx) {
    if (fd_ < 0) {
        return;
    }

    int queue = 0;
    if (flush_tx) queue |= TCOFLUSH;
    if (flush_rx) queue |= TCIFLUSH;

    tcflush(fd_, queue);
}

int SerialPort::available() {
    if (fd_ < 0) {
        return 0;
    }

#ifdef __linux__
    int bytes = 0;
    if (ioctl(fd_, FIONREAD, &bytes) < 0) {
        return 0;
    }
    return bytes;
#else
    return 0;
#endif
}

void SerialPort::set_rts(bool state) {
    if (fd_ < 0) {
        return;
    }

    int bits = TIOCM_RTS;
    ioctl(fd_, state ? TIOCMBIS : TIOCMBIC, &bits);
}

std::vector<std::string> SerialPort::list_ports() {
    std::vector<std::string> ports;

#ifdef __linux__
    // Scan /dev for tty devices
    const char* dev_dir = "/dev";
    DIR* dir = opendir(dev_dir);
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string name = entry->d_name;
            // Look for tty* or ttyUSB* or ttyAMA*
            if (name.find("tty") == 0 ||
                name.find("ttyUSB") == 0 ||
                name.find("ttyAMA") == 0 ||
                name.find("ttyACM") == 0 ||
                name.find("ttymxc") == 0) {
                ports.push_back(std::string("/dev/") + name);
            }
        }
        closedir(dir);
    }
#endif

    return ports;
}

} // namespace osdp
} // namespace accessd
