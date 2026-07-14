#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "ssd1306.h"
#include "ds1307.h"

#define I2C_MASTER_SCL_IO           GPIO_NUM_4       /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           GPIO_NUM_5       /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              I2C_NUM_0                   /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ          (100000)        /*!< I2C master clock frequency */
#define I2C_MASTER_TIMEOUT_MS       1000

#define SSD1306_ADDR                0x3C            /*!< Address of the display */
#define DS1307_ADDR                 0x68            /*!< Address of the clock */

static const char *OLED  = "OLED";
static const char *CLOCK = "CLOCK";

i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

// SSD1306 display config
i2c_device_config_t ssd1306_config = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = SSD1306_ADDR,
    .scl_speed_hz = I2C_MASTER_FREQ_HZ,
};

// DS1307 clock config
i2c_device_config_t ds1307_config = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = DS1307_ADDR,
    .scl_speed_hz = I2C_MASTER_FREQ_HZ,
};

/**
 * @brief Initialize clock DS1307
 */
static void ds1307_initialize(i2c_master_dev_handle_t dev_handle)
{
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
        ESP_LOGI(CLOCK, "Successfully seeded initial RTC system clock parameters!");
    }   
}

/**
 * @brief Test clock by printing setup time to console
 */
static void test_ds1307(i2c_master_dev_handle_t dev_handle)
{
    ds1307_time_t current_time;
    while (1) {
        if (ds1307_get_time(dev_handle, &current_time) == ESP_OK) {
            ESP_LOGI(CLOCK, "Date: 20%02d-%02d-%02d | Time: %02d:%02d:%02d (Day of Week: %d)",
                     current_time.year, current_time.month, current_time.day_of_month,
                     current_time.hour, current_time.min, current_time.sec,
                     current_time.day_of_week);
        } else {
            ESP_LOGE(CLOCK, "Failed reading data payloads over active I2C parameters.");
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* Setup and test OLED ssd1306 */
/**
 * @brief Initialize display
 */
static void init_ssd1306(i2c_master_dev_handle_t dev_handle)
{
    if (ssd1306_setup(dev_handle) == ESP_OK) {
        ESP_LOGI(OLED, "Successfully initial display!");
    }

    clear_framebuffer();
    if (ssd1306_flush(dev_handle) == ESP_OK) {
        ESP_LOGI(OLED, "Framebuffer flushed successfully");
    }
}

/**
 * @brief Test OLED display by sending static time
 */
static void test_ssd1306(i2c_master_dev_handle_t dev_handle)
{
    // Draw static time
    clear_framebuffer();
    draw_time(34, 30, 16, 05, 37);   

    if (ssd1306_flush(dev_handle) == ESP_OK) {
        ESP_LOGI(OLED, "Time rendered on display");
    }
}

/**
 * @brief Display current time on display
 */
static void time2display(i2c_master_dev_handle_t time_handle, i2c_master_dev_handle_t oled_handle)
{
    ds1307_time_t current_time;
    if (ds1307_get_time(time_handle, &current_time) == ESP_OK) {
        // Draw static time
        clear_framebuffer();
        draw_time(34, 30, current_time.hour, current_time.min, current_time.sec);   

        if (ssd1306_flush(oled_handle) == ESP_OK) {
            ESP_LOGI(OLED, "Time rendered on display");
        }

        ESP_LOGI(CLOCK, "Date: 20%02d-%02d-%02d | Time: %02d:%02d:%02d (Day of Week: %d)",
                    current_time.year, current_time.month, current_time.day_of_month,
                    current_time.hour, current_time.min, current_time.sec,
                    current_time.day_of_week);
    } else {
        ESP_LOGE(CLOCK, "Failed reading data payloads over active I2C parameters.");
    }
}

void app_main(void)
{
    i2c_master_bus_handle_t bus_handle;
    i2c_master_dev_handle_t ssd1306_handle;
    i2c_master_dev_handle_t ds1307_handle;

    // Create master
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));
    // Create OLED
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &ssd1306_config, &ssd1306_handle));
    // Create CLOCK
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &ds1307_config, &ds1307_handle));

    // OLED SSD1306 initialize and testing
    ESP_LOGI(OLED, "I2C initialized successfully"); 
    init_ssd1306(ssd1306_handle);
    //test_ssd1306(ssd1306_handle);
    
    // CLOCK DS1307 initialize and testing
    ESP_LOGI(CLOCK, "I2C initialized successfully");
    ds1307_initialize(ds1307_handle);
    //test_ds1307(ds1307_handle);

    while(1){
        time2display(ds1307_handle, ssd1306_handle);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
