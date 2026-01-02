# Serial Menu Standard - Camper Ecosystem

**Version**: 1.0
**Last Updated**: 2026-01-02
**Applies to**: All embedded devices with serial/UART interface (ESP32, Arduino, Raspberry Pi)

---

## Overview

This document defines the **standardized serial configuration menu** pattern for all devices in the camper ecosystem. A consistent serial interface enables field configuration without rebuilding firmware or complex web interfaces.

## Why Serial Configuration?

### Benefits
1. **No network required**: Configure device before WiFi is set up
2. **Field-serviceable**: Technicians can configure with laptop/phone + USB cable
3. **Universal**: Works on any platform (ESP32, Arduino, Linux)
4. **Debug-friendly**: Same interface used for configuration and troubleshooting
5. **Zero-dependency**: No web server, no app, just terminal emulator

### Use Cases
- Initial device setup (WiFi credentials, device ID)
- Calibration procedures (level sensors, temperature offsets)
- Troubleshooting (view current settings, sensor data)
- Runtime configuration changes (timezone, update intervals)

---

## Menu Activation Pattern

### Standard Activation Trigger
**Press 'm' key** to activate menu from idle state

**Implementation rationale**:
- Single lowercase letter = easy to type, hard to trigger accidentally
- 'm' = mnemonic for "menu"
- Works on all terminal types (no special keys)

### Example Output
```
[Lindi Level Sensor v1.0 - Ready]
[Press 'm' for menu]

I (12345) sensor: Pitch: 1.23°, Roll: -0.45°
I (13345) sensor: Pitch: 1.24°, Roll: -0.44°
m
╔════════════════════════════════════════════════════╗
║          Lindi Configuration Menu                  ║
║                                                    ║
║  1. WiFi Settings                                  ║
║  2. MQTT Configuration                             ║
║  3. Level Offset Calibration                       ║
║  4. System Information                             ║
║  5. View Sensor Data                               ║
║  6. Reboot Device                                  ║
║                                                    ║
║  Q. Quit Menu                                      ║
╔════════════════════════════════════════════════════╗
Enter choice:
```

### Menu Exit Pattern
- **'q' or 'Q' key**: Exit menu, return to normal operation
- **'x' or 'X' key**: Alternative exit (for consistency with common UIs)
- **Timeout**: Optional auto-exit after N minutes of inactivity

---

## Menu Structure

### Hierarchical Organization

**Main menu** → **Submenu** → **Configuration prompt** → **Confirmation**

```
Main Menu
├── 1. WiFi Settings
│   ├── 1. Set SSID
│   ├── 2. Set Password
│   └── Q. Back
├── 2. MQTT Configuration
│   ├── 1. Set Broker Address
│   ├── 2. Set Client ID
│   ├── 3. Set Username/Password
│   └── Q. Back
├── 3. Level Offset Calibration
│   ├── 1. Calibrate Pitch
│   ├── 2. Calibrate Roll
│   ├── 3. Reset to Zero
│   └── Q. Back
└── Q. Quit
```

### Numbered Options
- **1-9**: Primary menu items
- **A-Z**: Extended options (if >9 items)
- **Q**: Always "Quit" or "Back"
- **Single-key selection**: No need to press Enter after number/letter

---

## Implementation Patterns

### ESP32 (ESP-IDF)

**Serial input method** (use standard input, NOT direct UART):
```c
#include <stdio.h>

// Menu task (FreeRTOS)
static void serial_menu_task(void *pvParameters)
{
    bool menu_active = false;
    printf("\n[Press 'm' for menu]\n\n");

    while (1) {
        int c = fgetc(stdin);  // Use standard input (NOT uart_read_bytes)

        if (c != EOF) {
            if (!menu_active && (c == 'm' || c == 'M')) {
                menu_active = true;
                printf("\n");
                display_main_menu();
            } else if (menu_active) {
                if (c == 'q' || c == 'Q' || c == 'x' || c == 'X') {
                    menu_active = false;
                    printf("\n[Menu closed. Press 'm' to reopen]\n\n");
                } else {
                    handle_menu_input(c);
                    display_main_menu();  // Redisplay after action
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));  // Avoid tight loop
    }
}

// Display menu
static void display_main_menu(void)
{
    printf("\n");
    printf("╔════════════════════════════════════════════════════╗\n");
    printf("║          Device Configuration Menu                ║\n");
    printf("║                                                    ║\n");
    printf("║  1. WiFi Settings                                  ║\n");
    printf("║  2. MQTT Configuration                             ║\n");
    printf("║  3. Calibration                                    ║\n");
    printf("║  4. System Information                             ║\n");
    printf("║                                                    ║\n");
    printf("║  Q. Quit Menu                                      ║\n");
    printf("╚════════════════════════════════════════════════════╝\n");
    printf("Enter choice: ");
    fflush(stdout);
}

// Handle user input
static void handle_menu_input(char choice)
{
    switch (choice) {
        case '1':
            wifi_settings_menu();
            break;
        case '2':
            mqtt_settings_menu();
            break;
        case '3':
            calibration_menu();
            break;
        case '4':
            system_info_menu();
            break;
        default:
            printf("\nInvalid choice. Try again.\n");
            break;
    }
}
```

**IMPORTANT**: Use `fgetc(stdin)` instead of `uart_read_bytes()` to avoid conflicts with ESP-IDF console system.

### Arduino

```cpp
void setup() {
    Serial.begin(115200);
    Serial.println("\n[Press 'm' for menu]");
}

bool menuActive = false;

void loop() {
    if (Serial.available() > 0) {
        char c = Serial.read();

        if (!menuActive && (c == 'm' || c == 'M')) {
            menuActive = true;
            displayMainMenu();
        } else if (menuActive) {
            if (c == 'q' || c == 'Q') {
                menuActive = false;
                Serial.println("\n[Menu closed. Press 'm' to reopen]\n");
            } else {
                handleMenuInput(c);
                displayMainMenu();
            }
        }
    }

    // Normal sensor operation continues
    delay(10);
}

void displayMainMenu() {
    Serial.println("\n");
    Serial.println("╔════════════════════════════════════════════════════╗");
    Serial.println("║          Device Configuration Menu                ║");
    Serial.println("║                                                    ║");
    Serial.println("║  1. WiFi Settings                                  ║");
    Serial.println("║  2. Calibration                                    ║");
    Serial.println("║  3. System Information                             ║");
    Serial.println("║                                                    ║");
    Serial.println("║  Q. Quit Menu                                      ║");
    Serial.println("╚════════════════════════════════════════════════════╝");
    Serial.print("Enter choice: ");
}

void handleMenuInput(char choice) {
    switch (choice) {
        case '1':
            wifiSettingsMenu();
            break;
        case '2':
            calibrationMenu();
            break;
        case '3':
            systemInfoMenu();
            break;
        default:
            Serial.println("\nInvalid choice. Try again.");
            break;
    }
}
```

### Python (Raspberry Pi / Linux)

```python
import sys
import select
import tty
import termios

def read_char_non_blocking():
    """Read single character without blocking"""
    if select.select([sys.stdin], [], [], 0)[0]:
        return sys.stdin.read(1)
    return None

def display_main_menu():
    print("\n")
    print("╔════════════════════════════════════════════════════╗")
    print("║          Device Configuration Menu                ║")
    print("║                                                    ║")
    print("║  1. WiFi Settings                                  ║")
    print("║  2. MQTT Configuration                             ║")
    print("║  3. System Information                             ║")
    print("║                                                    ║")
    print("║  Q. Quit Menu                                      ║")
    print("╚════════════════════════════════════════════════════╝")
    print("Enter choice: ", end='', flush=True)

def handle_menu_input(choice):
    if choice == '1':
        wifi_settings_menu()
    elif choice == '2':
        mqtt_settings_menu()
    elif choice == '3':
        system_info_menu()
    else:
        print("\nInvalid choice. Try again.")

def main_loop():
    menu_active = False
    print("\n[Press 'm' for menu]\n")

    # Set terminal to raw mode for immediate character reading
    old_settings = termios.tcgetattr(sys.stdin)
    try:
        tty.setcbreak(sys.stdin.fileno())

        while True:
            c = read_char_non_blocking()

            if c is not None:
                if not menu_active and c.lower() == 'm':
                    menu_active = True
                    display_main_menu()
                elif menu_active:
                    if c.lower() in ['q', 'x']:
                        menu_active = False
                        print("\n[Menu closed. Press 'm' to reopen]\n")
                    else:
                        handle_menu_input(c)
                        display_main_menu()

            # Normal sensor operation continues
            time.sleep(0.01)

    finally:
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, old_settings)

if __name__ == "__main__":
    main_loop()
```

---

## Input Patterns

### Single-Key Selection (Preferred)
User presses key, menu responds immediately (no Enter required)

**Pros**: Fast, intuitive
**Cons**: Requires raw terminal mode (not all platforms support)

### Enter-Terminated Input
User types option and presses Enter

**Pros**: Universal compatibility
**Cons**: Slower, requires extra keypress

**Recommendation**: Use single-key where possible (ESP32, Arduino), fall back to Enter-terminated on platforms with input limitations

---

## User Input Validation

### String Input (WiFi SSID, Device ID)

```c
// Read string with validation
static bool read_string(const char *prompt, char *buffer, size_t max_len, bool allow_empty)
{
    printf("%s", prompt);
    fflush(stdout);

    size_t index = 0;
    while (1) {
        int c = fgetc(stdin);

        if (c == '\r' || c == '\n') {
            buffer[index] = '\0';
            printf("\n");

            if (index == 0 && !allow_empty) {
                printf("Error: Input cannot be empty. Try again.\n");
                printf("%s", prompt);
                fflush(stdout);
                continue;
            }
            return true;
        } else if (c == 27) {  // ESC key
            printf("\n[Cancelled]\n");
            return false;
        } else if (c == 127 || c == 8) {  // Backspace
            if (index > 0) {
                index--;
                printf("\b \b");  // Erase character on screen
                fflush(stdout);
            }
        } else if (c >= 32 && c <= 126 && index < max_len - 1) {  // Printable ASCII
            buffer[index++] = (char)c;
            putchar(c);
            fflush(stdout);
        }
    }
}

// Usage
char ssid[32];
if (read_string("Enter WiFi SSID: ", ssid, sizeof(ssid), false)) {
    // Save to NVS
    nvs_set_str(nvs_handle, "wifi_ssid", ssid);
    printf("✓ SSID saved: %s\n", ssid);
}
```

### Numeric Input (Integer)

```c
// Read integer with range validation
static bool read_int(const char *prompt, int32_t *value, int32_t min, int32_t max)
{
    char buffer[16];

    while (1) {
        if (!read_string(prompt, buffer, sizeof(buffer), false)) {
            return false;  // User cancelled
        }

        char *endptr;
        long val = strtol(buffer, &endptr, 10);

        if (*endptr != '\0') {
            printf("Error: Invalid number. Try again.\n");
            continue;
        }

        if (val < min || val > max) {
            printf("Error: Value must be between %ld and %ld. Try again.\n", (long)min, (long)max);
            continue;
        }

        *value = (int32_t)val;
        return true;
    }
}

// Usage
int32_t timezone;
if (read_int("Enter timezone offset (GMT-12 to GMT+12): ", &timezone, -12, 12)) {
    nvs_set_i32(nvs_handle, "timezone", timezone);
    printf("✓ Timezone set to GMT%+ld\n", (long)timezone);
}
```

### Numeric Input (Float)

```c
// Read float with range validation
static bool read_float(const char *prompt, float *value, float min, float max)
{
    char buffer[16];

    while (1) {
        if (!read_string(prompt, buffer, sizeof(buffer), false)) {
            return false;
        }

        char *endptr;
        float val = strtof(buffer, &endptr);

        if (*endptr != '\0') {
            printf("Error: Invalid number. Try again.\n");
            continue;
        }

        if (val < min || val > max) {
            printf("Error: Value must be between %.2f and %.2f. Try again.\n", min, max);
            continue;
        }

        *value = val;
        return true;
    }
}

// Usage
float pitch_offset;
if (read_float("Enter pitch offset (degrees): ", &pitch_offset, -30.0f, 30.0f)) {
    int32_t millideg = (int32_t)(pitch_offset * 1000);
    nvs_set_i32(nvs_handle, "pitch_off", millideg);
    printf("✓ Pitch offset set to %.3f°\n", pitch_offset);
}
```

### Yes/No Confirmation

```c
// Ask yes/no question
static bool confirm(const char *question)
{
    printf("%s (y/n): ", question);
    fflush(stdout);

    while (1) {
        int c = fgetc(stdin);
        if (c == 'y' || c == 'Y') {
            printf("y\n");
            return true;
        } else if (c == 'n' || c == 'N') {
            printf("n\n");
            return false;
        }
    }
}

// Usage
if (confirm("Reboot device now?")) {
    printf("Rebooting in 3 seconds...\n");
    vTaskDelay(pdMS_TO_TICKS(3000));
    esp_restart();
}
```

---

## Displaying Current Settings

### Formatted Table Output

```c
static void print_current_settings(void)
{
    nvs_handle_t nvs;
    nvs_open("device_cfg", NVS_READONLY, &nvs);

    printf("\n");
    printf("╔════════════════════════════════════════════════════╗\n");
    printf("║          Current Device Settings                  ║\n");
    printf("╠════════════════════════════════════════════════════╣\n");

    // WiFi settings
    char ssid[32] = "Not configured";
    size_t len = sizeof(ssid);
    nvs_get_str(nvs, "wifi_ssid", ssid, &len);
    printf("║ WiFi SSID:         %-31s ║\n", ssid);
    printf("║ WiFi Password:     %-31s ║\n", "********");  // Never display password

    // MQTT settings
    char client_id[32] = "auto";
    len = sizeof(client_id);
    nvs_get_str(nvs, "mqtt_client_id", client_id, &len);
    printf("║ MQTT Client ID:    %-31s ║\n", client_id);

    // Timezone
    int32_t timezone = 0;
    nvs_get_i32(nvs, "timezone", &timezone);
    printf("║ Timezone:          GMT%+ld                         ║\n", (long)timezone);

    // Level offsets
    int32_t pitch_off = 0, roll_off = 0;
    nvs_get_i32(nvs, "pitch_off", &pitch_off);
    nvs_get_i32(nvs, "roll_off", &roll_off);
    printf("║ Pitch Offset:      %+.3f°                      ║\n", pitch_off / 1000.0f);
    printf("║ Roll Offset:       %+.3f°                      ║\n", roll_off / 1000.0f);

    printf("╚════════════════════════════════════════════════════╝\n");
    printf("\n");

    nvs_close(nvs);
}
```

**Example output**:
```
╔════════════════════════════════════════════════════╗
║          Current Device Settings                  ║
╠════════════════════════════════════════════════════╣
║ WiFi SSID:         home_network                   ║
║ WiFi Password:     ********                       ║
║ MQTT Client ID:    lindi_AB12CD                   ║
║ Timezone:          GMT+1                          ║
║ Pitch Offset:      +0.123°                        ║
║ Roll Offset:       -0.045°                        ║
╚════════════════════════════════════════════════════╝
```

---

## Live Sensor Data Display

### Continuous Update Pattern

```c
static void show_sensor_data(void)
{
    printf("\n");
    printf("╔════════════════════════════════════════════════════╗\n");
    printf("║          Live Sensor Data                          ║\n");
    printf("║          (Press 'q' to stop)                       ║\n");
    printf("╚════════════════════════════════════════════════════╝\n");
    printf("\n");

    while (1) {
        // Read sensor values (platform-specific)
        float pitch, roll, temperature;
        get_sensor_values(&pitch, &roll, &temperature);

        // Display with ANSI escape codes for same-line update
        printf("\r");  // Carriage return to beginning of line
        printf("Pitch: %+6.2f° | Roll: %+6.2f° | Temp: %.1f°C   ", pitch, roll, temperature);
        fflush(stdout);

        // Check for 'q' key to exit
        int c = fgetc(stdin);
        if (c == 'q' || c == 'Q') {
            printf("\n\n");
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(100));  // Update at 10Hz
    }
}
```

**Alternative: Multi-line scrolling display**:
```c
static void show_sensor_data_scrolling(void)
{
    printf("\n[Live sensor data - Press 'q' to stop]\n");
    printf("Timestamp           | Pitch   | Roll    | Temperature\n");
    printf("-------------------------------------------------------\n");

    while (1) {
        time_t now = time(NULL);
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);

        float pitch, roll, temperature;
        get_sensor_values(&pitch, &roll, &temperature);

        printf("%02d:%02d:%02d | %+6.2f° | %+6.2f° | %.1f°C\n",
               timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
               pitch, roll, temperature);

        int c = fgetc(stdin);
        if (c == 'q' || c == 'Q') {
            printf("\n");
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(1000));  // 1 Hz
    }
}
```

---

## NVS Integration

### Namespace Organization

**Standard namespace naming**:
- Device configuration: `"device_cfg"`
- WiFi settings: `"wifi_cfg"` (or part of device_cfg)
- MQTT settings: `"mqtt_cfg"` (or part of device_cfg)
- Calibration data: `"calibration"` (or part of device_cfg)

**Recommendation**: Use single namespace `"device_cfg"` for simplicity unless data categories are very large

### Key Naming Conventions

**Use descriptive, abbreviated keys**:
- WiFi SSID: `"wifi_ssid"`
- WiFi Password: `"wifi_pass"`
- MQTT Client ID: `"mqtt_client_id"`
- MQTT Broker: `"mqtt_broker"`
- Timezone: `"timezone"`
- Pitch offset: `"pitch_off"` (NOT `"pitch_offset"` - keep short)
- Roll offset: `"roll_off"`

**Consistency is critical**: Ensure read and write operations use identical key names

### Safe NVS Write Pattern

```c
// Helper function: write with error handling
static esp_err_t nvs_set_str_safe(const char *namespace, const char *key, const char *value)
{
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(namespace, NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_str(nvs, key, value);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write NVS key '%s': %s", key, esp_err_to_name(err));
        nvs_close(nvs);
        return err;
    }

    err = nvs_commit(nvs);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(err));
    }

    nvs_close(nvs);
    return err;
}

// Similar for int32
static esp_err_t nvs_set_i32_safe(const char *namespace, const char *key, int32_t value)
{
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(namespace, NVS_READWRITE, &nvs);
    if (err != ESP_OK) return err;

    err = nvs_set_i32(nvs, key, value);
    if (err == ESP_OK) {
        err = nvs_commit(nvs);
    }

    nvs_close(nvs);
    return err;
}

// Usage
if (nvs_set_str_safe("device_cfg", "wifi_ssid", "home_network") == ESP_OK) {
    printf("✓ SSID saved successfully\n");
} else {
    printf("✗ Failed to save SSID\n");
}
```

### Immediate Effect Pattern

When configuration changes should take effect immediately (without reboot):

```c
// Example: Level offset calibration
static void configure_pitch_offset(void)
{
    float pitch_offset;
    if (read_float("Enter pitch offset (degrees): ", &pitch_offset, -30.0f, 30.0f)) {
        // Save to NVS
        int32_t millideg = (int32_t)(pitch_offset * 1000);
        nvs_set_i32_safe("device_cfg", "pitch_off", millideg);

        // Apply immediately to running system
        load_calibration_offsets();  // External function in main.c

        printf("✓ Pitch offset set to %.3f°\n", pitch_offset);
        printf("  Changes applied immediately.\n");
    }
}
```

**Key point**: After NVS write, call the function that reloads settings into runtime variables

---

## Reboot Notification

### Standard Reboot Messages

```c
static void reboot_device(void)
{
    if (confirm("Reboot device now?")) {
        printf("\n");
        printf("╔════════════════════════════════════════════════════╗\n");
        printf("║  Device will reboot in 3 seconds...               ║\n");
        printf("╚════════════════════════════════════════════════════╝\n");

        for (int i = 3; i > 0; i--) {
            printf("  %d...\n", i);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }

        printf("  Rebooting now.\n");
        vTaskDelay(pdMS_TO_TICKS(500));

        esp_restart();  // ESP32
        // NVIC_SystemReset();  // Arduino
        // os.system("sudo reboot");  // Raspberry Pi
    }
}
```

### Restart-Required Indicator

For settings that require restart to take effect:

```c
static void configure_wifi_ssid(void)
{
    char ssid[32];
    if (read_string("Enter WiFi SSID: ", ssid, sizeof(ssid), false)) {
        nvs_set_str_safe("device_cfg", "wifi_ssid", ssid);
        printf("✓ SSID saved: %s\n", ssid);
        printf("⚠ Restart required for changes to take effect.\n");
    }
}
```

---

## Error Handling

### User-Friendly Error Messages

**Good**:
```
✗ Failed to save WiFi password to flash storage.
  Reason: Storage full
  Solution: Factory reset required (Option 9 in main menu)
```

**Bad**:
```
Error: nvs_set_str returned 0x1101 (ESP_ERR_NVS_NOT_ENOUGH_SPACE)
```

### Error Message Template

```c
static void show_error(const char *operation, esp_err_t err)
{
    printf("✗ Failed to %s.\n", operation);
    printf("  Reason: %s\n", esp_err_to_name(err));

    // Suggest solutions
    if (err == ESP_ERR_NVS_NOT_ENOUGH_SPACE) {
        printf("  Solution: Factory reset required (Option 9 in main menu)\n");
    } else if (err == ESP_ERR_NVS_NOT_FOUND) {
        printf("  Solution: Setting not configured, using default value\n");
    }
}

// Usage
esp_err_t err = nvs_set_str_safe("device_cfg", "wifi_ssid", ssid);
if (err != ESP_OK) {
    show_error("save WiFi SSID", err);
}
```

---

## Factory Reset

### Standard Factory Reset Procedure

```c
static void factory_reset(void)
{
    printf("\n");
    printf("╔════════════════════════════════════════════════════╗\n");
    printf("║              ⚠ FACTORY RESET WARNING ⚠            ║\n");
    printf("║                                                    ║\n");
    printf("║  This will erase ALL settings:                    ║\n");
    printf("║  • WiFi credentials                                ║\n");
    printf("║  • MQTT configuration                              ║\n");
    printf("║  • Calibration data                                ║\n");
    printf("║  • All custom settings                             ║\n");
    printf("║                                                    ║\n");
    printf("║  Device will reboot after reset.                  ║\n");
    printf("╚════════════════════════════════════════════════════╝\n");
    printf("\n");

    if (confirm("Are you SURE you want to factory reset?")) {
        printf("Erasing NVS flash partition...\n");

        esp_err_t err = nvs_flash_erase();
        if (err == ESP_OK) {
            printf("✓ Factory reset complete.\n");
            printf("  Device will reboot in 3 seconds...\n");
            vTaskDelay(pdMS_TO_TICKS(3000));
            esp_restart();
        } else {
            show_error("erase flash", err);
        }
    } else {
        printf("[Factory reset cancelled]\n");
    }
}
```

---

## Implementation Checklist

When adding serial menu to a new device:

- [ ] Create serial menu task (FreeRTOS) or loop integration (Arduino)
- [ ] Use standard input (`fgetc(stdin)`) on ESP32, `Serial.read()` on Arduino
- [ ] Implement 'm' key activation pattern
- [ ] Create hierarchical menu structure (main → submenus)
- [ ] Add numbered options (1-9, A-Z) and 'Q' for quit
- [ ] Implement input validation (strings, integers, floats, yes/no)
- [ ] Display current settings in formatted table
- [ ] Integrate NVS read/write with error handling
- [ ] Add immediate effect pattern where applicable (reload settings)
- [ ] Include restart-required indicators for settings that need it
- [ ] Implement factory reset with confirmation prompt
- [ ] Add live sensor data display (optional)
- [ ] Test on actual hardware with terminal emulator
- [ ] Document menu options in user manual

---

## Terminal Emulator Recommendations

### Windows
- **PuTTY**: Serial COM port support, configurable baud rate
- **Tera Term**: Popular, easy to use
- **Arduino IDE Serial Monitor**: Simple, built-in

### macOS/Linux
- **screen**: `screen /dev/ttyUSB0 115200`
- **minicom**: `minicom -D /dev/ttyUSB0 -b 115200`
- **PuTTY**: Available on Linux too
- **Arduino IDE Serial Monitor**: Cross-platform

### Configuration
- **Baud rate**: 115200 (standard for ESP32/Arduino)
- **Data bits**: 8
- **Parity**: None
- **Stop bits**: 1
- **Flow control**: None
- **Local echo**: Enable (optional, for seeing typed characters)

---

## Related Ecosystem Standards

See also:
- [nvs_config_ecosystem.md](nvs_config_ecosystem.md) - Configuration storage patterns
- [timestamp_ecosystem.md](timestamp_ecosystem.md) - Standardized timestamp format
- [mqtt_ecosystem.md](mqtt_ecosystem.md) - MQTT communication standard

---

**Ecosystem Owner**: Syquens B.V.
**Maintained by**: V.N. Verbon
**Version Control**: Git repository `Lindi/export/`
