 /*
 * Task 3: State-Based Debounce Using Level Verification
 *
 * - The interrupt only signals that an event has occurred.
 * - In the main loop() or task:
 *   - Accept the event only if the button is still pressed.
 *   - Ignore events generated when the button is released.
 *
 * Expected Result:
 * - Exactly one response for each button press.
 * - Button release does not trigger any action.
 * 
 * Log example:
 * 276) TASK2: Button GPIO: 18
I (3406) TASK2: ISR#2, BUTTON_GPIO = 0, LED=1
I (5316) TASK2: ISR#3, BUTTON_GPIO = 0, LED=0
I (6296) TASK2: ISR#4, BUTTON_GPIO = 0, LED=1
I (7386) TASK2: ISR#5, BUTTON_GPIO = 0, LED=0
I (8396) TASK2: Ignored: button already released (bounce)
I (8556) TASK2: ISR#7, BUTTON_GPIO = 0, LED=1
I (12186) TASK2: ISR#8, BUTTON_GPIO = 0, LED=0
I (13946) TASK2: ISR#9, BUTTON_GPIO = 0, LED=1
I (15116) TASK2: ISR#10, BUTTON_GPIO = 0, LED=0
I (16116) TASK2: ISR#12, BUTTON_GPIO = 0, LED=1
I (16706) TASK2: Ignored: button already released (bounce)
I (17546) TASK2: ISR#14, BUTTON_GPIO = 0, LED=0
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

#define BUTTON_GPIO     GPIO_NUM_18   /* change to your pin */
#define TAG             "TASK2"

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

    while (true) {
        if (button_pressed) {
            button_pressed = false;
            if (gpio_get_level(BUTTON_GPIO) == 0) { // accept event if button still pressed
                uint32_t current = isr_counter;
                led_state = !led_state;
                gpio_set_level(GPIO_NUM_15, led_state);

                ESP_LOGI(TAG, "ISR#%" PRIu32 ", BUTTON_GPIO = %d, LED=%d",
                         current, gpio_get_level(BUTTON_GPIO), (int)led_state);
            } else {
                ESP_LOGI(TAG, "Ignored: button already released (bounce)");
            }
        }
            
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}