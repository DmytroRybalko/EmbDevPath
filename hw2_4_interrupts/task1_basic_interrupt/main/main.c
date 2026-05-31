 /*
 * Task 2: Software Debounce Using a Timer (Time-Based)
 *
 * Add time-based debounce logic:
 * - Ignore an interrupt if less than 50 ms has elapsed since the previous one.
 * - Implement the debounce logic outside the ISR.
 *
 * Expected Result:
 * - Fewer false trigger events caused by button bouncing.
 * - Trigger events may still occur when the button is released.
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

#define BUTTON_GPIO     GPIO_NUM_18   /* change to your pin */
#define TAG             "TASK2"
#define DEBOUNCE_TIME   150

static volatile uint32_t isr_counter = 0;
static volatile bool button_pressed = false;
static volatile bool led_state = false;

static void IRAM_ATTR button_isr_handler(void *arg)
{
    isr_counter++;
    button_pressed = true;
}

void app_main(void)
{
    // Set led pin
    gpio_set_direction(GPIO_NUM_15, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_15, 0);
    
    // Set button pin
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,     /* button: GPIO -> GND */
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_NEGEDGE,      /* FALLING edge */
    };
    
    gpio_config(&io_conf);

    /* --- Install ISR service and attach handler --- */
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO, button_isr_handler, NULL);

    ESP_LOGI(TAG, "Task 1 started. Press the button and watch the counter.");
    ESP_LOGI(TAG,"Button GPIO: %d", BUTTON_GPIO);

    uint64_t prev_time = 0;
    while (true) {
        if (button_pressed) {
            
            button_pressed = false;
            uint64_t now = esp_timer_get_time();
            
            if ((now - prev_time) > (DEBOUNCE_TIME * 1000ULL)) {
                uint32_t current = isr_counter;
                led_state = !led_state;
                gpio_set_level(GPIO_NUM_15, led_state);

                ESP_LOGI(TAG,
                     "ISR#%" PRIu32 ", BUTTON_GPIO = %d, debunce = %" PRIu64 "ms",
                     current,
                     gpio_get_level(BUTTON_GPIO),
                     (now - prev_time) / 1000);
            
                prev_time = now;
            }
        }
            
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}