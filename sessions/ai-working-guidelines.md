# AI Working Guidelines for Localizer Project

## Quick Reference for AI Assistants

This document provides efficient working patterns specific to the Localizer ESP32-C3 GPS tracker project.

---

## Pre-Flight Checks

Before making any changes:

1. **Review latest session log** in [sessions/](.) for recent context
2. **Check [config.h](../main/config.h)** for constant definitions before adding new ones
3. **Verify current [main.c](../main/main.c)** state (file may have been edited by formatter/user)
4. **Consider partition space** - only 1% free, avoid bloat

---

## Build and Flash Commands

### Standard Workflow
```powershell
# Build only
& "E:\Dev\Espressif\frameworks\esp-idf-v5.5\export.ps1"
cd e:\Dev\Localizer
idf.py build

# Build and flash
idf.py -p COM4 build flash

# Monitor serial output
idf.py -p COM4 monitor

# Combined build, flash, and monitor
idf.py -p COM4 build flash monitor
```

### Important Notes
- **Always** activate ESP-IDF environment first (export.ps1)
- Device is on **COM4**
- Flash speed is 460800 baud (automatic)
- Monitor speed likely 115200 baud (verify in code)

---

## Code Modification Rules

### Configuration Constants
✅ **DO**:
- Check [config.h](../main/config.h) for existing constants
- Add new constants to config.h (single source of truth)
- Use descriptive names with component prefix (e.g., `OLED_`, `GPS_`, `MQTT_`)

❌ **DON'T**:
- Define constants directly in main.c
- Create duplicate definitions
- Use ambiguous names (e.g., `WIDTH` instead of `DISPLAY_WIDTH`)

### Hardware-Specific Constraints
These values are **hardware-defined** and should NOT be changed:
- I2C pins: SCL=6, SDA=5
- GPS UART pins: TX=21, RX=20
- I2C addresses: OLED=0x3C, RTC=0x68
- OLED physical offsets: X=28, Y=24

### Display Coordinate System
- **Physical display**: 128x64 pixels (SSD1306)
- **Visible area**: 72x40 pixels
- **Offset**: (28, 24) from top-left
- **Always use**: `DISPLAY_WIDTH`/`DISPLAY_HEIGHT` for application code
- **OLED_WIDTH/OLED_HEIGHT**: Physical dimensions, for low-level driver only

---

## Common Pitfalls

### 1. Constant Names
**Problem**: Using wrong constant names causes undeclared identifier errors.

**Solution**: Reference table of correct names:
| Common Mistake | Correct Name |
|---|---|
| `WIFI_SSID` | `DEFAULT_WIFI_SSID` |
| `WIFI_PASSWORD` | `DEFAULT_WIFI_PASS` |
| `NTP_SERVER` | `NTP_SERVER_PRIMARY` |
| `MQTT_USERNAME` | `DEFAULT_MQTT_USER` |
| `MQTT_PASSWORD` | `DEFAULT_MQTT_PASS` |
| `I2C_SCL_PIN` | `I2C_MASTER_SCL_IO` |
| `I2C_SDA_PIN` | `I2C_MASTER_SDA_IO` |

### 2. CMakeLists.txt Source Registration
**Problem**: Forgot to add source files to `idf_component_register()`.

**Solution**: Always include SRCS parameter:
```cmake
idf_component_register(
    SRCS "main.c"  # Don't forget this!
    INCLUDE_DIRS "."
    REQUIRES nvs_flash esp_wifi esp_netif esp_http_client mqtt driver json
)
```

### 3. Display Offset Confusion
**Problem**: Drawing to wrong coordinates causes misaligned display.

**Solution**: 
- Use `DISPLAY_WIDTH`/`DISPLAY_HEIGHT` in application code (72x40)
- Apply `OLED_X_OFFSET`/`OLED_Y_OFFSET` only in low-level `oled_update()` function
- Never mix physical and logical coordinates

### 4. Partition Space Management
**Problem**: Binary size exceeds 1MB partition (currently at 99% full).

**Solution**:
- Check binary size after each significant addition
- Consider compiler optimization flags
- Recommend partition resize if feature adds >5KB

---

## ESP-IDF Patterns

### Component Dependencies
Current dependencies in CMakeLists.txt:
- `nvs_flash` - Non-volatile storage
- `esp_wifi` - WiFi driver
- `esp_netif` - Network interface
- `esp_http_client` - HTTP client (if needed)
- `mqtt` - MQTT client
- `driver` - Hardware drivers (I2C, UART, etc.)
- `json` - JSON parsing

### Initialization Order (from main.c)
1. NVS initialization
2. I2C initialization
3. OLED initialization
4. RTC initialization
5. GPS UART initialization
6. WiFi initialization
7. NTP client setup
8. MQTT client setup

**Critical**: Follow this order - some components depend on earlier initialization.

---

## Testing and Verification

### After Code Changes
1. **Build** - Verify compilation succeeds
2. **Check binary size** - Ensure <1MB (watch for partition warning)
3. **Flash** - Upload to device on COM4
4. **Monitor** - Check serial output for runtime behavior
5. **Hardware test** - Verify OLED, GPS, RTC, WiFi, MQTT functionality
6. **Document** - Add learnings to session log

### Serial Monitor Checklist
When monitoring, look for:
- [ ] "OLED initialized" message
- [ ] "RTC initialized" message  
- [ ] "GPS initialized" message
- [ ] WiFi connection status
- [ ] NTP time synchronization
- [ ] MQTT connection status
- [ ] GPS fix acquisition
- [ ] Coordinate updates on OLED

---

## File Structure Quick Reference

```
Localizer/
├── main/
│   ├── config.h          ← Single source of truth for constants
│   ├── main.c            ← All application logic (889 lines)
│   └── CMakeLists.txt    ← Component registration (must include SRCS)
├── documentation/
│   ├── architecture.md
│   ├── board_reference.md
│   └── hardware_tests.md
├── sessions/             ← Rolling knowledge base (you are here)
│   ├── README.md
│   ├── 2026-01-03_initial-build-and-display-fixes.md
│   └── ai-working-guidelines.md
└── build/                ← Generated files (don't edit)
```

---

## Efficient AI Workflows

### Multi-File Edits
When making multiple related changes:
- Use `multi_replace_string_in_file` for efficiency
- Batch independent reads in parallel
- Avoid sequential edits when parallelization is possible

### Session Log Updates
When completing significant work:
1. Create new session log with ISO date format: `YYYY-MM-DD_description.md`
2. Document lessons learned with context
3. Update this guidelines document if new patterns emerge
4. Link to modified files using relative paths

### Error Resolution Pattern
1. **Read error output** - Full compilation/runtime errors
2. **Locate in code** - Use grep/read_file to find exact location
3. **Check config.h** - Verify constant definitions
4. **Search for similar code** - Find patterns in existing implementation
5. **Make minimal change** - Fix specific issue, avoid scope creep
6. **Verify build** - Ensure no new errors introduced
7. **Document** - Add to session log if significant learning

---

## Communication with User

### When Asking Questions
- Be specific about what information is needed
- Provide context about why it's needed
- Offer reasonable default assumptions if available

### When Reporting Completion
- Confirm what was changed (file paths as markdown links)
- Report build/flash status
- Note any warnings or issues
- Suggest next steps if appropriate

### When Encountering Issues
- Explain the problem clearly
- Show relevant error messages
- Propose solution(s)
- Implement fix unless user input needed

---

## Session Log Maintenance

### When to Create New Session Log
- New day of work
- Major feature addition
- Significant refactoring
- Important debugging session

### What to Include
- **Summary**: Brief overview of session objectives
- **Lessons Learned**: Issues and solutions with code examples
- **Build Procedures**: Any new or updated workflows
- **Constraints**: New limitations discovered
- **Working Patterns**: Effective approaches for this project
- **Next Considerations**: Suggested follow-up work

### What NOT to Include
- Trivial changes (typo fixes, minor formatting)
- Redundant information already in previous logs
- Speculation without basis
- User-specific preferences (unless project-wide)

---

*Last Updated: 2026-01-03*  
*This document evolves with the project - update as new patterns emerge.*
