#include <stdint.h>
#include "driver/i2c_master.h"

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

/**
 * @brief Write time packet to DS1307
 */
esp_err_t ds1307_set_time(i2c_master_dev_handle_t dev_handle, const ds1307_time_t *time);

/**
 * @brief Read time packet from DS1307
 */
esp_err_t ds1307_get_time(i2c_master_dev_handle_t dev_handle, ds1307_time_t *time);