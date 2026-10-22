# BME280 + SSD1306 on ESP32-S3 (ESP-IDF)

This project reads temperature, pressure, and humidity from a BME280 over I2C and displays the values on an SSD1306 OLED.

## Features

- ESP-IDF project targeting ESP32-S3.
- I2C master on GPIO4 (SCL) and GPIO5 (SDA).
- BME280 initialization and weather-station style sensor configuration.
- Periodic sensor read every 5 seconds.
- OLED text rendering with `k0i05/esp_ssd1306`.

## Hardware

- ESP32-S3 board
- BME280 sensor (I2C)
- SSD1306 OLED (128x64, I2C)

### I2C Wiring

- SCL: GPIO4
- SDA: GPIO5
- BME280 address: `0x76`
- SSD1306 default address: `0x3C`

Make sure both devices share GND and are powered according to your module voltage requirements.

## Software Requirements

- ESP-IDF `v6.x` (project currently used with `v6.0.1`)
- Python environment managed by ESP-IDF tools
- Target: `esp32s3`

## Dependencies

From [main/CMakeLists.txt](main/CMakeLists.txt):

- `esp_driver_i2c`
- `bme280` (local component in `components/bme280`)
- `esp_ssd1306` (managed component)

From [main/idf_component.yml](main/idf_component.yml):

- `k0i05/esp_ssd1306`

## Build and Run

Run from project root:

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

If your serial port is different, replace `/dev/ttyUSB0`.

## Project Structure

- [CMakeLists.txt](CMakeLists.txt): top-level project declaration.
- [main/main.c](main/main.c): app entry, I2C bus/device setup, periodic read/display loop.
- [main/bme_280.c](main/bme_280.c): BME280 I2C adapter callbacks and sensor init routine.
- [main/bme_280.h](main/bme_280.h): public interface for BME280 app-side helper.
- [components/bme280](components/bme280): Bosch BME280 driver component.

## Runtime Flow

1. Create I2C master bus.
2. Add BME280 device on I2C.
3. Initialize SSD1306 display.
4. Initialize BME280 context and sensor mode/settings.
5. Enter main loop.
6. Read compensated BME280 data.
7. Display values on OLED.
8. Log values to serial terminal.
9. Delay 5 seconds.

## Notes

- Pressure from Bosch API is typically in Pa. If you want true hPa on OLED, divide by `100.0f` before formatting.
- If you see a flash-size warning at boot (binary header smaller than detected flash), update flash size in menuconfig to match your board.

## License

Project code follows your repository license. Third-party components keep their own licenses.