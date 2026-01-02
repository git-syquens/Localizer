# ESP32-C3 0.42" OLED Board Reference

**Board Model**: ABRobot ESP32-C3 0.42" OLED  
**Microcontroller**: ESP32-C3-FN4/FH4  
**Display**: 0.42" OLED (SSD1306, 72×40 effective pixels)  
**Form Factor**: 24.8mm × 20.45mm  
**Last Updated**: 2026-01-02

---

## Overview

This is a compact ESP32-C3 development board with integrated 0.42" OLED display. Despite its small size, it includes WiFi, Bluetooth, and sufficient GPIO pins for the Localizer GPS logger and NTP server project.

**Key Features**:
- ESP32-C3 RISC-V single-core @ 160MHz
- 4MB built-in SPI Flash
- 0.42" OLED display (I2C, SSD1306 controller)
- USB-C native support (no external USB-UART chip)
- WiFi 802.11b/g/n (2.4GHz)
- Bluetooth 5.0
- Ceramic antenna
- Onboard LED (GPIO8)
- 2.54mm pitch pins

---

## Board Specifications

| Specification | Value |
|--------------|-------|
| **Voltage** | 3.3V - 6V (5V recommended) |
| **WiFi** | 802.11b/g/n, 2.4GHz |
| **Bluetooth** | BT 5.0 |
| **Flash** | 4MB SPI Flash (built-in) |
| **RAM** | 400KB SRAM |
| **Interfaces** | 1× I2C, 1× SPI, 2× UART, 11× GPIO (PWM), 4× ADC |
| **Onboard LED** | GPIO8 (active LOW) |
| **Display** | 0.42" OLED, 72×40 pixels (SSD1306 @ 0x3C) |
| **USB** | USB-C (native CDC support) |
| **Dimensions** | 24.8mm × 20.45mm |
| **Pin Pitch** | 2.54mm (standard breadboard compatible) |
| **Pin Row Spacing** | 17.78mm (8 pins inclusive, 7 gaps) |

---

## Chip Hardware Details

### Core Specifications

| Property | Value |
|----------|-------|
| **Chip Model** | ESP32-C3 (QFN32) |
| **Chip Revision** | v0.4 (tested) |
| **CPU Architecture** | RISC-V 32-bit single-core |
| **CPU Frequency** | 160 MHz (configurable to 80 MHz) |
| **Clock Sources** | 40 MHz XTAL, 150 kHz RTC |
| **APB Clock** | 80 MHz (peripheral bus) |

### Features & Capabilities

| Feature | Status | Details |
|---------|--------|---------|
| **WiFi** | ✅ YES | 802.11 b/g/n (2.4GHz) - verified |
| **Bluetooth** | ✅ YES | Bluetooth 5.0 LE - verified |
| **IEEE 802.15.4** | ⚠️ NO* | *Not detected in hardware test (may require config) |
| **Embedded Flash** | ✅ YES | 4MB XMC SPI Flash - verified |

*Note: IEEE 802.15.4 support exists in hardware but may not be enabled in current build configuration.*

### Memory Configuration

| Memory Type | Size | Details |
|-------------|------|---------|XMC SPI Flash (embedded) |
| **Internal RAM** | 400 KB | SRAM |
| **Free Heap (boot)** | ~321 KB | After system initialization (tested)
| **Free Heap (boot)** | ~280-300 KB | After system initialization |
| **RTC Memory** | 8 KB | Preserved in deep sleep |

### Partition Layout

| Name | Type | SubType | Offset | Size | Usage |
|------|------|---------|--------|------|-------|
| **nvs** | data | nvs | 0x9000 | 24 KB | Non-volatile storage |
| **phy_init** | data | phy | 0xf000 | 4 KB | PHY calibration data |
| **factory** | app | factory | 0x10000 | 1 MB | Application firmware |

**Available Space**: ~3 MB for custom partitions (data logging, SPIFFS, etc.)

### Peripheral Inventory

| Peripheral | Quantity | Pins/Details |
|------------|----------|--------------|
| **UART** | 2 | UART0 (GPIO20/21), UART1 (conflicts with USB) |
| **I2C** | 1 | Any GPIO via software, hardware acceleration available |
| **SPI** | 3 | SPI0/1 (flash), SPI2/FSPI (available) |
| **GPIO Pins** | 22 | GPIO 0-21 |
| **ADC Channels** | 6 | ADC1: GPIO0-4 (5 ch), ADC2: GPIO5 (1 ch) |
| **ADC Resolution** | 12-bit | 0-4095 (0-3.3V with attenuation) |
| **Timer Groups** | 2 | 1 timer per group |
| **RMT Channels** | 4 | Remote control (IR, LED strips, etc.) |
| **LEDC Channels** | 6 | PWM for LED dimming |
| **USB Serial/JTAG** | 1 | Native USB-C (GPIO18/19) |

### MAC Addresses

The ESP32-C3 has unique MAC addresses for each interface:

| Interface | MAC Address | Notes |
|-----------|-------------|-------|
| **WiFi Station** | `0C:4E:A0:63:20:C8` | Tested 2026-01-02 |
| **WiFi SoftAP** | `0C:4E:A0:63:20:C9` | Base MAC + 1 |
| **Bluetooth** | `0C:4E:A0:63:20:CA` | Base MAC + 2 |
| **Ethernet** | `0C:4E:A0:63:20:CB` | Placeholder (no Ethernet hardware) |

*Hardware info test confirmed these values on 2026-01-02.*

### ADC Specifications

| Parameter | Value | Notes |
|-----------|-------|-------|
| **Resolution** | 12-bit (0-4095) | |
| **Voltage Range** | 0 - 3.3V | With 11dB attenuation |
| **ADC1 Channels** | 5 | GPIO 0-4 |
| **ADC2 Channels** | 1 | GPIO 5 |

#### ADC Attenuation Ranges

| Attenuation | Voltage Range | Typical Use |
|-------------|---------------|-------------|
| **0 dB** | 0 - 950 mV | Low voltage sensing |
| **2.5 dB** | 0 - 1250 mV | |
| **6 dB** | 0 - 1750 mV | |
| **11 dB** | 0 - 3100 mV | Battery monitoring (3.3V systems) |

**Note**: GPIO5 (ADC2_CH0) is shared with I2C and conflicts with WiFi. Use ADC1 channels (GPIO0-4) for reliable readings.

### Boot Modes

| GPIO9 (BOOT) | GPIO8 | GPIO2 | Mode |
|--------------|-------|-------|------|
| 0 (LOW) | X | X | **Download mode** (UART/USB flash) |
| 1 (HIGH) | X | X | **Normal boot** (run firmware) |

**Note**: GPIO9 has onboard pull-up and BOOT button. Hold button during reset to enter download mode.

### Temperature Range

| Parameter | Value |
|-----------|-------|
| **Operating** | -40°C to +85°C |
| **Storage** | -40°C to +125°C |

---

## Pinout Diagram

```
         ┌─────────────────┐
         │   ESP32-C3      │
         │   0.42" OLED    │
         │                 │
         │  ┌───────────┐  │
         │  │  OLED     │  │
         │  │  72x40    │  │
         │  └───────────┘  │
         │                 │
  GND ●──┤                 ├──● 3V3
   EN ●──┤                 ├──● GND
   10 ●──┤                 ├──● 21 (TX0, UART0)
    9 ●──┤                 ├──● 20 (RX0, UART0)
    8 ●──┤                 ├──● 7
    7 ●──┤                 ├──● 6 (SCL, I2C to OLED)
    6 ●──┤                 ├──● 5 (SDA, I2C to OLED)
    5 ●──┤                 ├──● 4
    4 ●──┤                 ├──● 3
    3 ●──┤                 ├──● 2
    2 ●──┤                 ├──● 1
    1 ●──┤                 ├──● 0
    0 ●──┤                 ├──● 5V (VUSB)
         │                 │
         │  [BOO]   [RST]  │
         │                 │
         │     [USB-C]     │
         └─────────────────┘

Legend:
BOO = BOOT button (GPIO9)
RST = RESET button (EN pin)
```

---

## GPIO Pin Assignments

### Available GPIO Pins

| GPIO | Function | Default State | Notes |
|------|----------|---------------|-------|
| **GPIO0** | ADC1_CH0 | LOW | ADC capable |
| **GPIO1** | ADC1_CH1 | LOW | ADC capable |
| **GPIO2** | ADC1_CH2 | LOW | ADC capable |
| **GPIO3** | ADC1_CH3 | LOW | ADC capable |
| **GPIO4** | ADC2_CH0 | LOW | ADC capable |
| **GPIO5** | I2C_SDA | HIGH (pull-up) | ⚠️ Connected to OLED |
| **GPIO6** | I2C_SCL | HIGH (pull-up) | ⚠️ Connected to OLED |
| **GPIO7** | General | LOW | Available |
| **GPIO8** | LED | HIGH (pull-up) | Active LOW (LED on when LOW) |
| **GPIO9** | BOOT Button | HIGH (pull-up) | Doubles as user button |
| **GPIO10** | General | LOW | Available |
| **GPIO20** | UART0_RX | HIGH (pull-up) | Hardware UART (available for GPS) |
| **GPIO21** | UART0_TX | HIGH (pull-up) | Hardware UART (available for GPS) |

### Reserved/Unavailable Pins

| GPIO | Function | Reason |
|------|----------|--------|
| **GPIO12-17** | SPI Flash | Connected to onboard 4MB flash |
| **GPIO18-19** | USB D+/D- | USB-C data lines |

---

## Recommended Pin Assignments for Localizer Project

### GPS Module Connection (UART)

| GPS Pin | ESP32-C3 Pin | Function |
|---------|--------------|----------|
| VCC | 3.3V | Power |
| GND | GND | Ground |
| TX | GPIO20 | GPS transmit → ESP receive (UART0_RX) |
| RX | GPIO21 | GPS receive ← ESP transmit (UART0_TX) |
| PPS | GPIO10 | Pulse-per-second (optional, high accuracy) |

**Note**: UART0 on pins 20/21 is independent of USB CDC, so you can use serial debugging over USB while communicating with GPS simultaneously.

### DS3231 RTC Connection (I2C)

⚠️ **Important**: The onboard OLED uses GPIO5/6 for I2C. The DS3231 RTC must share the same I2C bus.

| RTC Pin | ESP32-C3 Pin | Function |
|---------|--------------|----------|
| VCC | 3.3V | Power |
| GND | GND | Ground |
| SDA | GPIO5 | I2C data (shared with OLED) |
| SCL | GPIO6 | I2C clock (shared with OLED) |

**I2C Addresses**:
- OLED (SSD1306): `0x3C`
- DS3231 RTC: `0x68` (default)

Since these are different addresses, they can coexist on the same I2C bus without conflict.

⚠️ **Address Conflict Warning**: MPU6050 (accelerometer/gyroscope) also defaults to `0x68`. If adding motion sensing, configure MPU6050 to use `0x69` by connecting AD0 pin to VCC, or use a different IMU (BMI160, LSM6DS3, ICM-20948).

### Additional Sensors/Peripherals

| Peripheral | Recommended GPIO | Notes |
|------------|------------------|-------|
| Status LED | GPIO8 (onboard) | Active LOW (built-in blue LED) |
| User Button | GPIO9 (onboard) | Built-in BOOT button (pulled HIGH) |
| Analog Input | GPIO0-4 | ADC capable (e.g., battery monitoring) |
| Digital I/O | GPIO7, GPIO10 | General purpose |
| MPU6050 IMU (optional) | GPIO5/6 (I2C) | ⚠️ Set AD0=HIGH for address 0x69 (RTC conflict at 0x68) |

---

## OLED Display Details

### Display Specifications

| Parameter | Value |
|-----------|-------|
| **Controller** | SSD1306 |
| **Interface** | I2C (address 0x3C) |
| **Resolution** | 72×40 pixels (effective) |
| **Buffer Size** | 128×64 pixels (SSD1306 controller) |
| **Color** | Monochrome (white on black) |
| **I2C Pins** | SDA=GPIO5, SCL=GPIO6 |
| **I2C Speed** | Up to 400kHz |

### Display Offset

The 72×40 pixel display is mapped to the center of the SSD1306's 128×64 buffer:

```c
// Offset calculations
int width = 72;
int height = 40;
int xOffset = 28;  // Updated offset (was 30 in some sources)
int yOffset = 24;  // Updated offset (was 12 in some sources)
```

**Note**: Different sources report slightly different offsets (28/24 vs 30/12). Test both to see which centers your display correctly.

### U8g2 Library Configuration

**Recommended constructor** (no manual offset needed):

```c
#include <U8g2lib.h>

// Native 72x40 constructor (preferred)
U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 6, 5);

void setup() {
    u8g2.begin();
    u8g2.setContrast(255);      // Max brightness
    u8g2.setBusClock(400000);   // 400kHz I2C
}
```

**Alternative** (manual offset with 128x64 constructor):

```c
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 6, 5);

int width = 72;
int height = 40;
int xOffset = 28;
int yOffset = 24;

void setup() {
    u8g2.begin();
    u8g2.setContrast(255);
    u8g2.setBusClock(400000);
    
    // All drawing operations must add xOffset/yOffset
    u8g2.drawStr(xOffset + 10, yOffset + 20, "Hello");
}
```

---

## Serial/UART Configuration

### UART0 (Hardware Serial - Available for GPS)

- **RX**: GPIO20
- **TX**: GPIO21
- **Baud Rate**: Configurable (9600 for most GPS modules)
- **Status**: **Available** for application use (does not interfere with USB CDC)

**Example**:
```c
Serial0.begin(9600, SERIAL_8N1, 20, 21);  // GPS on UART0
```

### USB CDC (USB Serial - For Debugging)

- **Interface**: USB-C port
- **Pins**: GPIO18/19 (USB D+/D-)
- **Arduino**: Accessible as `Serial` (when USB CDC enabled)
- **Speed**: Full-speed USB (12 Mbps)

**Arduino IDE Settings**:
- Board: "ESP32C3 Dev Module"
- USB CDC On Boot: **Enabled**

**Example**:
```c
Serial.begin(115200);  // USB CDC debug output
Serial0.begin(9600);   // GPS on UART0
```

### UART1 (Not Available)

⚠️ **Warning**: Although ESP-IDF defines UART1, it conflicts with USB on this board. Do not use `Serial1` or the USB serial port will disappear.

---

## I2C Configuration

### Shared I2C Bus

The ESP32-C3 has only **one I2C peripheral**. On this board, it is routed to GPIO5/6 for the OLED display.

**I2C Devices on Bus**:
1. **OLED Display** (SSD1306): Address `0x3C`
2. **DS3231 RTC** (external): Address `0x68`

### I2C Pull-up Resistors

- GPIO5 (SDA): Pulled HIGH to 3.3V (onboard pull-up)
- GPIO6 (SCL): Pulled HIGH to 3.3V (onboard pull-up)

**Note**: The DS3231 module typically includes its own pull-up resistors. Multiple sets of pull-ups on the same bus are acceptable as long as they're not too strong (4.7kΩ - 10kΩ is typical).

### I2C Example

```c
#include <Wire.h>

void setup() {
    Wire.begin(5, 6);  // SDA=GPIO5, SCL=GPIO6
    Wire.setClock(400000);  // 400kHz (Fast I2C)
    
    // Scan for devices
    for (byte addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("I2C device found at 0x%02X\n", addr);
        }
    }
}
```

---

## Power Supply

### USB-C Power

- **Voltage**: 5V from USB
- **Current**: Typically 500mA (USB 2.0 standard)
- **Regulation**: Onboard ME6211C33 LDO (5V → 3.3V)

### External Power (5V Pin)

⚠️ **Schematic Note**: The 5V pin is connected to VUSB via a 1N5819 Schottky diode.

**Options**:
1. **5V input**: Connect regulated 5V to the 5V pin (recommended: 3.7V - 6V range)
2. **Battery**: Use 3.7V LiPo (may need external 5V boost converter)
3. **Solar**: 6V solar panel with voltage regulator

**Current Consumption**:
- **Active** (WiFi + GPS): ~200 mA @ 3.3V
- **WiFi TX peak**: ~350 mA (brief bursts)
- **Sleep mode**: <10 mA (GPS off, WiFi modem sleep)

### RTC Battery Backup

The DS3231 RTC module requires a separate CR2032 battery for time continuity during power loss.

---

## Onboard Components

### LED (GPIO8)

- **Color**: Blue
- **Control**: Active LOW (set GPIO8=LOW to turn ON)
- **Circuit**: GPIO8 → Resistor (R8) → LED → GND
- **Pull-up**: GPIO8 pulled HIGH via resistor R1

**Example**:
```c
#define LED_PIN 8

void setup() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);   // LED ON
    digitalWrite(LED_PIN, HIGH);  // LED OFF
}
```

### BOOT Button (GPIO9)

- **Label**: "BOO" or "BOOT"
- **Function**: Enter download/flash mode (hold during reset)
- **Secondary Use**: User button in application
- **Circuit**: GPIO9 pulled HIGH, button connects to GND

**Example**:
```c
#define BOOT_BUTTON_PIN 9

void setup() {
    pinMode(BOOT_BUTTON_PIN, INPUT);  // Internal pull-up active
}

void loop() {
    if (digitalRead(BOOT_BUTTON_PIN) == LOW) {
        Serial.println("Button pressed!");
    }
}
```

### RESET Button (EN)

- **Label**: "RST" or "RESET"
- **Function**: Hardware reset (reboots ESP32-C3)
- **Pin**: Connected to EN (enable) pin

---

## Programming & Flashing

### Arduino IDE Setup

1. **Install ESP32 Core**:
   - Add URL to Board Manager: `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
   - Install "esp32" by Espressif

2. **Board Settings**:
   - **Board**: "ESP32C3 Dev Module"
   - **USB CDC On Boot**: **Enabled**
   - **Flash Size**: 4MB
   - **Partition Scheme**: Default or "Minimal SPIFFS"
   - **Upload Speed**: 921600 (or 115200 if issues)

3. **First Upload**:
   - Hold **BOOT** button
   - Press and release **RESET** button
   - Release **BOOT** button
   - Click "Upload" in Arduino IDE

4. **Subsequent Uploads**:
   - Automatic reset/boot (no button pressing needed)

### ESP-IDF Setup

For the Localizer project using ESP-IDF v5.5:

```bash
# Set target
idf.py set-target esp32c3

# Configure (optional)
idf.py menuconfig

# Build
idf.py build

# Flash and monitor
idf.py -p COM3 flash monitor
```

**Important ESP-IDF Settings**:
- Target: `esp32c3`
- Flash size: 4MB
- Enable USB CDC for console: Yes

---

## Known Issues & Limitations

### I2C Shared with OLED

- **Issue**: External I2C devices must share pins GPIO5/6 with the OLED
- **Solution**: Use different I2C addresses (OLED=0x3C, RTC=0x68)
- **Limitation**: Cannot reassign I2C to GPIO8/9 (OLED hardware-wired to GPIO5/6)

### SPI Not Easily Available

- **Issue**: Fast SPI (FSPI) pins are used by I2C (GPIO5/6)
- **Solution**: Use software SPI on other GPIO pins if needed
- **Limitation**: Hardware SPI (GPIO12-17) reserved for flash memory

### UART1 Conflicts with USB

- **Issue**: Enabling Serial1 breaks USB CDC functionality
- **Root Cause**: UART1 pins (GPIO18/19) are physically USB D+/D-
- **Solution**: Only use UART0 (GPIO20/21) for peripherals

### WiFi Range

- **Issue**: Some users report limited WiFi range
- **Cause**: Small ceramic antenna, board layout
- **Solution**: Position board for best signal, avoid metal enclosures
- **Note**: Varies by board batch/manufacturer

### Display Offset Confusion

- **Issue**: Different sources report different offset values
- **Values**: (30, 12) vs (28, 24)
- **Solution**: Test both offsets, or use native 72×40 constructor

---

## Schematic Reference

GitHub repository with schematic and design files:
- **Repository**: [ESP32-C3-ABrobot-OLED](https://github.com/zhuhai-esp/ESP32-C3-ABrobot-OLED)
- **Schematic**: Available in repo (high-resolution version)

**Key Components** (from schematic):
- **ESP32-C3-FN4/FH4**: Main MCU
- **ME6211C33**: 3.3V LDO regulator
- **1N5819**: Schottky diode (VUSB protection)
- **R1**: GPIO8 pull-up (to LED)
- **R6**: GPIO9 pull-up (to BOOT button)
- **R8**: LED current limiting resistor

---

## Recommended Libraries

### OLED Display (U8g2)

```bash
# Arduino Library Manager
Search: "U8g2"
Install: "U8g2 by oliver"
```

**Usage**:
```c
#include <U8g2lib.h>
U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 6, 5);
```

### DS3231 RTC

```bash
# Arduino Library Manager
Search: "RTClib"
Install: "RTClib by Adafruit"
```

**Usage**:
```c
#include <RTClib.h>
RTC_DS3231 rtc;
rtc.begin();
```

### GPS (TinyGPS++)

```bash
# Arduino Library Manager
Search: "TinyGPS"
Install: "TinyGPS++ by Mikal Hart"
```

**Usage**:
```c
#include <TinyGPSPlus.h>
TinyGPSPlus gps;
Serial0.begin(9600, SERIAL_8N1, 20, 21);  // GPS on UART0
```

---

## Comparison with Other Boards

| Feature | ESP32-C3 0.42" OLED | ESP32-C3 DevKit | ESP32 DevKit v1 |
|---------|---------------------|-----------------|-----------------|
| MCU | ESP32-C3 (RISC-V) | ESP32-C3 (RISC-V) | ESP32 (Xtensa dual-core) |
| Flash | 4MB | 4MB | 4MB |
| Display | 0.42" OLED (built-in) | None | None |
| USB | USB-C (native CDC) | USB-C (native CDC) | Micro-USB (CH340) |
| GPIO Count | 11 usable | 15+ usable | 30+ usable |
| Size | 24.8×20.45mm | 48×25mm | 55×28mm |
| Cost | ~$3-5 | ~$2-3 | ~$4-6 |
| Best For | Compact projects with display | General purpose, more GPIO | Complex projects, more processing |

**Verdict for Localizer**: The built-in OLED is useful for status display (GPS fix, time, IP address) without external components. Compact size is ideal for portable GPS logger.

---

## Additional Resources

### Documentation

- **Blog Post**: [Kevin's ESP32-C3 0.42" OLED Guide](https://emalliab.wordpress.com/2025/02/12/esp32-c3-0-42-oled/)
- **GitHub Repo**: [ESP32-C3-ABrobot-OLED](https://github.com/zhuhai-esp/ESP32-C3-ABrobot-OLED)
- **Fritzing Part**: [Forum Post](https://forum.fritzing.org/t/esp32-c3-oled-0-42-mini-board-part/25830)

### Datasheets

- **ESP32-C3**: [Espressif Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf)
- **SSD1306**: [OLED Controller Datasheet](https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf)
- **DS3231**: [RTC Datasheet](https://datasheets.maximintegrated.com/en/ds/DS3231.pdf)

### Online Communities

- **Reddit**: r/esp32
- **Arduino Forum**: ESP32 section
- **ESP32.com**: Official Espressif forum

---

## Localizer Project Pin Summary

| Peripheral | GPIO | Function | Notes |
|------------|------|----------|-------|
| **GPS Module** | GPIO20 | UART0_RX | NEO-6M/7M TX → ESP RX |
| **GPS Module** | GPIO21 | UART0_TX | ESP TX → NEO-6M/7M RX |
| **GPS PPS** | GPIO10 | Digital Input | Optional, high-accuracy sync |
| **DS3231 RTC** | GPIO5 | I2C SDA | Shared with OLED |
| **DS3231 RTC** | GPIO6 | I2C SCL | Shared with OLED |
| **OLED Display** | GPIO5 | I2C SDA | Built-in, address 0x3C |
| **OLED Display** | GPIO6 | I2C SCL | Built-in, address 0x3C |
| **Status LED** | GPIO8 | Output | Built-in blue LED (active LOW) |
| **User Button** | GPIO9 | Input | Built-in BOOT button |
| **USB Serial** | USB-C | Debug | Native CDC, 115200 baud |

**Total GPIO Used**: 6 (GPIO5, 6, 8, 9, 10, 20, 21)  
**GPIO Remaining**: 5 (GPIO0, 1, 2, 3, 4, 7)

---

## Conclusion

The ESP32-C3 0.42" OLED board is an excellent choice for the Localizer project due to:

✅ **Compact size**: 24.8×20.45mm  
✅ **Built-in display**: Status information without external components  
✅ **Sufficient GPIO**: Enough pins for GPS, RTC, and future expansion  
✅ **Native USB**: No external USB-UART chip needed  
✅ **WiFi + Bluetooth**: Both 2.4GHz protocols available  
✅ **Low cost**: ~$3-5 per unit  
✅ **I2C available**: Shared bus works perfectly for OLED + RTC  

**Trade-offs**:
- Limited GPIO compared to larger ESP32 boards
- I2C pins fixed to GPIO5/6 (shared with OLED)
- WiFi range may be limited (small ceramic antenna)

For a GPS logger with NTP server functionality, this board provides an ideal balance of features, size, and cost.

---

**Reference Sources**:
- Kevin's Blog: https://emalliab.wordpress.com/2025/02/12/esp32-c3-0-42-oled/
- GitHub: https://github.com/zhuhai-esp/ESP32-C3-ABrobot-OLED

**Document Version**: 1.0  
**Author**: V.N. Verbon (Syquens B.V.)  
**Last Updated**: 2026-01-02
