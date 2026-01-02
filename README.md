# Localizer - GPS Logger & NTP Time Server

**ESP32-C3 based GPS logging device with integrated NTP time server for camper ecosystem**

## Overview

Localizer is a dual-purpose device that:
1. **GPS Logger**: Continuously logs GPS position, speed, and altitude data via MQTT
2. **NTP Time Server**: Provides precise time synchronization to other sensors in the camper ecosystem using an RTC module with battery backup

This device ensures time continuity across all sensors even when internet connectivity is lost, making it the authoritative time source for the local network.

## Hardware Requirements

- **ESP32-C3** development board
- **GPS Module**: NEO-6M, NEO-7M, or compatible UART GPS (NMEA 0183)
- **RTC Module**: DS3231 high-precision RTC with I2C interface
- **CR2032 Battery**: For RTC backup power
- **Power Supply**: 5V USB or 3.3V regulated

## Key Features

- ✅ GPS position logging to MQTT (1 Hz update rate)
- ✅ NTP server for local network time synchronization
- ✅ RTC battery backup maintains time during power loss
- ✅ Automatic NTP sync when internet available
- ✅ Fallback to RTC when offline
- ✅ Serial configuration menu (ecosystem standard)
- ✅ NVS persistent configuration storage
- ✅ MQTT integration with `mqtt.syquens.com`
- ✅ Millisecond-precision timestamps (ecosystem standard)

## Quick Start

### 1. Build and Flash

```bash
# Project is already configured for ESP32-C3
idf.py build
idf.py -p COM3 flash monitor
```

Replace `COM3` with your serial port.

### 2. Initial Configuration

Press `m` in the serial monitor to open the configuration menu:

```
1. WiFi Settings          - Configure network credentials
2. MQTT Configuration     - Set broker and client ID
3. GPS Configuration      - Set GPS UART pins
4. NTP Server Settings    - Enable/configure NTP service
5. System Information     - View device status
```

### 3. Verify Operation

Monitor the serial output for:
- GPS fix status and coordinates
- NTP server listening on UDP port 123
- MQTT publish confirmations
- Time synchronization status

## Documentation

See the [`documentation/`](documentation/) folder for detailed information:

- **[README.md](documentation/README.md)** - Comprehensive project documentation
- **[architecture.md](documentation/architecture.md)** - System architecture and design
- **[gps_logging.md](documentation/gps_logging.md)** - GPS integration and data format
- **[ntp_service.md](documentation/ntp_service.md)** - NTP server implementation
- **[configuration.md](documentation/configuration.md)** - Configuration guide

## Ecosystem Integration

This project follows the **Camper Ecosystem Standards**:
- MQTT communication standard ([mqtt_ecosystem.md](export/mqtt_ecosystem.md))
- NVS configuration patterns ([nvs_config_ecosystem.md](export/nvs_config_ecosystem.md))
- Serial menu interface ([serial_menu_ecosystem.md](export/serial_menu_ecosystem.md))
- Timestamp standard ([timestamp_ecosystem.md](export/timestamp_ecosystem.md))

## MQTT Topics

Published by this device:

| Topic | Update Rate | Description |
|-------|-------------|-------------|
| `camper/localizer_<MAC>/gps` | 1 Hz | GPS position, speed, altitude |
| `camper/localizer_<MAC>/status` | On change | Device online/offline status |
| `camper/localizer_<MAC>/time_sync` | Every 5 min | NTP sync status, time source |

## Development

### Prerequisites

- ESP-IDF v5.5 installed at `e:\Dev\Espressif\frameworks\esp-idf-v5.5`
- Python 3.11+ (included with ESP-IDF)
- Terminal emulator (PuTTY, Tera Term, or screen)

### Project Structure

```
Localizer/
├── main/
│   ├── main.c              - Application entry point
│   ├── gps_handler.c       - GPS parsing and logging
│   ├── ntp_server.c        - NTP server implementation
│   ├── rtc_driver.c        - DS3231 RTC interface
│   ├── mqtt_client.c       - MQTT communication
│   ├── serial_menu.c       - Configuration menu
│   └── CMakeLists.txt
├── documentation/          - Project documentation
├── export/                 - Ecosystem standards
├── CMakeLists.txt
└── sdkconfig.defaults
```

## License

Proprietary - Syquens B.V.

## Maintainer

**V.N. Verbon** - Syquens B.V.  
Version: 1.0  
Last Updated: 2026-01-02
