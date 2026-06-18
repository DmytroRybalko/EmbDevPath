#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <driver/gpio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

const static char *TAG = "EXAMPLE";

/*---------------------------------------------------------------
        ADC General Macros
---------------------------------------------------------------*/
//ADC1 Channels
#define EXAMPLE_ADC1_CHAN0          ADC_CHANNEL_2
#define EXAMPLE_ADC1_CHAN1          ADC_CHANNEL_3
#define EXAMPLE_ADC_ATTEN           ADC_ATTEN_DB_12
#define LED                         GPIO_NUM_15

static int   adc_raw;
int   avg_buf    = 0;
static int   count      = 0;
static int   filtered   = 0;
static float ema        = 0.0f;
int sensorValue[10];
static bool ema_flag = false;

uint16_t simple_moving_avg(int raw, int avg_sample)
{   
    int avg_buf = 0;
    for (int i = 0; i < avg_sample; i++) {
        sensorValue[i] = raw;
        avg_buf += sensorValue[i];
        printf("Item: %d, adc_raw: %d, raw %d, buffer: %d, result: %d\n",
                i, adc_raw, raw, sensorValue[i], avg_buf);
    }
    avg_buf /= 10;
    return avg_buf;
}

float exp_moving_avg(int raw, float alpha)
{
    if (!ema_flag) {
        ema = raw;
        ema_flag = true;
    }
    ema = alpha * (float)raw + (1.0f - alpha) * ema;
    return ema;
};

void app_main(void)
{
    // Set LED
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);
    gpio_set_level(LED, 0);
    //-------------ADC1 Init---------------//
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t config = {
        .atten = EXAMPLE_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_12,
    };
    
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, EXAMPLE_ADC1_CHAN0, &config));

    while (1) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, EXAMPLE_ADC1_CHAN0, &adc_raw));
        
        filtered = (int)exp_moving_avg(adc_raw, 0.2);
        
        //LED operation
        if (filtered > 3000) {
           printf("It's bright!\n");
            gpio_set_level(LED, 0);
        } else {
            printf("It's dark!\n");
            gpio_set_level(LED, 1);
        }
        
        ESP_LOGI(TAG, "Raw Data: %d EMA: %d",
                 adc_raw,
                 (int)exp_moving_avg(adc_raw, 0.2));
                
        // Teleplot data
        //printf(">adc_raw:%d\n", adc_raw);
        //printf(">SMA:%f\n", simple_moving_avg(adc_raw, 10));
        //printf(">EMA:%f\n", exp_moving_avg(adc_raw, 0.2));

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
