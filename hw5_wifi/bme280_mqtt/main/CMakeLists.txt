idf_component_register(SRCS "ssd_1306.c" "bme_280.c" "main.c" "mqtt_app.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_driver_i2c bme280 esp_ssd1306 mqtt esp_netif esp_wifi nvs_flash)
