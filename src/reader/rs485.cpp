#include "rs485.hpp"

#ifdef WINDOWS_PLATFORM
    #include <windows.h>
    #include <iostream>
    #include <cstring>
#else
    #include <cstring>
    #include <system_error>
    #include <iostream>
    #include <fcntl.h>
    #include <unistd.h>
#endif

namespace accessd {

#ifdef WINDOWS_PLATFORM

// ============================================================================
// Windows Implementation
// ============================================================================

RS485::RS485() : handle_(INVALID_HANDLE_VALUE) {}

RS485::~RS485() {
    close();
}

bool RS485::is_open() const {
    return handle_ != INVALID_HANDLE_VALUE;
}

bool RS485::open(const RS485Config& config) {
    config_ = config;

    // Convert port name (e.g., COM1, COM10)
    std::string win_port = config.port;
    if (win_port.find("COM") == 0 || win_port.find("com") == 0) {
        win_port = "\\\\.\\" + config.port;
    }

    // Open serial port
    handle_ = CreateFileA(
        win_port.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (handle_ == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open " << config.port
                  << " (Error: " << GetLastError() << ")" << std::endl;
        return false;
    }

    // Configure port
    if (!set_windows_comm_config()) {
        CloseHandle((HANDLE)handle_);
        handle_ = INVALID_HANDLE_VALUE;
        return false;
    }

    std::cout << "RS485 opened: " << config.port
              << " at " << config.baud_rate << " baud" << std::endl;

    return true;
}

void RS485::close() {
    if (handle_ != INVALID_HANDLE_VALUE) {
        CloseHandle((HANDLE)handle_);
        handle_ = INVALID_HANDLE_VALUE;
        std::cout << "RS485 closed: " << config_.port << std::endl;
    }
}

ssize_t RS485::read(uint8_t* buffer, size_t size) {
    if (handle_ == INVALID_HANDLE_VALUE) return -1;

    DWORD bytesRead = 0;
    if (!ReadFile((HANDLE)handle_, buffer, (DWORD)size, &bytesRead, NULL)) {
        return -1;
    }
    return (ssize_t)bytesRead;
}

ssize_t RS485::write(const uint8_t* buffer, size_t size) {
    if (handle_ == INVALID_HANDLE_VALUE) return -1;

    DWORD bytesWritten = 0;
    if (!WriteFile((HANDLE)handle_, buffer, (DWORD)size, &bytesWritten, NULL)) {
        return -1;
    }
    return (ssize_t)bytesWritten;
}

void RS485::flush_input() {
    if (handle_ != INVALID_HANDLE_VALUE) {
        PurgeComm((HANDLE)handle_, PURGE_RXCLEAR);
    }
}

void RS485::flush_output() {
    if (handle_ != INVALID_HANDLE_VALUE) {
        PurgeComm((HANDLE)handle_, PURGE_TXCLEAR);
    }
}

void RS485::flush() {
    if (handle_ != INVALID_HANDLE_VALUE) {
        PurgeComm((HANDLE)handle_, PURGE_RXCLEAR | PURGE_TXCLEAR);
    }
}

bool RS485::set_windows_comm_config() {
    DCB dcb = {0};
    dcb.DCBlength = sizeof(DCB);

    // Get current config
    if (!GetCommState((HANDLE)handle_, &dcb)) {
        std::cerr << "GetCommState failed" << std::endl;
        return false;
    }

    // Set baud rate
    dcb.BaudRate = baud_to_windows_baud(config_.baud_rate);

    // Set data bits
    dcb.ByteSize = (BYTE)config_.data_bits;

    // Set stop bits
    if (config_.stop_bits == 2) {
        dcb.StopBits = TWOSTOPBITS;
    } else {
        dcb.StopBits = ONESTOPBIT;
    }

    // Set parity
    switch (config_.parity) {
        case 'E':
            dcb.Parity = EVENPARITY;
            break;
        case 'O':
            dcb.Parity = ODDPARITY;
            break;
        case 'N':
        default:
            dcb.Parity = NOPARITY;
            break;
    }

    // Set flow control
    dcb.fOutxCtsFlow = config_.rtscts ? TRUE : FALSE;
    dcb.fRtsControl = config_.rtscts ? RTS_CONTROL_HANDSHAKE : RTS_CONTROL_DISABLE;

    // No software flow control
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;

    // Binary mode, no EOF checking
    dcb.fBinary = TRUE;
    dcb.fNull = FALSE;

    // Set timeouts (read immediately, write immediately)
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 0;

    if (!SetCommTimeouts((HANDLE)handle_, &timeouts)) {
        std::cerr << "SetCommTimeouts failed" << std::endl;
        return false;
    }

    // Apply DCB settings
    if (!SetCommState((HANDLE)handle_, &dcb)) {
        std::cerr << "SetCommState failed" << std::endl;
        return false;
    }

    return true;
}

DWORD RS485::baud_to_windows_baud(int baud) {
    switch (baud) {
        case 9600:   return CBR_9600;
        case 19200:  return CBR_19200;
        case 38400:  return CBR_38400;
        case 57600:  return CBR_57600;
        case 115200: return CBR_115200;
        default:     return CBR_9600;
    }
}

#else // POSIX_PLATFORM

// ============================================================================
// POSIX (Linux/Unix) Implementation
// ============================================================================

RS485::RS485() : fd_(-1) {}

RS485::~RS485() {
    close();
}

bool RS485::is_open() const {
    return fd_ >= 0;
}

bool RS485::open(const RS485Config& config) {
    config_ = config;

    // Open serial port
    fd_ = ::open(config.port.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd_ < 0) {
        std::cerr << "Failed to open " << config.port << ": "
                  << strerror(errno) << std::endl;
        return false;
    }

    // Save original settings
    if (tcgetattr(fd_, &orig_termios_) < 0) {
        std::cerr << "Failed to get termios: " << strerror(errno) << std::endl;
        ::close(fd_);
        fd_ = -1;
        return false;
    }

    // Configure port
    if (!set_termios()) {
        // Restore original settings
        tcsetattr(fd_, TCSANOW, &orig_termios_);
        ::close(fd_);
        fd_ = -1;
        return false;
    }

    // Flush buffers
    flush();

    std::cout << "RS485 opened: " << config.port
              << " at " << config.baud_rate << " baud" << std::endl;

    return true;
}

void RS485::close() {
    if (fd_ >= 0) {
        // Restore original settings
        tcsetattr(fd_, TCSANOW, &orig_termios_);
        ::close(fd_);
        fd_ = -1;
        std::cout << "RS485 closed: " << config_.port << std::endl;
    }
}

ssize_t RS485::read(uint8_t* buffer, size_t size) {
    if (fd_ < 0) return -1;
    return ::read(fd_, buffer, size);
}

ssize_t RS485::write(const uint8_t* buffer, size_t size) {
    if (fd_ < 0) return -1;
    return ::write(fd_, buffer, size);
}

void RS485::flush_input() {
    if (fd_ >= 0) {
        tcflush(fd_, TCIFLUSH);
    }
}

void RS485::flush_output() {
    if (fd_ >= 0) {
        tcflush(fd_, TCOFLUSH);
    }
}

void RS485::flush() {
    if (fd_ >= 0) {
        tcflush(fd_, TCIOFLUSH);
    }
}

bool RS485::set_termios() {
    struct termios termios;

    // Get current settings
    if (tcgetattr(fd_, &termios) < 0) {
        std::cerr << "tcgetattr failed: " << strerror(errno) << std::endl;
        return false;
    }

    // Set input flags
    termios.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
    termios.c_iflag |= (IGNPAR);  // Ignore parity errors

    // Set output flags
    termios.c_oflag &= ~(OPOST);

    // Set control flags
    termios.c_cflag &= ~(CBAUD | CBAUDEX | CSIZE | PARENB | CRTSCTS);
    termios.c_cflag |= (CS8 | CREAD | CLOCAL);

    // Set RTS/CTS
    if (config_.rtscts) {
        termios.c_cflag |= CRTSCTS;
    }

    // Set local flags
    termios.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);

    // Set baud rate
    speed_t speed = baud_to_speed(config_.baud_rate);
    cfsetospeed(&termios, speed);
    cfsetispeed(&termios, speed);

    // Set data bits (already CS8)
    // Set stop bits
    if (config_.stop_bits == 2) {
        termios.c_cflag |= CSTOPB;
    }

    // Set parity
    switch (config_.parity) {
        case 'E':
            termios.c_cflag |= PARENB;
            termios.c_cflag &= ~PARODD;
            break;
        case 'O':
            termios.c_cflag |= PARENB;
            termios.c_cflag |= PARODD;
            break;
        case 'N':
        default:
            termios.c_cflag &= ~PARENB;
            break;
    }

    // Apply settings
    if (tcsetattr(fd_, TCSANOW, &termios) < 0) {
        std::cerr << "tcsetattr failed: " << strerror(errno) << std::endl;
        return false;
    }

    return true;
}

speed_t RS485::baud_to_speed(int baud) {
    switch (baud) {
        case 9600:   return B9600;
        case 19200:  return B19200;
        case 38400:  return B38400;
        case 57600:  return B57600;
        case 115200: return B115200;
        default:     return B9600;
    }
}

#endif // POSIX_PLATFORM

} // namespace accessd
