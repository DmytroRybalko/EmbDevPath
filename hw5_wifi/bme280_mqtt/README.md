# BME280 MQTT Weather Station for ESP32-S3

This project turns an ESP32-S3 into a small IoT weather station. It reads temperature, pressure, and humidity from a Bosch BME280 sensor over I2C, shows the values on a 128x64 SSD1306 OLED, and publishes the readings over Wi-Fi using MQTT.

## Features

- ESP32-S3 target project built with ESP-IDF
- BME280 sensor readout via I2C
- SSD1306 OLED display output
- Wi-Fi station mode with MQTT over TLS to a public broker
- Periodic sensor sampling every 5 seconds
- MQTT publishing of temperature, pressure, and humidity values

## Hardware

- ESP32-S3 development board
- Bosch BME280 sensor module
- SSD1306 128x64 OLED display
- Common 3.3 V power and GND wiring

### I2C Wiring

The firmware uses the following configuration:

- SCL: GPIO4
- SDA: GPIO5
- BME280 I2C address: 0x76
- SSD1306 I2C address: 0x3C

## Wi-Fi and MQTT Setup

The Wi-Fi SSID and password are hardcoded in [main/mqtt_app.c](main/mqtt_app.c):

- `WIFI_SSID`
- `WIFI_PASS`

The MQTT broker is configured in the same file as:

- `mqtts://broker.hivemq.com:8883`

The app publishes readings to these topics:

- `dmytro/esp32/temperature`
- `dmytro/esp32/pressure`
- `dmytro/esp32/humidity`

The HiveMQ web dashbord for setting connectiong and subscribe for topics:
- `https://www.hivemq.com/demos/websocket-client/`
  
> Update the Wi-Fi credentials and broker settings before using this project on your network.

## Dependencies

This project uses:

- `esp_driver_i2c`
- local Bosch BME280 driver from [components/bme280](components/bme280)
- `esp_ssd1306` managed component
- `mqtt`, `esp_wifi`, `esp_netif`, and `nvs_flash`

## Build and Run

From the project root, run:

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

## Project Structure

- [CMakeLists.txt](CMakeLists.txt) — top-level ESP-IDF project definition
- [main/main.c](main/main.c) — app entry point, I2C setup, sensor polling, OLED updates, and MQTT publishing
- [main/mqtt_app.c](main/mqtt_app.c) — Wi-Fi and MQTT client setup
- [main/mqtt_app.h](main/mqtt_app.h) — MQTT helper API
- [main/bme_280.c](main/bme_280.c) — BME280 initialization and low-level sensor access
- [main/bme_280.h](main/bme_280.h) — BME280 helper interface
- [main/ssd_1306.c](main/ssd_1306.c) — SSD1306 display driver logic
- [main/idf_component.yml](main/idf_component.yml) — managed component configuration
- [components/bme280](components/bme280) — Bosch driver component

## Runtime Behavior

1. The ESP32 creates the I2C bus.
2. The BME280 sensor is added to the bus and initialized.
3. The SSD1306 OLED display is initialized.
4. Wi-Fi and MQTT are started.
5. The app enters a loop and reads compensated sensor data every 5 seconds.
6. The values are displayed on the OLED.
7. The same values are published to MQTT topics.

## Notes

- The code logs and publishes values in the sensor's native units and converts the pressure output for display as a simplified value.
- If you want to use a different Wi-Fi network or MQTT broker, update the configuration values in [main/mqtt_app.c](main/mqtt_app.c).
- In some setups, the OLED may require a different I2C address or pull-up configuration depending on the module.
- If a flash-size warning appears at boot, check the board configuration in ESP-IDF and match the flash size to the connected hardware.

## License

This project uses the repository's project license. Third-party libraries and vendor components retain their own licensing terms.
