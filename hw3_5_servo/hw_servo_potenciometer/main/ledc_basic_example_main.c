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

/*LOG
Pot: 2364 -> Angle: 103.7 -> Duty:  641
Pot: 2257 -> Angle: 99.0 -> Duty:  620
Pot: 2165 -> Angle: 94.9 -> Duty:  602
Pot: 2074 -> Angle: 91.1 -> Duty:  585
Pot: 1993 -> Angle: 87.5 -> Duty:  569
Pot: 1903 -> Angle: 83.5 -> Duty:  551
Pot: 1813 -> Angle: 79.7 -> Duty:  534
Pot: 1738 -> Angle: 76.3 -> Duty:  519
Pot: 1650 -> Angle: 72.4 -> Duty:  502
Pot: 1574 -> Angle: 69.1 -> Duty:  487
Pot: 1506 -> Angle: 66.2 -> Duty:  474
Pot: 1465 -> Angle: 64.3 -> Duty:  466
Pot: 1462 -> Angle: 64.1 -> Duty:  465
Pot: 1497 -> Angle: 65.7 -> Duty:  472
Pot: 1580 -> Angle: 69.3 -> Duty:  488
Pot: 1691 -> Angle: 74.2 -> Duty:  510
Pot: 1811 -> Angle: 79.4 -> Duty:  533
Pot: 1906 -> Angle: 83.7 -> Duty:  552
Pot: 1994 -> Angle: 87.5 -> Duty:  569
Pot: 2074 -> Angle: 91.1 -> Duty:  585
Pot: 2156 -> Angle: 94.7 -> Duty:  601
Pot: 2236 -> Angle: 98.1 -> Duty:  616
Pot: 2324 -> Angle: 102.2 -> Duty:  634
Pot: 2407 -> Angle: 105.7 -> Duty:  650
Pot: 2491 -> Angle: 109.4 -> Duty:  666
Pot: 2589 -> Angle: 113.6 -> Duty:  685
Pot: 2679 -> Angle: 117.7 -> Duty:  703
Pot: 2736 -> Angle: 120.2 -> Duty:  714
Pot: 2804 -> Angle: 123.1 -> Duty:  727
Pot: 2867 -> Angle: 126.0 -> Duty:  740
Pot: 2932 -> Angle: 128.7 -> Duty:  752
Pot: 2995 -> Angle: 131.6 -> Duty:  765
Pot: 3055 -> Angle: 134.1 -> Duty:  776
Pot: 3141 -> Angle: 137.9 -> Duty:  793
Pot: 3217 -> Angle: 141.3 -> Duty:  808
Pot: 3315 -> Angle: 145.6 -> Duty:  827
Pot: 3441 -> Angle: 151.2 -> Duty:  852
Pot: 3573 -> Angle: 157.1 -> Duty:  878
Pot: 3745 -> Angle: 164.5 -> Duty:  911
Pot: 3920 -> Angle: 172.1 -> Duty:  945
Pot: 4095 -> Angle: 180.0 -> Duty:  980
Pot: 4095 -> Angle: 180.0 -> Duty:  980
Pot: 4095 -> Angle: 180.0 -> Duty:  980
Pot: 4095 -> Angle: 180.0 -> Duty:  980
Pot: 4095 -> Angle: 180.0 -> Duty:  980
Pot: 4095 -> Angle: 180.0 -> Duty:  980
Pot: 4095 -> Angle: 180.0 -> Duty:  980
Pot: 4095 -> Angle: 180.0 -> Duty:  980
Pot: 4095 -> Angle: 180.0 -> Duty:  980
Pot: 4095 -> Angle: 180.0 -> Duty:  980
Pot: 4095 -> Angle: 180.0 -> Duty:  980
Pot: 4095 -> Angle: 180.0 -> Duty:  980
Pot: 4095 -> Angle: 180.0 -> Duty:  980
Pot: 3987 -> Angle: 175.1 -> Duty:  958
Pot: 3612 -> Angle: 158.6 -> Duty:  885
Pot: 3240 -> Angle: 142.2 -> Duty:  812
Pot: 2955 -> Angle: 129.8 -> Duty:  757
Pot: 2725 -> Angle: 119.7 -> Duty:  712
Pot: 2545 -> Angle: 111.8 -> Duty:  677
Pot: 2372 -> Angle: 104.2 -> Duty:  643
Pot: 2211 -> Angle: 97.0 -> Duty:  611
Pot: 2033 -> Angle: 89.3 -> Duty:  577
*/