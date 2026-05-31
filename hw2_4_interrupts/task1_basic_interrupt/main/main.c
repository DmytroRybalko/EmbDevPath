#include <stdio.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

void app_main(void)
{
    // Set button pin
    gpio_set_direction(GPIO_NUM_18, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_NUM_18, GPIO_FLOATING);
    // Set led pin
    gpio_set_direction(GPIO_NUM_15, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_15, 0);

    while(true) {
        if (gpio_get_level(GPIO_NUM_18)) {
            gpio_set_level(GPIO_NUM_15, 1);
        } else {
            gpio_set_level(GPIO_NUM_15, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
