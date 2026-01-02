# Hardware Tests - Localizer Board

**Board**: ESP32-C3 0.42" OLED  
**Chip**: ESP32-C3 (QFN32) revision v0.4  
**MAC**: 0C:4E:A0:63:20:C8 (WiFi)  
**Date**: 2026-01-02

---

## Completed Tests

### ✅ Hardware Information Scan - PASSED

**Test Date**: 2026-01-02  
**Result**: PASS

**Details**:
- **Chip Model**: ESP32-C3 (QFN32) revision v0.4
- **Flash**: 4MB XMC embedded flash
- **MAC WiFi**: 0C:4E:A0:63:20:C8
- **MAC Bluetooth**: 0C:4E:A0:63:20:CA
- **CPU**: 160 MHz RISC-V single-core
- **Free Heap**: 321 KB at boot
- **Features**: WiFi ✅, Bluetooth LE ✅, USB Serial/JTAG ✅
- **Peripherals**: 2 UART, 1 I2C, 2 SPI, 22 GPIO, 6 ADC channels

**Observations**:
- All expected features detected and verified
- IEEE 802.15.4 not enabled (requires menuconfig)
- XMC flash manufacturer (reliable)
- USB Serial/JTAG working correctly

### ✅ Blue LED (GPIO8) - PASSED

**Test Date**: 2026-01-02  
**Result**: PASS

**Details**:
- **GPIO Pin**: GPIO8
- **Control**: Active LOW (0 = ON, 1 = OFF)
- **Test Method**: Blink test at 2 Hz (500ms on/off cycle)
- **Observations**:
  - LED blinks correctly with software control
  - Active LOW logic confirmed (setting GPIO8=0 turns LED ON)
  - No hardware issues detected

**Code Verified**:
```c
#define LED_PIN GPIO_NUM_8
gpio_reset_pin(LED_PIN);
gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
gpio_set_level(LED_PIN, 0);  // LED ON
gpio_set_level(LED_PIN, 1);  // LED OFF
```

**Status**: Ready for use as status indicator in main application

---

## Pending Tests

### ⏳ OLED Display (I2C, GPIO5/6)
- [ ] I2C bus initialization
- [ ] SSD1306 controller detection (address 0x3C)
- [ ] Display clear/write test
- [ ] Verify 72×40 pixel area
- [ ] Test display offset (28, 24)

### ⏳ UART0 (GPIO20/21)
- [ ] UART initialization test
- [ ] Loopback test (TX→RX)
- [ ] Verify independence from USB CDC
- [ ] GPS module communication test

### ⏳ I2C Bus (GPIO5/6)
- [ ] I2C bus scan
- [ ] DS3231 RTC detection (address 0x68)
- [ ] Multiple device test (OLED + RTC)
- [ ] I2C speed test (100kHz, 400kHz)

### ⏳ Boot Button (GPIO9)
- [ ] Input reading test
- [ ] Pull-up verification
- [ ] Debounce testing

### ⏳ ADC Pins (GPIO0-4)
- [ ] ADC reading test
- [ ] Voltage range verification
- [ ] Noise/stability test

---

## Test Equipment

- USB-C cable with power switch
- Serial terminal (115200 baud)
- Multimeter (for voltage verification)
- Logic analyzer (optional, for I2C debugging)

---

## Notes

### OLED Display Power Behavior
- Display RAM (GDDRAM) is volatile but persists during software reset
- Power cycle required to clear display if not initialized by firmware
- Verified: Cutting USB power clears display content

### ESP32-C3 USB CDC
- Auto-reset works correctly for flashing
- No manual BOOT button press required
- COM port: COM4

---

**Last Updated**: 2026-01-02  
**Tester**: V.N. Verbon
