# Timestamp Standard - Camper Ecosystem

**Version**: 1.0
**Last Updated**: 2026-01-02
**Applies to**: All ESP32/embedded devices in camper ecosystem

---

## Overview

This document defines the **mandatory timestamp standard** for all sensor data published within the camper ecosystem. All devices (ESP32 boards, Raspberry Pi, Arduino, etc.) MUST use this exact format for data correlation and time-series analysis.

## Standard Definition

### Format Specification

- **Type**: `uint64_t` (unsigned 64-bit integer)
- **Unit**: Milliseconds since Unix epoch (January 1, 1970 00:00:00 UTC)
- **Precision**: 1 millisecond (0.001 seconds)
- **Range**: Valid until year 2262
- **Timezone**: Always UTC (never local time)

### Mandatory Implementation Pattern

**C/C++ (ESP32, Arduino)**:
```c
#include <sys/time.h>
#include <time.h>

// Get current time with millisecond precision
struct timeval tv;
gettimeofday(&tv, NULL);

// Calculate milliseconds since Unix epoch
uint64_t timestamp_ms = (uint64_t)tv.tv_sec * 1000ULL + (uint64_t)(tv.tv_usec / 1000);
```

**Python (Raspberry Pi, Linux)**:
```python
import time

# Get current time in milliseconds
timestamp_ms = int(time.time() * 1000)
```

**JavaScript/Node.js**:
```javascript
// Get current time in milliseconds
const timestamp_ms = Date.now();
```

### JSON Message Format

All MQTT messages MUST include the timestamp field:

```json
{
  "client_id": "device_identifier",
  "timestamp_ms": 1735840123456,
  "sensor_type": "level|gps|temperature|voltage|etc",
  ...additional sensor-specific fields
}
```

**Field Requirements**:
- **Field name**: `"timestamp_ms"` (exact spelling, lowercase)
- **JSON type**: Number (NOT string)
- **Always present**: Every message MUST have this field

## Why This Standard?

### 1. Cross-Sensor Correlation
- GPS location + vehicle level + temperature = complete environmental picture
- Millisecond precision enables accurate event timing
- Example: Correlate vehicle tilt with GPS position during cornering

### 2. No Timezone Confusion
- All timestamps in UTC
- Timezone conversion happens during data analysis/visualization
- No ambiguity during DST transitions
- International travel: timestamp remains consistent

### 3. Future-Proof
- 64-bit integer supports 292 million years of timestamps
- No Y2K38 problem (unlike 32-bit `time_t`)
- Compatible with all modern platforms

### 4. Cross-Platform Compatibility
- Python, JavaScript, SQL, Excel all understand Unix timestamps
- Easy conversion to human-readable dates
- Standard across IoT ecosystem

### 5. Data Analysis Ready
- Time-series databases (InfluxDB, TimescaleDB) native support
- Pandas DataFrame indexing
- Easy to plot, filter, aggregate

## Converting Timestamps

### To Human-Readable DateTime

**Python**:
```python
from datetime import datetime

timestamp_ms = 1735840123456

# UTC datetime
dt_utc = datetime.utcfromtimestamp(timestamp_ms / 1000.0)
print(dt_utc.strftime("%Y-%m-%d %H:%M:%S.%f")[:-3])
# Output: 2025-01-02 12:15:23.456

# With timezone (example: Europe/Amsterdam)
import pytz
local_tz = pytz.timezone('Europe/Amsterdam')
dt_local = datetime.fromtimestamp(timestamp_ms / 1000.0, tz=pytz.UTC)
dt_local = dt_local.astimezone(local_tz)
print(dt_local.strftime("%Y-%m-%d %H:%M:%S.%f %Z")[:-3])
# Output: 2025-01-02 13:15:23.456 CET
```

**JavaScript**:
```javascript
const timestamp_ms = 1735840123456;

// ISO 8601 format
const isoString = new Date(timestamp_ms).toISOString();
console.log(isoString);  // 2025-01-02T12:15:23.456Z

// Localized format
const options = {
  year: 'numeric', month: '2-digit', day: '2-digit',
  hour: '2-digit', minute: '2-digit', second: '2-digit',
  fractionalSecondDigits: 3
};
console.log(new Date(timestamp_ms).toLocaleString('nl-NL', options));
// 02-01-2025 13:15:23,456 (Dutch locale, CET)
```

**SQL (PostgreSQL)**:
```sql
-- Convert milliseconds to timestamp
SELECT to_timestamp(1735840123456 / 1000.0) AS datetime;
-- Output: 2025-01-02 12:15:23.456+00

-- Format as string
SELECT to_char(
  to_timestamp(timestamp_ms / 1000.0),
  'YYYY-MM-DD HH24:MI:SS.MS'
) AS formatted_time
FROM sensor_data;
```

**Excel/LibreOffice Calc**:
```
# In cell A1: timestamp_ms value (1735840123456)
# In cell B1: Convert to Excel datetime
=(A1/1000/86400)+DATE(1970,1,1)

# Format cell B1 as: YYYY-MM-DD HH:MM:SS
```

### From Human-Readable DateTime

**Python**:
```python
from datetime import datetime

# From ISO string
dt = datetime.fromisoformat("2025-01-02T12:15:23.456")
timestamp_ms = int(dt.timestamp() * 1000)

# From string with format
dt = datetime.strptime("2025-01-02 12:15:23.456", "%Y-%m-%d %H:%M:%S.%f")
timestamp_ms = int(dt.timestamp() * 1000)
```

**JavaScript**:
```javascript
// From ISO string
const timestamp_ms = new Date("2025-01-02T12:15:23.456Z").getTime();

// From components
const timestamp_ms = Date.UTC(2025, 0, 2, 12, 15, 23, 456);  // Month is 0-indexed
```

## Time Synchronization Requirements

All devices MUST synchronize their system clocks:

### ESP32 Devices
- **Method**: NTP (Network Time Protocol)
- **Servers**: `pool.ntp.org`, `time.google.com`
- **Sync on**: WiFi connection established
- **Auto-resync**: Every 1 hour (ESP-IDF default)
- **Before publishing**: Verify time is synced (not epoch start)

**Validation Example** (C):
```c
time_t now;
time(&now);
if (now < 1704067200) {  // Before 2024-01-01
    ESP_LOGW(TAG, "Time not synced, skipping sensor publish");
    return;
}
// Proceed with timestamp creation
```

### Raspberry Pi / Linux
- **Method**: systemd-timesyncd or chrony
- **Configuration**: `/etc/systemd/timesyncd.conf`
- **Servers**: Debian/Ubuntu pool by default
- **Verify sync**: `timedatectl status`

### Arduino (with RTC)
- **Method**: DS3231 RTC module
- **Initial sync**: Set via NTP or manual
- **Fallback**: Battery-backed RTC maintains time during power loss
- **Drift**: ±2 ppm (< 1 minute/year)

## Data Correlation Examples

### Correlating Level + GPS Data

**Python with pandas**:
```python
import pandas as pd

# Load level sensor data (pitch/roll from ESP32)
level_df = pd.read_json('level_data.json')
# Columns: timestamp_ms, client_id, pitch, roll

# Load GPS data (from separate GPS module)
gps_df = pd.read_csv('gps_data.csv')
# Columns: timestamp_ms, latitude, longitude, speed

# Merge on nearest timestamp (500ms tolerance)
merged = pd.merge_asof(
    level_df.sort_values('timestamp_ms'),
    gps_df.sort_values('timestamp_ms'),
    on='timestamp_ms',
    direction='nearest',
    tolerance=500
)

# Now you have: timestamp_ms, pitch, roll, latitude, longitude, speed
print(merged.head())
```

### Time-Series Plotting

**Python with matplotlib**:
```python
import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_json('sensor_data.json')
df['datetime'] = pd.to_datetime(df['timestamp_ms'], unit='ms', utc=True)
df = df.set_index('datetime')

# Plot sensor value over time
plt.figure(figsize=(12, 6))
plt.plot(df.index, df['sensor_value'])
plt.xlabel('Time (UTC)')
plt.ylabel('Sensor Value')
plt.title('Sensor Data Over Time')
plt.grid(True)
plt.show()
```

## Common Mistakes to Avoid

### ❌ WRONG: Using relative time
```c
// DON'T DO THIS
uint64_t timestamp_ms = millis();  // Relative to boot, not synchronized!
```

### ❌ WRONG: Second-precision only
```c
// DON'T DO THIS
time_t now = time(NULL);
uint64_t timestamp_ms = now * 1000;  // Missing sub-second data!
```

### ❌ WRONG: Local timezone
```c
// DON'T DO THIS
struct tm local_time;
localtime_r(&now, &local_time);  // Use gmtime_r() instead!
```

### ❌ WRONG: String format in JSON
```json
// DON'T DO THIS
{
  "timestamp_ms": "1735840123456"  // String, not number!
}
```

### ✅ CORRECT: Standard implementation
```c
struct timeval tv;
gettimeofday(&tv, NULL);
uint64_t timestamp_ms = (uint64_t)tv.tv_sec * 1000ULL + (uint64_t)(tv.tv_usec / 1000);

cJSON_AddNumberToObject(root, "timestamp_ms", (double)timestamp_ms);
```

## Testing and Validation

### Verify Device Time is Correct

**ESP32 Serial Console**:
```
I (12345) main: Current time: 2025-01-02 12:15:23 UTC
```

**Raspberry Pi**:
```bash
timedatectl
# Output should show:
#   System clock synchronized: yes
#   NTP service: active
```

### Timestamp Accuracy Test

**Python script** (subscribe to MQTT and compare):
```python
import paho.mqtt.client as mqtt
import json
import time

def on_message(client, userdata, msg):
    data = json.loads(msg.payload)
    device_ts = data['timestamp_ms']
    system_ts = int(time.time() * 1000)
    diff_ms = abs(device_ts - system_ts)

    print(f"Device: {device_ts}, System: {system_ts}, Diff: {diff_ms}ms")

    if diff_ms > 1000:
        print("⚠️  WARNING: Time difference > 1 second!")
    else:
        print("✓ Time synchronized")

client = mqtt.Client()
client.on_message = on_message
client.connect("mqtt.example.com", 1883)
client.subscribe("camper/+/+")  # Subscribe to all devices
client.loop_forever()
```

## Database Storage Best Practices

### PostgreSQL
```sql
CREATE TABLE sensor_data (
    id SERIAL PRIMARY KEY,
    device_id VARCHAR(32) NOT NULL,
    sensor_type VARCHAR(32) NOT NULL,
    timestamp_ms BIGINT NOT NULL,
    timestamp TIMESTAMPTZ GENERATED ALWAYS AS (
        to_timestamp(timestamp_ms / 1000.0)
    ) STORED,
    data JSONB NOT NULL
);

-- Index for time-based queries
CREATE INDEX idx_timestamp_ms ON sensor_data(timestamp_ms);
CREATE INDEX idx_timestamp ON sensor_data(timestamp);
CREATE INDEX idx_device_sensor_time ON sensor_data(device_id, sensor_type, timestamp_ms);
```

### InfluxDB
```python
from influxdb_client import InfluxDBClient, Point
from datetime import datetime

point = Point("sensor_reading") \
    .tag("device_id", "lindi_AB12CD") \
    .tag("sensor_type", "level") \
    .field("pitch", 1.234) \
    .field("roll", -0.567) \
    .time(datetime.utcfromtimestamp(timestamp_ms / 1000.0))

write_api.write(bucket="camper", record=point)
```

## Implementation Checklist

When adding a new device/sensor to the ecosystem:

- [ ] Include `<sys/time.h>` and `<time.h>` (or equivalent for platform)
- [ ] Implement NTP time synchronization
- [ ] Validate time is synced before first publish
- [ ] Use standard millisecond calculation pattern
- [ ] Store as `uint64_t timestamp_ms`
- [ ] Add to JSON as `"timestamp_ms": <number>`
- [ ] Never use local timezone
- [ ] Test timestamp accuracy against known reference
- [ ] Document sensor type, topic, and update rate
- [ ] Add to ecosystem sensor inventory

## Ecosystem Device Examples

### Level Sensor (ESP32 + MPU6050)
- **Device ID**: `lindi_<MAC>`
- **Topic**: `camper/device/level`
- **Update Rate**: 1 Hz
- **Implementation**: ESP-IDF with NTP sync

### GPS Tracker (ESP32 + GPS)
- **Device ID**: `gps_<MAC>`
- **Topic**: `camper/device/gps`
- **Update Rate**: 1 Hz (or configurable)
- **Implementation**: ESP-IDF with NTP sync

### Temperature Monitor (Arduino + DS18B20)
- **Device ID**: `temp_<MAC>`
- **Topic**: `camper/device/temperature`
- **Update Rate**: 0.1 Hz (every 10 seconds)
- **Implementation**: Arduino with RTC module

### Battery Monitor (Raspberry Pi + INA219)
- **Device ID**: `battery_rpi`
- **Topic**: `camper/device/battery`
- **Update Rate**: 0.2 Hz (every 5 seconds)
- **Implementation**: Python with systemd-timesyncd

## Related Ecosystem Standards

See also:
- [mqtt_ecosystem.md](mqtt_ecosystem.md) - MQTT broker and topic structure
- [serial_menu_ecosystem.md](serial_menu_ecosystem.md) - Standardized serial configuration interface
- [nvs_config_ecosystem.md](nvs_config_ecosystem.md) - Configuration storage patterns

---

**Ecosystem Owner**: Syquens B.V.
**Maintained by**: V.N. Verbon
**Version Control**: Git repository `Lindi/export/`
