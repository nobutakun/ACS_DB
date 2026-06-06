# Build and Install Guide (Raspberry Pi 4)

Official guide based on [libosdp documentation](https://doc.osdp.dev/libosdp/build-and-install).

## Overview

This project uses **libosdp** for OSDP protocol handling. You can either:
1. Install libosdp system-wide (recommended for production)
2. Let CMake fetch and build libosdp automatically (for development)

## Option 1: Install libosdp System-Wide (Recommended)

### Step 1: Install Dependencies

```bash
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libsqlite3-dev
```

### Step 2: Build and Install libosdp

```bash
# Clone libosdp
cd ~
git clone https://github.com/goToMain/libosdp --recurse-submodules
cd libosdp

# Build
cmake -B build . -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel $(nproc)

# Install to system (/usr/local/lib and /usr/local/include)
sudo cmake --install build

# Verify installation
pkg-config --modversion libosdp
```

### Step 3: Build This Project

```bash
cd ~/accessd
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
```

## Option 2: Automatic Fetch (Development)

CMake will automatically download and build libosdp if not found system-wide:

```bash
cd ~/accessd
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## Option 3: Using vcpkg (Recommended for Windows)

```bash
# Install vcpkg
git clone https://github.com/microsoft/vcpkg
cd vcpkg
./bootstrap-vcpkg.sh

# Install libosdp
./vcpkg install libosdp

# Build project with vcpkg toolchain
cd ~/accessd
cmake -B build -DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

## libosdp Build Options

libosdp can be configured with CMake options:

```bash
cmake -B build . \
    -DCMAKE_BUILD_TYPE=Release \
    -DOPT_OSDP_PACKET_TRACE=OFF \    # Enable packet trace for debugging
    -DOPT_OSDP_DATA_TRACE=OFF \      # Enable data buffer tracing
    -DOPT_BUILD_SHARED=ON \           # Build shared library
    -DOPT_BUILD_STATIC=ON             # Build static library
```

| Option | Default | Description |
|--------|---------|-------------|
| `OPT_OSDP_PACKET_TRACE` | OFF | Raw packet trace for diagnostics |
| `OPT_OSDP_DATA_TRACE` | OFF | Command/reply data tracing |
| `OPT_BUILD_SHARED` | ON | Build shared library (.so) |
| `OPT_BUILD_STATIC` | ON | Build static library (.a) |

## Serial Port Configuration

### Disable Serial Console (for UART pins)

```bash
sudo raspi-config
# Interface Options → Serial Port
# Disable serial console, enable serial hardware
```

### Add User to Dialout Group

```bash
sudo usermod -a -G dialout $USER
# Logout and login again
```

### Configure RS485 Port

```bash
# Check available serial ports
ls -l /dev/tty*

# Common ports:
# /dev/ttyUSB0 - USB-to-RS485 adapter
# /dev/ttyAMA0 - Raspberry Pi UART pins
```

## Configuration Files

### Application Config

```bash
sudo nano /etc/accessd/accessd.conf
```

```ini
[rs485]
port = /dev/ttyUSB0
baudrate = 9600

[database]
path = /var/lib/accessd/accessd.db

[logging]
level = INFO
file = /var/log/accessd/accessd.log
```

## Systemd Service

Create `/etc/systemd/system/accessd.service`:

```ini
[Unit]
Description=Access Control System
After=network.target

[Service]
Type=simple
ExecStart=/usr/local/bin/accessd /etc/accessd/accessd.conf
Restart=always
RestartSec=5
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
```

```bash
sudo systemctl daemon-reload
sudo systemctl enable accessd
sudo systemctl start accessd
sudo systemctl status accessd
```

## Testing OSDP Connection

### View Logs

```bash
sudo journalctl -u accessd -f
```

### Monitor Serial Traffic

```bash
sudo apt install minicom
sudo minicom -D /dev/ttyUSB0 -b 9600
```

### Test with libosdp PD Simulator

```bash
# On another terminal or device
cd ~/libosdp/build
./examples/pd_example -p /dev/ttyUSB1 -a 0
```

## OSDP Reader Database Configuration

```sql
INSERT INTO readers (
    device_uid, name, reader_type, connection_type,
    osdp_address, baudrate, use_encryption, rs485_port, master_key
) VALUES (
    'READER001', 'Main Entrance', 'card', 'osdp',
    0, 9600, 1, '/dev/ttyUSB0', 
    X'0123456789ABCDEF0123456789ABCDEF'
);
```

### Reader Fields

| Field | Description | Example |
|-------|-------------|---------|
| `osdp_address` | PD address (0-127) | 0, 1, 2... |
| `baudrate` | Baud rate | 9600, 19200, 38400, 57600, 115200 |
| `use_encryption` | Enable AES-128 | 0 or 1 |
| `rs485_port` | Serial port | /dev/ttyUSB0, /dev/ttyAMA0 |
| `master_key` | SCBK (16 bytes) | Hex string (32 hex chars) |

## Troubleshooting

### libosdp Not Found

```bash
# Check if installed
pkg-config --modversion libosdp

# If not found, build from source
cd ~/libosdp/build
sudo cmake --install .
```

### Serial Port Permission

```bash
sudo usermod -a -G dialout $USER
# Then logout and login again
```

### OSDP Handshake Issues

Enable debug logging in accessd.conf:

```ini
[logging]
level = DEBUG
```

Check logs:

```bash
sudo journalctl -u accessd -f | grep OSDP
```

### RS485 Bus Issues

- Check termination resistors (120Ω at both ends)
- Verify correct baud rate on all devices
- Check ground connections
- Verify OSDP addresses are unique

## Cross-Compilation (From Windows to Raspberry Pi)

```bash
# Install ARM toolchain
sudo apt install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf

# Configure for ARM
mkdir build && cd build
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-linux-gnueabihf.cmake \
    -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## Production Deployment

### 1. Build Release Binary

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
```

### 2. Create Required Directories

```bash
sudo mkdir -p /var/lib/accessd
sudo mkdir -p /var/log/accessd
sudo chown $USER:$USER /var/lib/accessd
sudo chown $USER:$USER /var/log/accessd
```

### 3. Enable and Start Service

```bash
sudo systemctl enable accessd
sudo systemctl start accessd
sudo systemctl status accessd
```

## Resources

- [libosdp Official Docs](https://doc.osdp.dev)
- [libosdp GitHub](https://github.com/goToMain/libosdp)
- [libosdp Build and Install](https://doc.osdp.dev/libosdp/build-and-install)
- [Raspberry Pi Serial Config](https://www.raspberrypi.com/documentation/computing/raspberry-pi.html)
