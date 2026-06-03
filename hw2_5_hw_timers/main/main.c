#include <stdio.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>

#define LED_G GPIO_NUM_15

volatile BaseType_t toggle;

void IRAM_ATTR timer_callback(void *arg)
{
    static int toggle = 0;
    gpio_set_level(LED_G, toggle);
    toggle ^= 1;
}

void app_main(void)
{
    gpio_set_direction(LED_G, GPIO_MODE_OUTPUT);

    const esp_timer_create_args_t my_timer_args = {
        .callback = &timer_callback,
        .name = "my_timer"
    };
    esp_timer_handle_t timer_handler;

    esp_timer_create(&my_timer_args, &timer_handler);
    esp_timer_start_periodic(timer_handler, 1000000);

    for(;;)
    {

        vTaskDelay(100/portTICK_PERIOD_MS);
    }
}
