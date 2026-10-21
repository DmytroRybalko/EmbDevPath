#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "driver/i2c_master.h"
#include <ssd1306.h>
#include "font_latin_8x8.h"

/*
Here is test files for ssd1306 oled display driver from K0I05 (see components)
*/
void ssd1306_test(ssd1306_handle_t dev_hdl)
{
    for ( ;; ) {
        ESP_LOGI(TAG, "######################## SSD1306 - START #########################");
        //
        int center = 1, top = 1, bottom = 4;
        char lineChar[16];
        uint8_t image[24];

        ESP_LOGI(TAG, "Panel is 128x64");

        // Display x3 text
        ESP_LOGI(TAG, "Display x3 Text");
        ssd1306_clear_display(dev_hdl, false);
        ssd1306_set_contrast(dev_hdl, 0xff);
        ssd1306_display_text_x3(dev_hdl, 0, "Hello", false);
        vTaskDelay(3000 / portTICK_PERIOD_MS);

        // Display x2 text
        ESP_LOGI(TAG, "Display x2 Text");
        ssd1306_clear_display(dev_hdl, false);
        ssd1306_set_contrast(dev_hdl, 0xff);
        ssd1306_display_text_x2(dev_hdl, 0, "{xTEXTx}", false);
        ssd1306_display_text_x2(dev_hdl, 2, " X2-X2", false);
        vTaskDelay(3000 / portTICK_PERIOD_MS);

        // Display text
        ESP_LOGI(TAG, "Display Text");
        ssd1306_clear_display(dev_hdl, false);
        ssd1306_set_contrast(dev_hdl, 0xff);
        ssd1306_display_text(dev_hdl, 0, "SSD1306 128x64", false);
        ssd1306_display_text(dev_hdl, 1, "Hello World!!", false);
        ssd1306_display_text(dev_hdl, 2, "SSD1306 128x64", true);
        ssd1306_display_text(dev_hdl, 3, "Hello World!!", true);
        vTaskDelay(3000 / portTICK_PERIOD_MS);
        
        // Display Count Down
        ESP_LOGI(TAG, "Display Count Down");
        memset(image, 0, sizeof(image));
        ssd1306_display_image(dev_hdl, top, (6*8-1), image, sizeof(image));
        ssd1306_display_image(dev_hdl, top+1, (6*8-1), image, sizeof(image));
        ssd1306_display_image(dev_hdl, top+2, (6*8-1), image, sizeof(image));
        for(int font = 0x39; font > 0x30; font--) {
            memset(image, 0, sizeof(image));
            ssd1306_display_image(dev_hdl, top+1, (7*8-1), image, 8);
            memcpy(image, font_latin_8x8_tr[font], 8);
            if (dev_hdl->dev_config.flip_enabled) ssd1306_flip_buffer(image, 8);
            ssd1306_display_image(dev_hdl, top+1, (7*8-1), image, 8);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        };
        
        //
        ESP_LOGI(TAG, "######################## SSD1306 - END ###########################");
        //
        //
        // pause the task per defined wait period
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
    //
    // free resources
    ssd1306_delete( dev_hdl );
    vTaskDelete( NULL );
};


/**
 * @brief Demo how ssd1306_display_text function show float numbers, converted to string via snprintf() functions
 * 
 * @param handle SSD1306 device handle.
 * @return printed to console.
 */
void ssd1306_test_display_data(ssd1306_handle_t dev_hdl)
{
    float t = 20.345, p = 104.542, h = 34.38;
    char text[16 + 1];
    
    while (true) {
        // Display text
        ESP_LOGI(TAG, "Display Text");
        ssd1306_clear_display(dev_hdl, false);
        ssd1306_set_contrast(dev_hdl, 0xff);
        ssd1306_display_text(dev_hdl, 0, "SSD1306 128x64", false);
        // Convert temperature to string
        snprintf(text, sizeof text, "T: %6.1f C", t);
        ssd1306_display_text(dev_hdl, 2, text, false);
        // Convert pressure to string
        snprintf(text, sizeof text, "P: %6.1f hPa", p / 100.0f);
        ssd1306_display_text(dev_hdl, 4, text, false);
        // Convert humidity to string
        snprintf(text, sizeof text, "H: %6.1f %%", h);
        ssd1306_display_text(dev_hdl, 6, text, false);
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
        
    vTaskDelay(pdMS_TO_TICKS(2000));
     // free resources
    ssd1306_delete( dev_hdl );
    vTaskDelete( NULL );
};