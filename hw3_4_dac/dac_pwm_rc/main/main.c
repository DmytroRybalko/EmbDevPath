#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/ledc.h>
#include "esp_log.h"
#include "sdkconfig.h"

// Set 
#define BUZZER_PIN      5
#define LEDC_CHANNEL    LEDC_CHANNEL_0
#define LEDC_TIMER      LEDC_TIMER_0
#define BASE_FREQUENCY  440 
#define LEDC_RESOLUTION LEDC_TIMER_10_BIT
#define BASE_DUTY       512

// Set music
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523

// Set notes
typedef struct {
    int frequency;
    int duration_ms;
} Note;

const Note melody[] = {
    {NOTE_C4, 400}, {NOTE_C4, 400},
    {NOTE_G4, 400}, {NOTE_G4, 400},
    {NOTE_A4, 400}, {NOTE_A4, 400},
    {NOTE_G4, 800},
    {NOTE_F4, 400}, {NOTE_F4, 400},
    {NOTE_E4, 400}, {NOTE_E4, 400},
    {NOTE_D4, 400}, {NOTE_D4, 400},
    {NOTE_C4, 800},
    {0,      2000},
};

static const char *TAG = "BUZZER";

void playNote(int frequency, int duration_ms)
{
    if (frequency == 0) {
        // Silence — turn off sound
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
    } else {
        // Set frequency
        ESP_ERROR_CHECK(ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, frequency));
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, BASE_DUTY));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
        ESP_LOGI(TAG, "Note: %d Hz, duration: %d ms", frequency, duration_ms);
    }

    vTaskDelay(pdMS_TO_TICKS(duration_ms));

    // Pause between notes
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    vTaskDelay(pdMS_TO_TICKS(50));
}

void app_main(void)
{
    ledc_timer_config_t timer_config = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_RESOLUTION,
        .timer_num       = LEDC_TIMER,
        .freq_hz         = BASE_FREQUENCY,
        .clk_cfg         = LEDC_USE_APB_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_config));
    ESP_LOGI(TAG, "Timer OK");

    ledc_channel_config_t channel_config = {
        .gpio_num       = BUZZER_PIN,
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .duty           = BASE_DUTY,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_config));
    ESP_LOGI(TAG, "Channel OK");    

    while(true) {
        int n = sizeof(melody) / sizeof(melody[0]);
        for (int i = 0; i < n; i++) {
            playNote(melody[i].frequency, melody[i].duration_ms);
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
