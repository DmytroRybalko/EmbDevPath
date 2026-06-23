#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/ledc.h>
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "sdkconfig.h"

#define MOTOR_PWM_PIN   15
//#define POT_PIN         16
#define LEDC_CHANNEL    1
#define LEDC_TIMER      0
#define LEDC_FREQUENCY  20000   // 20 kHz for motor
#define LEDC_RESOLUTION 10      // 10-bit
// Read data from potenciometer
#define ADC_CHAN0       ADC_CHANNEL_2
#define ADC_ATTEN       ADC_ATTEN_DB_12

const static char *TAG = "MOTOR_PWM";
static int readVoltage;

void app_main(void)
{
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
    
    // Setup motor configuration
    ledc_timer_config_t timer_config = {
        .speed_mode         = LEDC_LOW_SPEED_MODE,
        .duty_resolution    = LEDC_RESOLUTION,
        .timer_num          = LEDC_TIMER,
        .freq_hz            = LEDC_FREQUENCY,
        .clk_cfg            = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer_config);

    ledc_channel_config_t channel_config = {
        .gpio_num       = MOTOR_PWM_PIN,
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .duty           = 0,
        .hpoint         = 0
    };
    ledc_channel_config(&channel_config);
    while(true) {
        // Read potentiometer
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHAN0, &readVoltage));
        // Remap adc data до duty cycle (0-1023)
        int dutyCycle = readVoltage >> 2;  // 0..4095 → 0..1023

        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL, dutyCycle);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL);
        
        ESP_LOGI(TAG, "Pot: %4d -> Speed: %3d%% -> Duty: %4d",
                 readVoltage,
                 (dutyCycle * 100) / 1023,
                 dutyCycle);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/*LOG
I (18178) MOTOR_PWM: Pot: 2792 -> Speed:  68% -> Duty:  698

I (18278) MOTOR_PWM: Pot: 2712 -> Speed:  66% -> Duty:  678

I (18378) MOTOR_PWM: Pot: 2837 -> Speed:  69% -> Duty:  709

I (18478) MOTOR_PWM: Pot: 2654 -> Speed:  64% -> Duty:  663

I (18578) MOTOR_PWM: Pot: 2332 -> Speed:  56% -> Duty:  583

I (18678) MOTOR_PWM: Pot: 2944 -> Speed:  71% -> Duty:  736

I (18778) MOTOR_PWM: Pot: 2566 -> Speed:  62% -> Duty:  641

I (18878) MOTOR_PWM: Pot: 2091 -> Speed:  51% -> Duty:  522

I (18978) MOTOR_PWM: Pot: 2131 -> Speed:  52% -> Duty:  532

I (19078) MOTOR_PWM: Pot: 2395 -> Speed:  58% -> Duty:  598

I (19178) MOTOR_PWM: Pot: 2841 -> Speed:  69% -> Duty:  710

I (19278) MOTOR_PWM: Pot: 2347 -> Speed:  57% -> Duty:  586

I (19378) MOTOR_PWM: Pot: 1879 -> Speed:  45% -> Duty:  469

I (19478) MOTOR_PWM: Pot: 1785 -> Speed:  43% -> Duty:  446

I (19578) MOTOR_PWM: Pot: 1834 -> Speed:  44% -> Duty:  458

I (19678) MOTOR_PWM: Pot: 2423 -> Speed:  59% -> Duty:  605

I (19778) MOTOR_PWM: Pot: 2263 -> Speed:  55% -> Duty:  565

I (19878) MOTOR_PWM: Pot: 2467 -> Speed:  60% -> Duty:  616

I (19978) MOTOR_PWM: Pot: 2159 -> Speed:  52% -> Duty:  539

I (20078) MOTOR_PWM: Pot: 1748 -> Speed:  42% -> Duty:  437

I (20178) MOTOR_PWM: Pot: 2097 -> Speed:  51% -> Duty:  524

I (20278) MOTOR_PWM: Pot: 2078 -> Speed:  50% -> Duty:  519

I (20378) MOTOR_PWM: Pot: 2086 -> Speed:  50% -> Duty:  521

I (20478) MOTOR_PWM: Pot: 2079 -> Speed:  50% -> Duty:  519

I (20578) MOTOR_PWM: Pot: 2087 -> Speed:  50% -> Duty:  521

I (20678) MOTOR_PWM: Pot: 2089 -> Speed:  51% -> Duty:  522

I (20778) MOTOR_PWM: Pot: 2107 -> Speed:  51% -> Duty:  526

I (20878) MOTOR_PWM: Pot: 2090 -> Speed:  51% -> Duty:  522

I (20978) MOTOR_PWM: Pot: 1908 -> Speed:  46% -> Duty:  477

I (21078) MOTOR_PWM: Pot: 1875 -> Speed:  45% -> Duty:  468

I (21178) MOTOR_PWM: Pot: 2095 -> Speed:  51% -> Duty:  523

I (21278) MOTOR_PWM: Pot: 2093 -> Speed:  51% -> Duty:  523

I (21378) MOTOR_PWM: Pot: 2131 -> Speed:  52% -> Duty:  532

I (21478) MOTOR_PWM: Pot: 2088 -> Speed:  51% -> Duty:  522

I (21578) MOTOR_PWM: Pot: 2133 -> Speed:  52% -> Duty:  533

I (21678) MOTOR_PWM: Pot: 1831 -> Speed:  44% -> Duty:  457

I (21778) MOTOR_PWM: Pot: 1999 -> Speed:  48% -> Duty:  499

I (21878) MOTOR_PWM: Pot: 1702 -> Speed:  41% -> Duty:  425

I (21978) MOTOR_PWM: Pot: 1523 -> Speed:  37% -> Duty:  380

I (22078) MOTOR_PWM: Pot: 1380 -> Speed:  33% -> Duty:  345

I (22178) MOTOR_PWM: Pot: 1228 -> Speed:  30% -> Duty:  307

I (22278) MOTOR_PWM: Pot: 1081 -> Speed:  26% -> Duty:  270

I (22378) MOTOR_PWM: Pot:  904 -> Speed:  22% -> Duty:  226

I (22478) MOTOR_PWM: Pot:  691 -> Speed:  16% -> Duty:  172
*/