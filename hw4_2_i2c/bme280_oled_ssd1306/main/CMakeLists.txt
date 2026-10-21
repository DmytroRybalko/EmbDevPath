idf_component_register(SRCS "bme_280.c" "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_driver_i2c bme280 esp_ssd1306)
