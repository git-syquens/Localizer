# DS3231 RTC Module (AT24C32) - Connection Guide

**Module**: DS3231 AT24C32 RTC Module  
**Chipset**: DS3231 High Precision RTC + AT24C32 EEPROM  
**Interface**: I2C  
**Accuracy**: ±2ppm (±1 minute/year)  
**Last Updated**: 2026-01-03

---

## Overview

The DS3231 is a highly accurate real-time clock (RTC) module with integrated temperature-compensated crystal oscillator (TCXO). The AT24C32 variant includes a 32Kbit (4KB) EEPROM for additional storage.

**Key Features**:
- **RTC Chip**: DS3231 with TCXO
- **Accuracy**: ±2ppm (0-40°C), ±3.5ppm (-40°C to +85°C)
- **Interface**: I2C (3.3V and 5V compatible)
- **I2C Address**: 0x68 (RTC), 0x57 (EEPROM)
- **Battery Backup**: CR2032 coin cell (maintains time during power loss)
- **Current**: ~100µA (with battery backup)
- **Temperature Sensor**: Built-in, ±3°C accuracy
- **Alarm Functions**: Two programmable alarms
- **Square Wave Output**: 32.768kHz, 1kHz, 4.096kHz, 8.192kHz

---

## Pin Connections to ESP32-C3

### Pin Mapping

| RTC Module Pin | ESP32-C3 Pin | Function | Notes |
|----------------|--------------|----------|-------|
| **VCC** | **3.3V** | Power supply | 3.3V - 5.5V compatible |
| **GND** | **GND** | Ground | Common ground |
| **SDA** | **GPIO5** | I2C data | **Shared with OLED display** |
| **SCL** | **GPIO6** | I2C clock | **Shared with OLED display** |

### Connection Diagram

```
DS3231 RTC Module          ESP32-C3 Board
┌────────────────┐         ┌────────────────┐
│                │         │                │
│  VCC      GND  │         │  3V3      GND  │
│   ●        ●   │         │   ●        ●   │
│   │        │   │         │   │        │   │
│   │        └───┼─────────┼───┘        │   │
│   └────────────┼─────────┼────────────┘   │
│                │         │                │
│  SDA      SCL  │         │  GPIO5   GPIO6 │
│   ●        ●   │         │    ●       ●   │
│   │        │   │         │    │       │   │
│   │        └───┼─────────┼────┘       │   │
│   └────────────┼─────────┼────────────┘   │
│                │         │                │
│  SQW      32K  │         │                │
│   ●        ●   │         │  (Not used)    │
│                │         │                │
│  [CR2032]      │         │                │
└────────────────┘         └────────────────┘

VCC → 3.3V
GND → GND  
SDA → GPIO5 (shared I2C bus with OLED at 0x3C)
SCL → GPIO6 (shared I2C bus with OLED at 0x3C)
```

### I2C Bus Sharing

⚠️ **IMPORTANT**: The DS3231 RTC shares the I2C bus with the onboard OLED display:

| Device | I2C Address | Pin SDA | Pin SCL |
|--------|-------------|---------|---------|
| **OLED (SSD1306)** | 0x3C (60) | GPIO5 | GPIO6 |
| **DS3231 RTC** | 0x68 (104) | GPIO5 | GPIO6 |
| **AT24C32 EEPROM** | 0x57 (87) | GPIO5 | GPIO6 |

Since all devices have **different I2C addresses**, they can coexist on the same bus without conflict.

⚠️ **Address Conflict Warning**: 
- **MPU6050** (accelerometer) defaults to address **0x68** (same as DS3231)
- If adding MPU6050, configure it to use address 0x69 by connecting AD0 pin to VCC
- Alternative: Use different IMU (BMI160, LSM6DS3, ICM-20948)

---

## Pull-up Resistors

### Onboard Pull-ups

The ESP32-C3 board has **onboard pull-up resistors** on GPIO5/6 for the OLED.  
The DS3231 module also typically has **onboard 4.7kΩ pull-ups** on SDA/SCL.

**Result**: Multiple pull-up resistors in parallel are acceptable for I2C:
- ESP32 board: ~4.7kΩ (estimated)
- DS3231 module: 4.7kΩ (typical)
- **Effective resistance**: ~2.35kΩ (parallel combination)

This is within acceptable range for I2C Fast Mode (400kHz).

### Adding External Pull-ups

**Not required** for this setup. The combined onboard pull-ups are sufficient for:
- Short cables (< 20cm)
- Standard I2C speed (100kHz) or Fast Mode (400kHz)
- Two I2C devices on bus (OLED + RTC)

**If experiencing I2C issues** (only if needed):
- Add **2.2kΩ** pull-up resistors on SDA/SCL
- Place resistors between SDA/SCL and 3.3V
- This may be needed for long cables (> 50cm) or more than 3 devices

---

## Capacitor Recommendations

### Power Decoupling

**Basic Setup (short cables < 10cm)**:
- **No additional capacitors required** - Module has onboard decoupling

**Standard Setup (cables 10cm - 50cm)**:
- **10µF electrolytic** across VCC/GND near ESP32
- **100nF ceramic** across VCC/GND near RTC module

**Long Cable Setup (cables > 50cm)**:
- **100µF electrolytic** across VCC/GND near ESP32
- **10µF electrolytic** across VCC/GND near RTC module
- **100nF ceramic** at both ends (ESP32 and RTC)

### Capacitor Placement

```
Cable Length Recommendations:

< 10cm:  No caps needed (module has onboard decoupling)
         ┌─────┐     ┌─────┐
         │ESP32├─────┤ RTC │
         └─────┘     └─────┘

10-50cm: Add local decoupling at both ends
         ┌─────┐     ┌─────┐
    C1 ══╡ESP32├─────┤ RTC ╞══ C2
         └─────┘     └─────┘
         10µF        100nF

> 50cm:  Add bulk and decoupling caps
         ┌─────┐     ┌─────┐
C1+C2 ═══╡ESP32├─────┤ RTC ╞═══ C3+C4
         └─────┘     └─────┘
         100µF+100nF  10µF+100nF
```

**Capacitor Specifications**:
- **Ceramic caps**: X7R or better, 6.3V or higher
- **Electrolytic caps**: Low ESR, 6.3V or higher
- Place capacitors as close as possible to power pins

---

## Battery Backup

### CR2032 Coin Cell

The DS3231 module uses a **CR2032** lithium coin cell battery for backup power:

**Battery Life**:
- Typical: 5-8 years (depends on ambient temperature)
- Current draw: ~100µA during backup mode
- Maintains time and alarms during main power loss

**Battery Installation**:
1. Insert CR2032 battery into holder (positive side up, marked with +)
2. Battery powers RTC when VCC is disconnected
3. RTC automatically switches between VCC and battery

**Battery Replacement**:
- Replace when time starts drifting significantly
- No need to remove VCC during battery replacement
- Time is preserved during battery swap if VCC is connected

---

## DS3231 Registers

### Time and Date Registers

| Register | Address | Function | Format |
|----------|---------|----------|--------|
| Seconds | 0x00 | 00-59 | BCD |
| Minutes | 0x01 | 00-59 | BCD |
| Hours | 0x02 | 1-12 + AM/PM or 00-23 | BCD |
| Day | 0x03 | 1-7 | BCD |
| Date | 0x04 | 01-31 | BCD |
| Month | 0x05 | 01-12 | BCD |
| Year | 0x06 | 00-99 | BCD |

**BCD Format**: Each byte stores two decimal digits
- Example: 0x45 = 45 seconds, 0x12 = 12 hours

### Control Registers

| Register | Address | Function |
|----------|---------|----------|
| Alarm 1 Seconds | 0x07 | Alarm 1 configuration |
| Alarm 1 Minutes | 0x08 | Alarm 1 configuration |
| Alarm 1 Hours | 0x09 | Alarm 1 configuration |
| Alarm 1 Day/Date | 0x0A | Alarm 1 configuration |
| Alarm 2 Minutes | 0x0B | Alarm 2 configuration |
| Alarm 2 Hours | 0x0C | Alarm 2 configuration |
| Alarm 2 Day/Date | 0x0D | Alarm 2 configuration |
| Control | 0x0E | Control register |
| Control/Status | 0x0F | Status register |
| Aging Offset | 0x10 | Crystal aging compensation |
| Temp MSB | 0x11 | Temperature (high byte) |
| Temp LSB | 0x12 | Temperature (low byte) |

---

## Software Configuration (ESP-IDF)

### I2C Initialization

The I2C bus is already initialized for the OLED display. Simply add the RTC device to the same bus:

```c
#include "driver/i2c_master.h"

#define RTC_I2C_ADDR    0x68
#define EEPROM_I2C_ADDR 0x57

extern i2c_master_bus_handle_t i2c_bus_handle; // Shared with OLED

static i2c_master_dev_handle_t rtc_dev_handle;

void rtc_init(void)
{
    // Add RTC device to existing I2C bus
    i2c_device_config_t rtc_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = RTC_I2C_ADDR,
        .scl_speed_hz = 400000, // 400kHz
    };
    
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_handle, 
                                               &rtc_config, 
                                               &rtc_dev_handle));
    ESP_LOGI(TAG, "DS3231 RTC initialized at 0x%02X", RTC_I2C_ADDR);
}
```

### Reading Time

```c
#include <time.h>

// BCD to Decimal conversion
uint8_t bcd_to_dec(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

// Decimal to BCD conversion
uint8_t dec_to_bcd(uint8_t dec) {
    return ((dec / 10) << 4) | (dec % 10);
}

esp_err_t rtc_read_time(struct tm *timeinfo)
{
    uint8_t data[7];
    uint8_t reg = 0x00;
    
    // Write register address, then read 7 bytes
    esp_err_t ret = i2c_master_transmit_receive(rtc_dev_handle, 
                                                 &reg, 1, 
                                                 data, 7, 
                                                 pdMS_TO_TICKS(100));
    if (ret != ESP_OK) return ret;
    
    timeinfo->tm_sec = bcd_to_dec(data[0] & 0x7F);
    timeinfo->tm_min = bcd_to_dec(data[1] & 0x7F);
    timeinfo->tm_hour = bcd_to_dec(data[2] & 0x3F); // 24-hour format
    timeinfo->tm_wday = bcd_to_dec(data[3] & 0x07) - 1; // 0-6
    timeinfo->tm_mday = bcd_to_dec(data[4] & 0x3F);
    timeinfo->tm_mon = bcd_to_dec(data[5] & 0x1F) - 1; // 0-11
    timeinfo->tm_year = bcd_to_dec(data[6]) + 100; // Years since 1900
    
    return ESP_OK;
}
```

### Writing Time

```c
esp_err_t rtc_set_time(const struct tm *timeinfo)
{
    uint8_t data[8];
    
    data[0] = 0x00; // Starting register address
    data[1] = dec_to_bcd(timeinfo->tm_sec);
    data[2] = dec_to_bcd(timeinfo->tm_min);
    data[3] = dec_to_bcd(timeinfo->tm_hour); // 24-hour format
    data[4] = dec_to_bcd(timeinfo->tm_wday + 1); // 1-7
    data[5] = dec_to_bcd(timeinfo->tm_mday);
    data[6] = dec_to_bcd(timeinfo->tm_mon + 1); // 1-12
    data[7] = dec_to_bcd(timeinfo->tm_year % 100); // 00-99
    
    return i2c_master_transmit(rtc_dev_handle, data, 8, pdMS_TO_TICKS(100));
}
```

### Reading Temperature

```c
float rtc_read_temperature(void)
{
    uint8_t data[2];
    uint8_t reg = 0x11; // Temperature MSB register
    
    esp_err_t ret = i2c_master_transmit_receive(rtc_dev_handle, 
                                                 &reg, 1, 
                                                 data, 2, 
                                                 pdMS_TO_TICKS(100));
    if (ret != ESP_OK) return 0.0f;
    
    // Temperature = MSB + (LSB >> 6) * 0.25
    int8_t msb = (int8_t)data[0];
    uint8_t lsb = (data[1] >> 6) & 0x03;
    
    return (float)msb + (lsb * 0.25f);
}
```

---

## Time Synchronization Strategy

For the Localizer project, implement this synchronization hierarchy:

1. **GPS Fix** (highest priority): Set RTC when GPS achieves valid fix
2. **NTP Sync** (secondary): Update RTC via NTP when WiFi connected
3. **RTC Backup** (fallback): Use RTC time when GPS/NTP unavailable

### GPS → RTC Sync Logic

```c
void sync_rtc_from_gps(struct tm *gps_time)
{
    struct tm rtc_time;
    rtc_read_time(&rtc_time);
    
    // Calculate time difference
    time_t gps_epoch = mktime(gps_time);
    time_t rtc_epoch = mktime(&rtc_time);
    int64_t diff_ms = llabs((int64_t)(gps_epoch - rtc_epoch) * 1000);
    
    if (diff_ms > 1000) { // More than 1 second difference
        rtc_set_time(gps_time);
        ESP_LOGI(TAG, "RTC synced from GPS (diff: %lld ms)", diff_ms);
        
        // Update display: "RTC SYNC" in green
    } else {
        ESP_LOGI(TAG, "RTC already in sync (diff: %lld ms)", diff_ms);
    }
}
```

### Display Status Logic

```c
typedef enum {
    RTC_STATUS_LOCAL,  // Not synced (RED)
    RTC_STATUS_SYNC    // Synced within 1ms (GREEN)
} rtc_status_t;

rtc_status_t check_rtc_sync_status(void)
{
    // Compare RTC time to GPS time (if GPS has fix)
    // Or compare to NTP time (if WiFi connected)
    // Return RTC_STATUS_SYNC if within 1ms tolerance
    
    if (gps_has_fix) {
        int64_t diff_ms = get_time_diff_gps_rtc();
        return (diff_ms < 1) ? RTC_STATUS_SYNC : RTC_STATUS_LOCAL;
    }
    
    return RTC_STATUS_LOCAL; // Default to LOCAL if no reference
}
```

---

## AT24C32 EEPROM (Optional)

The module includes a 32Kbit (4KB) EEPROM at I2C address **0x57**.

### Use Cases
- Store GPS calibration data
- Log timestamps of events
- Store WiFi credentials backup
- Configuration data

### Reading EEPROM

```c
#define EEPROM_I2C_ADDR 0x57

esp_err_t eeprom_read(uint16_t address, uint8_t *data, size_t len)
{
    uint8_t addr_bytes[2] = {(address >> 8) & 0xFF, address & 0xFF};
    
    return i2c_master_transmit_receive(eeprom_dev_handle, 
                                        addr_bytes, 2, 
                                        data, len, 
                                        pdMS_TO_TICKS(100));
}
```

### Writing EEPROM

```c
esp_err_t eeprom_write_byte(uint16_t address, uint8_t data)
{
    uint8_t buffer[3];
    buffer[0] = (address >> 8) & 0xFF;
    buffer[1] = address & 0xFF;
    buffer[2] = data;
    
    esp_err_t ret = i2c_master_transmit(eeprom_dev_handle, buffer, 3, 
                                         pdMS_TO_TICKS(100));
    
    // EEPROM requires 5ms write cycle time
    vTaskDelay(pdMS_TO_TICKS(5));
    
    return ret;
}
```

---

## Troubleshooting

### RTC Not Responding

1. **Check I2C address**: Verify 0x68 with I2C scanner
2. **Check wiring**: SDA=GPIO5, SCL=GPIO6
3. **Check power**: Measure 3.3V at VCC pin
4. **Check battery**: Ensure CR2032 is installed correctly

### Time Not Keeping Accurately

1. **Check battery**: Replace CR2032 if > 5 years old
2. **Temperature**: DS3231 is most accurate at 0-40°C
3. **Check crystal**: Module may have faulty crystal (rare)
4. **Sync periodically**: Update from GPS/NTP to compensate drift

### I2C Bus Conflicts

1. **Scan I2C bus**: Use I2C scanner to detect devices
2. **Check addresses**: OLED=0x3C, RTC=0x68, EEPROM=0x57
3. **Avoid MPU6050**: Conflicts with RTC at 0x68
4. **Pull-up check**: Too many pull-ups can cause issues (< 2kΩ total may be too strong)

---

## Performance Specifications

| Parameter | Value |
|-----------|-------|
| **Accuracy** | ±2ppm (0°C to +40°C) |
| **Drift** | ~1 minute/year |
| **Operating Temp** | -40°C to +85°C |
| **Battery Life** | 5-8 years (CR2032) |
| **I2C Speed** | Up to 400kHz |
| **Current (Active)** | ~200µA @ 3.3V |
| **Current (Backup)** | ~100µA (battery only) |

---

## References

- [DS3231 Datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/DS3231.pdf)
- [AT24C32 EEPROM Datasheet](https://www.microchip.com/en-us/product/AT24C32)
- ESP-IDF I2C Master Driver Documentation
