# NVS Configuration Standard - Camper Ecosystem

**Version**: 1.0
**Last Updated**: 2026-01-02
**Applies to**: ESP32 devices in camper ecosystem (using ESP-IDF NVS library)

---

## Overview

This document defines the **standardized Non-Volatile Storage (NVS) configuration pattern** for all ESP32 devices in the camper ecosystem. Consistent NVS usage ensures reliable persistent settings, predictable behavior, and easier troubleshooting.

## What is NVS?

**NVS (Non-Volatile Storage)** is a flash-based key-value storage system on ESP32 that:
- Survives power cycles and reboots
- Stores configuration settings (WiFi, MQTT, calibration data)
- Provides wear leveling to extend flash lifespan
- Supports multiple data types (int, string, blob)

**Not suitable for**:
- High-frequency data logging (use SD card or external storage)
- Large data blobs (>4KB per key)
- Rapid write cycles (flash has limited write endurance)

---

## Namespace Organization

### Standard Namespace Naming

**Single namespace approach** (recommended for simplicity):
```c
#define NVS_NAMESPACE "device_cfg"
```

All device settings stored under one namespace: `device_cfg`

**Multi-namespace approach** (for complex devices):
```c
#define NVS_WIFI      "wifi_cfg"
#define NVS_MQTT      "mqtt_cfg"
#define NVS_SENSOR    "sensor_cfg"
#define NVS_SYSTEM    "system_cfg"
```

**Recommendation**: Use single namespace unless you have >20 keys or clear logical separation

### Namespace Examples by Device Type

| Device Type | Namespace | Rationale |
|-------------|-----------|-----------|
| Level sensor | `device_cfg` | Single namespace, <15 keys |
| GPS tracker | `device_cfg` | Simple configuration |
| Battery monitor | `device_cfg` | Minimal settings |
| Complex controller | `wifi_cfg`, `mqtt_cfg`, `control_cfg` | Logical separation for 30+ keys |

---

## Key Naming Conventions

### Standard Key Format

**Pattern**: `<category>_<setting>`

**Examples**:
- WiFi SSID: `wifi_ssid`
- WiFi Password: `wifi_pass`
- MQTT Broker: `mqtt_broker`
- MQTT Client ID: `mqtt_client_id`
- MQTT Username: `mqtt_user`
- MQTT Password: `mqtt_pass`
- Timezone: `timezone`
- Language: `language`
- Pitch offset: `pitch_off`
- Roll offset: `roll_off`
- Update interval: `update_interval`
- Dark theme: `dark_theme`
- Accent color: `accent_color`

### Key Naming Rules

1. **Lowercase only**: No uppercase letters
2. **Underscores for spaces**: `wifi_ssid`, not `WiFiSSID` or `wifi-ssid`
3. **Abbreviate long words**: `pitch_off` not `pitch_offset` (saves flash)
4. **Consistent prefixes**: All WiFi keys start with `wifi_`, MQTT with `mqtt_`
5. **Max 15 characters**: NVS key limit (keep short for efficiency)

### Reserved Key Names

**Do NOT use these keys** (potential conflicts):
- `phy_init` (ESP32 PHY calibration data)
- `nvs.net80211` (WiFi calibration)
- Any key starting with `nvs.` (system reserved)

---

## Data Types & Storage

### Supported NVS Data Types

| C Type | NVS Function | Use Case | Example |
|--------|--------------|----------|---------|
| `int8_t` | `nvs_set_i8` | Small signed integers | Temperature offset (-10 to +10) |
| `uint8_t` | `nvs_set_u8` | Small unsigned integers | LED brightness (0-255) |
| `int16_t` | `nvs_set_i16` | Medium signed integers | Calibration offset (millidegrees) |
| `uint16_t` | `nvs_set_u16` | Medium unsigned integers | Update interval (seconds) |
| `int32_t` | `nvs_set_i32` | Large signed integers | Timezone (-12 to +12), offsets |
| `uint32_t` | `nvs_set_u32` | Large unsigned integers | Bitmasks, flags |
| `int64_t` | `nvs_set_i64` | Very large signed integers | Timestamps (milliseconds) |
| `uint64_t` | `nvs_set_u64` | Very large unsigned integers | 64-bit counters |
| `char*` | `nvs_set_str` | Strings | WiFi SSID, MQTT broker address |
| `void*` | `nvs_set_blob` | Binary data | Calibration matrices, certificates |

### Type Selection Guidelines

**Use smallest type that fits your range**:
- Booleans → `uint8_t` (0 or 1)
- Timezone (-12 to +12) → `int32_t`
- Offsets in millidegrees → `int32_t`
- Strings → `nvs_set_str`
- WiFi credentials → `nvs_set_str`

**Avoid unnecessary precision**:
- Store angles as millidegrees (int32) instead of float
- Store percentages as 0-100 (uint8) instead of 0.0-1.0 (float)

---

## Default Values

### Default Value Strategy

**Always provide defaults** for every NVS key to handle:
- First boot (no settings saved yet)
- Factory reset (all settings erased)
- Corrupted NVS partition

### Implementation Pattern

```c
// Helper function: read int32 with default
static int32_t nvs_get_i32_default(nvs_handle_t nvs, const char *key, int32_t default_value)
{
    int32_t value;
    esp_err_t err = nvs_get_i32(nvs, key, &value);

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "NVS key '%s' not found, using default: %ld", key, (long)default_value);
        return default_value;
    } else if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read NVS key '%s': %s, using default", key, esp_err_to_name(err));
        return default_value;
    }

    return value;
}

// Similar for string
static void nvs_get_str_default(nvs_handle_t nvs, const char *key, char *buffer, size_t max_len, const char *default_value)
{
    size_t len = max_len;
    esp_err_t err = nvs_get_str(nvs, key, buffer, &len);

    if (err == ESP_ERR_NVS_NOT_FOUND || err != ESP_OK) {
        strncpy(buffer, default_value, max_len - 1);
        buffer[max_len - 1] = '\0';
        ESP_LOGI(TAG, "NVS key '%s' not found, using default: %s", key, default_value);
    }
}

// Usage
nvs_handle_t nvs;
nvs_open("device_cfg", NVS_READONLY, &nvs);

int32_t timezone = nvs_get_i32_default(nvs, "timezone", 0);  // Default: GMT+0
char mqtt_broker[64];
nvs_get_str_default(nvs, "mqtt_broker", mqtt_broker, sizeof(mqtt_broker), "mqtt.syquens.com");

nvs_close(nvs);
```

### Standard Default Values

| Setting | Default Value | Rationale |
|---------|---------------|-----------|
| Timezone | 0 (GMT+0) | Universal starting point |
| Language | `"en"` | English as default |
| MQTT Broker | `"mqtt.syquens.com"` | Ecosystem standard |
| MQTT Port | 8883 | Secure MQTT |
| Update Interval | 1000 (1 Hz) | Reasonable default |
| Dark Theme | 0 (light mode) | Visibility preference |
| Pitch/Roll Offset | 0 | Uncalibrated state |
| WiFi SSID | `""` (empty) | Must be configured |
| WiFi Password | `""` (empty) | Must be configured |

---

## Read/Write Patterns

### Safe Write Pattern (with error handling)

```c
// Write string to NVS with full error handling
static esp_err_t nvs_set_str_safe(const char *namespace, const char *key, const char *value)
{
    nvs_handle_t nvs;
    esp_err_t err;

    // Open NVS in read/write mode
    err = nvs_open(namespace, NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace '%s': %s", namespace, esp_err_to_name(err));
        return err;
    }

    // Write value
    err = nvs_set_str(nvs, key, value);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write NVS key '%s': %s", key, esp_err_to_name(err));
        nvs_close(nvs);
        return err;
    }

    // Commit changes to flash
    err = nvs_commit(nvs);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "NVS key '%s' saved: %s", key, value);
    }

    nvs_close(nvs);
    return err;
}

// Write int32 to NVS with full error handling
static esp_err_t nvs_set_i32_safe(const char *namespace, const char *key, int32_t value)
{
    nvs_handle_t nvs;
    esp_err_t err;

    err = nvs_open(namespace, NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace '%s': %s", namespace, esp_err_to_name(err));
        return err;
    }

    err = nvs_set_i32(nvs, key, value);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write NVS key '%s': %s", key, esp_err_to_name(err));
        nvs_close(nvs);
        return err;
    }

    err = nvs_commit(nvs);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "NVS key '%s' saved: %ld", key, (long)value);
    }

    nvs_close(nvs);
    return err;
}
```

### Read Pattern (with defaults)

```c
static void load_device_settings(void)
{
    nvs_handle_t nvs;
    esp_err_t err = nvs_open("device_cfg", NVS_READONLY, &nvs);

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to open NVS, using defaults: %s", esp_err_to_name(err));
        // Use all default values
        return;
    }

    // Read settings with defaults
    g_timezone = nvs_get_i32_default(nvs, "timezone", 0);
    g_update_interval_ms = nvs_get_i32_default(nvs, "update_interval", 1000);
    g_dark_theme = nvs_get_u8_default(nvs, "dark_theme", 0);

    char language[8];
    nvs_get_str_default(nvs, "language", language, sizeof(language), "en");
    set_ui_language(language);

    // Calibration offsets (in millidegrees)
    int32_t pitch_off_millideg = nvs_get_i32_default(nvs, "pitch_off", 0);
    int32_t roll_off_millideg = nvs_get_i32_default(nvs, "roll_off", 0);
    g_pitch_offset = pitch_off_millideg / 1000.0f;
    g_roll_offset = roll_off_millideg / 1000.0f;

    ESP_LOGI(TAG, "Settings loaded: timezone=GMT%+ld, lang=%s, theme=%s",
             (long)g_timezone, language, g_dark_theme ? "dark" : "light");

    nvs_close(nvs);
}
```

### Batch Read Pattern

For reading multiple related settings efficiently:

```c
static void load_wifi_settings(char *ssid, size_t ssid_len, char *pass, size_t pass_len)
{
    nvs_handle_t nvs;
    esp_err_t err = nvs_open("device_cfg", NVS_READONLY, &nvs);

    if (err == ESP_OK) {
        // Read WiFi SSID
        size_t len = ssid_len;
        err = nvs_get_str(nvs, "wifi_ssid", ssid, &len);
        if (err != ESP_OK) {
            ssid[0] = '\0';  // Empty string if not found
        }

        // Read WiFi password
        len = pass_len;
        err = nvs_get_str(nvs, "wifi_pass", pass, &len);
        if (err != ESP_OK) {
            pass[0] = '\0';
        }

        nvs_close(nvs);
    } else {
        ssid[0] = '\0';
        pass[0] = '\0';
    }
}

// Usage
char ssid[32], pass[64];
load_wifi_settings(ssid, sizeof(ssid), pass, sizeof(pass));

if (strlen(ssid) > 0) {
    ESP_LOGI(TAG, "WiFi configured: %s", ssid);
    connect_wifi(ssid, pass);
} else {
    ESP_LOGW(TAG, "WiFi not configured, entering AP mode");
}
```

---

## Initialization & First Boot

### NVS Initialization Pattern

```c
void app_main(void)
{
    // Initialize NVS (MUST be called before any NVS operations)
    esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated or version changed, erase and retry
        ESP_LOGW(TAG, "NVS partition issue, erasing and reinitializing");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_LOGI(TAG, "NVS initialized successfully");

    // Load device settings from NVS
    load_device_settings();

    // Continue with application initialization
    // ...
}
```

### First-Boot Detection

```c
static bool is_first_boot(void)
{
    nvs_handle_t nvs;
    esp_err_t err = nvs_open("device_cfg", NVS_READONLY, &nvs);

    if (err != ESP_OK) {
        return true;  // NVS not accessible = first boot
    }

    uint8_t initialized = 0;
    err = nvs_get_u8(nvs, "initialized", &initialized);
    nvs_close(nvs);

    return (err == ESP_ERR_NVS_NOT_FOUND || initialized == 0);
}

static void set_initialized_flag(void)
{
    nvs_set_u8_safe("device_cfg", "initialized", 1);
}

// Usage
void app_main(void)
{
    nvs_flash_init();

    if (is_first_boot()) {
        ESP_LOGI(TAG, "First boot detected, using default settings");
        set_initialized_flag();
    } else {
        ESP_LOGI(TAG, "Loading saved settings from NVS");
    }

    load_device_settings();
}
```

---

## Factory Reset

### Complete NVS Erase

```c
static esp_err_t factory_reset(void)
{
    ESP_LOGW(TAG, "Performing factory reset...");

    // Erase entire NVS partition
    esp_err_t err = nvs_flash_erase();

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Factory reset complete, reinitializing NVS");

        // Reinitialize NVS
        err = nvs_flash_init();
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "NVS reinitialized, device will reboot");
            vTaskDelay(pdMS_TO_TICKS(1000));
            esp_restart();
        }
    }

    return err;
}
```

### Selective Namespace Erase

Erase only specific namespace (preserve others):

```c
static esp_err_t erase_namespace(const char *namespace)
{
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(namespace, NVS_READWRITE, &nvs);

    if (err == ESP_OK) {
        err = nvs_erase_all(nvs);  // Erase all keys in this namespace
        if (err == ESP_OK) {
            err = nvs_commit(nvs);
        }
        nvs_close(nvs);
    }

    return err;
}

// Usage: Reset only WiFi settings
erase_namespace("wifi_cfg");
```

---

## Storage Optimization

### Flash Wear Leveling

NVS automatically handles wear leveling, but you can optimize:

**Minimize writes**:
- Only write when value actually changes
- Batch related writes together
- Avoid high-frequency writes (>1/min)

```c
// Good: Check if value changed before writing
static void update_accent_color(uint32_t new_color)
{
    static uint32_t last_saved_color = 0xFFFFFFFF;  // Invalid initial value

    if (new_color != last_saved_color) {
        nvs_set_u32_safe("device_cfg", "accent_color", new_color);
        last_saved_color = new_color;
    }
}

// Bad: Writing on every update (unnecessary wear)
static void update_accent_color_bad(uint32_t new_color)
{
    nvs_set_u32_safe("device_cfg", "accent_color", new_color);  // Writes every time!
}
```

### Partition Size

**Default NVS partition**: 24KB (in `partitions.csv`)

```csv
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 0x200000,
```

**Recommendations**:
- 24KB (0x6000) sufficient for most devices (<50 keys)
- Increase to 48KB (0xC000) for complex devices (>100 keys)
- Never use >10% of flash for NVS

---

## Debugging & Troubleshooting

### NVS Statistics

```c
static void print_nvs_stats(void)
{
    nvs_stats_t nvs_stats;
    esp_err_t err = nvs_get_stats(NULL, &nvs_stats);

    if (err == ESP_OK) {
        printf("NVS Statistics:\n");
        printf("  Used entries:    %d / %d\n", nvs_stats.used_entries, nvs_stats.total_entries);
        printf("  Free entries:    %d\n", nvs_stats.free_entries);
        printf("  Namespace count: %d\n", nvs_stats.namespace_count);
        printf("  Usage:           %.1f%%\n",
               (float)nvs_stats.used_entries / nvs_stats.total_entries * 100.0f);
    } else {
        printf("Failed to get NVS stats: %s\n", esp_err_to_name(err));
    }
}
```

### List All Keys in Namespace

```c
static void list_nvs_keys(const char *namespace)
{
    nvs_iterator_t it = nvs_entry_find(NVS_DEFAULT_PART_NAME, namespace, NVS_TYPE_ANY);

    printf("NVS keys in namespace '%s':\n", namespace);

    while (it != NULL) {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);

        printf("  Key: %-15s | Type: ", info.key);

        switch (info.type) {
            case NVS_TYPE_U8:   printf("uint8\n"); break;
            case NVS_TYPE_I8:   printf("int8\n"); break;
            case NVS_TYPE_U16:  printf("uint16\n"); break;
            case NVS_TYPE_I16:  printf("int16\n"); break;
            case NVS_TYPE_U32:  printf("uint32\n"); break;
            case NVS_TYPE_I32:  printf("int32\n"); break;
            case NVS_TYPE_U64:  printf("uint64\n"); break;
            case NVS_TYPE_I64:  printf("int64\n"); break;
            case NVS_TYPE_STR:  printf("string\n"); break;
            case NVS_TYPE_BLOB: printf("blob\n"); break;
            default:            printf("unknown\n"); break;
        }

        it = nvs_entry_next(it);
    }

    nvs_release_iterator(it);
}
```

### Common Error Codes

| Error Code | Meaning | Solution |
|------------|---------|----------|
| `ESP_ERR_NVS_NOT_FOUND` | Key doesn't exist | Use default value |
| `ESP_ERR_NVS_NOT_ENOUGH_SPACE` | NVS partition full | Factory reset or increase partition size |
| `ESP_ERR_NVS_INVALID_HANDLE` | NVS not opened | Call `nvs_open()` first |
| `ESP_ERR_NVS_READ_ONLY` | Trying to write in readonly mode | Open with `NVS_READWRITE` |
| `ESP_ERR_NVS_INVALID_LENGTH` | String/blob too large | Reduce data size or use multiple keys |

---

## Security Considerations

### Sensitive Data Storage

**WiFi passwords, MQTT credentials**:
- NVS is NOT encrypted by default
- Anyone with physical access to flash can read NVS
- For high-security applications, use **NVS Encryption** (ESP-IDF feature)

### NVS Encryption (Optional)

Enable flash encryption in `menuconfig`:
```
Security features → Enable flash encryption on boot
```

**Trade-offs**:
- ✅ NVS data encrypted at rest
- ✅ Protection against physical attacks
- ❌ Increased complexity
- ❌ Firmware updates require encryption keys
- ❌ Cannot use `esptool.py` for direct flash access

**Recommendation**: For camper ecosystem, unencrypted NVS is acceptable (low-risk environment)

---

## Example: Complete Device Configuration

### Configuration Structure

```c
// Device configuration structure
typedef struct {
    // WiFi settings
    char wifi_ssid[32];
    char wifi_pass[64];

    // MQTT settings
    char mqtt_broker[64];
    uint16_t mqtt_port;
    char mqtt_client_id[32];
    char mqtt_user[32];
    char mqtt_pass[64];

    // System settings
    int32_t timezone;
    char language[8];
    uint8_t dark_theme;
    uint32_t accent_color;

    // Sensor calibration
    float pitch_offset;
    float roll_offset;

    // Update intervals
    uint32_t sensor_update_ms;
    uint32_t mqtt_publish_ms;
} device_config_t;

static device_config_t g_config;
```

### Load Configuration

```c
static void load_configuration(void)
{
    nvs_handle_t nvs;
    esp_err_t err = nvs_open("device_cfg", NVS_READONLY, &nvs);

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to open NVS, using defaults");
        set_default_configuration();
        return;
    }

    // WiFi settings
    nvs_get_str_default(nvs, "wifi_ssid", g_config.wifi_ssid, sizeof(g_config.wifi_ssid), "");
    nvs_get_str_default(nvs, "wifi_pass", g_config.wifi_pass, sizeof(g_config.wifi_pass), "");

    // MQTT settings
    nvs_get_str_default(nvs, "mqtt_broker", g_config.mqtt_broker, sizeof(g_config.mqtt_broker), "mqtt.syquens.com");
    g_config.mqtt_port = nvs_get_u16_default(nvs, "mqtt_port", 8883);
    nvs_get_str_default(nvs, "mqtt_client_id", g_config.mqtt_client_id, sizeof(g_config.mqtt_client_id), "");
    nvs_get_str_default(nvs, "mqtt_user", g_config.mqtt_user, sizeof(g_config.mqtt_user), "camper_device");
    nvs_get_str_default(nvs, "mqtt_pass", g_config.mqtt_pass, sizeof(g_config.mqtt_pass), "");

    // System settings
    g_config.timezone = nvs_get_i32_default(nvs, "timezone", 0);
    nvs_get_str_default(nvs, "language", g_config.language, sizeof(g_config.language), "en");
    g_config.dark_theme = nvs_get_u8_default(nvs, "dark_theme", 0);
    g_config.accent_color = nvs_get_u32_default(nvs, "accent_color", 0x0000FF);  // Blue

    // Sensor calibration (stored as millidegrees)
    int32_t pitch_off_millideg = nvs_get_i32_default(nvs, "pitch_off", 0);
    int32_t roll_off_millideg = nvs_get_i32_default(nvs, "roll_off", 0);
    g_config.pitch_offset = pitch_off_millideg / 1000.0f;
    g_config.roll_offset = roll_off_millideg / 1000.0f;

    // Update intervals
    g_config.sensor_update_ms = nvs_get_u32_default(nvs, "sensor_update_ms", 100);
    g_config.mqtt_publish_ms = nvs_get_u32_default(nvs, "mqtt_publish_ms", 1000);

    nvs_close(nvs);

    ESP_LOGI(TAG, "Configuration loaded successfully");
}
```

### Save Configuration

```c
static esp_err_t save_configuration(void)
{
    ESP_LOGI(TAG, "Saving configuration to NVS");

    // WiFi settings
    nvs_set_str_safe("device_cfg", "wifi_ssid", g_config.wifi_ssid);
    nvs_set_str_safe("device_cfg", "wifi_pass", g_config.wifi_pass);

    // MQTT settings
    nvs_set_str_safe("device_cfg", "mqtt_broker", g_config.mqtt_broker);
    nvs_set_u16_safe("device_cfg", "mqtt_port", g_config.mqtt_port);
    nvs_set_str_safe("device_cfg", "mqtt_client_id", g_config.mqtt_client_id);
    nvs_set_str_safe("device_cfg", "mqtt_user", g_config.mqtt_user);
    nvs_set_str_safe("device_cfg", "mqtt_pass", g_config.mqtt_pass);

    // System settings
    nvs_set_i32_safe("device_cfg", "timezone", g_config.timezone);
    nvs_set_str_safe("device_cfg", "language", g_config.language);
    nvs_set_u8_safe("device_cfg", "dark_theme", g_config.dark_theme);
    nvs_set_u32_safe("device_cfg", "accent_color", g_config.accent_color);

    // Sensor calibration (convert to millidegrees for storage)
    int32_t pitch_off_millideg = (int32_t)(g_config.pitch_offset * 1000.0f);
    int32_t roll_off_millideg = (int32_t)(g_config.roll_offset * 1000.0f);
    nvs_set_i32_safe("device_cfg", "pitch_off", pitch_off_millideg);
    nvs_set_i32_safe("device_cfg", "roll_off", roll_off_millideg);

    // Update intervals
    nvs_set_u32_safe("device_cfg", "sensor_update_ms", g_config.sensor_update_ms);
    nvs_set_u32_safe("device_cfg", "mqtt_publish_ms", g_config.mqtt_publish_ms);

    ESP_LOGI(TAG, "Configuration saved successfully");
    return ESP_OK;
}
```

---

## Implementation Checklist

When adding NVS configuration to a new device:

- [ ] Initialize NVS with `nvs_flash_init()` in `app_main()`
- [ ] Define namespace constant (e.g., `"device_cfg"`)
- [ ] Document all NVS keys in code comments or separate file
- [ ] Use consistent key naming (lowercase, underscores)
- [ ] Implement safe read/write helper functions
- [ ] Provide default values for all settings
- [ ] Handle first-boot scenario gracefully
- [ ] Add factory reset functionality
- [ ] Test with corrupted NVS (erase and verify defaults work)
- [ ] Log NVS operations for debugging
- [ ] Add NVS statistics to system info menu
- [ ] Document NVS key usage in user manual
- [ ] Keep partition size appropriate (24KB default)

---

## Related Ecosystem Standards

See also:
- [serial_menu_ecosystem.md](serial_menu_ecosystem.md) - Serial configuration interface patterns
- [timestamp_ecosystem.md](timestamp_ecosystem.md) - Standardized timestamp format
- [mqtt_ecosystem.md](mqtt_ecosystem.md) - MQTT communication standard

---

**Ecosystem Owner**: Syquens B.V.
**Maintained by**: V.N. Verbon
**Version Control**: Git repository `Lindi/export/`
