#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "driver/i2c_master.h"
#include "bme280.h"
#include "bme280_defs.h"

#define I2C_MASTER_SCL_IO           GPIO_NUM_4       /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           GPIO_NUM_5       /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              I2C_NUM_0                   /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ          (100000)        /*!< I2C master clock frequency */
#define I2C_MASTER_TIMEOUT_MS       1000

#define BME280_ADDR                 0x76            /*!< Address of the clock */

static const char *TAG = "BME280";

i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

i2c_device_config_t bme280_config = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = BME280_ADDR,
    .scl_speed_hz = I2C_MASTER_FREQ_HZ,
};

/**
 * @brief Read a sequence of bytes from a bme280 sensor registers
 */
static BME280_INTF_RET_TYPE bme280_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t length, void *intf_ptr)
{
    i2c_master_dev_handle_t dev = (i2c_master_dev_handle_t)intf_ptr;

    esp_err_t err = i2c_master_transmit_receive(dev, &reg_addr, 1, reg_data, length, I2C_MASTER_TIMEOUT_MS);
    
    return (err == ESP_OK) ? BME280_INTF_RET_SUCCESS : -1;
}

/**
 * @brief Write a byte to a bme280 sensor register
 */
static BME280_INTF_RET_TYPE bme280_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t length, void *intf_ptr)
{
    i2c_master_dev_handle_t dev = (i2c_master_dev_handle_t)intf_ptr;

    uint8_t buf[32];
    if (length + 1 > sizeof(buf)) {
        return -1;                      
    }
    buf[0] = reg_addr;
    memcpy(&buf[1], reg_data, length);

    esp_err_t err = i2c_master_transmit(dev, buf, length + 1, I2C_MASTER_TIMEOUT_MS);
    return (err == ESP_OK) ? BME280_INTF_RET_SUCCESS : -1;
}

/*!
 * Delay function 
 */
static void bme280_delay_us(uint32_t period, void *intf_ptr)
{
    esp_rom_delay_us(period);
}

// Init bme280 struct
static struct bme280_dev s_bme280;

// Structure for settings
struct bme280_settings settings;

// Structure for storing data from sensor
struct bme280_data comp_data;

void detect_sensor(i2c_master_bus_handle_t bus)
{
    const uint8_t addrs[] = {0x76, 0x77};

    for (int i = 0; i < 2; i++) {
        // i2c_master_probe є в ESP-IDF 5.2+
        if (i2c_master_probe(bus, addrs[i], 100) != ESP_OK) {
            continue;
        }

        i2c_device_config_t cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = addrs[i],
            .scl_speed_hz = 100000,
        };
        i2c_master_dev_handle_t dev;
        ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &cfg, &dev));

        uint8_t reg = 0xD0, id = 0;
        ESP_ERROR_CHECK(i2c_master_transmit_receive(dev, &reg, 1, &id, 1, 100));

        ESP_LOGI(TAG, "addr 0x%02X, chip ID 0x%02X -> %s", addrs[i], id,
                 id == 0x58 ? "BMP280" :
                 id == 0x60 ? "BME280" : "unknown");

        i2c_master_bus_rm_device(dev);
    }
}

void app_main(void)
{
    i2c_master_bus_handle_t bus_handle;
    i2c_master_dev_handle_t bme280_handle;

    // Create master
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));
    // Create bme280
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &bme280_config, &bme280_handle));
    
    // Fill bme280 init structure
    s_bme280.intf       = BME280_I2C_INTF;
    s_bme280.intf_ptr   = bme280_handle;
    s_bme280.read       = bme280_i2c_read;
    s_bme280.write      = bme280_i2c_write;
    s_bme280.delay_us   = bme280_delay_us;

    // Init bme280 sensor
    int8_t rslt = bme280_init(&s_bme280);
    if (rslt != BME280_OK) {
        ESP_LOGE(TAG, "init failed: %d", rslt);
    } else {
        ESP_LOGI(TAG, "BME280 sensor initialize successfully!");
    }

    /* Always read the current settings before writing, especially when all the configuration is not modified */
    rslt = bme280_get_sensor_settings(&settings, &s_bme280);
    if (rslt != BME280_OK) { ESP_LOGE(TAG, "set settings failed: %d", rslt); }

    /* Configuring the over-sampling rate, filter coefficient and standby time */
    /* Now it follow oficial recommendations for weather station mode */
    settings.filter = BME280_FILTER_COEFF_OFF; 

    /* Over-sampling rate for humidity, temperature and pressure */
    settings.osr_h = BME280_OVERSAMPLING_1X;
    settings.osr_p = BME280_OVERSAMPLING_1X;
    settings.osr_t = BME280_OVERSAMPLING_1X;

    /* Setting the standby time
    This establish time between measurements
    */
    settings.standby_time = BME280_STANDBY_TIME_1000_MS;

    rslt = bme280_set_sensor_settings(BME280_SEL_ALL_SETTINGS, &settings, &s_bme280);
    if (rslt != BME280_OK) { ESP_LOGE(TAG, "set settings failed: %d", rslt); }

    /* Always set the power mode after setting the configuration */
    rslt = bme280_set_sensor_mode(BME280_POWERMODE_NORMAL, &s_bme280);
    if (rslt != BME280_OK) { ESP_LOGE(TAG, "set sensor mode failed: %d", rslt); }

    /* Read compensated data */
    while (true)
    {
        rslt = bme280_get_sensor_data(BME280_ALL, &comp_data, &s_bme280);
        if (rslt == BME280_OK) {
            ESP_LOGI(TAG, "T = %0.1f(deg C); P = %0.1f(Pa); H = %0.1f(%%)", \
                comp_data.temperature, \
                comp_data.pressure, \
                comp_data.humidity \
            );
        } else {
            ESP_LOGE(TAG, "read data failed: %d", rslt);
        };
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
