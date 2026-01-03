# Quick Reference - Localizer GPS Tracker

## Pin Connections

### GPS Module (GY-GPS6MV2)
```
GPS VCC  → ESP32 3.3V
GPS GND  → ESP32 GND
GPS TX   → ESP32 GPIO20 (UART0_RX)
GPS RX   → ESP32 GPIO21 (UART0_TX)
```

### RTC Module (DS3231)
```
RTC VCC  → ESP32 3.3V
RTC GND  → ESP32 GND
RTC SDA  → ESP32 GPIO5 (shared with OLED)
RTC SCL  → ESP32 GPIO6 (shared with OLED)
```

### OLED Display (Onboard)
```
Already connected via GPIO5/6 (I2C)
```

## Capacitors

### Short Cables (< 10cm)
No additional capacitors needed.

### Medium Cables (10-50cm)
- 10µF electrolytic near ESP32
- 100nF ceramic near each module

### Long Cables (> 50cm)
- 100µF + 100nF near ESP32
- 10µF + 100nF near each module

## Display Layout

```
Line 1: GPS: INIT (red) → GPS FIX OK (green)
Line 2: RTC LOCAL (red) → RTC SYNC (green)
Line 3: WIFI (left) + NTP (right) - color based on status
Line 4: Scrolling GPS data (SAT:8 51.5,-0.1 14:32:45)
Line 5: Scrolling location (Street/City/CountryCode)
```

## Configuration

Edit `main/config.h`:
```c
#define DEFAULT_WIFI_SSID       "YourSSID"
#define DEFAULT_WIFI_PASS       "YourPassword"
#define DEFAULT_MQTT_BROKER     "mqtt.syquens.com"
#define DEFAULT_MQTT_USER       "camper_device"
#define DEFAULT_MQTT_PASS       "your_password"
```

## Build & Flash

```powershell
cd e:\Dev\Localizer
idf.py build
idf.py flash
idf.py monitor
```

## Documentation

- Full details: [IMPLEMENTATION.md](IMPLEMENTATION.md)
- GPS hardware: [documentation/gps_module.md](documentation/gps_module.md)
- RTC hardware: [documentation/rtc_module.md](documentation/rtc_module.md)
- Board pinout: [documentation/board_reference.md](documentation/board_reference.md)
- MQTT format: [export/mqtt_ecosystem.md](export/mqtt_ecosystem.md)
- NVS config: [export/nvs_config_ecosystem.md](export/nvs_config_ecosystem.md)
