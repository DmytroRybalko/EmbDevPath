#include <string.h>
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "driver/i2c_master.h"
#include "bme280.h"

/**
 * @brief Read a sequence of bytes from a bme280 sensor registers
 */
static BME280_INTF_RET_TYPE bme280_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t length, void *intf_ptr)
{
    i2c_master_dev_handle_t dev = (i2c_master_dev_handle_t)intf_ptr;

    esp_err_t err = i2c_master_transmit_receive(dev, &reg_addr, 1, reg_data, length, 1000);
    
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

    esp_err_t err = i2c_master_transmit(dev, buf, length + 1, 1000);
    return (err == ESP_OK) ? BME280_INTF_RET_SUCCESS : -1;
}

/*!
 * Delay function 
 */
static void bme280_delay_us(uint32_t period, void *intf_ptr)
{
    esp_rom_delay_us(period);
}

int8_t init_bme280(i2c_master_dev_handle_t dev_hdl, struct bme280_dev *dev_data)
{   
    // Structure for settings
    struct bme280_settings settings = {0};
    
    // Fill bme280 init structure
    dev_data->intf       = BME280_I2C_INTF;
    dev_data->intf_ptr   = dev_hdl;
    dev_data->read       = bme280_i2c_read;
    dev_data->write      = bme280_i2c_write;
    dev_data->delay_us   = bme280_delay_us;

    // Init bme280 sensor
    int8_t rslt = bme280_init(dev_data);
    if (rslt != BME280_OK) {
        ESP_LOGE("BME280", "init failed: %d", rslt);
    } else {
        ESP_LOGI("BME280", "BME280 sensor initialize successfully!");
    }

    /* Always read the current settings before writing, especially when all the configuration is not modified */
    rslt = bme280_get_sensor_settings(&settings, dev_data);
    if (rslt != BME280_OK) { ESP_LOGE("BME280", "get settings failed: %d", rslt); }

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

    rslt = bme280_set_sensor_settings(BME280_SEL_ALL_SETTINGS, &settings, dev_data);
    if (rslt != BME280_OK) { ESP_LOGE("BME280", "set settings failed: %d", rslt); }

    /* Always set the power mode after setting the configuration */
    rslt = bme280_set_sensor_mode(BME280_POWERMODE_NORMAL, dev_data);
    if (rslt != BME280_OK) { ESP_LOGE("BME280", "set sensor mode failed: %d", rslt); }

    // Return error if not success
    if (rslt != BME280_OK) {
        ESP_LOGE("BME280", "init failed: %d", rslt);
    }
    
    return rslt;
}

/**
 * @brief Check type of sensor: bme280 vs bmp280 
 */
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

        ESP_LOGI("BME280", "addr 0x%02X, chip ID 0x%02X -> %s", addrs[i], id,
                 id == 0x58 ? "BMP280" :
                 id == 0x60 ? "BME280" : "unknown");

        i2c_master_bus_rm_device(dev);
    }
}