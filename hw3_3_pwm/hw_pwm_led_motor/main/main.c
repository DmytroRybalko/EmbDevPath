#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/ledc.h>
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "sdkconfig.h"

// LED defines
#define LED_PIN         16
#define LED_CHANNEL     0
#define LED_TIMER       0
#define LED_FREQUENCY   1000     // 1 kHz
#define LED_RESOLUTION  10      // 10-bit (0-1023)

// MOTOR difinse
#define MOTOR_PIN        15
#define MOTOR_CHANNEL    1
#define MOTOR_TIMER      1
#define MOTOR_FREQUENCY  20000   // 20 kHz for motor
#define MOTOR_RESOLUTION 10      // 10-bit

// Read data from potenciometer
#define ADC_CHAN0        ADC_CHANNEL_2
#define ADC_ATTEN        ADC_ATTEN_DB_12

const static char *TAG = "MOTOR_LED";
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
    
    // Setup LED configuration
    ledc_timer_config_t led_timer_config = {
        .speed_mode         = LEDC_LOW_SPEED_MODE,
        .duty_resolution    = LED_RESOLUTION,
        .timer_num          = LED_TIMER,
        .freq_hz            = LED_FREQUENCY,
        .clk_cfg            = LEDC_AUTO_CLK
    };
    ledc_timer_config(&led_timer_config);

    ledc_channel_config_t led_channel_config = {
        .gpio_num       = LED_PIN,
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LED_CHANNEL,
        .timer_sel      = LED_TIMER,
        .duty           = 0,
        .hpoint         = 0
    };
    ledc_channel_config(&led_channel_config);

    // Setup motor configuration
    ledc_timer_config_t motor_timer_config = {
        .speed_mode         = LEDC_LOW_SPEED_MODE,
        .duty_resolution    = MOTOR_RESOLUTION,
        .timer_num          = MOTOR_TIMER,
        .freq_hz            = MOTOR_FREQUENCY,
        .clk_cfg            = LEDC_AUTO_CLK
    };
    ledc_timer_config(&motor_timer_config);

    ledc_channel_config_t motor_channel_config = {
        .gpio_num       = MOTOR_PIN,
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = MOTOR_CHANNEL,
        .timer_sel      = MOTOR_TIMER,
        .duty           = 0,
        .hpoint         = 0
    };
    ledc_channel_config(&motor_channel_config);
    while(true) {
        // Read potentiometer
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHAN0, &readVoltage));
        // Remap adc data до duty cycle (0-1023)
        int dutyCycle = readVoltage >> 2;  // 0..4095 → 0..1023

        // LED operated
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LED_CHANNEL, dutyCycle);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LED_CHANNEL);
        
        // Motor operated
        ledc_set_duty(LEDC_LOW_SPEED_MODE, MOTOR_CHANNEL, dutyCycle);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, MOTOR_CHANNEL);

        ESP_LOGI(TAG, "Pot: %4d -> Speed: %3d%% -> Duty: %4d",
                 readVoltage,
                 (dutyCycle * 100) / 1023,
                 dutyCycle);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* LOG
I (114278) MOTOR_LED: Pot: 2951 -> Speed:  72% -> Duty:  737
I (114778) MOTOR_LED: Pot: 3255 -> Speed:  79% -> Duty:  813
I (115278) MOTOR_LED: Pot: 3567 -> Speed:  87% -> Duty:  891
I (115778) MOTOR_LED: Pot: 3943 -> Speed:  96% -> Duty:  985
I (116278) MOTOR_LED: Pot: 4095 -> Speed: 100% -> Duty: 1023
I (116778) MOTOR_LED: Pot: 4095 -> Speed: 100% -> Duty: 1023
I (117278) MOTOR_LED: Pot: 4095 -> Speed: 100% -> Duty: 1023
I (117778) MOTOR_LED: Pot: 4095 -> Speed: 100% -> Duty: 1023
I (118278) MOTOR_LED: Pot: 4095 -> Speed: 100% -> Duty: 1023
I (118778) MOTOR_LED: Pot: 3747 -> Speed:  91% -> Duty:  936
I (119278) MOTOR_LED: Pot: 3289 -> Speed:  80% -> Duty:  822
I (119778) MOTOR_LED: Pot: 2984 -> Speed:  72% -> Duty:  746
I (120278) MOTOR_LED: Pot: 2603 -> Speed:  63% -> Duty:  650
I (120778) MOTOR_LED: Pot: 2348 -> Speed:  57% -> Duty:  587
I (121278) MOTOR_LED: Pot: 2238 -> Speed:  54% -> Duty:  559
I (121778) MOTOR_LED: Pot: 1948 -> Speed:  47% -> Duty:  487
I (122278) MOTOR_LED: Pot: 1685 -> Speed:  41% -> Duty:  421
I (122778) MOTOR_LED: Pot: 1399 -> Speed:  34% -> Duty:  349
I (123278) MOTOR_LED: Pot: 1263 -> Speed:  30% -> Duty:  315
I (123778) MOTOR_LED: Pot: 1067 -> Speed:  26% -> Duty:  266
I (124278) MOTOR_LED: Pot:  726 -> Speed:  17% -> Duty:  181
I (124778) MOTOR_LED: Pot:  203 -> Speed:   4% -> Duty:   50
I (125278) MOTOR_LED: Pot:    0 -> Speed:   0% -> Duty:    0
I (125778) MOTOR_LED: Pot:   21 -> Speed:   0% -> Duty:    5
I (126278) MOTOR_LED: Pot:  595 -> Speed:  14% -> Duty:  148
I (126778) MOTOR_LED: Pot:  763 -> Speed:  18% -> Duty:  190
I (127278) MOTOR_LED: Pot:  821 -> Speed:  20% -> Duty:  205
I (127778) MOTOR_LED: Pot:  885 -> Speed:  21% -> Duty:  221
I (128278) MOTOR_LED: Pot:  904 -> Speed:  22% -> Duty:  226
I (128778) MOTOR_LED: Pot: 1123 -> Speed:  27% -> Duty:  280
I (129278) MOTOR_LED: Pot: 1960 -> Speed:  47% -> Duty:  490
I (129778) MOTOR_LED: Pot: 2259 -> Speed:  55% -> Duty:  564
I (130278) MOTOR_LED: Pot: 2761 -> Speed:  67% -> Duty:  690
I (130778) MOTOR_LED: Pot: 3213 -> Speed:  78% -> Duty:  803
I (131278) MOTOR_LED: Pot: 3635 -> Speed:  88% -> Duty:  908
I (131778) MOTOR_LED: Pot: 4095 -> Speed: 100% -> Duty: 1023
I (132278) MOTOR_LED: Pot: 4095 -> Speed: 100% -> Duty: 1023
I (132778) MOTOR_LED: Pot: 4095 -> Speed: 100% -> Duty: 1023
I (133278) MOTOR_LED: Pot: 4095 -> Speed: 100% -> Duty: 1023
I (133778) MOTOR_LED: Pot: 4095 -> Speed: 100% -> Duty: 1023
I (134278) MOTOR_LED: Pot: 4095 -> Speed: 100% -> Duty: 1023
I (134778) MOTOR_LED: Pot: 4095 -> Speed: 100% -> Duty: 1023
I (135278) MOTOR_LED: Pot: 4095 -> Speed: 100% -> Duty: 1023
I (135778) MOTOR_LED: Pot: 4095 -> Speed: 100% -> Duty: 1023
I (136278) MOTOR_LED: Pot: 3849 -> Speed:  94% -> Duty:  962
I (136778) MOTOR_LED: Pot: 3613 -> Speed:  88% -> Duty:  903
I (137278) MOTOR_LED: Pot: 3308 -> Speed:  80% -> Duty:  827
I (137778) MOTOR_LED: Pot: 3287 -> Speed:  80% -> Duty:  821
I (138278) MOTOR_LED: Pot: 3282 -> Speed:  80% -> Duty:  820
I (138778) MOTOR_LED: Pot: 3279 -> Speed:  80% -> Duty:  819
I (139278) MOTOR_LED: Pot: 3283 -> Speed:  80% -> Duty:  820
I (139778) MOTOR_LED: Pot: 3283 -> Speed:  80% -> Duty:  820
I (140278) MOTOR_LED: Pot: 3285 -> Speed:  80% -> Duty:  821
I (140778) MOTOR_LED: Pot: 3277 -> Speed:  80% -> Duty:  819
I (141278) MOTOR_LED: Pot: 3147 -> Speed:  76% -> Duty:  786
I (141778) MOTOR_LED: Pot: 2821 -> Speed:  68% -> Duty:  705
I (142278) MOTOR_LED: Pot: 2601 -> Speed:  63% -> Duty:  650
I (142778) MOTOR_LED: Pot: 2787 -> Speed:  68% -> Duty:  696
I (143278) MOTOR_LED: Pot: 3903 -> Speed:  95% -> Duty:  975
I (143778) MOTOR_LED: Pot: 4095 -> Speed: 100% -> Duty: 1023
I (144278) MOTOR_LED: Pot: 4095 -> Speed: 100% -> Duty: 1023
I (144778) MOTOR_LED: Pot: 2943 -> Speed:  71% -> Duty:  735
I (145278) MOTOR_LED: Pot: 2389 -> Speed:  58% -> Duty:  597
I (145778) MOTOR_LED: Pot: 1951 -> Speed:  47% -> Duty:  487
I (146278) MOTOR_LED: Pot: 1416 -> Speed:  34% -> Duty:  354
I (146778) MOTOR_LED: Pot: 1175 -> Speed:  28% -> Duty:  293
I (147278) MOTOR_LED: Pot: 1111 -> Speed:  27% -> Duty:  277
I (147778) MOTOR_LED: Pot: 1103 -> Speed:  26% -> Duty:  275
I (148278) MOTOR_LED: Pot: 1129 -> Speed:  27% -> Duty:  282
I (148778) MOTOR_LED: Pot: 1132 -> Speed:  27% -> Duty:  283
I (149278) MOTOR_LED: Pot: 1130 -> Speed:  27% -> Duty:  282
I (149778) MOTOR_LED: Pot: 1129 -> Speed:  27% -> Duty:  282
I (150278) MOTOR_LED: Pot: 1131 -> Speed:  27% -> Duty:  282
I (150778) MOTOR_LED: Pot: 1131 -> Speed:  27% -> Duty:  282
I (151278) MOTOR_LED: Pot: 1129 -> Speed:  27% -> Duty:  282
I (151778) MOTOR_LED: Pot: 1131 -> Speed:  27% -> Duty:  282
I (152278) MOTOR_LED: Pot: 1128 -> Speed:  27% -> Duty:  282
I (152778) MOTOR_LED: Pot: 1131 -> Speed:  27% -> Duty:  282
I (153278) MOTOR_LED: Pot: 1130 -> Speed:  27% -> Duty:  282
I (153778) MOTOR_LED: Pot: 1131 -> Speed:  27% -> Duty:  282
I (154278) MOTOR_LED: Pot: 1135 -> Speed:  27% -> Duty:  283
I (154778) MOTOR_LED: Pot: 1131 -> Speed:  27% -> Duty:  282
I (155278) MOTOR_LED: Pot: 1133 -> Speed:  27% -> Duty:  283
I (155778) MOTOR_LED: Pot: 1133 -> Speed:  27% -> Duty:  283
I (156278) MOTOR_LED: Pot: 1132 -> Speed:  27% -> Duty:  283
*/