#include <stdio.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>
#include <esp_log.h>

#define LED GPIO_NUM_15
#define SHORT_TIME 2 * 1000 * 1000 // 2 seconds
#define LONG_TIME  5 * 1000 * 1000 // 5 seconds

static const char* TAG = "LED BLINK";

static void long_timer_callback(void *arg) {
    // Activate LED
    gpio_set_level(LED, 1);
    
    int64_t time_since_boot = esp_timer_get_time();
    ESP_LOGI(TAG, "Turn on LED and oneshot timer called, time since boot: %lld us", time_since_boot);
    
    // Get callback from short period timer
    esp_timer_handle_t oneshot_timer_handle = (esp_timer_handle_t) arg;

    /* Safety guard: stop oneshot timer if still active.
    * Should never trigger since oneshot (2s) always expires
    * before periodic fires again (8s), but protects against
    * future interval changes. */
    if (esp_timer_is_active(oneshot_timer_handle)) {
        ESP_ERROR_CHECK(esp_timer_stop(oneshot_timer_handle));
    }

    // Restart oneshot timer that turn off LED
    ESP_ERROR_CHECK(esp_timer_start_once(oneshot_timer_handle, SHORT_TIME));
    time_since_boot = esp_timer_get_time();
    ESP_LOGI(TAG, "Restarted oneshot timer with 2s period, time since boot: %lld us",
            time_since_boot);
}

static void short_timer_callback(void* arg) {
    // Deactivate LED
    gpio_set_level(LED, 0);
    
    int64_t time_since_boot = esp_timer_get_time();
    ESP_LOGI(TAG, "I turn off LED, time since boot: %lld us", time_since_boot);
}

void app_main(void)
{
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);
    gpio_set_level(LED, 1);

    // Set shot timer
    const esp_timer_create_args_t short_timer_args = {
            .callback = &short_timer_callback,
            .name = "short_timer"
    };
    esp_timer_handle_t short_timer_handler;
    ESP_ERROR_CHECK(esp_timer_create(&short_timer_args, &short_timer_handler));

    // Set long timer
    const esp_timer_create_args_t long_timer_args = {
        .callback = &long_timer_callback,
        .arg = (void*) short_timer_handler,
        .name = "long_timer"
    };
    esp_timer_handle_t long_timer_handler;
    esp_timer_create(&long_timer_args, &long_timer_handler);

    // Start the timers
    ESP_ERROR_CHECK(esp_timer_start_once(short_timer_handler, SHORT_TIME));
    ESP_ERROR_CHECK(esp_timer_start_periodic(long_timer_handler, LONG_TIME));
    ESP_LOGI(TAG, "Started timers, time since boot: %lld us", esp_timer_get_time());
    
    while(true) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
