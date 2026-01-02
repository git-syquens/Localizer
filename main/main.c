#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_mac.h"
#include "esp_clk_tree.h"
#include "soc/soc_caps.h"

static const char *TAG = "OLED_TEST";

// I2C Configuration
#define I2C_MASTER_SCL_IO           6      // GPIO6
#define I2C_MASTER_SDA_IO           5      // GPIO5
#define I2C_MASTER_FREQ_HZ          400000 // 400kHz
#define I2C_MASTER_TIMEOUT_MS       1000

// SSD1306 OLED Configuration
#define OLED_I2C_ADDR               0x3C
#define OLED_WIDTH                  128
#define OLED_HEIGHT                 64
#define OLED_VISIBLE_WIDTH          72
#define OLED_VISIBLE_HEIGHT         40
#define OLED_X_OFFSET               28     // Updated offset from board reference
#define OLED_Y_OFFSET               24     // Updated offset from board reference

// SSD1306 Commands
#define OLED_CMD_DISPLAY_OFF        0xAE
#define OLED_CMD_DISPLAY_ON         0xAF
#define OLED_CMD_SET_CONTRAST       0x81
#define OLED_CMD_SET_MUX_RATIO      0xA8
#define OLED_CMD_SET_DISPLAY_OFFSET 0xD3
#define OLED_CMD_SET_START_LINE     0x40
#define OLED_CMD_SET_SEGMENT_REMAP  0xA1
#define OLED_CMD_SET_COM_SCAN_DEC   0xC8
#define OLED_CMD_SET_COM_PINS       0xDA
#define OLED_CMD_SET_PRECHARGE      0xD9
#define OLED_CMD_SET_VCOMH          0xDB
#define OLED_CMD_CHARGE_PUMP        0x8D
#define OLED_CMD_DEACTIVATE_SCROLL  0x2E
#define OLED_CMD_MEMORY_MODE        0x20
#define OLED_CMD_COLUMN_ADDR        0x21
#define OLED_CMD_PAGE_ADDR          0x22

static i2c_master_bus_handle_t i2c_bus_handle;
static i2c_master_dev_handle_t oled_dev_handle;

// Simple 5x7 font (ASCII 32-127)
static const uint8_t font_5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // (space)
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
    {0x00, 0x01, 0x02, 0x04, 0x00}, // `
    {0x20, 0x54, 0x54, 0x54, 0x78}, // a
    {0x7F, 0x48, 0x44, 0x44, 0x38}, // b
    {0x38, 0x44, 0x44, 0x44, 0x20}, // c
    {0x38, 0x44, 0x44, 0x48, 0x7F}, // d
    {0x38, 0x54, 0x54, 0x54, 0x18}, // e
    {0x08, 0x7E, 0x09, 0x01, 0x02}, // f
    {0x0C, 0x52, 0x52, 0x52, 0x3E}, // g
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // h
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // i
    {0x20, 0x40, 0x44, 0x3D, 0x00}, // j
    {0x7F, 0x10, 0x28, 0x44, 0x00}, // k
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // l
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // m
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // n
    {0x38, 0x44, 0x44, 0x44, 0x38}, // o
    {0x7C, 0x14, 0x14, 0x14, 0x08}, // p
    {0x08, 0x14, 0x14, 0x18, 0x7C}, // q
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // r
    {0x48, 0x54, 0x54, 0x54, 0x20}, // s
    {0x04, 0x3F, 0x44, 0x40, 0x20}, // t
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // u
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // v
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // w
    {0x44, 0x28, 0x10, 0x28, 0x44}, // x
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // y
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // z
};

static uint8_t framebuffer[OLED_WIDTH * OLED_HEIGHT / 8] = {0};

esp_err_t oled_write_command(uint8_t cmd)
{
    uint8_t data[2] = {0x00, cmd}; // Control byte (0x00 = command)
    return i2c_master_transmit(oled_dev_handle, data, 2, I2C_MASTER_TIMEOUT_MS);
}

esp_err_t oled_write_data(uint8_t *data, size_t len)
{
    uint8_t buffer[len + 1];
    buffer[0] = 0x40; // Control byte (0x40 = data)
    memcpy(buffer + 1, data, len);
    return i2c_master_transmit(oled_dev_handle, buffer, len + 1, I2C_MASTER_TIMEOUT_MS);
}

esp_err_t oled_init(void)
{
    esp_err_t ret;
    
    // Initialization sequence
    ret = oled_write_command(OLED_CMD_DISPLAY_OFF);
    if (ret != ESP_OK) return ret;
    
    ret = oled_write_command(OLED_CMD_SET_MUX_RATIO);
    if (ret != ESP_OK) return ret;
    ret = oled_write_command(0x3F); // 64 lines
    if (ret != ESP_OK) return ret;
    
    ret = oled_write_command(OLED_CMD_SET_DISPLAY_OFFSET);
    if (ret != ESP_OK) return ret;
    ret = oled_write_command(0x00); // No offset
    if (ret != ESP_OK) return ret;
    
    ret = oled_write_command(OLED_CMD_SET_START_LINE | 0x00);
    if (ret != ESP_OK) return ret;
    
    ret = oled_write_command(OLED_CMD_SET_SEGMENT_REMAP); // Column 127 mapped to SEG0
    if (ret != ESP_OK) return ret;
    
    ret = oled_write_command(OLED_CMD_SET_COM_SCAN_DEC); // Scan from COM[N-1] to COM0
    if (ret != ESP_OK) return ret;
    
    ret = oled_write_command(OLED_CMD_SET_COM_PINS);
    if (ret != ESP_OK) return ret;
    ret = oled_write_command(0x12); // Alternative COM pin config
    if (ret != ESP_OK) return ret;
    
    ret = oled_write_command(OLED_CMD_SET_CONTRAST);
    if (ret != ESP_OK) return ret;
    ret = oled_write_command(0xFF); // Maximum brightness
    if (ret != ESP_OK) return ret;
    
    ret = oled_write_command(0xA4); // Display follows RAM content
    if (ret != ESP_OK) return ret;
    
    ret = oled_write_command(0xA6); // Normal display (not inverted)
    if (ret != ESP_OK) return ret;
    
    ret = oled_write_command(OLED_CMD_SET_PRECHARGE);
    if (ret != ESP_OK) return ret;
    ret = oled_write_command(0xF1); // Phase 1: 1 DCLK, Phase 2: 15 DCLK
    if (ret != ESP_OK) return ret;
    
    ret = oled_write_command(OLED_CMD_SET_VCOMH);
    if (ret != ESP_OK) return ret;
    ret = oled_write_command(0x40); // VCOMH deselect level
    if (ret != ESP_OK) return ret;
    
    ret = oled_write_command(OLED_CMD_CHARGE_PUMP);
    if (ret != ESP_OK) return ret;
    ret = oled_write_command(0x14); // Enable charge pump
    if (ret != ESP_OK) return ret;
    
    ret = oled_write_command(OLED_CMD_DEACTIVATE_SCROLL);
    if (ret != ESP_OK) return ret;
    
    ret = oled_write_command(OLED_CMD_MEMORY_MODE);
    if (ret != ESP_OK) return ret;
    ret = oled_write_command(0x00); // Horizontal addressing mode
    if (ret != ESP_OK) return ret;
    
    ret = oled_write_command(OLED_CMD_DISPLAY_ON);
    if (ret != ESP_OK) return ret;
    
    ESP_LOGI(TAG, "OLED initialized successfully");
    return ESP_OK;
}

void oled_clear(void)
{
    memset(framebuffer, 0, sizeof(framebuffer));
}

void oled_set_pixel(int x, int y, bool on)
{
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) return;
    
    int byte_idx = x + (y / 8) * OLED_WIDTH;
    int bit_idx = y % 8;
    
    if (on) {
        framebuffer[byte_idx] |= (1 << bit_idx);
    } else {
        framebuffer[byte_idx] &= ~(1 << bit_idx);
    }
}

void oled_draw_char(int x, int y, char c)
{
    if (c < 32 || c > 122) c = ' ';
    const uint8_t *glyph = font_5x7[c - 32];
    
    for (int col = 0; col < 5; col++) {
        for (int row = 0; row < 7; row++) {
            if (glyph[col] & (1 << row)) {
                oled_set_pixel(x + col, y + row, true);
            }
        }
    }
}

void oled_draw_string(int x, int y, const char *str)
{
    while (*str) {
        oled_draw_char(x, y, *str);
        x += 6; // 5 pixel width + 1 pixel spacing
        str++;
    }
}

void oled_draw_line(int x0, int y0, int x1, int y1, bool on)
{
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        oled_set_pixel(x0, y0, on);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void oled_draw_rect(int x, int y, int w, int h, bool filled, bool on)
{
    if (filled) {
        for (int i = 0; i < w; i++) {
            for (int j = 0; j < h; j++) {
                oled_set_pixel(x + i, y + j, on);
            }
        }
    } else {
        oled_draw_line(x, y, x + w - 1, y, on);
        oled_draw_line(x + w - 1, y, x + w - 1, y + h - 1, on);
        oled_draw_line(x + w - 1, y + h - 1, x, y + h - 1, on);
        oled_draw_line(x, y + h - 1, x, y, on);
    }
}

void oled_draw_circle(int cx, int cy, int radius, bool on)
{
    int x = radius;
    int y = 0;
    int err = 0;

    while (x >= y) {
        oled_set_pixel(cx + x, cy + y, on);
        oled_set_pixel(cx + y, cy + x, on);
        oled_set_pixel(cx - y, cy + x, on);
        oled_set_pixel(cx - x, cy + y, on);
        oled_set_pixel(cx - x, cy - y, on);
        oled_set_pixel(cx - y, cy - x, on);
        oled_set_pixel(cx + y, cy - x, on);
        oled_set_pixel(cx + x, cy - y, on);

        if (err <= 0) {
            y += 1;
            err += 2 * y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

void oled_invert(void)
{
    for (int i = 0; i < sizeof(framebuffer); i++) {
        framebuffer[i] = ~framebuffer[i];
    }
}

esp_err_t oled_update(void)
{
    esp_err_t ret;
    
    // Set column address (0 to 127)
    ret = oled_write_command(OLED_CMD_COLUMN_ADDR);
    if (ret != ESP_OK) return ret;
    ret = oled_write_command(0x00);
    if (ret != ESP_OK) return ret;
    ret = oled_write_command(0x7F); // 127
    if (ret != ESP_OK) return ret;
    
    // Set page address (0 to 7)
    ret = oled_write_command(OLED_CMD_PAGE_ADDR);
    if (ret != ESP_OK) return ret;
    ret = oled_write_command(0x00);
    if (ret != ESP_OK) return ret;
    ret = oled_write_command(0x07); // 7 pages (64/8)
    if (ret != ESP_OK) return ret;
    
    // Send framebuffer in chunks
    const size_t chunk_size = 128;
    for (size_t i = 0; i < sizeof(framebuffer); i += chunk_size) {
        size_t len = (i + chunk_size > sizeof(framebuffer)) ? (sizeof(framebuffer) - i) : chunk_size;
        ret = oled_write_data(&framebuffer[i], len);
        if (ret != ESP_OK) return ret;
    }
    
    return ESP_OK;
}

void app_main(void)
{
    ESP_LOGI(TAG, "OLED Display Test - Syquens B.V.");
    ESP_LOGI(TAG, "Initializing I2C bus on GPIO5 (SDA) and GPIO6 (SCL)...");
    
    // Configure I2C master bus
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus_handle));
    ESP_LOGI(TAG, "I2C bus initialized");
    
    // Add OLED device to bus
    i2c_device_config_t oled_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = OLED_I2C_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_handle, &oled_config, &oled_dev_handle));
    ESP_LOGI(TAG, "OLED device added to I2C bus at address 0x%02X", OLED_I2C_ADDR);
    
    // Initialize OLED
    vTaskDelay(pdMS_TO_TICKS(100)); // Wait for OLED to power up
    ESP_ERROR_CHECK(oled_init());
    
    // Clear display
    oled_clear();
    ESP_ERROR_CHECK(oled_update());
    ESP_LOGI(TAG, "Display cleared");
    
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // ==== DEMO 1: Text Capacity Test ====
    ESP_LOGI(TAG, "Demo 1: Maximum text capacity (72x40 pixels, 5x7 font)");
    oled_clear();
    
    // Show text capacity: 12 chars wide x 5 lines
    int x = OLED_X_OFFSET;
    int y = OLED_Y_OFFSET;
    
    oled_draw_string(x, y, "Line 1: 12ch");
    oled_draw_string(x, y + 8, "Line 2: Data");
    oled_draw_string(x, y + 16, "Line 3: More");
    oled_draw_string(x, y + 24, "Line 4: Text");
    oled_draw_string(x, y + 32, "Line 5: End!");
    
    ESP_ERROR_CHECK(oled_update());
    ESP_LOGI(TAG, "Showing 5 lines x 12 characters = 60 chars max");
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    // ==== DEMO 2: Graphics Test ====
    ESP_LOGI(TAG, "Demo 2: Graphics - lines, rectangles, circles");
    oled_clear();
    
    // Draw border around visible area
    oled_draw_rect(OLED_X_OFFSET, OLED_Y_OFFSET, OLED_VISIBLE_WIDTH, OLED_VISIBLE_HEIGHT, false, true);
    
    // Draw diagonal lines
    oled_draw_line(OLED_X_OFFSET + 5, OLED_Y_OFFSET + 5, OLED_X_OFFSET + 25, OLED_Y_OFFSET + 25, true);
    oled_draw_line(OLED_X_OFFSET + 25, OLED_Y_OFFSET + 5, OLED_X_OFFSET + 5, OLED_Y_OFFSET + 25, true);
    
    // Draw circles
    oled_draw_circle(OLED_X_OFFSET + 50, OLED_Y_OFFSET + 15, 8, true);
    oled_draw_circle(OLED_X_OFFSET + 50, OLED_Y_OFFSET + 15, 5, true);
    
    // Draw filled rectangles
    oled_draw_rect(OLED_X_OFFSET + 30, OLED_Y_OFFSET + 28, 10, 8, true, true);
    
    ESP_ERROR_CHECK(oled_update());
    ESP_LOGI(TAG, "Graphics displayed");
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    // ==== DEMO 3: Patterns and Inversion ====
    ESP_LOGI(TAG, "Demo 3: Patterns and screen inversion");
    oled_clear();
    
    // Checkerboard pattern
    for (int i = OLED_X_OFFSET; i < OLED_X_OFFSET + OLED_VISIBLE_WIDTH; i += 4) {
        for (int j = OLED_Y_OFFSET; j < OLED_Y_OFFSET + OLED_VISIBLE_HEIGHT; j += 4) {
            oled_draw_rect(i, j, 2, 2, true, true);
        }
    }
    ESP_ERROR_CHECK(oled_update());
    vTaskDelay(pdMS_TO_TICKS(1500));
    
    // Invert display
    oled_invert();
    ESP_ERROR_CHECK(oled_update());
    vTaskDelay(pdMS_TO_TICKS(1500));
    
    // ==== DEMO 4: Dual Scrolling Lines ====
    ESP_LOGI(TAG, "Demo 4: Dual scrolling text animation");
    
    const char *scroll_text1 = "Syquens B.V. - ESP32-C3 OLED Display     ";
    const char *scroll_text2 = "0.42 inch - 72x40 pixels - SSD1306     ";
    int scroll_len1 = strlen(scroll_text1);
    int scroll_len2 = strlen(scroll_text2);
    int scroll_width1 = scroll_len1 * 6;
    int scroll_width2 = scroll_len2 * 6;
    
    // Scroll for 10 seconds
    for (int offset = 0; offset < scroll_width1; offset += 2) {
        oled_clear();
        
        // Top scrolling line (left to right)
        int x1 = OLED_X_OFFSET + OLED_VISIBLE_WIDTH - offset;
        if (x1 < OLED_X_OFFSET) x1 += scroll_width1;
        oled_draw_string(x1, OLED_Y_OFFSET + 8, scroll_text1);
        
        // Bottom scrolling line (right to left)
        int x2 = OLED_X_OFFSET - (offset % scroll_width2);
        oled_draw_string(x2, OLED_Y_OFFSET + 24, scroll_text2);
        
        ESP_ERROR_CHECK(oled_update());
        vTaskDelay(pdMS_TO_TICKS(30));
    }
    
    // ==== DEMO 5: Final Screen - Company Logo Style ====
    ESP_LOGI(TAG, "Demo 5: Final display - Syquens B.V.");
    oled_clear();
    
    // Draw decorative border
    oled_draw_rect(OLED_X_OFFSET + 2, OLED_Y_OFFSET + 2, OLED_VISIBLE_WIDTH - 4, OLED_VISIBLE_HEIGHT - 4, false, true);
    oled_draw_rect(OLED_X_OFFSET + 4, OLED_Y_OFFSET + 4, OLED_VISIBLE_WIDTH - 8, OLED_VISIBLE_HEIGHT - 8, false, true);
    
    // Center text
    const char *company = "Syquens B.V.";
    int text_width = strlen(company) * 6;
    x = OLED_X_OFFSET + (OLED_VISIBLE_WIDTH - text_width) / 2;
    y = OLED_Y_OFFSET + (OLED_VISIBLE_HEIGHT - 7) / 2;
    
    oled_draw_string(x, y, company);
    
    ESP_ERROR_CHECK(oled_update());
    ESP_LOGI(TAG, "Demo complete! Display shows final screen.");
    ESP_LOGI(TAG, "Screen specs: 72x40 pixels, ~12 chars/line, 5 lines max");
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
