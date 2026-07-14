#include <stdio.h>
#include "driver/i2c_master.h"
#include "ds1307.h"

#define DS1307_REG_START 0x00        /*!< Start register */

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