#include <stdio.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>

#define LED GPIO_NUM_15

static volatile bool led_event = false;
static bool led_state = false;

void IRAM_ATTR timer_short(void *arg)
{
    led_event = true;
}

void led_handle()
{
    if (led_event) {
        led_event = false;
        led_state = !led_state;
        gpio_set_level(LED, led_state);
    }
}

void app_main(void)
{
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);

    const esp_timer_create_args_t my_timer_args = {
        .callback = &timer_short,
        .name = "my_timer"
    };
    esp_timer_handle_t timer_handler;

    esp_timer_create(&my_timer_args, &timer_handler);
    esp_timer_start_periodic(timer_handler, 1000000);

    while(true)
    {
        led_handle();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
