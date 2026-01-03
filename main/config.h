/**
 * Localizer Configuration Header
 * 
 * GPS/RTC/WiFi/NTP/MQTT Tracker
 * Syquens B.V. - 2026
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// DISPLAY CONFIGURATION
// ============================================================================
#define DISPLAY_WIDTH           72
#define DISPLAY_HEIGHT          40
#define DISPLAY_LINES           5
#define DISPLAY_CHARS_PER_LINE  12
#define DISPLAY_UPDATE_MS       100

// ============================================================================
// I2C CONFIGURATION
// ============================================================================
#define I2C_MASTER_SCL_IO       6
#define I2C_MASTER_SDA_IO       5
#define I2C_MASTER_FREQ_HZ      400000
#define I2C_MASTER_TIMEOUT_MS   1000

// ============================================================================
// OLED CONFIGURATION (SSD1306)
// ============================================================================
#define OLED_I2C_ADDR           0x3C
#define OLED_WIDTH              128
#define OLED_HEIGHT             64
#define OLED_VISIBLE_WIDTH      72
#define OLED_VISIBLE_HEIGHT     40
#define OLED_X_OFFSET           28
#define OLED_Y_OFFSET           24

// ============================================================================
// GPS CONFIGURATION (GY-GPS6MV2 / NEO-6M)
// ============================================================================
#define GPS_UART_NUM            UART_NUM_1
#define GPS_TX_PIN              2   // ESP32-C3 TX → GPS RX (GPIO2)
#define GPS_RX_PIN              3   // ESP32-C3 RX ← GPS TX (GPIO3)
#define GPS_BAUD_RATE           9600
#define GPS_BUFFER_SIZE         1024
#define GPS_FIX_TIMEOUT_MS      60000  // 60 seconds for initial fix

// ============================================================================
// RTC CONFIGURATION (DS3231)
// ============================================================================
#define RTC_I2C_ADDR            0x68
#define EEPROM_I2C_ADDR         0x57
#define RTC_SYNC_TOLERANCE_MS   1000  // 1 second tolerance for "SYNC" status

// ============================================================================
// WIFI CONFIGURATION
// ============================================================================
#define WIFI_CONNECT_TIMEOUT_MS 30000  // 30 seconds
#define WIFI_RETRY_MAX          5
#define WIFI_RETRY_DELAY_MS     5000

// WiFi credentials - externalized to wifi_credentials.h (excluded from git)
#include "wifi_credentials.h"
#define DEFAULT_WIFI_SSID       WIFI_SSID
#define DEFAULT_WIFI_PASS       WIFI_PASSWORD

// ============================================================================
// NTP CONFIGURATION
// ============================================================================
#define NTP_SERVER_PRIMARY      "pool.ntp.org"
#define NTP_SERVER_SECONDARY    "time.nist.gov"
#define NTP_SYNC_INTERVAL_MS    3600000  // 1 hour
#define NTP_SYNC_TIMEOUT_MS     10000    // 10 seconds

// ============================================================================
// MQTT CONFIGURATION
// ============================================================================
#define MQTT_BROKER_URI         "mqtts://mqtt.syquens.com:8883"
#define MQTT_CLIENT_ID_PREFIX   "localizer_"
#define MQTT_KEEPALIVE_SEC      60
#define MQTT_RECONNECT_MS       5000

// Default MQTT credentials (can be overridden via NVS)
#define DEFAULT_MQTT_BROKER     "mqtt.syquens.com"
#define DEFAULT_MQTT_PORT       8883
#define DEFAULT_MQTT_USER       "camper_device"
#define DEFAULT_MQTT_PASS       "your_mqtt_password"

// MQTT Topics
#define MQTT_TOPIC_BASE         "camper"
#define MQTT_TOPIC_GPS          "gps"
#define MQTT_TOPIC_STATUS       "status"
#define MQTT_TOPIC_LOCATION     "location"

// ============================================================================
// GEOLOCATION CONFIGURATION
// ============================================================================
#define GEOLOCATION_UPDATE_MS   5000   // Update every 5 seconds
#define GEOLOCATION_API_URL     "http://api.bigdatacloud.net/data/reverse-geocode-client"

// ============================================================================
// NVS CONFIGURATION KEYS
// ============================================================================
#define NVS_NAMESPACE           "device_cfg"
#define NVS_WIFI_SSID           "wifi_ssid"
#define NVS_WIFI_PASS           "wifi_pass"
#define NVS_MQTT_BROKER         "mqtt_broker"
#define NVS_MQTT_PORT           "mqtt_port"
#define NVS_MQTT_USER           "mqtt_user"
#define NVS_MQTT_PASS           "mqtt_pass"
#define NVS_MQTT_CLIENT_ID      "mqtt_client"
#define NVS_DEVICE_ID           "device_id"

// ============================================================================
// COLOR CODES (for future color OLED support)
// ============================================================================
#define COLOR_RED               0
#define COLOR_GREEN             1
#define COLOR_WHITE             1  // Monochrome displays treat all as white

#endif // CONFIG_H
