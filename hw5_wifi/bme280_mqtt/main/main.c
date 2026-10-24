#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "bme280.h"
#include "bme_280.h"
#include <ssd1306.h>
#include "font_latin_8x8.h"
#include "mqtt_app.h"

#define I2C_MASTER_SCL_IO           GPIO_NUM_4       /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           GPIO_NUM_5       /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              I2C_NUM_0        /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ          (100000)         /*!< I2C master clock frequency */
#define I2C_MASTER_TIMEOUT_MS       1000

#define BME280_ADDR                 0x76            /*!< Address of the bme280 sensor */

static const char *OLED = "SSD1306";
static const char *BME = "BME280";

// i2c master bus config
i2c_master_bus_config_t bus_config = {
    .i2c_port = I2C_MASTER_NUM,
    .sda_io_num = I2C_MASTER_SDA_IO,
    .scl_io_num = I2C_MASTER_SCL_IO,
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .glitch_ignore_cnt = 7,
    .flags.enable_internal_pullup = true,
    };

// BME280 sensor config
i2c_device_config_t bme280_config = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = BME280_ADDR,
    .scl_speed_hz = I2C_MASTER_FREQ_HZ,
};

/**
 * @brief Demo how ssd1306_display_text function show float numbers, converted to string via snprintf() functions
 * 
 * @param handle SSD1306 device handle.
 * @return printed to console.
 */
void ssd1306_display_bme280(ssd1306_handle_t dev_hdl, struct bme280_data *comp_data)
{
    // Define buffer to save converted to chars values
    char text[16 + 1];

    // Display Text
    ssd1306_clear_display(dev_hdl, false);
    ssd1306_set_contrast(dev_hdl, 0xff);
    ssd1306_display_text(dev_hdl, 0, "SSD1306 128x64", false);
    
    // Convert temperature to string
    snprintf(text, sizeof text, "T: %4.1f\xB0" "C", comp_data->temperature);
    ssd1306_display_text(dev_hdl, 2, text, false);
    
    // Convert pressure to string
    snprintf(text, sizeof text, "P: %4.1f MPa", comp_data->pressure / 1000.0f);
    ssd1306_display_text(dev_hdl, 4, text, false);
    
    // Convert humidity to string
    snprintf(text, sizeof text, "H: %4.1f %%", comp_data->humidity);
    ssd1306_display_text(dev_hdl, 6, text, false);
};

void app_main(void)
{
    // Structure for storing data from sensor
    struct bme280_data comp_data = {0};
    // Init bme280 struct
    struct bme280_dev s_bme280 = {0};

    // Define handles
    i2c_master_bus_handle_t bus_handle;
    i2c_master_dev_handle_t bme280_handle;
    
    // initialize i2c ssd1306 configuration
    ssd1306_config_t ssd1306_config = I2C_SSD1306_128x64_CONFIG_DEFAULT;
    ssd1306_handle_t ssd1306_handle;

    // Create master bus connection
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));
    
    // Create bme280 connection
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &bme280_config, &bme280_handle));
    
    // Init ssd1306 oled
    ssd1306_init(bus_handle, &ssd1306_config, &ssd1306_handle);
    if (ssd1306_handle == NULL) {
        ESP_LOGE(OLED, "ssd1306 handle init failed");
        assert(ssd1306_handle);
    }
    
    // Init bme280 sensor
    init_bme280(bme280_handle, &s_bme280);

    // Activate mqqt connection

    mqtt_app_main();
    
    // Main loop for reading and displaying temperature, pressure and humidity data
    while (true) {
        /* Read compensated data */
        int8_t rslt = bme280_get_sensor_data(BME280_ALL, &comp_data, &s_bme280);
        if (rslt == BME280_OK) {
            
            // Display temperature, pressure and humidity on ssd1306
            ssd1306_display_bme280(ssd1306_handle, &comp_data);
            
            // Log same data to terminal
            /*ESP_LOGI(BME, "T = %0.1f(deg C); P = %0.1f(MPa); H = %0.1f(%%)", \
                comp_data.temperature, \
                comp_data.pressure / 1000.0f, \
                comp_data.humidity \
            );*/
        } else {
            ESP_LOGE(BME, "read data failed: %d", rslt);
        };
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
    
    // TEST ZONE
    //ssd1306_test(ssd1306_handle);
    //ssd1306_test_display_data(ssd1306_handle);
}

