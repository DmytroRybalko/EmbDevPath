#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <driver/ledc.h>
#include "sdkconfig.h"

#define LED_PIN 15
#define LEDC_CHANNEL 0
#define LEDC_TIMER 0
#define LEDC_FREQUENCY 1000     // 1 kHz
#define LEDC_RESOLUTION 10      // 10-bit (0-1023)

void app_main(void)
{
    ledc_timer_config_t timer_config = {
        .speed_mode         = LEDC_LOW_SPEED_MODE,
        .duty_resolution    = LEDC_RESOLUTION,
        .timer_num          = LEDC_TIMER,
        .freq_hz            = LEDC_FREQUENCY,
        .clk_cfg            = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer_config);

    ledc_channel_config_t channel_config = {
        .gpio_num       = LED_PIN,
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .duty           = 0,
        .hpoint         = 0
    };
    ledc_channel_config(&channel_config);

    while(true) {
        // Fade-in: поступово збільшуємо яскравість з 0% до 100%
        for (int duty = 0; duty <= 1023; duty += 10) {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL, duty);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL);
            vTaskDelay(pdMS_TO_TICKS(20));
            printf("LED duty cycle: %d\n", duty);
        }

        vTaskDelay(pdMS_TO_TICKS(500));

        // Fade-out: поступово зменшуємо яскравість з 100% до 0%
        for (int duty = 1023; duty >= 0; duty -= 10) {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL, duty);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL);
            printf("LED duty cycle: %d\n", duty);
            vTaskDelay(pdMS_TO_TICKS(20));
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
