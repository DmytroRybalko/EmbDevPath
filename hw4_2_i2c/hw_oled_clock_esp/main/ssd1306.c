#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "ssd1306.h"

#define SSD1306_REG_START           0x00   /*!< Start register */

#define FRAMEBUFFER_SIZE            1024   // 128 * 64 / 8
#define SSD1306_WIDTH               128
#define SSD1306_HEIGHT              64
#define SSD1306_PAGES               (SSD1306_HEIGHT / 8)

static uint8_t framebuffer[FRAMEBUFFER_SIZE];

static const char *TAG = "SSD1306";

/**
 * @brief Setup command buffer to display (control byte 0x00 + command bytes)
 */
esp_err_t ssd1306_setup(i2c_master_dev_handle_t dev_handle)
{
    uint8_t initCommands[] = {
        SSD1306_REG_START,
        0xAE,        // DISPLAY OFF
        0xD5, 0x80,  // SET DISPLAY CLOCK DIV RATIO
        0xA8, 0x3F,  // SET MULTIPLEX RATIO
        0xD3, 0x00,  // SET DISPLAY OFFSET
        0x40,        // SET START LINE
        0x8D, 0x14,  // ENABLE CHARGE PUMP
        0x20, 0x00,  // SET MEMORY ADDRESSING MODE (horizontal)
        0xA1,        // SET COM OUTPUT SCAN DIRECTION
        0xC8,        // SET COM PINS HARDWARE CONFIGURATION
        0xDA, 0x12,  // SET COM OUTPUT SCAN DIRECTION
        0x81, 0xCF,  // SET CONTRAST CONTROL
        0xD9, 0xF1,  // SET PRE-CHARGE PERIOD
        0xDB, 0x40,  // SET VCOMH DESELECT LEVEL
        0xA4,        // ENTIRE DISPLAY ON
        0xA6,        // NORMAL DISPLAY (not inverted)
        0xAF         // DISPLAY ON
    };

    return i2c_master_transmit(dev_handle, initCommands, sizeof(initCommands), -1);
}

/**
 * @brief Clear the in-memory framebuffer (does not touch the display yet)
 */
void clear_framebuffer(void)
{
    memset(framebuffer, 0x00, FRAMEBUFFER_SIZE);
}

/**
 * @brief Set (or clear) a single pixel in the framebuffer.
 *        SSD1306 vertical byte layout: each byte covers 8 vertical pixels
 *        within one "page" (page = y / 8, bit = y % 8).
 */
void set_pixel(int x, int y, bool color)
{
    if (x < 0 || x >= SSD1306_WIDTH || y < 0 || y >= SSD1306_HEIGHT) {
        return; // out of bounds, ignore silently
    }

    uint8_t page = y / 8;
    uint8_t bit  = y % 8;
    uint16_t index = page * SSD1306_WIDTH + x;

    if (color) {
        framebuffer[index] |= (1 << bit);
    } else {
        framebuffer[index] &= ~(1 << bit);
    }
}

/**
 * @brief Set the column/page address window to the full screen.
 *        Required before flushing in horizontal addressing mode
 *        (which you already enabled via 0x20, 0x00 in initCommands).
 */
esp_err_t ssd1306_set_addr_window(i2c_master_dev_handle_t dev_handle)
{
    esp_err_t ret;

    uint8_t col_cmd[] = { 0x21, 0x00, SSD1306_WIDTH - 1 };   // SET_COLUMN_ADDR: start=0, end=127
    ret = i2c_master_transmit(dev_handle, col_cmd, sizeof(col_cmd), -1);
    if (ret != ESP_OK) return ret;

    uint8_t page_cmd[] = { 0x22, 0x00, SSD1306_PAGES - 1 };  // SET_PAGE_ADDR: start=0, end=7
    ret = i2c_master_transmit(dev_handle, page_cmd, sizeof(page_cmd), -1);
    return ret;
}

/**
 * @brief Push the whole framebuffer to the display over I2C.
 *        Control byte 0x40 marks the following bytes as pixel data (Co=0, D/C#=1).
 */
esp_err_t ssd1306_flush(i2c_master_dev_handle_t dev_handle)
{
    esp_err_t ret = ssd1306_set_addr_window(dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set address window");
        return ret;
    }

    static uint8_t data_buf[FRAMEBUFFER_SIZE + 1];
    data_buf[0] = 0x40; // data control byte
    memcpy(&data_buf[1], framebuffer, FRAMEBUFFER_SIZE);

    return i2c_master_transmit(dev_handle, data_buf, sizeof(data_buf), -1);
}

/**
 * @brief Draw a single character glyph into the framebuffer at (x, y).
 *        Each byte in font_latin_8x8_tr[c] represents one vertical column
 *        of 8 pixels (bit0 = top pixel of that column, bit7 = bottom),
 *        matching the SSD1306 page-based memory layout.
 *
 * @param x, y   top-left corner of the glyph in pixel coordinates
 * @param c      character code (Unicode code point / ASCII, e.g. 'A', '0', ':')
 */
void draw_char(int x, int y, unsigned char c)
{
    // unsigned char range is 0-255, which always fits FONT_LATIN_8x8_ROWS_SIZE (256) —
    // no bounds check needed, GCC would flag it as dead code (-Werror=type-limits)

    for (int col = 0; col < FONT_LATIN_8x8_COLS_SIZE; col++) {
        uint8_t col_byte = font_latin_8x8_tr[c][col];

        for (int row = 0; row < FONT_CHAR_HEIGHT; row++) {
            bool pixel_on = (col_byte >> row) & 0x01;
            set_pixel(x + col, y + row, pixel_on);
        }
    }
}

/**
 * @brief Draw a null-terminated ASCII string starting at (x, y),
 *        advancing by FONT_CHAR_WIDTH pixels per character.
 */
void draw_string(int x, int y, const char *str)
{
    int cursor_x = x;

    while (*str != '\0') {
        draw_char(cursor_x, y, (unsigned char)*str);
        cursor_x += FONT_CHAR_WIDTH;
        str++;
    }
}

/**
 * @brief Render hours:minutes:seconds as text into the framebuffer.
 *        Does NOT call ssd1306_flush() — caller decides when to push to display.
 */
void draw_time(int x, int y, uint8_t hours, uint8_t minutes, uint8_t seconds)
{
    char time_str[9]; // "HH:MM:SS" + null
    hours   %= 24;
    minutes %= 60;
    seconds %= 60;

    snprintf(time_str, sizeof(time_str), "%02u:%02u:%02u", hours, minutes, seconds);

    draw_string(x, y, time_str);
}