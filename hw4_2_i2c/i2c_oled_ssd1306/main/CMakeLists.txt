idf_component_register(SRCS "u8g2_demo.c" "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_driver_i2c esp_driver_gpio u8g2)
