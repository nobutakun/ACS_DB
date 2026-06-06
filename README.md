# Access Control System (ACS-DB)

Access Control System with OSDP support for Raspberry Pi 4 and Linux.

## Overview

This is an access control system that supports OSDP (Open Supervised Device Protocol) readers via RS485 communication. It uses **libosdp** for OSDP protocol handling, providing secure, encrypted communication with access control devices.

## Features

- ✅ **OSDP Support**: Full OSDP 2.2 protocol support via libosdp
- ✅ **AES-128 Encryption**: Secure channel with encrypted communication
- ✅ **RS485 Communication**: Multi-drop RS485 bus support
- ✅ **SQLite Database**: Local credential storage and access logging
- ✅ **Multiple Reader Types**: Card, fingerprint, face recognition, QR code
- ✅ **Access Rules**: Configurable access control rules
- ✅ **Event Logging**: Complete audit trail
- ✅ **Door Control**: Integration with door locks and sensors
- ✅ **LED/Buzzer Control**: Reader feedback control

## Quick Start

Based on [libosdp official documentation](https://doc.osdp.dev/libosdp/build-and-install).

### Option 1: Using System libosdp (Recommended)

```bash
# Install libosdp system-wide first
git clone https://github.com/goToMain/libosdp --recurse-submodules
cd libosdp
cmake -B build . -DCMAKE_BUILD_TYPE=Release
sudo cmake --install build

# Then build this project
cd accessd
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
```

### Option 2: Automatic (CMake fetches libosdp)

```bash
cd accessd
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## libosdp Integration

This project uses libosdp's CMake package:

```cmake
find_package(LibOSDP CONFIG REQUIRED)
target_link_libraries(accessd PRIVATE libosdp::osdp)
```

### Why libosdp?

- **Production-Ready**: Battle-tested OSDP 2.2 implementation
- **Secure Channel**: AES-128 encryption built-in
- **Compliant**: Full OSDP specification compliance
- **Maintained**: Active development and community support
- **Cross-Platform**: Linux, Raspberry Pi, Windows, bare-metal

## Configuration

Edit `/etc/accessd/accessd.conf`:

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

## Adding OSDP Readers

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

## Project Structure

```
accessd/
├── CMakeLists.txt           # Uses find_package(LibOSDP)
├── README.md                # This file
├── BUILD_RASPBERRYPI.md     # Raspberry Pi build guide
├── include/                 # Header files
├── src/
│   ├── reader/
│   │   ├── osdp_connection.cpp  # OSDP CP (uses libosdp C API)
│   │   └── rs485.cpp           # RS485 driver
│   └── ...
└── config/                  # Configuration files
```

## Old OSDP Implementation (Deprecated)

The previous implementation in `include/osdp/` and `src/osdp/` is deprecated and not compiled.

## Running

```bash
# Manual
/usr/local/bin/accessd /etc/accessd/accessd.conf

# Systemd service
sudo systemctl start accessd
sudo systemctl enable accessd
```

## Troubleshooting

| Issue | Solution |
|-------|----------|
| libosdp not found | Install: `sudo cmake --install ~/libosdp/build` |
| Serial permission | `sudo usermod -a -G dialout $USER` |
| OSDP handshake fails | Verify SCBK key, check logs |

## Resources

- [libosdp Official Docs](https://doc.osdp.dev)
- [Build Guide](BUILD_RASPBERRYPI.md)
- [libosdp GitHub](https://github.com/goToMain/libosdp)
