# GY-GPS6MV2 GPS Module - Connection Guide

**Module**: GY-GPS6MV2 (NEO-6M based)  
**Chipset**: u-blox NEO-6M  
**Interface**: UART (Serial)  
**Protocol**: NMEA 0183  
**Last Updated**: 2026-01-03

---

## Overview

The GY-GPS6MV2 is a compact GPS module based on the u-blox NEO-6M chipset. It provides reliable GPS position fixes via UART interface using standard NMEA sentences.

**Key Features**:
- **Chipset**: u-blox NEO-6M
- **Channels**: 50 channels
- **Update Rate**: 1Hz (default), up to 5Hz configurable
- **Accuracy**: 2.5m CEP (Circular Error Probable)
- **Cold Start**: ~27s
- **Hot Start**: ~1s
- **Interface**: UART (TTL 3.3V/5V compatible)
- **Default Baud Rate**: 9600 bps (8N1)
- **Voltage**: 3.3V - 5V
- **Current**: ~45mA active, ~20mA backup
- **Antenna**: Built-in ceramic patch antenna (25mm x 25mm)

---

## Pin Connections to ESP32-C3

### Pin Mapping

| GPS Module Pin | ESP32-C3 Pin | Function | Notes |
|----------------|--------------|----------|-------|
| **VCC** | **3.3V** or **5V** | Power supply | 3.3V recommended (module has onboard regulator) |
| **GND** | **GND** | Ground | Common ground |
| **TX** | **GPIO20** (UART0_RX) | GPS transmit → ESP receive | NMEA data output from GPS |
| **RX** | **GPIO21** (UART0_TX) | GPS receive ← ESP transmit | Commands to GPS (optional) |

### Connection Diagram

```
GY-GPS6MV2 Module          ESP32-C3 Board
┌────────────────┐         ┌────────────────┐
│                │         │                │
│  VCC      GND  │         │  3V3      GND  │
│   ●        ●   │         │   ●        ●   │
│   │        │   │         │   │        │   │
│   │        └───┼─────────┼───┘        │   │
│   └────────────┼─────────┼────────────┘   │
│                │         │                │
│  TX       RX   │         │  GPIO20  GPIO21│
│   ●        ●   │         │    ●       ●   │
│   │        │   │         │    │       │   │
│   │        └───┼─────────┼────┘       │   │
│   └────────────┼─────────┼────────────┘   │
│                │         │                │
│  [Ceramic      │         │                │
│   Antenna]     │         │                │
└────────────────┘         └────────────────┘

VCC  → 3.3V (or 5V if module has regulator)
GND  → GND
TX   → GPIO20 (UART0_RX) - GPS sends data to ESP
RX   → GPIO21 (UART0_TX) - ESP sends commands to GPS (optional)
```

### Important Notes

1. **Power Supply**: The GY-GPS6MV2 typically has an onboard 3.3V regulator, so it can accept either 3.3V or 5V input. Using 3.3V is recommended to reduce heat and power consumption.

2. **TX/RX Crossover**: GPS module's TX pin connects to ESP32's RX pin (GPIO20), and GPS RX connects to ESP32 TX (GPIO21). This is standard UART crossover wiring.

3. **UART Configuration**: 
   - Baud Rate: 9600 bps (default)
   - Data Bits: 8
   - Parity: None
   - Stop Bits: 1

4. **Signal Levels**: The module outputs 3.3V TTL signals, fully compatible with ESP32-C3.

---

## Capacitor Recommendations

### Power Decoupling

**Basic Setup (short cables < 10cm)**:
- **No additional capacitors required** - The module has onboard decoupling

**Standard Setup (cables 10cm - 50cm)**:
- **10µF electrolytic** capacitor across VCC/GND near ESP32
- **100nF ceramic** capacitor across VCC/GND near GPS module

**Long Cable Setup (cables > 50cm)**:
- **100µF electrolytic** across VCC/GND near ESP32
- **10µF electrolytic** across VCC/GND near GPS module
- **100nF ceramic** at both ends (ESP32 and GPS)

### Capacitor Placement

```
Cable Length Recommendations:

< 10cm:  No caps needed (module has onboard decoupling)
         ┌─────┐     ┌─────┐
         │ESP32├─────┤ GPS │
         └─────┘     └─────┘

10-50cm: Add local decoupling at both ends
         ┌─────┐     ┌─────┐
    C1 ══╡ESP32├─────┤ GPS ╞══ C2
         └─────┘     └─────┘
         10µF        100nF

> 50cm:  Add bulk and decoupling caps
         ┌─────┐     ┌─────┐
C1+C2 ═══╡ESP32├─────┤ GPS ╞═══ C3+C4
         └─────┘     └─────┘
         100µF+100nF  10µF+100nF
```

**Capacitor Specifications**:
- **Ceramic caps**: X7R or better, 6.3V or higher
- **Electrolytic caps**: Low ESR, 6.3V or higher
- Place capacitors as close as possible to power pins

### Current Surge Protection

The GPS module can draw surge current during initial satellite acquisition:
- **Typical**: 45mA continuous
- **Peak**: Up to 67mA during acquisition

If using long cables or weak power supply:
- Add **220µF low-ESR electrolytic** near GPS module
- Ensures stable voltage during current peaks

---

## NMEA Sentence Output

The NEO-6M outputs standard NMEA 0183 sentences at 9600 baud, 1Hz update rate:

### Common NMEA Sentences

| Sentence | Description | Example |
|----------|-------------|---------|
| **$GPGGA** | Global Positioning System Fix Data | Position, altitude, fix quality |
| **$GPGSA** | GPS DOP and Active Satellites | Satellite usage, HDOP, VDOP |
| **$GPGSV** | GPS Satellites in View | Satellite count, elevation, azimuth, SNR |
| **$GPRMC** | Recommended Minimum Specific GPS Data | Position, speed, date/time |
| **$GPVTG** | Track Made Good and Ground Speed | Course, speed over ground |
| **$GPGLL** | Geographic Position - Lat/Lon | Position, time |

### Example NMEA Output

```
$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
$GPGSA,A,3,04,05,,09,12,,,24,,,,,2.5,1.3,2.1*39
$GPGSV,2,1,08,01,40,083,46,02,17,308,41,12,07,344,39,14,22,228,45*75
$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A
```

**Fix Indicators**:
- **Fix Quality** (in GPGGA): 0=invalid, 1=GPS fix, 2=DGPS fix
- **Fix Status** (in GPRMC): A=active (valid), V=void (invalid)
- **Fix Mode** (in GPGSA): 1=no fix, 2=2D fix, 3=3D fix

---

## Time-to-First-Fix (TTFF)

| Start Type | Conditions | Typical TTFF |
|------------|------------|--------------|
| **Cold Start** | No almanac, no position, no time | ~27 seconds |
| **Warm Start** | Almanac available, approximate position known | ~27 seconds |
| **Hot Start** | Valid almanac, position, time | ~1 second |

**Factors Affecting TTFF**:
- Clear sky view (avoid indoors, under trees, near buildings)
- Number of visible satellites (minimum 4 for 3D fix)
- Satellite signal strength
- Atmospheric conditions

---

## Software Configuration (ESP-IDF)

### UART Initialization

```c
#include "driver/uart.h"

#define GPS_UART_NUM        UART_NUM_0
#define GPS_TX_PIN          GPIO_NUM_21  // ESP32 TX → GPS RX
#define GPS_RX_PIN          GPIO_NUM_20  // ESP32 RX ← GPS TX
#define GPS_BAUD_RATE       9600

void gps_init(void)
{
    const uart_config_t uart_config = {
        .baud_rate = GPS_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    
    // Install UART driver
    uart_driver_install(GPS_UART_NUM, 1024 * 2, 0, 0, NULL, 0);
    uart_param_config(GPS_UART_NUM, &uart_config);
    uart_set_pin(GPS_UART_NUM, GPS_TX_PIN, GPS_RX_PIN, 
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}
```

### Reading NMEA Data

```c
char nmea_buffer[256];
int len = uart_read_bytes(GPS_UART_NUM, (uint8_t*)nmea_buffer, 
                          sizeof(nmea_buffer) - 1, pdMS_TO_TICKS(100));
if (len > 0) {
    nmea_buffer[len] = '\0';
    printf("GPS: %s", nmea_buffer);
}
```

---

## Troubleshooting

### No GPS Data

1. **Check wiring**: Verify TX/RX crossover (GPS TX → ESP32 RX)
2. **Check power**: Measure 3.3V at module VCC pin
3. **Check baud rate**: Default is 9600 bps
4. **LED indicator**: Most modules have LED that blinks when locked (check module datasheet)

### GPS Not Getting Fix

1. **Antenna placement**: Needs clear view of sky (not indoors)
2. **Wait time**: Cold start can take 27+ seconds
3. **Check NMEA output**: Should see GPGGA sentences with fix quality > 0
4. **Satellite count**: Minimum 4 satellites needed for 3D fix

### Intermittent Data

1. **Add decoupling capacitor**: 10µF near GPS module
2. **Shorten cable**: Keep < 20cm if possible
3. **Check ground connection**: Solid GND connection critical
4. **Power supply**: Ensure ESP32 power supply can handle 45mA GPS current

---

## Performance Optimization

### Increasing Update Rate

The NEO-6M can be configured for higher update rates (up to 5Hz):

```c
// Send UBX command to set 5Hz update rate (200ms interval)
// (Requires UBX protocol, not covered here - use u-center software or library)
```

**Note**: Higher update rates increase current consumption and CPU load.

### Reducing Power Consumption

For battery-powered applications:
- Use **backup mode** when position not needed
- Enable **power save mode** via UBX commands
- Poll GPS periodically instead of continuous operation

---

## External Antenna

If using external active antenna:
- **Antenna Power**: Check if module provides 3.3V antenna bias
- **Cable**: Use 50Ω coaxial cable (RG174 or similar)
- **Connector**: Most modules use U.FL connector
- **Decoupling**: Add 10µF cap if cable > 1 meter

---

## References

- [u-blox NEO-6M Datasheet](https://www.u-blox.com/en/product/neo-6-series)
- [NMEA 0183 Specification](https://www.nmea.org/content/STANDARDS/NMEA_0183_Standard)
- ESP-IDF UART Driver Documentation
