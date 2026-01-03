# Session: WiFi and MQTT Credentials Externalization
**Date:** 2026-01-03  
**Focus:** Proper credential management and MQTT TLS configuration

---

## Summary
Successfully externalized WiFi and MQTT credentials to separate header files excluded from git. Fixed MQTT TLS certificate verification using ESP-IDF's built-in certificate bundle. Resolved NVS override issues.

---

## Key Lessons Learned

### 1. **Credential Management Pattern**
✅ **CORRECT APPROACH:**
```c
// In wifi_credentials.h (excluded from git)
#define WIFI_SSID "your-network"
#define WIFI_PASSWORD "your-password"

// In mqtt_credentials.h (excluded from git)
#define MQTT_BROKER_URI "mqtts://broker.com:8883"
#define MQTT_USER "username"
#define MQTT_PASSWORD "password"

// In config.h
#include "wifi_credentials.h"
#include "mqtt_credentials.h"
#define DEFAULT_WIFI_SSID WIFI_SSID
#define DEFAULT_WIFI_PASS WIFI_PASSWORD
#define DEFAULT_MQTT_USER MQTT_USER
#define DEFAULT_MQTT_PASS MQTT_PASSWORD
```

❌ **WRONG APPROACHES:**
- Hardcoding credentials directly in config.h (gets committed to git)
- Relying solely on NVS (requires manual entry on every device)
- Using runtime-only configuration without build-time defaults

### 2. **MQTT TLS Certificate Verification**
✅ **CORRECT IMPLEMENTATION:**
```c
#include "esp_crt_bundle.h"  // MUST include this header

static void mqtt_init(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = config_mqtt_broker,
        .broker.verification.crt_bundle_attach = esp_crt_bundle_attach,  // Use built-in CA bundle
        .credentials.username = config_mqtt_user,
        .credentials.authentication.password = config_mqtt_pass,
    };
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
}
```

❌ **COMMON MISTAKES:**
- Setting `.broker.verification.use_global_ca_store = true` without attaching bundle
- Missing `#include "esp_crt_bundle.h"` → compile error
- Not enabling `CONFIG_MBEDTLS_CERTIFICATE_BUNDLE=y` in sdkconfig
- Trying to skip verification entirely (security risk)

**Error Symptoms:**
- "global_cacert is NULL" → forgot to attach bundle
- "No server verification option set" → missing verification config
- "esp_crt_bundle_attach undeclared" → missing header include

### 3. **NVS vs Build-Time Credentials**
✅ **PROPER SEPARATION:**
- **Build-time (header files):** Default credentials for initial connection
- **NVS (runtime):** Optional overrides for field deployment changes

❌ **CRITICAL BUG:**
```c
// This OVERWRITES header file credentials with old NVS values!
len = sizeof(config_mqtt_user);
nvs_get_str(nvs_handle, "mqtt_user", config_mqtt_user, &len);  // ❌ WRONG
```

**Solution:** Only load from NVS if you want runtime overrides. For build-time credentials, comment out NVS loading:
```c
// MQTT credentials now come from mqtt_credentials.h, not NVS
// len = sizeof(config_mqtt_user);
// nvs_get_str(nvs_handle, "mqtt_user", config_mqtt_user, &len);
```

### 4. **Git Exclusion**
Must update `.gitignore`:
```gitignore
# WiFi/MQTT Credentials (sensitive)
main/wifi_credentials.h
main/mqtt_credentials.h
```

Create example templates for repository:
- `wifi_credentials.h.example`
- `mqtt_credentials.h.example`

---

## Implementation Checklist

- [x] Create `main/wifi_credentials.h` with actual credentials
- [x] Create `main/wifi_credentials.h.example` with placeholders
- [x] Create `main/mqtt_credentials.h` with actual credentials
- [x] Create `main/mqtt_credentials.h.example` with placeholders
- [x] Add both credential files to `.gitignore`
- [x] Include credential headers in `config.h`
- [x] Include `esp_crt_bundle.h` in main.c
- [x] Set `.broker.verification.crt_bundle_attach = esp_crt_bundle_attach`
- [x] Remove/comment out NVS credential loading if using build-time credentials
- [x] Verify `CONFIG_MBEDTLS_CERTIFICATE_BUNDLE=y` in sdkconfig

---

## Common Errors and Solutions

| Error | Cause | Solution |
|-------|-------|----------|
| `global_cacert is NULL` | Using `use_global_ca_store` without attaching bundle | Use `crt_bundle_attach = esp_crt_bundle_attach` |
| `esp_crt_bundle_attach undeclared` | Missing header | Add `#include "esp_crt_bundle.h"` |
| Wrong credentials used | NVS overwriting header values | Comment out `nvs_get_str()` for mqtt_user/pass |
| Connection refused | Incorrect username/password | Check mqtt_credentials.h values |
| Certificate verification failed | CA bundle not enabled | Verify `CONFIG_MBEDTLS_CERTIFICATE_BUNDLE=y` |

---

## Files Modified
- `main/main.c` - Added `#include "esp_crt_bundle.h"`, configured MQTT TLS, disabled NVS credential loading
- `main/config.h` - Added `#include "wifi_credentials.h"` and `#include "mqtt_credentials.h"`
- `.gitignore` - Added credential file exclusions
- Created: `main/wifi_credentials.h`, `main/wifi_credentials.h.example`
- Created: `main/mqtt_credentials.h`, `main/mqtt_credentials.h.example`

---

## Related Sessions
- [2026-01-03 Initial Build and Display Fixes](2026-01-03_initial-build-and-display-fixes.md) - Partition size, GPS UART conflicts
- See: `export/nvs_config_ecosystem.md` - NVS configuration standards

---

## Best Practices for Future Projects

1. **Always externalize credentials** to separate header files from day one
2. **Include esp_crt_bundle.h** for any MQTT/HTTPS TLS connections
3. **Use certificate bundle verification** instead of disabling security
4. **Decide early:** Build-time credentials (header files) OR runtime credentials (NVS), don't mix
5. **Create .example files** for all excluded credential files
6. **Test credential loading** with debug logs initially, remove before production
7. **Never log passwords** to serial output, even during development
