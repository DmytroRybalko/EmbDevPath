/* i2c - Simple Example

   Simple I2C example that shows how to initialize I2C
   as well as reading and writing from and to registers for a sensor connected over I2C.

   The sensor used in this example is a DS1307 clock.
*/
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c_master.h"

static const char *TAG = "DS1307";

#define I2C_MASTER_SCL_IO           GPIO_NUM_4       /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           GPIO_NUM_5       /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              I2C_NUM_0                   /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ          (100000) /*!< I2C master clock frequency */
#define I2C_MASTER_TIMEOUT_MS       1000

#define DS1307_ADDR                 0x68        /*!< Address of the clock */
#define DS1307_REG_START            0x00        /*!< Start register */

// Structure matching the DS1307 memory map layout
typedef struct {
    uint8_t sec;
    uint8_t min;
    uint8_t hour;
    uint8_t day_of_week; // 1-7 (e.g. Sunday = 1)
    uint8_t day_of_month;// 1-31
    uint8_t month;       // 1-12
    uint8_t year;        // 0-99 (Offset from year 2000)
} ds1307_time_t;

// Utility functions for BCD conversion
static uint8_t bcd_to_dec(uint8_t val) {
    return ((val >> 4) * 10) + (val & 0x0F);
}

static uint8_t dec_to_bcd(uint8_t val) {
    return ((val / 10) << 4) + (val % 10);
}

/**
 * @brief Write time packet to DS1307
 */
esp_err_t ds1307_set_time(i2c_master_dev_handle_t dev_handle, const ds1307_time_t *time) {
    uint8_t tx_buf[8];
    tx_buf[0] = DS1307_REG_START;
    
    // Convert decimal entries to BCD format while forcing 24-hour mode
    tx_buf[1] = dec_to_bcd(time->sec) & 0x7F; // Bit 7 (CH) = 0 enables oscillator
    tx_buf[2] = dec_to_bcd(time->min);
    tx_buf[3] = dec_to_bcd(time->hour) & 0x3F; // Clear bit 6 to enforce 24-hour mode
    tx_buf[4] = dec_to_bcd(time->day_of_week);
    tx_buf[5] = dec_to_bcd(time->day_of_month);
    tx_buf[6] = dec_to_bcd(time->month);
    tx_buf[7] = dec_to_bcd(time->year);

    // Send payload: [Register address, data0, data1, ...]
    return i2c_master_transmit(dev_handle, tx_buf, sizeof(tx_buf), -1);
}

/**
 * @brief Read time packet from DS1307
 */
esp_err_t ds1307_get_time(i2c_master_dev_handle_t dev_handle, ds1307_time_t *time) {
    uint8_t reg_addr = DS1307_REG_START;
    uint8_t rx_buf[7];

    // Transmit target register address, then read 7 bytes back
    esp_err_t ret = i2c_master_transmit_receive(dev_handle, &reg_addr, 1, rx_buf, sizeof(rx_buf), -1);
    if (ret != ESP_OK) {
        return ret;
    }

    // Convert raw hardware BCD register values back to Decimal integers
    time->sec          = bcd_to_dec(rx_buf[0] & 0x7F); // Strip CH flag
    time->min          = bcd_to_dec(rx_buf[1]);
    time->hour         = bcd_to_dec(rx_buf[2] & 0x3F); // Strip 12/24 hour bit flag
    time->day_of_week  = bcd_to_dec(rx_buf[3] & 0x07);
    time->day_of_month = bcd_to_dec(rx_buf[4] & 0x3F);
    time->month        = bcd_to_dec(rx_buf[5] & 0x1F);
    time->year         = bcd_to_dec(rx_buf[6]);

    return ESP_OK;
}

static void i2c_master_init(i2c_master_bus_handle_t *bus_handle, i2c_master_dev_handle_t *dev_handle)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, bus_handle));

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = DS1307_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(*bus_handle, &dev_config, dev_handle));
}

void app_main(void)
{
    
    uint8_t data[7];
    i2c_master_bus_handle_t bus_handle;
    i2c_master_dev_handle_t dev_handle;
    
    i2c_master_init(&bus_handle, &dev_handle);
    ESP_LOGI(TAG, "I2C initialized successfully");

    // Manually setup current time
    ds1307_time_t initial_time = {
        .sec = 0,
        .min = 45,
        .hour = 12,
        .day_of_week = 6,   // Friday
        .day_of_month = 10,
        .month = 7,         // July
        .year = 26          // 2026
    };
    if (ds1307_set_time(dev_handle, &initial_time) == ESP_OK) {
        ESP_LOGI(TAG, "Successfully seeded initial RTC system clock parameters!");
    }

    ds1307_time_t current_time;
    while (1) {
        if (ds1307_get_time(dev_handle, &current_time) == ESP_OK) {
            ESP_LOGI(TAG, "Date: 20%02d-%02d-%02d | Time: %02d:%02d:%02d (Day of Week: %d)",
                     current_time.year, current_time.month, current_time.day_of_month,
                     current_time.hour, current_time.min, current_time.sec,
                     current_time.day_of_week);
        } else {
            ESP_LOGE(TAG, "Failed reading data payloads over active I2C parameters.");
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}