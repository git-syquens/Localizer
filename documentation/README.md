# Localizer - Complete Documentation

**ESP32-C3 GPS Logger & NTP Time Server**  
**Version**: 1.0  
**Last Updated**: 2026-01-02  
**Target Hardware**: ESP32-C3 + GPS Module + DS3231 RTC

---

## Table of Contents

1. [Project Overview](#project-overview)
2. [Hardware Setup](#hardware-setup)
3. [Software Architecture](#software-architecture)
4. [Configuration Guide](#configuration-guide)
5. [GPS Logging](#gps-logging)
6. [NTP Time Service](#ntp-time-service)
7. [MQTT Integration](#mqtt-integration)
8. [Troubleshooting](#troubleshooting)
9. [API Reference](#api-reference)

---

## Project Overview

### Purpose

Localizer serves as the **authoritative time and location source** for the camper ecosystem. It combines two critical functions:

1. **GPS Logging**: Continuous tracking of vehicle position, speed, altitude, and heading
2. **NTP Time Server**: Provides synchronized time to all ecosystem devices via local network

### Why This Matters

**Problem**: In remote locations (mountains, forests, deserts), internet connectivity is unreliable. Without internet:
- Sensors lose time synchronization (NTP servers unreachable)
- Data timestamps drift, making correlation impossible
- No GPS positioning data for context

**Solution**: Localizer maintains accurate time via battery-backed RTC and provides both:
- Local NTP server for sensor time synchronization
- GPS position logging when satellites are visible

### Design Philosophy

- **Resilience**: Continue operation without internet (RTC backup)
- **Accuracy**: DS3231 RTC ±2ppm (< 1 minute/year drift)
- **Reliability**: Battery backup ensures time continuity during power loss
- **Standards compliance**: Follows ecosystem MQTT, NVS, and timestamp standards

---

## Hardware Setup

### Bill of Materials (BOM)

| Component | Part Number | Quantity | Purpose | Notes |
|-----------|-------------|----------|---------|-------|
| ESP32-C3 Dev Board | ESP32-C3-DevKitC-02 | 1 | Main controller | RISC-V, WiFi, 160MHz |
| GPS Module | NEO-6M or NEO-7M | 1 | Position/time | UART, NMEA 0183 |
| RTC Module | DS3231 | 1 | Precision timekeeping | I2C, ±2ppm accuracy |
| CR2032 Battery | CR2032 | 1 | RTC backup power | 220mAh, 3V |
| Jumper Wires | - | 10+ | Connections | Dupont male-female |

### Wiring Diagram

#### GPS Module → ESP32-C3

| GPS Pin | ESP32-C3 Pin | Function |
|---------|--------------|----------|
| VCC | 3.3V | Power |
| GND | GND | Ground |
| TX | GPIO20 (RX) | GPS transmit → ESP receive |
| RX | GPIO21 (TX) | GPS receive ← ESP transmit (optional) |
| PPS | GPIO10 | Pulse-per-second (optional, high accuracy) |

#### DS3231 RTC → ESP32-C3

| RTC Pin | ESP32-C3 Pin | Function |
|---------|--------------|----------|
| VCC | 3.3V | Power |
| GND | GND | Ground |
| SDA | GPIO8 (I2C_SDA) | I2C data |
| SCL | GPIO9 (I2C_SCL) | I2C clock |
| 32K | - | Not used (optional square wave output) |
| SQW | - | Not used |

#### Power Supply

- **USB 5V**: Standard development/testing
- **Battery/Solar**: For permanent installation (requires 5V regulator or 3.3V direct)

### Assembly Notes

1. **GPS Antenna**: External antenna recommended for vehicle installation (roof mount)
2. **RTC Battery**: Insert CR2032 before first power-up to initialize RTC
3. **I2C Pull-ups**: DS3231 module typically includes 4.7kΩ pull-ups (verify if using bare chip)
4. **GPS Fix Time**: Cold start may take 30-60 seconds in open sky

---

## Software Architecture

See [architecture.md](architecture.md) for detailed system design.

### Component Overview

```
┌─────────────────────────────────────────────────────────────┐
│                     ESP32-C3 Main Application                │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐       │
│  │ GPS Handler  │  │  NTP Server  │  │ MQTT Client  │       │
│  │              │  │              │  │              │       │
│  │ - Parse NMEA │  │ - UDP:123    │  │ - Publish    │       │
│  │ - Extract    │  │ - Respond to │  │ - Subscribe  │       │
│  │   position   │  │   requests   │  │ - LWT        │       │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘       │
│         │                 │                 │               │
│         └─────────────────┼─────────────────┘               │
│                           │                                 │
│  ┌──────────────┐  ┌──────┴───────┐  ┌──────────────┐       │
│  │ RTC Driver   │  │ Time Manager │  │ Serial Menu  │       │
│  │ (DS3231)     │  │              │  │              │       │
│  │              │  │ - NTP sync   │  │ - Config     │       │
│  │ - Read time  │  │ - RTC backup │  │ - Calibrate  │       │
│  │ - Set time   │  │ - System clk │  │ - Info       │       │
│  └──────────────┘  └──────────────┘  └──────────────┘       │
│                                                               │
│  ┌──────────────────────────────────────────────────┐        │
│  │              NVS Configuration Storage            │        │
│  │  - WiFi credentials  - GPS settings               │        │
│  │  - MQTT broker       - NTP config                 │        │
│  └──────────────────────────────────────────────────┘        │
└─────────────────────────────────────────────────────────────┘
```

### Task Architecture (FreeRTOS)

| Task Name | Priority | Stack Size | Function |
|-----------|----------|------------|----------|
| `gps_task` | 5 | 4096 | Parse GPS NMEA sentences, publish to MQTT |
| `ntp_server_task` | 6 | 4096 | Listen for NTP requests, respond with time |
| `mqtt_task` | 4 | 8192 | Handle MQTT events, publish data |
| `time_sync_task` | 3 | 2048 | Sync with internet NTP, update RTC |
| `serial_menu_task` | 2 | 4096 | Configuration menu interface |

### Boot Sequence

1. **NVS Initialization**: Load configuration from flash
2. **RTC Initialization**: Read current time from DS3231
3. **Set System Clock**: Use RTC time as initial value
4. **WiFi Connection**: Connect to configured network
5. **NTP Sync**: Sync with internet NTP (if available)
6. **Update RTC**: Write NTP time to RTC
7. **GPS Initialization**: Start UART reception
8. **NTP Server Start**: Begin listening on UDP:123
9. **MQTT Connection**: Connect to broker, publish online status
10. **Main Loop**: Continuous GPS logging and NTP serving

---

## Configuration Guide

See [configuration.md](configuration.md) for step-by-step setup instructions.

### Quick Configuration Checklist

- [ ] Connect to serial terminal (115200 baud)
- [ ] Press `m` to open menu
- [ ] Configure WiFi SSID and password
- [ ] Set MQTT broker address (`mqtt.syquens.com`)
- [ ] Set device client ID (auto-generated from MAC or custom)
- [ ] Configure GPS UART pins (default: RX=GPIO20, TX=GPIO21)
- [ ] Enable NTP server (default: enabled)
- [ ] Set timezone offset for display (UTC±12)
- [ ] Save and reboot

### Essential Settings

```
WiFi SSID:          <your_network>
WiFi Password:      <your_password>
MQTT Broker:        mqtt.syquens.com
MQTT Port:          8883
MQTT Client ID:     localizer_<MAC>
GPS UART RX Pin:    GPIO20
GPS UART TX Pin:    GPIO21
GPS Baud Rate:      9600
RTC I2C SDA Pin:    GPIO8
RTC I2C SCL Pin:    GPIO9
NTP Server Enabled: Yes
Timezone Offset:    +1 (CET) or 0 (UTC)
```

---

## GPS Logging

See [gps_logging.md](gps_logging.md) for detailed GPS implementation.

### Supported NMEA Sentences

- **$GPGGA**: Time, position, fix quality, satellites
- **$GPRMC**: Position, speed, course, date
- **$GPVTG**: Course and speed
- **$GPGSA**: DOP and active satellites

### Published Data Format

**Topic**: `camper/localizer_<MAC>/gps`

**Message** (1 Hz):
```json
{
  "client_id": "localizer_AB12CD",
  "timestamp_ms": 1735840123456,
  "sensor_type": "gps",
  "data": {
    "latitude": 52.370216,
    "longitude": 4.895168,
    "altitude_m": 12.5,
    "speed_kmh": 45.3,
    "heading_deg": 270.5,
    "satellites": 8,
    "fix_quality": 1,
    "hdop": 1.2
  }
}
```

### Fix Quality Codes

| Code | Meaning |
|------|---------|
| 0 | Invalid / No fix |
| 1 | GPS fix (SPS) |
| 2 | DGPS fix |
| 3 | PPS fix |
| 4 | Real-Time Kinematic (RTK) |
| 5 | Float RTK |
| 6 | Estimated (dead reckoning) |

---

## NTP Time Service

See [ntp_service.md](ntp_service.md) for NTP server implementation details.

### How It Works

1. **NTP Client Request**: Device sends UDP packet to port 123
2. **Localizer Response**: Returns current time with microsecond precision
3. **Client Sync**: Device updates system clock

### Time Source Priority

1. **Internet NTP** (when available): `pool.ntp.org`, `time.google.com`
2. **GPS PPS** (if connected): Pulse-per-second for sub-millisecond accuracy
3. **DS3231 RTC**: Battery-backed fallback during offline periods

### NTP Server Configuration

```c
// Default settings (configurable via serial menu)
#define NTP_SERVER_PORT        123
#define NTP_STRATUM            1      // Primary time source (GPS)
#define NTP_PRECISION          -20    // ~1 microsecond (2^-20 seconds)
#define NTP_POLL_INTERVAL      64     // Recommend client poll every 64s
```

### Client Configuration Example

**ESP32 Client** (using another ESP32 as NTP client):
```c
// In menuconfig or sdkconfig.defaults
CONFIG_LWIP_SNTP_SERVERS="localizer.local"
CONFIG_LWIP_SNTP_UPDATE_DELAY=60000  // Sync every 60 seconds

// Or via code
sntp_setservername(0, "localizer.local");
sntp_init();
```

**Linux/Raspberry Pi**:
```bash
# Edit /etc/systemd/timesyncd.conf
[Time]
NTP=localizer.local
FallbackNTP=pool.ntp.org
```

---

## MQTT Integration

### Connection Parameters

```c
Broker:      mqtt.syquens.com
Port:        8883 (TLS/SSL)
Protocol:    MQTT v3.1.1
Username:    camper_device
Password:    <stored in NVS>
Client ID:   localizer_<MAC_ADDRESS>
Keep-Alive:  60 seconds
Clean Session: false (persistent session for QoS 1/2)
```

### Topics Published

| Topic | QoS | Retain | Update Rate | Payload |
|-------|-----|--------|-------------|---------|
| `camper/localizer_<MAC>/gps` | 0 | No | 1 Hz | GPS position data |
| `camper/localizer_<MAC>/status` | 1 | Yes | On change | Online/offline status |
| `camper/localizer_<MAC>/time_sync` | 1 | Yes | Every 5 min | NTP sync status |

### Topics Subscribed

| Topic | QoS | Purpose |
|-------|-----|---------|
| `camper/localizer_<MAC>/cmd/#` | 1 | Remote commands |

### Command Examples

**Force GPS re-acquisition**:
```json
Topic: camper/localizer_AB12CD/cmd/gps_reset
Payload: {"command": "gps_reset"}
```

**Trigger NTP sync**:
```json
Topic: camper/localizer_AB12CD/cmd/sync_time
Payload: {"command": "sync_time"}
```

**Response**:
```json
Topic: camper/localizer_AB12CD/rsp/sync_time
Payload: {
  "command": "sync_time",
  "status": "success",
  "result": {
    "time_source": "internet_ntp",
    "sync_time_ms": 1735840123456,
    "accuracy_ms": 5
  }
}
```

---

## Troubleshooting

### GPS Issues

**No GPS fix**:
- Check antenna placement (clear sky view required)
- Verify UART wiring (TX → RX crossover)
- Monitor NMEA sentences in serial output
- Cold start can take 30-60 seconds

**Incorrect coordinates**:
- Verify NMEA parsing code
- Check for magnetic vs. true north heading
- Confirm WGS84 datum

### NTP Issues

**Clients not syncing**:
- Verify firewall allows UDP:123
- Check network connectivity (ping test)
- Confirm NTP server task is running
- Use `ntpdate -q localizer.local` for testing

**Time drift**:
- Check DS3231 battery voltage (should be >2.8V)
- Verify I2C communication (no errors in logs)
- Calibrate RTC aging offset if needed

### MQTT Issues

**Connection refused**:
- Verify WiFi connection established
- Check broker credentials in NVS
- Test with `mosquitto_sub` from PC
- Ensure TLS certificates valid

**Messages not publishing**:
- Check MQTT_EVENT_CONNECTED in logs
- Verify topic names match standard
- Test with QoS 1 (guaranteed delivery)

### Time Synchronization Issues

**System time incorrect**:
- Check NTP sync status in menu
- Verify RTC time (`i2cget` command)
- Force NTP sync via serial command
- Check timezone offset setting

---

## API Reference

### GPS Handler

```c
// Initialize GPS module
esp_err_t gps_init(uart_port_t uart_num, int rx_pin, int tx_pin, int baud_rate);

// Parse NMEA sentence
bool gps_parse_nmea(const char *sentence, gps_data_t *data);

// Get last known position
esp_err_t gps_get_position(gps_data_t *data);

// Check if fix is valid
bool gps_has_fix(void);
```

### RTC Driver

```c
// Initialize DS3231 RTC
esp_err_t rtc_init(i2c_port_t i2c_num, int sda_pin, int scl_pin);

// Read current time
esp_err_t rtc_get_time(struct tm *timeinfo);

// Set RTC time
esp_err_t rtc_set_time(const struct tm *timeinfo);

// Get temperature from RTC
float rtc_get_temperature(void);
```

### NTP Server

```c
// Initialize and start NTP server
esp_err_t ntp_server_init(uint16_t port);

// Stop NTP server
void ntp_server_stop(void);

// Get server statistics
esp_err_t ntp_server_get_stats(ntp_stats_t *stats);
```

### Time Manager

```c
// Sync with internet NTP
esp_err_t time_sync_with_ntp(const char *server);

// Update RTC from system time
esp_err_t time_update_rtc(void);

// Get time source (NTP, RTC, GPS)
time_source_t time_get_source(void);

// Get sync accuracy estimate
uint32_t time_get_accuracy_ms(void);
```

---

## Performance Specifications

| Parameter | Value | Notes |
|-----------|-------|-------|
| GPS Update Rate | 1 Hz | Configurable up to 10 Hz (NEO-M8) |
| NTP Accuracy | <10 ms | With GPS PPS: <1 ms |
| RTC Accuracy | ±2 ppm | < 1 minute/year drift |
| MQTT Latency | <100 ms | On local network |
| Power Consumption | ~200 mA | @ 3.3V (GPS + ESP32-C3 active) |
| Standby Power | <1 µA | RTC only (battery backup) |
| Battery Life | ~5 years | CR2032 for RTC backup |

---

## Safety & Legal

### GPS Usage

- **Not for safety-critical navigation**: This is a logging device, not a certified navigation system
- **Driver distraction**: Do not interact with serial menu while driving
- **Data privacy**: GPS logs may contain personal location data (GDPR considerations)

### Radio Compliance

- ESP32-C3 WiFi: FCC/CE certified (check module certification)
- GPS receiver: Passive, no transmission

### Disclaimer

This device is provided for educational and personal use. The authors assume no liability for damages resulting from use of this device or data it produces.

---

## Ecosystem Standards Compliance

This project implements:
- ✅ **MQTT Standard**: Topic structure, QoS, LWT ([mqtt_ecosystem.md](../export/mqtt_ecosystem.md))
- ✅ **NVS Configuration**: Key naming, defaults, safe writes ([nvs_config_ecosystem.md](../export/nvs_config_ecosystem.md))
- ✅ **Serial Menu**: 'm' activation, input validation ([serial_menu_ecosystem.md](../export/serial_menu_ecosystem.md))
- ✅ **Timestamp Standard**: uint64_t milliseconds UTC ([timestamp_ecosystem.md](../export/timestamp_ecosystem.md))

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-01-02 | Initial release |

---

## Support

**Maintainer**: V.N. Verbon (Syquens B.V.)  
**Repository**: Git repository `Localizer/`  
**Documentation**: [documentation/](.)  
**Ecosystem Standards**: [export/](../export/)

For issues, suggestions, or contributions, contact the maintainer.
