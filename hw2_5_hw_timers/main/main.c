#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"

#define LED GPIO_NUM_15

static gptimer_handle_t gptimer = NULL;
static volatile bool led_state = false; 

// Timer interrupt callback
static bool IRAM_ATTR onTimer(gptimer_handle_t timer,
                               const gptimer_alarm_event_data_t *edata,
                               void *user_ctx)
{
    // Toggle LED
    led_state = !led_state;
    gpio_set_level(LED, led_state);
    return false; // No high-priority task woken
}

void app_main(void)
{
    // Configure LED GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // Create and configure the GPTimer (1 MHz resolution → 1 tick = 1 µs)
    gptimer_config_t timer_config = {
        .clk_src       = GPTIMER_CLK_SRC_DEFAULT,
        .direction     = GPTIMER_COUNT_UP,
        .resolution_hz = 1 * 1000 * 1000, // 1 MHz  (replaces prescaler=80 on 80 MHz clock)
    };
    gptimer_new_timer(&timer_config, &gptimer);

    gptimer_event_callbacks_t cbs = {
        .on_alarm = onTimer,
    };
    gptimer_register_event_callbacks(gptimer, &cbs, NULL);

    gptimer_enable(gptimer);

    // Set alarm: fire every 1 000 000 ticks = 1 s, auto-reload
    gptimer_alarm_config_t alarm_config = {
        .alarm_count                = 1000000, 
        .reload_count               = 0,
        .flags.auto_reload_on_alarm = true,    
    };
    gptimer_set_alarm_action(gptimer, &alarm_config);

    gptimer_start(gptimer);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}