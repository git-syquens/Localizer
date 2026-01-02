# Localizer - System Architecture

**Version**: 1.0  
**Last Updated**: 2026-01-02

---

## Overview

This document describes the complete software and hardware architecture of the Localizer GPS logger and NTP time server.

## System Block Diagram

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           EXTERNAL SYSTEMS                              │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                           │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐         │
│  │  GPS Satellites │  │  MQTT Broker    │  │  NTP Pool       │         │
│  │  (GNSS)         │  │  mqtt.syquens   │  │  pool.ntp.org   │         │
│  └────────┬────────┘  └────────┬────────┘  └────────┬────────┘         │
│           │                    │                    │                   │
└───────────┼────────────────────┼────────────────────┼───────────────────┘
            │                    │                    │
            │ NMEA 0183          │ MQTT over TLS      │ NTP (UDP:123)
            │ @ 9600 baud        │ TCP:8883           │
            │                    │                    │
┌───────────┼────────────────────┼────────────────────┼───────────────────┐
│           ▼                    ▼                    ▼                   │
│  ┌────────────────────────────────────────────────────────────┐        │
│  │                   WiFi Stack (lwIP)                        │        │
│  │  - DHCP client      - TLS/SSL       - UDP/TCP              │        │
│  └────────────────────────────────────────────────────────────┘        │
│                                                                           │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │                 ESP32-C3 FreeRTOS Application                    │  │
│  ├──────────────────────────────────────────────────────────────────┤  │
│  │                                                                    │  │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐  │  │
│  │  │   GPS Handler   │  │   NTP Server    │  │  MQTT Client    │  │  │
│  │  │                 │  │                 │  │                 │  │  │
│  │  │ - UART RX       │  │ - UDP listener  │  │ - Publish GPS   │  │  │
│  │  │ - NMEA parser   │  │ - NTPv4 resp.   │  │ - Publish time  │  │  │
│  │  │ - Data extract  │  │ - Stratum 1/2   │  │ - Subscribe cmd │  │  │
│  │  │ - Validation    │  │ - Microsec acc. │  │ - LWT handling  │  │  │
│  │  └────────┬────────┘  └────────┬────────┘  └────────┬────────┘  │  │
│  │           │                    │                    │            │  │
│  │           └────────────────────┼────────────────────┘            │  │
│  │                                │                                 │  │
│  │  ┌─────────────────────────────┴───────────────────────────┐    │  │
│  │  │                    Time Manager                          │    │  │
│  │  │                                                           │    │  │
│  │  │  - Internet NTP sync (priority 1)                        │    │  │
│  │  │  - GPS PPS sync (priority 2, if available)               │    │  │
│  │  │  - RTC fallback (priority 3)                             │    │  │
│  │  │  - System clock management                               │    │  │
│  │  │  - Accuracy estimation                                   │    │  │
│  │  └─────────────────┬────────────────────┬───────────────────┘    │  │
│  │                    │                    │                        │  │
│  │  ┌─────────────────┴──────┐  ┌──────────┴─────────────────┐     │  │
│  │  │     RTC Driver         │  │   Serial Menu Handler       │     │  │
│  │  │     (DS3231)           │  │                             │     │  │
│  │  │                        │  │  - 'm' activation           │     │  │
│  │  │  - I2C communication   │  │  - Configuration UI         │     │  │
│  │  │  - Read/write time     │  │  - NVS integration          │     │  │
│  │  │  - Temperature sensor  │  │  - Input validation         │     │  │
│  │  └────────────────────────┘  └─────────────────────────────┘     │  │
│  │                                                                    │  │
│  │  ┌──────────────────────────────────────────────────────────┐    │  │
│  │  │              NVS Configuration Storage                    │    │  │
│  │  │  Namespace: "device_cfg"                                  │    │  │
│  │  │                                                            │    │  │
│  │  │  - wifi_ssid, wifi_pass                                   │    │  │
│  │  │  - mqtt_broker, mqtt_port, mqtt_client_id                 │    │  │
│  │  │  - gps_rx_pin, gps_tx_pin, gps_baud                       │    │  │
│  │  │  - rtc_sda_pin, rtc_scl_pin                               │    │  │
│  │  │  - ntp_enabled, timezone                                  │    │  │
│  │  └──────────────────────────────────────────────────────────┘    │  │
│  │                                                                    │  │
│  └────────────────────────────────────────────────────────────────────┘  │
│                                                                           │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │                     Hardware Abstraction Layer                   │  │
│  │  - GPIO  - UART  - I2C  - Timer  - WiFi  - Flash                 │  │
│  └──────────────────────────────────────────────────────────────────┘  │
│                                                                           │
│                         ESP-IDF v5.5 Framework                           │
└───────────────────────────────────────────────────────────────────────────┘
            │                    │                    │
            ▼                    ▼                    ▼
      ┌──────────┐         ┌──────────┐         ┌──────────┐
      │ GPS      │         │ DS3231   │         │ ESP32-C3 │
      │ Module   │         │ RTC      │         │ GPIO/I2C │
      │ UART     │         │ I2C      │         │ UART     │
      └──────────┘         └──────────┘         └──────────┘
```

---

## Component Design

### 1. GPS Handler

**Purpose**: Interface with GPS module, parse NMEA sentences, extract position data

**Implementation Details**:

```c
// GPS data structure
typedef struct {
    double latitude;         // Decimal degrees (WGS84)
    double longitude;        // Decimal degrees (WGS84)
    float altitude_m;        // Meters above sea level
    float speed_kmh;         // Ground speed (km/h)
    float heading_deg;       // True heading (degrees)
    uint8_t satellites;      // Number of satellites in use
    uint8_t fix_quality;     // 0=invalid, 1=GPS, 2=DGPS
    float hdop;              // Horizontal dilution of precision
    uint64_t timestamp_ms;   // GPS time (UTC)
    bool valid;              // Data validity flag
} gps_data_t;

// NMEA sentence parsers
bool parse_gpgga(const char *sentence, gps_data_t *data);
bool parse_gprmc(const char *sentence, gps_data_t *data);
bool parse_gpvtg(const char *sentence, gps_data_t *data);

// UART configuration
#define GPS_UART_NUM        UART_NUM_1
#define GPS_UART_BAUD_RATE  9600
#define GPS_RX_BUF_SIZE     1024
#define GPS_TX_BUF_SIZE     256
```

**State Machine**:
1. **Wait for '$'**: Start of NMEA sentence
2. **Buffer characters**: Until '*' (checksum marker)
3. **Validate checksum**: XOR of all characters between '$' and '*'
4. **Parse sentence**: Extract fields based on sentence type
5. **Update data**: Only if checksum valid and fix present

**Performance**:
- Update rate: 1 Hz (configurable to 5 Hz or 10 Hz)
- Latency: <50 ms from GPS to MQTT publish
- Buffer: Ring buffer for sentences (1024 bytes)

---

### 2. NTP Server

**Purpose**: Provide network time synchronization to ecosystem devices

**NTPv4 Packet Structure**:

```c
typedef struct {
    uint8_t li_vn_mode;          // Leap indicator (2), version (3), mode (3)
    uint8_t stratum;             // Stratum level (1=primary, 2=secondary)
    int8_t poll;                 // Poll interval (log2 seconds)
    int8_t precision;            // Precision (log2 seconds)
    uint32_t root_delay;         // Round-trip delay to primary source
    uint32_t root_dispersion;    // Dispersion to primary source
    uint32_t reference_id;       // Reference clock identifier
    uint64_t reference_timestamp;// Last time clock was set
    uint64_t origin_timestamp;   // Client timestamp (from request)
    uint64_t receive_timestamp;  // Server received request
    uint64_t transmit_timestamp; // Server sent response
} ntp_packet_t;
```

**Time Source Selection**:

```c
typedef enum {
    TIME_SOURCE_NONE,        // No time available
    TIME_SOURCE_INTERNET_NTP,// Internet NTP (most accurate)
    TIME_SOURCE_GPS,         // GPS time (very accurate)
    TIME_SOURCE_RTC,         // DS3231 RTC (fallback)
} time_source_t;

// Stratum assignment
// Internet NTP synced → Stratum 2 (secondary reference)
// GPS PPS synced      → Stratum 1 (primary reference)
// RTC only            → Stratum 3 (tertiary reference)
```

**Algorithm**:

1. **Receive Request**: UDP packet on port 123
2. **Capture Timestamps**:
   - `receive_timestamp`: When packet arrived (microsecond precision)
   - `transmit_timestamp`: When response sent
3. **Echo Origin**: Copy client's transmit timestamp to origin field
4. **Set Stratum**: Based on current time source
5. **Send Response**: UDP reply to client

**Accuracy Factors**:
- Network latency: ±1-10 ms (LAN)
- System clock accuracy: ±5 ms (Internet NTP synced)
- With GPS PPS: <1 ms (requires hardware interrupt)

---

### 3. RTC Driver (DS3231)

**Purpose**: Battery-backed real-time clock for time continuity

**I2C Register Map**:

| Register | Address | Function |
|----------|---------|----------|
| Seconds | 0x00 | BCD seconds (0-59) |
| Minutes | 0x01 | BCD minutes (0-59) |
| Hours | 0x02 | BCD hours (0-23) |
| Day | 0x03 | Day of week (1-7) |
| Date | 0x04 | BCD date (1-31) |
| Month | 0x05 | BCD month (1-12) |
| Year | 0x06 | BCD year (0-99) |
| Temperature | 0x11-0x12 | Temperature (10-bit, 0.25°C resolution) |

**Driver Functions**:

```c
// Initialize I2C and verify DS3231 presence
esp_err_t ds3231_init(i2c_port_t i2c_num, int sda_pin, int scl_pin);

// Read current time (BCD to binary conversion)
esp_err_t ds3231_get_time(struct tm *timeinfo);

// Set time (binary to BCD conversion)
esp_err_t ds3231_set_time(const struct tm *timeinfo);

// Read temperature sensor
float ds3231_get_temperature(void);

// Set aging offset (±127, calibration)
esp_err_t ds3231_set_aging_offset(int8_t offset);
```

**Accuracy Specifications**:
- Typical: ±2 ppm (< 1 minute/year)
- Temperature compensated: -40°C to +85°C
- Aging offset: Adjustable for long-term drift compensation

**Battery Backup**:
- CR2032 lithium cell (220 mAh)
- Backup current: <1 µA
- Expected life: >5 years
- Auto-switchover when VCC drops

---

### 4. Time Manager

**Purpose**: Centralized time synchronization and source management

**Architecture**:

```c
typedef struct {
    time_source_t current_source;      // Active time source
    uint64_t last_ntp_sync_ms;         // Last successful NTP sync
    uint64_t last_gps_sync_ms;         // Last GPS time update
    uint64_t last_rtc_update_ms;       // Last RTC write
    uint32_t estimated_accuracy_ms;    // Current accuracy estimate
    bool ntp_sync_in_progress;         // Sync operation active
} time_manager_state_t;
```

**Synchronization Strategy**:

1. **Boot**:
   - Read RTC time → Set system clock
   - Mark source as `TIME_SOURCE_RTC`
   - Estimated accuracy: ±1000 ms

2. **WiFi Connected**:
   - Start NTP sync with `pool.ntp.org`
   - On success: Update system clock + RTC
   - Mark source as `TIME_SOURCE_INTERNET_NTP`
   - Estimated accuracy: ±10 ms

3. **GPS Fix Acquired**:
   - Extract GPS time from NMEA sentence
   - Compare with system clock (sanity check)
   - If difference > 500 ms: Update system clock
   - Optional: Use PPS for microsecond sync
   - Mark source as `TIME_SOURCE_GPS`
   - Estimated accuracy: ±100 ms (without PPS), ±1 ms (with PPS)

4. **Periodic Maintenance**:
   - Re-sync with internet NTP every 1 hour
   - Update RTC every 24 hours (reduce flash wear)
   - Monitor clock drift (accuracy degradation)

**Failover Logic**:

```
Internet available → Internet NTP (best)
    ↓ Fail
GPS fix available → GPS time
    ↓ Fail
RTC battery OK → RTC time (fallback)
    ↓ Fail
No accurate time available → Log error, continue with last known time
```

---

### 5. MQTT Client

**Purpose**: Publish GPS data and status to ecosystem broker

**Connection State Machine**:

```
[Disconnected] ──WiFi OK──→ [Connecting] ──CONNACK──→ [Connected]
      ↑                           │                        │
      │                           │ Fail                   │
      │                           ↓                        │
      │                    [Retry (backoff)]               │
      │                                                    │
      └────────────────────── Network loss ────────────────┘
```

**Published Messages**:

**GPS Data** (1 Hz):
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

**Time Sync Status** (every 5 minutes):
```json
{
  "client_id": "localizer_AB12CD",
  "timestamp_ms": 1735840123456,
  "sensor_type": "time_sync",
  "data": {
    "source": "internet_ntp",
    "accuracy_ms": 5,
    "last_sync_ms": 1735840000000,
    "ntp_server": "pool.ntp.org",
    "rtc_temperature_c": 25.5
  }
}
```

**Last Will & Testament** (LWT):
```json
Topic: camper/localizer_AB12CD/status
Payload: {"client_id": "localizer_AB12CD", "status": "offline"}
QoS: 1
Retain: true
```

---

### 6. Serial Menu Handler

**Purpose**: User-friendly configuration interface via UART

**Menu Structure**:

```
Main Menu
├── 1. WiFi Settings
│   ├── 1. Set SSID
│   ├── 2. Set Password
│   ├── 3. Test Connection
│   └── Q. Back
├── 2. MQTT Configuration
│   ├── 1. Set Broker Address
│   ├── 2. Set Client ID
│   ├── 3. Set Username/Password
│   ├── 4. Test Connection
│   └── Q. Back
├── 3. GPS Configuration
│   ├── 1. Set UART Pins (RX/TX)
│   ├── 2. Set Baud Rate
│   ├── 3. View Live NMEA
│   └── Q. Back
├── 4. NTP Server Settings
│   ├── 1. Enable/Disable Server
│   ├── 2. View Statistics
│   ├── 3. Force Time Sync
│   └── Q. Back
├── 5. System Information
│   ├── Current time & source
│   ├── GPS status & position
│   ├── RTC temperature
│   ├── NVS statistics
│   ├── WiFi connection info
│   └── Uptime & memory
├── 6. Factory Reset
└── Q. Quit Menu
```

**Input Validation**:
- WiFi SSID: 1-32 characters, printable ASCII
- WiFi Password: 8-63 characters
- MQTT Broker: Valid hostname or IP
- GPIO Pins: Valid ESP32-C3 GPIO numbers
- Baud Rate: 4800, 9600, 19200, 38400, 57600, 115200

---

## FreeRTOS Task Design

### Task Priority Rationale

```c
// Higher number = higher priority
#define GPS_TASK_PRIORITY       5   // High: Real-time GPS data
#define NTP_SERVER_TASK_PRIORITY 6  // Highest: Time-critical responses
#define MQTT_TASK_PRIORITY      4   // Medium-high: Regular publishing
#define TIME_SYNC_TASK_PRIORITY 3   // Medium: Background sync
#define SERIAL_MENU_TASK_PRIORITY 2 // Low: User interaction
```

**Reasoning**:
- **NTP Server** (6): Must respond to client requests with minimal latency
- **GPS Handler** (5): Real-time sensor data capture (UART interrupts)
- **MQTT Client** (4): Important but tolerates brief delays
- **Time Sync** (3): Background task, not time-critical
- **Serial Menu** (2): Lowest priority, interactive (human latency acceptable)

### Task Stack Sizes

```c
#define GPS_TASK_STACK_SIZE         4096  // NMEA parsing buffers
#define NTP_SERVER_TASK_STACK_SIZE  4096  // UDP socket buffers
#define MQTT_TASK_STACK_SIZE        8192  // TLS/SSL overhead
#define TIME_SYNC_TASK_STACK_SIZE   2048  // Minimal processing
#define SERIAL_MENU_TASK_STACK_SIZE 4096  // Printf buffers, cJSON
```

### Inter-Task Communication

**Queues**:
```c
// GPS data queue (GPS task → MQTT task)
QueueHandle_t gps_data_queue;
typedef struct {
    gps_data_t gps_data;
    uint64_t timestamp_ms;
} gps_queue_item_t;

// Command queue (MQTT task → GPS/NTP tasks)
QueueHandle_t command_queue;
typedef struct {
    char command[32];
    char params[128];
} command_t;
```

**Mutexes**:
```c
// Protect shared time source state
SemaphoreHandle_t time_manager_mutex;

// Protect NVS access (serial menu + MQTT commands)
SemaphoreHandle_t nvs_mutex;
```

**Event Groups**:
```c
// System events
EventGroupHandle_t system_events;
#define WIFI_CONNECTED_BIT      BIT0
#define MQTT_CONNECTED_BIT      BIT1
#define GPS_FIX_ACQUIRED_BIT    BIT2
#define NTP_SYNCED_BIT          BIT3
```

---

## Memory Management

### Flash Partition Table

```csv
# Name,      Type, SubType,  Offset,   Size,     Flags
nvs,         data, nvs,      0x9000,   0x6000,   # 24KB NVS
phy_init,    data, phy,      0xf000,   0x1000,   # 4KB PHY
factory,     app,  factory,  0x10000,  0x200000, # 2MB App
```

**Partition Sizes**:
- **NVS** (24KB): Stores ~50 configuration keys
- **PHY** (4KB): WiFi calibration data
- **Factory** (2MB): Application firmware

### RAM Usage Estimate

| Component | Heap (bytes) | Stack (bytes) | Total |
|-----------|--------------|---------------|-------|
| GPS Task | 2048 | 4096 | 6144 |
| NTP Server | 1024 | 4096 | 5120 |
| MQTT Client | 8192 | 8192 | 16384 |
| Time Sync | 512 | 2048 | 2560 |
| Serial Menu | 2048 | 4096 | 6144 |
| cJSON | 4096 | - | 4096 |
| WiFi/lwIP | 32768 | - | 32768 |
| **Total** | **~50KB** | **~22KB** | **~72KB** |

**ESP32-C3 Resources**:
- Total RAM: 400KB
- Available after system: ~300KB
- **Usage**: ~72KB (24% utilization)
- **Headroom**: 76% free for buffers, TLS, etc.

---

## Power Management

### Active Mode (GPS + WiFi + MQTT)

```c
Component          Current (mA)
ESP32-C3 active    ~80
WiFi TX (peak)     ~350 (brief bursts)
WiFi RX            ~100
GPS module         ~50
Total average      ~200 mA @ 3.3V = 660 mW
```

### Low-Power Mode (Optional future enhancement)

```c
// GPS sleep mode (no fix needed)
gps_enter_sleep_mode();  // Reduces GPS to ~10 mA

// WiFi modem sleep (keep connection, reduce power)
esp_wifi_set_ps(WIFI_PS_MIN_MODEM);  // Reduces WiFi to ~30 mA

Total average      ~120 mA @ 3.3V = 396 mW
```

### RTC Battery Backup

```c
Standby current    <1 µA
CR2032 capacity    220 mAh
Battery life       220000 / 0.001 = 220,000 hours
                   ~25 years (theoretical)
                   ~5-10 years (practical, self-discharge)
```

---

## Security Considerations

### MQTT TLS/SSL

```c
// Certificate-based authentication
esp_mqtt_client_config_t mqtt_cfg = {
    .broker.address.uri = "mqtts://mqtt.syquens.com:8883",
    .broker.verification.certificate = (const char *)server_cert_pem_start,
    .credentials.username = "camper_device",
    .credentials.authentication.password = "<password>",
};
```

**Encryption**: TLS 1.2 with AES-128-GCM or AES-256-GCM

### WiFi Security

- **WPA2-PSK**: Minimum requirement
- **WPA3**: Supported on ESP32-C3 (recommended)

### NVS Security

- **Password storage**: Plaintext in NVS (acceptable for low-risk environment)
- **Alternative**: Enable flash encryption for sensitive deployments
- **Access control**: Physical access to device = full access to NVS

### NTP Security

- **No authentication**: Standard NTP (port 123) is unauthenticated
- **Mitigation**: Restrict to local network only (firewall rule)
- **Future**: NTPv4 authentication (MD5 signatures) if needed

---

## Error Handling & Recovery

### Watchdog Timers

```c
// Task watchdog (detect hung tasks)
esp_task_wdt_init(30, true);  // 30 second timeout, panic on timeout
esp_task_wdt_add(gps_task_handle);
esp_task_wdt_add(ntp_server_task_handle);
esp_task_wdt_add(mqtt_task_handle);

// Interrupt watchdog (detect interrupt storm)
// Enabled by default in ESP-IDF
```

### Automatic Recovery Strategies

| Failure | Detection | Recovery |
|---------|-----------|----------|
| GPS UART timeout | No data for 10s | Reinitialize UART, log error |
| MQTT disconnect | Event callback | Exponential backoff reconnect |
| WiFi disconnect | Event callback | Auto-reconnect, fallback AP mode |
| NTP sync fail | Timeout (5s) | Retry with different server |
| RTC I2C error | I2C transaction fail | Retry 3x, then use system clock |
| NVS corruption | nvs_flash_init() error | Erase and reinitialize (factory reset) |
| Out of memory | malloc() returns NULL | Log error, skip operation |
| Task stack overflow | FreeRTOS check | Panic and reboot (logged) |

---

## Testing & Validation

### Unit Tests

- NMEA parser: Valid/invalid sentences, checksum validation
- NTP packet encoding/decoding
- RTC BCD conversion (binary ↔ BCD)
- Time source selection logic

### Integration Tests

- GPS → MQTT flow (end-to-end latency)
- NTP client synchronization (compare with external NTP)
- Time source failover (disconnect internet, verify RTC fallback)
- Serial menu (automate input, verify NVS writes)

### Performance Tests

- NTP response latency (measure with Wireshark)
- GPS update rate stability (verify 1 Hz ±10%)
- MQTT publish rate (1 Hz sustained)
- Memory leak detection (run for 24+ hours)

---

## Scalability & Future Enhancements

### Potential Features

1. **Multi-constellation GNSS**: Support GLONASS, Galileo, BeiDou (requires compatible GPS module)
2. **SD Card Logging**: Local storage for GPS tracks (offline resilience)
3. **Web Configuration**: HTTP server alternative to serial menu
4. **OTA Firmware Updates**: Remote firmware updates via MQTT
5. **GPS Geofencing**: Trigger alerts when entering/exiting defined areas
6. **Dead Reckoning**: Estimate position during GPS outages (IMU integration)
7. **Stratum 0 Mode**: GPS disciplined oscillator (GPSDO) with atomic clock accuracy

### Hardware Variants

- **ESP32-S3**: Dual-core, more RAM (for web UI)
- **External Flash**: 4MB+ for logging
- **LoRa Module**: Long-range communication (non-internet environments)

---

## References

### GPS / GNSS

- NMEA 0183 Standard: [https://www.nmea.org/](https://www.nmea.org/)
- u-blox NEO-M8 Receiver Description: Hardware integration guide

### NTP

- RFC 5905: Network Time Protocol Version 4
- [https://www.ntp.org/](https://www.ntp.org/)

### ESP-IDF

- ESP-IDF Programming Guide: [https://docs.espressif.com/projects/esp-idf/en/v5.5/](https://docs.espressif.com/projects/esp-idf/en/v5.5/)
- FreeRTOS Documentation: Task scheduling, queues, semaphores

### DS3231 RTC

- DS3231 Datasheet: I2C real-time clock with temperature compensation
- Maxim Integrated Application Notes

---

**Document Version**: 1.0  
**Author**: V.N. Verbon (Syquens B.V.)  
**Last Updated**: 2026-01-02
