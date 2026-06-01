 /*
 * Task 4: Polling + Debounce (Without Interrupts)
 *
 * - Remove interrupts completely.
 * - Poll the button state every 5–10 ms.
 * - Implement debounce using a state machine.
 *
 * Expected Result:
 * - The most stable button handling behavior.
 * - Slightly increased response latency.
  
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

#define BUTTON_GPIO     GPIO_NUM_18   /* change to your pin */
#define TAG             "TASK4"

void app_main(void)
{
    gpio_set_direction(GPIO_NUM_15, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_15, 0);

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,      // ← без переривань
    };
    gpio_config(&io_conf);

    ESP_LOGI(TAG, "Task 4 started. Button GPIO: %d", BUTTON_GPIO);

    while (true) {
        int level = gpio_get_level(BUTTON_GPIO);
        ESP_LOGI(TAG, "GPIO=%d", level);

        vTaskDelay(pdMS_TO_TICKS(30));           
    }
}