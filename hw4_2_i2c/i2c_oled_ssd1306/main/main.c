#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#include "u8g2.h"
#include "u8g2_demo.h"

static const char *TAG = "U8G2";

#define I2C_MASTER_NUM    I2C_NUM_0              /*!< I2C master port number */
#define I2C_MASTER_SDA_IO GPIO_NUM_5            /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_SCL_IO GPIO_NUM_4            /*!< GPIO number used for I2C master clock */
#define I2C_FREQ_HZ       100000                 /*!< I2C master clock frequency */
#define I2C_TIMEOUT_MS    1000                   /*!< I2C master timeout */

/* Display Configuration (configurable via menuconfig) */
#define I2C_DISPLAY_ADDRESS  (0x3C)    /*!< Display I2C address */

static i2c_master_bus_handle_t i2c_bus_handle = NULL;
static i2c_master_dev_handle_t ssd1306_handle = NULL;


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

static uint8_t u8x8_byte_i2c_cb(u8x8_t *u8x8, uint8_t msg,
                                uint8_t arg_int, void *arg_ptr)
{
    static uint8_t buffer[132];  /*!< Control byte + payload */
    static uint8_t buf_idx;

    switch (msg) {
    case U8X8_MSG_BYTE_INIT:
        /* Device is initialized in app_main */
        break;

    case U8X8_MSG_BYTE_START_TRANSFER:
        buf_idx = 0;
        break;

    case U8X8_MSG_BYTE_SET_DC:
        break;

    case U8X8_MSG_BYTE_SEND: {
        uint8_t *data = (uint8_t *)arg_ptr;
        for (uint8_t i = 0; i < arg_int; i++) {
            if (buf_idx >= sizeof(buffer)) {
                ESP_LOGE(TAG, "u8g2 tx buffer overflow");
                return 0;
            }
            buffer[buf_idx++] = data[i];
        }
        break;
    }

    case U8X8_MSG_BYTE_END_TRANSFER:
        if (ssd1306_handle == NULL) {
            ESP_LOGE(TAG, "ssd1306 handle is NULL");
            return 0;
        }
        if (buf_idx < 2) {
            return 1;
        }

        esp_err_t ret = i2c_master_transmit(ssd1306_handle, buffer, buf_idx, I2C_TIMEOUT_MS);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "I2C tx failed: %s (len=%u ctrl=0x%02X)",
                     esp_err_to_name(ret), buf_idx, buffer[0]);
            return 0;
        }
        break;

    default:
        return 0;
    }
    return 1;
}

static uint8_t u8x8_gpio_delay_cb(u8x8_t *u8x8, uint8_t msg,
                                  uint8_t arg_int, void *arg_ptr)
{
    switch (msg) {
        case U8X8_MSG_GPIO_AND_DELAY_INIT:
            ESP_LOGI(TAG, "GPIO and delay initialization completed");
            break;

        case U8X8_MSG_DELAY_MILLI:
            vTaskDelay(pdMS_TO_TICKS(arg_int));
            return 1;

        case U8X8_MSG_DELAY_10MICRO:
            esp_rom_delay_us(arg_int * 10);
            return 1;

        case U8X8_MSG_DELAY_100NANO:
            __asm__ __volatile__("nop");
            return 1;

        case U8X8_MSG_DELAY_I2C:
            break;

        default:
            return 0;
    }
    return 1;
}

static void show_demo_cycle(u8g2_t* u8g2, int demo_cycle)
{
    u8g2_ClearBuffer(u8g2);
    u8g2_SetFont(u8g2, u8g2_font_ncenB08_tr);
    u8g2_DrawStr(u8g2, 25, 25, "Demo Cycle");
    u8g2_SetFont(u8g2, u8g2_font_ncenB14_tr);
    char cycle_str[16];
    snprintf(cycle_str, sizeof(cycle_str), "%d", demo_cycle);
    u8g2_DrawStr(u8g2, 55, 45, cycle_str);
    u8g2_SendBuffer(u8g2);
    vTaskDelay(pdMS_TO_TICKS(2000));    /* delay for showing static display */
}

void app_main(void)
{
    u8g2_t u8g2;
    
    // Set I2C master bus configuration
    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port   = I2C_MASTER_NUM,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    // Initialize the I2C bus
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &i2c_bus_handle));

    // Set I2C device configuration
    i2c_device_config_t ssd1306_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = I2C_DISPLAY_ADDRESS,
        .scl_speed_hz    = I2C_FREQ_HZ,
        .scl_wait_us = 0,  /* Use default value */
        .flags.disable_ack_check = false,
    };

    // Initialize the OLED SSD1306 I2C device on the bus
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_handle, &ssd1306_config, &ssd1306_handle));

    /*
     * Initialize U8G2 for your display type.
     * Change the u8g2_Setup_xxx function below to match your display controller,
     * e.g. SSD1306, SH1106, SSD1327, etc.
     * See: https://github.com/olikraus/u8g2/wiki/u8g2setupc
     */
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(
        &u8g2, U8G2_R0,
        u8x8_byte_i2c_cb,   /* I2C communication callback */
        u8x8_gpio_delay_cb  /* GPIO and delay callback */
    );

    /* Initialize display hardware */
    ESP_LOGI(TAG, "Initializing display...");
    u8g2_InitDisplay(&u8g2);
    ESP_LOGI(TAG, "Setting power mode...");
    u8g2_SetPowerSave(&u8g2, 0);  /* Wake up display */
    ESP_LOGI(TAG, "Display initialization completed");

    /* Main demo loop - runs continuously */
    int demo_cycle = 0;

    while(1) {
        ESP_LOGI(TAG, "Demo cycle: %d", ++demo_cycle);
        check_address(i2c_bus_handle);
        
        /* TEST FILLED WHITE BOX */
        u8g2_ClearBuffer(&u8g2);
        u8g2_DrawBox(&u8g2, 0, 0, 128, 64);
        u8g2_SendBuffer(&u8g2);
        vTaskDelay(pdMS_TO_TICKS(3000));

        /* Execute demo sequence */
        demo_text_display(&u8g2);
        demo_shapes(&u8g2);
        demo_pixels(&u8g2);
        demo_progress_bar(&u8g2);
        demo_animation(&u8g2);
        demo_bitmap(&u8g2);
        show_demo_cycle(&u8g2, demo_cycle);
    }
}
