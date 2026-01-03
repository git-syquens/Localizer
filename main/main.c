/**
 * Localizer - GPS/RTC/WiFi/NTP/MQTT Tracker
 * 
 * ESP32-C3 with 0.42" OLED Display
 * Syquens B.V. - 2026
 * 
 * Features:
 * - GPS fix detection and time synchronization
 * - RTC (DS3231) with GPS/NTP sync
 * - WiFi connection management
 * - NTP time service
 * - MQTT telemetry publishing
 * - Real-time location lookup
 * - 5-line OLED status display
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "esp_http_client.h"
#include "mqtt_client.h"
#include "nvs_flash.h"
#include "cJSON.h"
#include "config.h"

static const char *TAG = "LOCALIZER";

// Event groups
static EventGroupHandle_t s_event_group;
#define WIFI_CONNECTED_BIT      BIT0
#define GPS_FIX_BIT             BIT1
#define RTC_SYNCED_BIT          BIT2
#define NTP_SYNCED_BIT          BIT3

// Global handles
static i2c_master_bus_handle_t i2c_bus_handle = NULL;
static i2c_master_dev_handle_t oled_dev_handle = NULL;
static i2c_master_dev_handle_t rtc_dev_handle = NULL;
static esp_mqtt_client_handle_t mqtt_client = NULL;

// GPS data structure
typedef struct {
    bool fix_valid;
    float latitude;
    float longitude;
    int satellites;
    int hour;
    int minute;
    int second;
    int day;
    int month;
    int year;
    float speed_knots;
} gps_data_t;

static gps_data_t gps_data = {0};

// Location data
static char location_street[128] = "Initializing...";
static char location_city[64] = "";
static char location_country[16] = "";

// Display scroll positions
static int scroll_pos_line4 = 0;
static int scroll_pos_line5 = 0;

// Configuration stored in NVS
static char config_wifi_ssid[32] = DEFAULT_WIFI_SSID;
static char config_wifi_pass[64] = DEFAULT_WIFI_PASS;
static char config_mqtt_broker[128] = MQTT_BROKER_URI;
static char config_mqtt_user[64] = DEFAULT_MQTT_USER;
static char config_mqtt_pass[64] = DEFAULT_MQTT_PASS;
static bool gps_debug_enabled = false;
typedef enum {
    RTC_SYNC_GPS = 0,
    RTC_SYNC_NTP = 1
} rtc_sync_source_t;
static rtc_sync_source_t rtc_sync_source = RTC_SYNC_GPS;

// 5x7 font for OLED display
static const uint8_t font_5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // space (32)
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // )
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
    {0x08, 0x14, 0x22, 0x41, 0x00}, // <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // =
    {0x00, 0x41, 0x22, 0x14, 0x08}, // >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // ?
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // @
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // Z
    {0x00, 0x7F, 0x41, 0x41, 0x00}, // [
    {0x02, 0x04, 0x08, 0x10, 0x20}, // backslash
    {0x00, 0x41, 0x41, 0x7F, 0x00}, // ]
    {0x04, 0x02, 0x01, 0x02, 0x04}, // ^
    {0x40, 0x40, 0x40, 0x40, 0x40}, // _
};

// OLED frame buffer (72x40 pixels = 72*5 bytes)
static uint8_t oled_buffer[72 * 5] = {0};

// ============================================================================
// OLED Display Functions (SSD1306 - 72x40)
// ============================================================================

#define OLED_PAGES  5

static void oled_write_command(uint8_t cmd) {
    uint8_t data[2] = {0x00, cmd};
    i2c_master_transmit(oled_dev_handle, data, 2, 1000);
}

static void oled_init(void) {
    vTaskDelay(pdMS_TO_TICKS(100));
    
    oled_write_command(0xAE); // Display off
    oled_write_command(0xD5); // Set display clock
    oled_write_command(0x80);
    oled_write_command(0xA8); // Set multiplex
    oled_write_command(0x27); // 40 rows
    oled_write_command(0xD3); // Set display offset
    oled_write_command(0x00);
    oled_write_command(0x40); // Set start line
    oled_write_command(0x8D); // Charge pump
    oled_write_command(0x14);
    oled_write_command(0x20); // Memory mode
    oled_write_command(0x00); // Horizontal
    oled_write_command(0xA1); // Segment remap
    oled_write_command(0xC8); // COM scan direction
    oled_write_command(0xDA); // COM pins
    oled_write_command(0x12);
    oled_write_command(0x81); // Contrast
    oled_write_command(0xCF);
    oled_write_command(0xD9); // Precharge
    oled_write_command(0xF1);
    oled_write_command(0xDB); // VCOM detect
    oled_write_command(0x40);
    oled_write_command(0xA4); // Resume display
    oled_write_command(0xA6); // Normal display
    oled_write_command(0xAF); // Display on
    
    ESP_LOGI(TAG, "OLED initialized");
}

static void oled_clear(void) {
    memset(oled_buffer, 0, sizeof(oled_buffer));
}

static void oled_set_pixel(int x, int y, bool on) {
    if (x >= 0 && x < DISPLAY_WIDTH && y >= 0 && y < DISPLAY_HEIGHT) {
        int page = y / 8;
        int bit = y % 8;
        int index = page * DISPLAY_WIDTH + x;
        
        if (on) {
            oled_buffer[index] |= (1 << bit);
        } else {
            oled_buffer[index] &= ~(1 << bit);
        }
    }
}

static void oled_draw_char(int x, int y, char c) {
    if (c < 32 || c > 95) c = 32; // Map to space if out of range
    
    const uint8_t *glyph = font_5x7[c - 32];
    
    for (int col = 0; col < 5; col++) {
        for (int row = 0; row < 7; row++) {
            if (glyph[col] & (1 << row)) {
                oled_set_pixel(x + col, y + row, true);
            }
        }
    }
}

static void oled_draw_string(int x, int y, const char *str) {
    int pos = x;
    while (*str && pos < DISPLAY_WIDTH) {
        oled_draw_char(pos, y, *str);
        pos += 6; // 5 pixels + 1 spacing
        str++;
    }
}

static void oled_update(void) {
    // Set column and page address with X offset for 72x40 visible area on 128x64 display
    oled_write_command(0x21); // Column address
    oled_write_command(OLED_X_OFFSET);
    oled_write_command(OLED_X_OFFSET + DISPLAY_WIDTH - 1);
    oled_write_command(0x22); // Page address
    oled_write_command(0x00);
    oled_write_command(OLED_PAGES - 1);
    
    // Send data
    for (int page = 0; page < OLED_PAGES; page++) {
        uint8_t data[DISPLAY_WIDTH + 1];
        data[0] = 0x40; // Data mode
        memcpy(&data[1], &oled_buffer[page * DISPLAY_WIDTH], DISPLAY_WIDTH);
        i2c_master_transmit(oled_dev_handle, data, DISPLAY_WIDTH + 1, 1000);
    }
}

// ============================================================================
// RTC Functions (DS3231)
// ============================================================================

#define DS3231_ADDR 0x68
#define DS3231_REG_SEC    0x00
#define DS3231_REG_MIN    0x01
#define DS3231_REG_HOUR   0x02
#define DS3231_REG_DAY    0x04
#define DS3231_REG_MONTH  0x05
#define DS3231_REG_YEAR   0x06

static uint8_t bcd_to_dec(uint8_t val) {
    return (val / 16 * 10) + (val % 16);
}

static uint8_t dec_to_bcd(uint8_t val) {
    return (val / 10 * 16) + (val % 10);
}

static void rtc_write_reg(uint8_t reg, uint8_t val) {
    uint8_t data[2] = {reg, val};
    i2c_master_transmit(rtc_dev_handle, data, 2, 1000);
}

static uint8_t rtc_read_reg(uint8_t reg) {
    uint8_t data = reg;
    uint8_t val = 0;
    
    i2c_master_transmit(rtc_dev_handle, &data, 1, 1000);
    i2c_master_receive(rtc_dev_handle, &val, 1, 1000);
    
    return val;
}

static void rtc_set_time(int year, int month, int day, int hour, int min, int sec) {
    rtc_write_reg(DS3231_REG_SEC, dec_to_bcd(sec));
    rtc_write_reg(DS3231_REG_MIN, dec_to_bcd(min));
    rtc_write_reg(DS3231_REG_HOUR, dec_to_bcd(hour));
    rtc_write_reg(DS3231_REG_DAY, dec_to_bcd(day));
    rtc_write_reg(DS3231_REG_MONTH, dec_to_bcd(month));
    rtc_write_reg(DS3231_REG_YEAR, dec_to_bcd(year - 2000));
    
    ESP_LOGI(TAG, "RTC set to %04d-%02d-%02d %02d:%02d:%02d", 
             year, month, day, hour, min, sec);
}

static void rtc_get_time(int *year, int *month, int *day, int *hour, int *min, int *sec) {
    *sec = bcd_to_dec(rtc_read_reg(DS3231_REG_SEC));
    *min = bcd_to_dec(rtc_read_reg(DS3231_REG_MIN));
    *hour = bcd_to_dec(rtc_read_reg(DS3231_REG_HOUR) & 0x3F);
    *day = bcd_to_dec(rtc_read_reg(DS3231_REG_DAY));
    *month = bcd_to_dec(rtc_read_reg(DS3231_REG_MONTH) & 0x1F);
    *year = bcd_to_dec(rtc_read_reg(DS3231_REG_YEAR)) + 2000;
}

// ============================================================================
// GPS NMEA Parsing
// ============================================================================

static float nmea_to_decimal(const char *coord, char dir) {
    if (!coord || strlen(coord) == 0) return 0.0;
    
    // Parse DDMM.MMMM format
    float value = atof(coord);
    int degrees = (int)(value / 100);
    float minutes = value - (degrees * 100);
    float decimal = degrees + (minutes / 60.0);
    
    if (dir == 'S' || dir == 'W') {
        decimal = -decimal;
    }
    
    return decimal;
}

static void parse_gprmc(const char *sentence) {
    // $GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A
    char *tokens[15];
    char buffer[256];
    strncpy(buffer, sentence, sizeof(buffer) - 1);
    
    int count = 0;
    char *ptr = strtok(buffer, ",");
    while (ptr && count < 15) {
        tokens[count++] = ptr;
        ptr = strtok(NULL, ",");
    }
    
    if (count < 10) return;
    
    // Check validity
    if (tokens[2][0] == 'A') {
        gps_data.fix_valid = true;
        xEventGroupSetBits(s_event_group, GPS_FIX_BIT);
        
        // Parse time (hhmmss)
        if (strlen(tokens[1]) >= 6) {
            char tmp[3] = {0};
            strncpy(tmp, tokens[1], 2); gps_data.hour = atoi(tmp);
            strncpy(tmp, tokens[1] + 2, 2); gps_data.minute = atoi(tmp);
            strncpy(tmp, tokens[1] + 4, 2); gps_data.second = atoi(tmp);
        }
        
        // Parse date (ddmmyy)
        if (strlen(tokens[9]) >= 6) {
            char tmp[3] = {0};
            strncpy(tmp, tokens[9], 2); gps_data.day = atoi(tmp);
            strncpy(tmp, tokens[9] + 2, 2); gps_data.month = atoi(tmp);
            strncpy(tmp, tokens[9] + 4, 2); gps_data.year = 2000 + atoi(tmp);
        }
        
        // Parse position
        gps_data.latitude = nmea_to_decimal(tokens[3], tokens[4][0]);
        gps_data.longitude = nmea_to_decimal(tokens[5], tokens[6][0]);
        gps_data.speed_knots = atof(tokens[7]);
    } else {
        gps_data.fix_valid = false;
        xEventGroupClearBits(s_event_group, GPS_FIX_BIT);
    }
}

static void parse_gpgga(const char *sentence) {
    // $GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
    char *tokens[15];
    char buffer[256];
    strncpy(buffer, sentence, sizeof(buffer) - 1);
    
    int count = 0;
    char *ptr = strtok(buffer, ",");
    while (ptr && count < 15) {
        tokens[count++] = ptr;
        ptr = strtok(NULL, ",");
    }
    
    if (count < 8) return;
    
    // Get satellite count
    gps_data.satellites = atoi(tokens[7]);
}

static void parse_nmea_sentence(const char *sentence) {
    if (strncmp(sentence, "$GPRMC", 6) == 0) {
        parse_gprmc(sentence);
    } else if (strncmp(sentence, "$GPGGA", 6) == 0) {
        parse_gpgga(sentence);
    }
}

// ============================================================================
// WiFi Event Handler
// ============================================================================

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "WiFi connecting...");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(s_event_group, WIFI_CONNECTED_BIT);
        esp_wifi_connect();
        ESP_LOGI(TAG, "WiFi reconnecting...");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "WiFi connected, IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));
    
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    
    // Copy WiFi credentials from config
    strncpy((char*)wifi_config.sta.ssid, config_wifi_ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, config_wifi_pass, sizeof(wifi_config.sta.password) - 1);
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "WiFi initialized");
}

// ============================================================================
// NTP Time Sync Callback
// ============================================================================

static void time_sync_notification_cb(struct timeval *tv) {
    ESP_LOGI(TAG, "NTP time synchronized");
    xEventGroupSetBits(s_event_group, NTP_SYNCED_BIT);
    
    // Update RTC from NTP if NTP sync is selected
    if (rtc_sync_source == RTC_SYNC_NTP) {
        time_t now = tv->tv_sec;
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);
        
        rtc_set_time(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                     timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        xEventGroupSetBits(s_event_group, RTC_SYNCED_BIT);
        ESP_LOGI(TAG, "RTC synced from NTP");
    }
}

static void ntp_init(void) {
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, NTP_SERVER_PRIMARY);
    esp_sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    esp_sntp_init();
    
    ESP_LOGI(TAG, "NTP initialized, server: %s", NTP_SERVER_PRIMARY);
}

// ============================================================================
// MQTT Event Handler
// ============================================================================

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, 
                               int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT connected");
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT disconnected");
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT error");
        break;
    default:
        break;
    }
}

// ============================================================================
// Serial Menu Functions
// ============================================================================

static void serial_print_menu(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════╗\n");
    printf("║        Localizer Configuration Menu            ║\n");
    printf("║                                                ║\n");
    printf("║  1. MQTT Broker Configuration                  ║\n");
    printf("║  2. RTC Sync Source                            ║\n");
    printf("║  3. GPS Debug Output                           ║\n");
    printf("║  4. View Current Settings                      ║\n");
    printf("║  5. Save Settings to NVS                       ║\n");
    printf("║  6. Reboot Device                              ║\n");
    printf("║                                                ║\n");
    printf("║  Q. Quit Menu                                  ║\n");
    printf("╚════════════════════════════════════════════════╝\n");
    printf("Enter choice: ");
}

static void serial_configure_mqtt(void) {
    char input[128];
    
    printf("\n=== MQTT Configuration ===\n");
    printf("Current broker: %s\n", config_mqtt_broker);
    printf("Enter new broker URI (or press Enter to keep): ");
    fflush(stdout);
    
    if (fgets(input, sizeof(input), stdin) != NULL) {
        // Remove newline
        input[strcspn(input, "\n")] = 0;
        if (strlen(input) > 0) {
            strncpy(config_mqtt_broker, input, sizeof(config_mqtt_broker) - 1);
            printf("Broker updated to: %s\n", config_mqtt_broker);
        }
    }
    
    printf("Current username: %s\n", config_mqtt_user);
    printf("Enter new username (or press Enter to keep): ");
    fflush(stdout);
    
    if (fgets(input, sizeof(input), stdin) != NULL) {
        input[strcspn(input, "\n")] = 0;
        if (strlen(input) > 0) {
            strncpy(config_mqtt_user, input, sizeof(config_mqtt_user) - 1);
            printf("Username updated to: %s\n", config_mqtt_user);
        }
    }
    
    printf("Current password: %s\n", config_mqtt_pass);
    printf("Enter new password (or press Enter to keep): ");
    fflush(stdout);
    
    if (fgets(input, sizeof(input), stdin) != NULL) {
        input[strcspn(input, "\n")] = 0;
        if (strlen(input) > 0) {
            strncpy(config_mqtt_pass, input, sizeof(config_mqtt_pass) - 1);
            printf("Password updated\n");
        }
    }
    
    printf("\nMQTT configuration updated (remember to save with option 5)\n");
}

static void serial_configure_rtc_sync(void) {
    char input[10];
    
    printf("\n=== RTC Sync Source ===\n");
    printf("Current source: %s\n", rtc_sync_source == RTC_SYNC_GPS ? "GPS" : "WiFi/NTP");
    printf("1. GPS (default)\n");
    printf("2. WiFi/NTP\n");
    printf("Enter choice (1 or 2): ");
    fflush(stdout);
    
    if (fgets(input, sizeof(input), stdin) != NULL) {
        if (input[0] == '1') {
            rtc_sync_source = RTC_SYNC_GPS;
            printf("RTC sync source set to GPS\n");
        } else if (input[0] == '2') {
            rtc_sync_source = RTC_SYNC_NTP;
            printf("RTC sync source set to WiFi/NTP\n");
        } else {
            printf("Invalid choice\n");
        }
    }
}

static void serial_configure_gps_debug(void) {
    char input[10];
    
    printf("\n=== GPS Debug Output ===\n");
    printf("Current state: %s\n", gps_debug_enabled ? "ENABLED" : "DISABLED");
    printf("1. Enable\n");
    printf("2. Disable\n");
    printf("Enter choice (1 or 2): ");
    fflush(stdout);
    
    if (fgets(input, sizeof(input), stdin) != NULL) {
        if (input[0] == '1') {
            gps_debug_enabled = true;
            printf("GPS debug output ENABLED\n");
        } else if (input[0] == '2') {
            gps_debug_enabled = false;
            printf("GPS debug output DISABLED\n");
        } else {
            printf("Invalid choice\n");
        }
    }
}

static void serial_view_settings(void) {
    printf("\n=== Current Settings ===\n");
    printf("MQTT Broker:    %s\n", config_mqtt_broker);
    printf("MQTT Username:  %s\n", config_mqtt_user);
    printf("MQTT Password:  %s\n", config_mqtt_pass);
    printf("RTC Sync:       %s\n", rtc_sync_source == RTC_SYNC_GPS ? "GPS" : "WiFi/NTP");
    printf("GPS Debug:      %s\n", gps_debug_enabled ? "ENABLED" : "DISABLED");
    printf("\nGPS Status:\n");
    printf("  Fix:          %s\n", gps_data.fix_valid ? "VALID" : "NO FIX");
    printf("  Satellites:   %d\n", gps_data.satellites);
    printf("  Latitude:     %.6f\n", gps_data.latitude);
    printf("  Longitude:    %.6f\n", gps_data.longitude);
    printf("  Time:         %02d:%02d:%02d\n", gps_data.hour, gps_data.minute, gps_data.second);
    printf("\nLocation:\n");
    printf("  Street:       %s\n", location_street);
    printf("  City:         %s\n", location_city);
    printf("  Country:      %s\n", location_country);
}

static void serial_save_settings(void) {
    printf("\nSaving settings to NVS...\n");
    
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("config", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        printf("Error opening NVS: %s\n", esp_err_to_name(err));
        return;
    }
    
    nvs_set_str(nvs_handle, "mqtt_broker", config_mqtt_broker);
    nvs_set_str(nvs_handle, "mqtt_user", config_mqtt_user);
    nvs_set_str(nvs_handle, "mqtt_pass", config_mqtt_pass);
    nvs_set_u8(nvs_handle, "rtc_sync_src", (uint8_t)rtc_sync_source);
    nvs_set_u8(nvs_handle, "gps_debug", (uint8_t)gps_debug_enabled);
    
    err = nvs_commit(nvs_handle);
    if (err == ESP_OK) {
        printf("Settings saved successfully!\n");
    } else {
        printf("Error saving settings: %s\n", esp_err_to_name(err));
    }
    
    nvs_close(nvs_handle);
}

static void serial_load_settings(void) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("config", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "No saved config, using defaults");
        return;
    }
    
    size_t len;
    
    // Load WiFi credentials
    len = sizeof(config_wifi_ssid);
    nvs_get_str(nvs_handle, "wifi_ssid", config_wifi_ssid, &len);
    
    len = sizeof(config_wifi_pass);
    nvs_get_str(nvs_handle, "wifi_pass", config_wifi_pass, &len);
    
    // Load MQTT settings
    len = sizeof(config_mqtt_broker);
    nvs_get_str(nvs_handle, "mqtt_broker", config_mqtt_broker, &len);
    
    // Validate MQTT broker - if it contains non-printable or GPS data, reset to default
    bool invalid = false;
    for (int i = 0; i < strlen(config_mqtt_broker); i++) {
        if (config_mqtt_broker[i] == '$' || config_mqtt_broker[i] == '*' || config_mqtt_broker[i] < 32) {
            invalid = true;
            break;
        }
    }
    if (invalid) {
        strcpy(config_mqtt_broker, MQTT_BROKER_URI);
        ESP_LOGI(TAG, "Corrupted MQTT broker detected, reset to default");
    }
    
    len = sizeof(config_mqtt_user);
    nvs_get_str(nvs_handle, "mqtt_user", config_mqtt_user, &len);
    
    len = sizeof(config_mqtt_pass);
    nvs_get_str(nvs_handle, "mqtt_pass", config_mqtt_pass, &len);
    
    uint8_t temp;
    if (nvs_get_u8(nvs_handle, "rtc_sync_src", &temp) == ESP_OK) {
        rtc_sync_source = (rtc_sync_source_t)temp;
    }
    
    if (nvs_get_u8(nvs_handle, "gps_debug", &temp) == ESP_OK) {
        gps_debug_enabled = (bool)temp;
    }
    
    nvs_close(nvs_handle);
    ESP_LOGI(TAG, "Settings loaded from NVS");
}

static void serial_menu_task(void *pvParameters) {
    bool in_menu = false;
    uint8_t data;
    
    printf("\n[Localizer GPS Tracker - Ready]\n");
    printf("[Press ` (backtick) for menu]\n\n");
    
    while (1) {
        // Read from UART0 (console) with timeout
        int len = uart_read_bytes(UART_NUM_0, &data, 1, pdMS_TO_TICKS(100));
        
        if (len <= 0) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }
        
        int c = (int)data;
        
        // Ignore invalid characters
        if (c == 0 || c == 0xFF) {
            continue;
        }
        
        // Check for backtick key to enter menu
        if (!in_menu && c == '`') {
            in_menu = true;
            serial_print_menu();
            continue;
        }
        
        // Process menu commands only when in menu
        if (in_menu) {
            switch (c) {
                case '1':
                    serial_configure_mqtt();
                    serial_print_menu();
                    break;
                case '2':
                    serial_configure_rtc_sync();
                    serial_print_menu();
                    break;
                case '3':
                    serial_configure_gps_debug();
                    serial_print_menu();
                    break;
                case '4':
                    serial_view_settings();
                    serial_print_menu();
                    break;
                case '5':
                    serial_save_settings();
                    serial_print_menu();
                    break;
                case '6':
                    printf("\nRebooting...\n");
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    esp_restart();
                    break;
                case 'q':
                case 'Q':
                case 'x':
                case 'X':
                    printf("\nExiting menu...\n\n");
                    in_menu = false;
                    break;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void mqtt_init(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = config_mqtt_broker,
        .credentials.username = config_mqtt_user,
        .credentials.authentication.password = config_mqtt_pass,
    };
    
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
    
    ESP_LOGI(TAG, "MQTT client started");
}

static void mqtt_publish_gps(void) {
    if (!mqtt_client) return;
    
    char topic[128];
    char payload[256];
    
    snprintf(topic, sizeof(topic), "camper/device01/gps");
    snprintf(payload, sizeof(payload), 
             "{\"lat\":%.6f,\"lon\":%.6f,\"sats\":%d,\"speed\":%.1f,\"fix\":%s}",
             gps_data.latitude, gps_data.longitude, gps_data.satellites,
             gps_data.speed_knots, gps_data.fix_valid ? "true" : "false");
    
    esp_mqtt_client_publish(mqtt_client, topic, payload, 0, 1, 0);
}

static void mqtt_publish_location(void) {
    if (!mqtt_client) return;
    
    char topic[128];
    char payload[256];
    
    snprintf(topic, sizeof(topic), "camper/device01/location");
    snprintf(payload, sizeof(payload), 
             "{\"street\": \"%s\",\"city\":\"%s\",\"country\":\"%s\"}",
             location_street, location_city, location_country);
    
    esp_mqtt_client_publish(mqtt_client, topic, payload, 0, 1, 0);
}

// ============================================================================
// HTTP Geolocation Lookup
// ============================================================================

static char http_response_buffer[4096];
static int http_response_len = 0;

static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    switch(evt->event_id) {
    case HTTP_EVENT_ON_DATA:
        if (http_response_len + evt->data_len < sizeof(http_response_buffer)) {
            memcpy(http_response_buffer + http_response_len, evt->data, evt->data_len);
            http_response_len += evt->data_len;
            http_response_buffer[http_response_len] = 0;
        }
        break;
    default:
        break;
    }
    return ESP_OK;
}

static void lookup_location(void) {
    if (!gps_data.fix_valid) return;
    
    char url[256];
    snprintf(url, sizeof(url), 
             "https://nominatim.openstreetmap.org/reverse?format=json&lat=%.6f&lon=%.6f",
             gps_data.latitude, gps_data.longitude);
    
    http_response_len = 0;
    memset(http_response_buffer, 0, sizeof(http_response_buffer));
    
    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .timeout_ms = 5000,
        .user_agent = "Localizer/1.0",
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);
    
    if (err == ESP_OK) {
        cJSON *root = cJSON_Parse(http_response_buffer);
        if (root) {
            cJSON *address = cJSON_GetObjectItem(root, "address");
            if (address) {
                cJSON *road = cJSON_GetObjectItem(address, "road");
                cJSON *city = cJSON_GetObjectItem(address, "city");
                cJSON *town = cJSON_GetObjectItem(address, "town");
                cJSON *village = cJSON_GetObjectItem(address, "village");
                cJSON *country_code = cJSON_GetObjectItem(address, "country_code");
                
                if (road && road->valuestring) {
                    strncpy(location_street, road->valuestring, sizeof(location_street) - 1);
                }
                
                if (city && city->valuestring) {
                    strncpy(location_city, city->valuestring, sizeof(location_city) - 1);
                } else if (town && town->valuestring) {
                    strncpy(location_city, town->valuestring, sizeof(location_city) - 1);
                } else if (village && village->valuestring) {
                    strncpy(location_city, village->valuestring, sizeof(location_city) - 1);
                }
                
                if (country_code && country_code->valuestring) {
                    strncpy(location_country, country_code->valuestring, sizeof(location_country) - 1);
                }
                
                ESP_LOGI(TAG, "Location: %s, %s, %s", 
                        location_street, location_city, location_country);
            }
            cJSON_Delete(root);
        }
    } else {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
    }
    
    esp_http_client_cleanup(client);
}

// ============================================================================
// GPS UART Task
// ============================================================================

static void gps_task(void *pvParameters) {
    uart_config_t uart_config = {
        .baud_rate = GPS_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    
    uart_driver_install(GPS_UART_NUM, GPS_BUFFER_SIZE, 0, 0, NULL, 0);
    uart_param_config(GPS_UART_NUM, &uart_config);
    uart_set_pin(GPS_UART_NUM, GPS_TX_PIN, GPS_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    
    ESP_LOGI(TAG, "GPS UART initialized on UART%d (TX:%d RX:%d)", GPS_UART_NUM, GPS_TX_PIN, GPS_RX_PIN);
    
    char line_buffer[256];
    int line_pos = 0;
    bool gps_time_synced = false;
    
    while (1) {
        uint8_t data;
        int len = uart_read_bytes(GPS_UART_NUM, &data, 1, 100 / portTICK_PERIOD_MS);
        
        if (len > 0) {
            if (data == '\n') {
                line_buffer[line_pos] = 0;
                if (line_pos > 0 && line_buffer[0] == '$') {
                    parse_nmea_sentence(line_buffer);
                    
                    // Debug output if enabled
                    if (gps_debug_enabled && gps_data.fix_valid) {
                        printf("[GPS] Lat: %.6f, Lon: %.6f, Sats: %d, Time: %02d:%02d:%02d, Speed: %.1f kts\n",
                               gps_data.latitude, gps_data.longitude, gps_data.satellites,
                               gps_data.hour, gps_data.minute, gps_data.second, gps_data.speed_knots);
                        printf("[LOC] Street: %s, City: %s, Country: %s\n",
                               location_street, location_city, location_country);
                    }
                    
                    // Sync RTC from GPS when first fix is acquired (if GPS sync is selected)
                    if (gps_data.fix_valid && !gps_time_synced && rtc_sync_source == RTC_SYNC_GPS) {
                        rtc_set_time(gps_data.year, gps_data.month, gps_data.day,
                                   gps_data.hour, gps_data.minute, gps_data.second);
                        xEventGroupSetBits(s_event_group, RTC_SYNCED_BIT);
                        gps_time_synced = true;
                        ESP_LOGI(TAG, "RTC synced from GPS");
                    }
                }
                line_pos = 0;
            } else if (line_pos < sizeof(line_buffer) - 1) {
                line_buffer[line_pos++] = data;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ============================================================================
// Location Lookup Task
// ============================================================================

static void location_task(void *pvParameters) {
    while (1) {
        EventBits_t bits = xEventGroupWaitBits(s_event_group, 
                                               WIFI_CONNECTED_BIT | GPS_FIX_BIT,
                                               pdFALSE, pdTRUE, portMAX_DELAY);
        
        if ((bits & (WIFI_CONNECTED_BIT | GPS_FIX_BIT)) == 
            (WIFI_CONNECTED_BIT | GPS_FIX_BIT)) {
            lookup_location();
            mqtt_publish_location();
        }
        
        vTaskDelay(pdMS_TO_TICKS(5000)); // Every 5 seconds
    }
}

// ============================================================================
// MQTT Publish Task
// ============================================================================

static void mqtt_publish_task(void *pvParameters) {
    while (1) {
        EventBits_t bits = xEventGroupWaitBits(s_event_group, 
                                               WIFI_CONNECTED_BIT,
                                               pdFALSE, pdFALSE, portMAX_DELAY);
        
        if (bits & WIFI_CONNECTED_BIT) {
            if (gps_data.fix_valid) {
                mqtt_publish_gps();
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(2000)); // Every 2 seconds
    }
}

// ============================================================================
// Display Update Task
// ============================================================================

static void display_task(void *pvParameters) {
    TickType_t last_wake_time = xTaskGetTickCount();
    
    while (1) {
        oled_clear();
        
        EventBits_t bits = xEventGroupGetBits(s_event_group);
        
        // Line 1: GPS status
        if (bits & GPS_FIX_BIT) {
            oled_draw_string(0, 0, "GPS FIX OK");
        } else {
            oled_draw_string(0, 0, "GPS: INIT");
        }
        
        // Line 2: RTC status
        if (bits & RTC_SYNCED_BIT) {
            oled_draw_string(0, 8, "RTC SYNC");
        } else {
            oled_draw_string(0, 8, "RTC LOCAL");
        }
        
        // Line 3: WiFi and NTP status (centered separator)
        if (bits & WIFI_CONNECTED_BIT) {
            oled_draw_string(0, 16, "WIFI");
        } else {
            oled_draw_string(0, 16, "----");
        }
        
        oled_draw_string(30, 16, "---");  // Centered separator
        
        if (bits & NTP_SYNCED_BIT) {
            oled_draw_string(48, 16, "NTP");
        } else {
            oled_draw_string(48, 16, "---");
        }
        
        // Line 4: Scrolling GPS data
        char line4[128];
        snprintf(line4, sizeof(line4), 
                "%.6f %.6f %02d:%02d:%02d SAT:%d  ",
                gps_data.latitude, gps_data.longitude,
                gps_data.hour, gps_data.minute, gps_data.second,
                gps_data.satellites);
        
        int line4_len = strlen(line4) * 6; // 6 pixels per char
        if (line4_len > DISPLAY_WIDTH) {
            int offset = scroll_pos_line4 % line4_len;
            // Draw scrolling text
            for (int i = 0; i < strlen(line4); i++) {
                int x = i * 6 - offset;
                if (x >= -6 && x < DISPLAY_WIDTH) {
                    oled_draw_char(x, 24, line4[i]);
                }
            }
            scroll_pos_line4 = (scroll_pos_line4 + 2) % line4_len;
        } else {
            oled_draw_string(0, 24, line4);
        }
        
        // Line 5: Scrolling location
        char line5[256];
        snprintf(line5, sizeof(line5), "%s %s %s  ",
                location_street, location_city, location_country);
        
        int line5_len = strlen(line5) * 6;
        if (line5_len > DISPLAY_WIDTH) {
            int offset = scroll_pos_line5 % line5_len;
            for (int i = 0; i < strlen(line5); i++) {
                int x = i * 6 - offset;
                if (x >= -6 && x < DISPLAY_WIDTH) {
                    oled_draw_char(x, 32, line5[i]);
                }
            }
            scroll_pos_line5 = (scroll_pos_line5 + 2) % line5_len;
        } else {
            oled_draw_string(0, 32, line5);
        }
        
        oled_update();
        
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(100)); // 10 Hz refresh
    }
}

// ============================================================================
// Main Application
// ============================================================================

void app_main(void) {
    ESP_LOGI(TAG, "Localizer starting...");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Load settings from NVS
    serial_load_settings();
    
    // Create event group
    s_event_group = xEventGroupCreate();
    
    // Initialize I2C bus
    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &i2c_bus_handle));
    ESP_LOGI(TAG, "I2C bus initialized");
    
    // Initialize OLED device
    i2c_device_config_t oled_dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = OLED_I2C_ADDR,
        .scl_speed_hz = 400000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_handle, &oled_dev_cfg, &oled_dev_handle));
    
    // Initialize RTC device
    i2c_device_config_t rtc_dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = DS3231_ADDR,
        .scl_speed_hz = 400000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_handle, &rtc_dev_cfg, &rtc_dev_handle));
    
    // Initialize OLED display
    oled_init();
    oled_clear();
    oled_draw_string(0, 0, "Localizer");
    oled_draw_string(0, 8, "Starting...");
    oled_update();
    
    // Initialize WiFi
    wifi_init();
    
    // Wait for WiFi connection
    xEventGroupWaitBits(s_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, 
                       pdMS_TO_TICKS(10000));
    
    // Initialize NTP
    ntp_init();
    
    // Initialize MQTT
    mqtt_init();
    
    // Create tasks
    xTaskCreate(gps_task, "gps_task", 4096, NULL, 5, NULL);
    xTaskCreate(display_task, "display_task", 4096, NULL, 4, NULL);
    xTaskCreate(location_task, "location_task", 8192, NULL, 3, NULL);
    xTaskCreate(mqtt_publish_task, "mqtt_task", 4096, NULL, 3, NULL);
    // Serial menu permanently disabled - GPS shares UART0 with console on ESP32-C3
    // xTaskCreate(serial_menu_task, "serial_menu", 4096, NULL, 2, NULL);
    
    ESP_LOGI(TAG, "Localizer running");
}
