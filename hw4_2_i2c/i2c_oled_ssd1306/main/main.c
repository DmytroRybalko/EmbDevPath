#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c_master.h"

// Settings
static const uint16_t       ssd1306_addr          = 0x3C;   // TMP102/105 I2C address
static const uint32_t       ssd1306_scl_speed_hz  = 100000; // 100kHz (standard mode)

void check_address(i2c_master_bus_handle_t bus_handle)
{
    //i2c_master_bus_handle_t bus_handle;
    esp_err_t err;
    for (uint8_t i = 3; i < 0x78; i++){
        err = i2c_master_probe(bus_handle, i, 1000);
        //printf("I2C Scanner status: %d at address 0x%X \n", err, i);
        if (err == ESP_OK){
            printf("I2C Scanner found I2C device at: 0x%X \n", i);
        }
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
}


void app_main(void)
{
    //uint8_t reg;
    //uint8_t data[2];
    
    // Set I2C master bus configuration
    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = GPIO_NUM_20,
        .sda_io_num = GPIO_NUM_21,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

     // Initialize the I2C bus
    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &bus_handle));

    // Set I2C device configuration
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = ssd1306_addr,
        .scl_speed_hz    = ssd1306_scl_speed_hz
    };

    // Initialize the OLED SSD1306 I2C device on the bus
    i2c_master_dev_handle_t ssd1306_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &ssd1306_handle));

    while(1) {
        check_address(bus_handle);
    }
}
