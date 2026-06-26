/* LEDC Servo basic example s*/
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_pm.h"
#include "esp_adc/adc_oneshot.h"
#include "sdkconfig.h"

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          (2) // Define the output GPIO
#define SERVO_CHANNEL           LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY               (4096) // Set duty to 50%. (2 ** 13) * 50% = 4096
#define LEDC_CLK_SRC            LEDC_AUTO_CLK
#define LEDC_FREQUENCY          (50) // Frequency in Hertz.
#define SERVO_MIN_DUTY          (180)  // theory ->(410) 0° = 1 мс
#define SERVO_MID_DUTY          (614)  // 90°  = 1.5 мс  
#define SERVO_MAX_DUTY          (980) // theory (819) 180° = 2 мс 1000
#define ADC_CHAN0               ADC_CHANNEL_4 // Read data from potenciometer
#define ADC_ATTEN               ADC_ATTEN_DB_12

static int readPot;

int filter_adc_avg(adc_oneshot_unit_handle_t handle, adc_channel_t chan)
{
    const int ADC_SAMPLES = 10;
    int sum = 0;
    for (int i = 0; i < ADC_SAMPLES; i++) {
        int val;
        adc_oneshot_read(handle, chan, &val);
        sum += val;
    }
    return sum / ADC_SAMPLES;
}

float duty2angle(uint32_t duty)
{
    if (duty <= SERVO_MIN_DUTY) return 0.0f;
    if (duty >= SERVO_MAX_DUTY) return 180.0f;

    return (float)(duty - SERVO_MIN_DUTY)
           / (float)(SERVO_MAX_DUTY - SERVO_MIN_DUTY)
           * 180.0f;
}

static void servo_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t servo_timer = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = LEDC_DUTY_RES,
        .timer_num        = LEDC_TIMER,
        .freq_hz          = LEDC_FREQUENCY,  
        .clk_cfg          = LEDC_CLK_SRC,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&servo_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = SERVO_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .gpio_num       = LEDC_OUTPUT_IO,
        .duty           = SERVO_MIN_DUTY, // Set duty to 0%
        .hpoint         = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

void app_main(void)
{
    // Set the LEDC peripheral configuration
    servo_init();
    
    // Setup ADC for reading data from potentiometer
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc_handle));

    adc_oneshot_chan_cfg_t config = {
        .atten      = ADC_ATTEN,
        .bitwidth   = ADC_BITWIDTH_12,
    };

    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHAN0, &config));
    
    while(true) {
        // Read potentiometer
        int readPot = filter_adc_avg(adc_handle, ADC_CHAN0);
        // Remap adc data до duty cycle (0-1023)
        int dutyCycle = SERVO_MIN_DUTY + (readPot * (SERVO_MAX_DUTY - SERVO_MIN_DUTY)) / 4095;

        ledc_set_duty(LEDC_LOW_SPEED_MODE, SERVO_CHANNEL, dutyCycle);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, SERVO_CHANNEL);
        
        printf("Pot: %4d -> Angle: %.1f -> Duty: %4d\n",
               readPot,
               duty2angle(dutyCycle),
               dutyCycle);

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
