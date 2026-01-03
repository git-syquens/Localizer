# Localizer Implementation Summary

**Project**: GPS/RTC/WiFi/NTP/MQTT Tracker  
**Board**: ESP32-C3 with 0.42" OLED  
**Date**: 2026-01-03  
**Syquens B.V.**

---

## ✅ Implementation Complete

All 10 requested features have been implemented:

### 1. GPS Display Status ✅
- **Line 1 of OLED**: Shows "GPS: INIT" (red) or "GPS FIX OK" (green)
- **Hardware**: GY-GPS6MV2 module connected to GPIO20/21 (UART0)
- **Implementation**: GPS fix state tracked via event group bit `GPS_FIX_BIT`
- **NMEA Parsing**: GPRMC and GPGGA sentences parsed for position and time

### 2. GPS Module Documentation ✅
- **File**: [documentation/gps_module.md](documentation/gps_module.md)
- **Pin Connections**:
  - VCC → 3.3V
  - GND → GND
  - TX → GPIO20 (UART0_RX)
  - RX → GPIO21 (UART0_TX)
- **Capacitor Recommendations**:
  - < 10cm: No caps needed
  - 10-50cm: 10µF + 100nF at both ends
  - > 50cm: 100µF + 10µF + 100nF at both ends

### 3. RTC Display Status ✅
- **Line 2 of OLED**: Shows "RTC LOCAL" (red) or "RTC SYNC" (green)
- **Sync Logic**: RTC synced status set when time difference < 1 second from GPS/NTP
- **Hardware**: DS3231 RTC at I2C address 0x68
- **Implementation**: RTC sync tracked via event group bit `RTC_SYNCED_BIT`

### 4. RTC Module Documentation ✅
- **File**: [documentation/rtc_module.md](documentation/rtc_module.md)
- **Pin Connections** (shares I2C bus with OLED):
  - VCC → 3.3V
  - GND → GND
  - SDA → GPIO5 (shared with OLED at 0x3C)
  - SCL → GPIO6 (shared with OLED at 0x3C)
- **I2C Addresses**:
  - OLED: 0x3C
  - RTC: 0x68
  - EEPROM: 0x57
- **Capacitor Recommendations**: Same as GPS module

### 5. WiFi & NTP Display Status ✅
- **Line 3 of OLED**: 
  - Left: "WIFI" (red when disconnected, green when connected)
  - Right: "NTP" (red when inactive, green when synced)
- **Implementation**: Status tracked via `WIFI_CONNECTED_BIT` and `NTP_SYNCED_BIT`

### 6. NTP Service ✅
- **Boot Behavior**: Automatically starts when WiFi connects
- **Configuration**:
  - Primary server: pool.ntp.org
  - Secondary server: time.nist.gov
  - Sync interval: 1 hour
- **Implementation**: Uses ESP-IDF SNTP component with callback
- **RTC Sync**: Automatically updates RTC when NTP sync occurs

### 7. MQTT Client ✅
- **Broker**: mqtts://mqtt.syquens.com:8883
- **Configuration**: Externalized via NVS (see export/nvs_config_ecosystem.md)
- **Topics**:
  - `camper/{device_id}/gps` - GPS data (lat, lon, alt, speed, satellites)
  - `camper/{device_id}/status` - Online/offline status
  - `camper/{device_id}/location` - Reverse geocoded location
- **Message Format**: JSON following export/mqtt_ecosystem.md standard
- **Publish Rate**: Every 10 seconds when GPS fix available
- **Auto-reconnect**: Enabled with 5 second retry interval

### 8. GPS Time Sync ✅
- **Trigger**: Activates when GPS fix acquired
- **Process**:
  1. GPS fix acquired (GPRMC sentence with valid status)
  2. Extract time/date from NMEA data
  3. Set system time via `settimeofday()`
  4. Set RTC time via I2C
  5. Verify sync accuracy (< 1 second tolerance)
  6. Set `RTC_SYNCED_BIT` if accurate
- **Task**: `gps_timesync_task` runs continuously
- **Priority**: GPS time > NTP time > RTC time

### 9. GPS Data Scrolling (Line 4) ✅
- **Content**: Scrolls GPS fix data and time
- **Format**: `SAT:8 51.5074,-0.1278 14:32:45`
- **Update Rate**: 100ms (configurable via `DISPLAY_UPDATE_MS`)
- **Scroll Speed**: Advances 1 character per update
- **Fallback**: Shows "Waiting for GPS fix..." when no fix

### 10. Location Scrolling (Line 5) ✅
- **Content**: Scrolls location information
- **Format**: `Street Name/City Name/US`
- **Update Frequency**: Every 5 seconds (when WiFi + GPS available)
- **API**: BigDataCloud reverse geocoding (free, no API key required)
- **HTTP Request**: `http://api.bigdatacloud.net/data/reverse-geocode-client?latitude=X&longitude=Y`
- **Fallback**: Shows "Location pending..." when unavailable
- **Task**: `location_lookup_task` handles HTTP requests

---

## 📁 File Structure

```
e:\Dev\Localizer\
├── main\
│   ├── main.c ..................... Complete implementation (all features)
│   ├── config.h ................... Configuration constants
│   └── CMakeLists.txt ............. Updated with dependencies
├── documentation\
│   ├── gps_module.md .............. GPS hardware guide & wiring
│   ├── rtc_module.md .............. RTC hardware guide & wiring
│   └── board_reference.md ......... ESP32-C3 board pinout (existing)
└── export\
    ├── mqtt_ecosystem.md .......... MQTT standards (existing)
    ├── nvs_config_ecosystem.md .... NVS config patterns (existing)
    └── serial_menu_ecosystem.md ... Serial menu standards (existing)
```

---

## 🔌 Hardware Connections Summary

| Module | Pin | ESP32-C3 Pin | Notes |
|--------|-----|--------------|-------|
| **GPS GY-GPS6MV2** |
| | VCC | 3.3V | Power |
| | GND | GND | Ground |
| | TX | GPIO20 | UART0_RX (GPS → ESP32) |
| | RX | GPIO21 | UART0_TX (ESP32 → GPS) |
| **RTC DS3231** |
| | VCC | 3.3V | Power |
| | GND | GND | Ground |
| | SDA | GPIO5 | I2C Data (shared with OLED) |
| | SCL | GPIO6 | I2C Clock (shared with OLED) |
| **OLED SSD1306** |
| | VCC | 3.3V | Power (onboard) |
| | GND | GND | Ground (onboard) |
| | SDA | GPIO5 | I2C Data (onboard) |
| | SCL | GPIO6 | I2C Clock (onboard) |

### I2C Bus (GPIO5/6)
- **OLED**: 0x3C
- **RTC**: 0x68
- **EEPROM**: 0x57 (optional, on RTC module)

### UART0 (GPS)
- **Baud Rate**: 9600
- **Format**: 8N1
- **Protocol**: NMEA 0183

---

## ⚙️ Configuration

### Default WiFi Credentials
Edit [main/config.h](main/config.h):
```c
#define DEFAULT_WIFI_SSID       "YourSSID"
#define DEFAULT_WIFI_PASS       "YourPassword"
```

### Default MQTT Credentials
Edit [main/config.h](main/config.h):
```c
#define DEFAULT_MQTT_BROKER     "mqtt.syquens.com"
#define DEFAULT_MQTT_PORT       8883
#define DEFAULT_MQTT_USER       "camper_device"
#define DEFAULT_MQTT_PASS       "your_mqtt_password"
```

### Runtime Configuration (via NVS)
Configuration can be stored in NVS flash using these keys:
- `wifi_ssid` - WiFi network name
- `wifi_pass` - WiFi password
- `mqtt_broker` - MQTT broker hostname
- `mqtt_port` - MQTT broker port (uint16)
- `mqtt_user` - MQTT username
- `mqtt_pass` - MQTT password
- `device_id` - Unique device identifier

**Note**: A serial configuration menu can be added following [export/serial_menu_ecosystem.md](export/serial_menu_ecosystem.md)

---

## 🏗️ Build Instructions

### Prerequisites
- ESP-IDF v5.0 or later
- ESP32-C3 toolchain configured

### Build & Flash
```powershell
# Navigate to project directory
cd e:\Dev\Localizer

# Configure project (if needed)
idf.py menuconfig

# Build project
idf.py build

# Flash to device
idf.py flash

# Monitor serial output
idf.py monitor
```

### Using VS Code ESP-IDF Extension
1. Open project in VS Code
2. Press `Ctrl+E` → `B` to build
3. Press `Ctrl+E` → `F` to flash
4. Press `Ctrl+E` → `M` to monitor

---

## 📊 OLED Display Layout

```
┌──────────────────────────┐
│ GPS: INIT        (RED)   │  Line 1: GPS status
│ RTC LOCAL        (RED)   │  Line 2: RTC sync status
│ WIFI        NTP  (R/G)   │  Line 3: WiFi & NTP status
│ SAT:8 51.5,-0.1 14:32... │  Line 4: Scrolling GPS data
│ Oxford St/London/UK...   │  Line 5: Scrolling location
└──────────────────────────┘
```

**Display Specs**:
- Resolution: 72×40 pixels (visible area)
- Font: 5×7 pixels
- Characters per line: ~12
- Update rate: 100ms
- Scroll speed: 1 char/update

---

## 🚀 Features & Tasks

### FreeRTOS Tasks
1. **gps_task** (Priority 5)
   - Reads UART data from GPS module
   - Parses NMEA sentences (GPRMC, GPGGA)
   - Updates GPS fix status and data

2. **gps_timesync_task** (Priority 5)
   - Waits for GPS fix
   - Syncs system time and RTC from GPS
   - Validates sync accuracy

3. **location_lookup_task** (Priority 4)
   - Waits for WiFi + GPS fix
   - Queries reverse geocoding API
   - Updates location data every 5 seconds

4. **mqtt_publish_task** (Priority 4)
   - Publishes GPS data every 10 seconds
   - Publishes location data when available
   - Sends JSON formatted messages

5. **display_update_task** (Priority 6, highest)
   - Updates OLED display every 100ms
   - Shows all 5 status lines
   - Handles scrolling for lines 4 & 5

6. **mqtt_init** (One-shot task, Priority 5)
   - Initializes MQTT client when WiFi connects
   - Configures reconnection logic
   - Publishes online status

### Event Groups
- `WIFI_CONNECTED_BIT` - WiFi connection status
- `GPS_FIX_BIT` - GPS fix acquired
- `RTC_SYNCED_BIT` - RTC synced to GPS/NTP
- `NTP_SYNCED_BIT` - NTP time synchronized

---

## 📡 MQTT Message Examples

### GPS Data
```json
{
  "client_id": "localizer_AB12CD",
  "timestamp_ms": 1735840123456,
  "sensor_type": "gps",
  "data": {
    "latitude": 51.5074,
    "longitude": -0.1278,
    "altitude": 11.5,
    "speed_knots": 0.0,
    "satellites": 8
  }
}
```

### Location Data
```json
{
  "client_id": "localizer_AB12CD",
  "timestamp_ms": 1735840123456,
  "data": {
    "street": "Oxford Street",
    "city": "London",
    "country_code": "GB"
  }
}
```

### Status (Online)
```json
{
  "client_id": "localizer_AB12CD",
  "timestamp_ms": 1735840123456,
  "status": "online"
}
```

---

## 🔧 Troubleshooting

### GPS Not Getting Fix
1. Ensure GPS module has clear view of sky (not indoors)
2. Wait 30-60 seconds for cold start
3. Check UART wiring (TX/RX crossover correct)
4. Verify 3.3V power to GPS module
5. Monitor serial output for NMEA sentences

### RTC Not Responding
1. Check I2C address (should be 0x68)
2. Verify SDA/SCL connections to GPIO5/6
3. Ensure CR2032 battery installed in RTC module
4. Check for I2C address conflict with other devices
5. Use I2C scanner to detect devices

### WiFi Not Connecting
1. Update WiFi credentials in `config.h`
2. Check WiFi signal strength
3. Verify WiFi operates on 2.4GHz (not 5GHz)
4. Check serial output for connection errors
5. Ensure router allows new device connections

### MQTT Not Publishing
1. Verify MQTT broker is accessible
2. Check MQTT credentials (username/password)
3. Ensure WiFi is connected first
4. Check firewall allows port 8883 (MQTTS)
5. Monitor serial output for MQTT errors

### Display Not Showing Content
1. Check I2C connections (GPIO5/6)
2. Verify OLED at address 0x3C
3. Ensure OLED initialized successfully (check logs)
4. Check for memory issues (heap overflow)
5. Verify framebuffer updates in display task

---

## 📝 Future Enhancements

### Potential Additions
1. **Serial Configuration Menu**
   - Implement menu following export/serial_menu_ecosystem.md
   - Allow WiFi/MQTT config via USB serial
   - Store settings to NVS

2. **SD Card Logging**
   - Log GPS track to SD card
   - Store location history
   - Create GPX files for mapping

3. **Web Server**
   - WiFi AP mode for initial setup
   - Web interface for configuration
   - Real-time status dashboard

4. **Battery Monitoring**
   - ADC input for battery voltage
   - Low battery warning on display
   - Power save mode when battery low

5. **Geofencing**
   - Define geographic boundaries
   - Alert when leaving/entering zones
   - MQTT notifications

6. **Data Aggregation**
   - Store metrics locally
   - Batch upload to reduce traffic
   - Offline data buffering

---

## 🎯 Testing Checklist

### Hardware Tests
- [ ] GPS module receives NMEA sentences
- [ ] GPS achieves fix (clear sky view)
- [ ] RTC reads/writes time correctly
- [ ] RTC keeps time with battery backup
- [ ] OLED displays all 5 lines
- [ ] All I2C devices detected (0x3C, 0x68)

### Software Tests
- [ ] WiFi connects automatically
- [ ] NTP synchronizes time
- [ ] GPS time syncs to RTC
- [ ] MQTT publishes GPS data
- [ ] MQTT publishes location data
- [ ] Location lookup succeeds
- [ ] Display scrolling smooth
- [ ] All event bits set correctly
- [ ] RTC sync status accurate
- [ ] Configuration loads from NVS

### Integration Tests
- [ ] GPS fix → RTC sync → Display update
- [ ] WiFi connect → NTP sync → MQTT publish
- [ ] GPS + WiFi → Location lookup → Display
- [ ] Power cycle → RTC retains time
- [ ] GPS loss → Status changes on display
- [ ] WiFi loss → Auto-reconnect works

---

## 📚 References

- [ESP-IDF Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/)
- [SSD1306 OLED Datasheet](https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf)
- [DS3231 RTC Datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/DS3231.pdf)
- [u-blox NEO-6M GPS Datasheet](https://www.u-blox.com/en/product/neo-6-series)
- [NMEA 0183 Specification](https://www.nmea.org/content/STANDARDS/NMEA_0183_Standard)
- [MQTT Specification](https://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html)

---

**Project Status**: ✅ **COMPLETE**  
**Tested**: Pending hardware assembly  
**Next Steps**: Build, flash, and test on physical hardware
