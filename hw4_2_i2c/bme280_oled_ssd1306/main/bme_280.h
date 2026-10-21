#ifndef BME_280_H
#define BME_280_H

#include "driver/i2c_master.h"
#include "bme280_defs.h"

/**
 * @brief Initialize and configure the BME280 sensor over I2C.
 *
 * This function fills the provided `bme280_dev` context with I2C callbacks,
 * initializes the device, applies weather-station style oversampling settings,
 * and sets normal power mode.
 *
 * @param[in] dev_hdl I2C device handle for the BME280 sensor.
 * @param[out] dev_data Pointer to the BME280 device context to initialize.
 * @return `0` on success, otherwise a BME280 driver error code.
 */
uint8_t init_bme280(i2c_master_dev_handle_t dev_hdl, struct bme280_dev *dev_data);

#endif