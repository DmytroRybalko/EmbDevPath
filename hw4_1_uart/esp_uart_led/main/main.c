#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_timer.h"

// UART configuration
#define TX_PIN          (4)
#define RX_PIN          (5)
#define UART_PORT_NUM   (CONFIG_EXAMPLE_UART_PORT_NUM)
#define UART_BAUD_RATE  (115200)
#define UART_NUM        UART_NUM_1

#define LED_PIN         GPIO_NUM_15
#define BUT_PIN         GPIO_NUM_8
#define DEBOUNCE_TIME   (150)

static const int RX_BUF_SIZE = 1024;
static volatile bool buttonPressed = false;
static volatile uint8_t ledToggle = 0;
uint64_t prevTime = 0;

static void IRAM_ATTR button_isr_handler(void *arg)
{
    uint64_t now = esp_timer_get_time();
    if (now - prevTime > DEBOUNCE_TIME * 1000ULL) {
        buttonPressed = true;
        prevTime = now;
    }
}

void init_uart(void)
{
    const uart_config_t uart_config = {
        .baud_rate  = UART_BAUD_RATE,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    uart_driver_install(UART_NUM, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

void init_led(void)
{
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_conf);
    gpio_set_level(LED_PIN, 0);  // вимкнено за замовчуванням
}

void init_button(void)
{
    gpio_config_t but_conf = {
        .pin_bit_mask = (1ULL << BUT_PIN),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&but_conf);
    //gpio_set_level(LED_PIN, 0);  // вимкнено за замовчуванням
    /* --- Install ISR service and attach handler --- */
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUT_PIN, button_isr_handler, NULL);
}

void app_main(void)
{
    init_uart();
    vTaskDelay(100 / portTICK_PERIOD_MS);
    uart_flush(UART_NUM);       
    init_led();
    init_button();
    
    while(true) {
        // UART communication
        // Send led status outside
        if (buttonPressed) {
            char *TX_DATA = "1\r\n";
            int txBytes = uart_write_bytes(UART_NUM_1, TX_DATA, strlen(TX_DATA));
            ESP_LOGI("ESP", "Sent %d bytes, button=%d", txBytes, gpio_get_level(BUT_PIN));
            buttonPressed = false;
        }

        // Get led status outside
        uint8_t data[16] = {0}; // set buffer for reading from reciver
        int rxBytes = uart_read_bytes(UART_NUM_1, data, sizeof(data) - 1, pdMS_TO_TICKS(100));
        
        if ((char)data[0] == '1') {
            ledToggle ^= 1;
            gpio_set_level(LED_PIN, ledToggle);
        }    

        if (rxBytes > 0) {
            data[rxBytes] = 0;
            ESP_LOGI("APP_MAIN", "Read %d bytes: %s", rxBytes, data);
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
