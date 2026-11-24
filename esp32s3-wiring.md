# ESP32-S3 N16R8: wiring and first run

Build target: `pio run -e esp32s3` (pins are in `include/config.h`, section `TERRA_ESP32S3`).

## Wiring

| Part | Module pin | ESP32-S3 GPIO |
|---|---|---|
| Thermal sensor AMG8833 | SDA | 21 |
| | SCL | 47 |
| | VIN / GND | 3V3 / GND |
| Camera (OV2640 or OV5640) | XCLK | 15 |
| | SIOD (SDA) / SIOC (SCL) | 4 / 5 |
| | D0 to D7 (Y2 to Y9) | 11, 9, 8, 10, 12, 18, 17, 16 |
| | VSYNC / HREF / PCLK | 6 / 7 / 13 |
| | PWDN / RESET | not connected (-1) |
| Gas sensor (analog) | AO through a divider | 1 |
| Sound sensor (analog) | AO | 2 |
| Hazard alert (CAT6 pin 8) | | 14 |
| WS2812 status LED | DIN | 38 |

Notes
- This camera wiring is the Freenove ESP32-S3-WROOM-CAM layout. A generic N16R8 board has **no camera connector**, so you wire the camera yourself (or buy an S3 board with a camera socket and change the `CAM_PIN_*` lines).
- Never use GPIO 26 to 37: the flash and the octal PSRAM use them. GPIO 0, 3, 45 and 46 are strapping pins; avoid them.
- The ESP32-S3 ADC reads **3.3 V maximum**. If your MQ gas sensor runs at 5 V, put a resistor divider (for example 10k and 20k) between its output and GPIO 1.
- Use the **UART** USB-C port to flash and to read the serial monitor.

## First run

1. `pio run -e esp32s3 -t upload`, then `pio device monitor`.
2. The monitor prints lines starting with `#`: thermal sensor found, camera ready, and the viewer address.
3. On your phone join the Wi-Fi **TERRA-ROVER** (password in `config.h`, change it), then open `http://192.168.4.1/`.
4. Pick Camera, Thermal, Overlay or Side by side at the top of the page.

If the room Wi-Fi is better for you, put its name and password in `WIFI_STA_SSID` and `WIFI_STA_PASS`. The monitor prints the address to open.

## Calibration (do this once, in the real room)

- **Left and right swapped:** set `THERMAL_FLIP_X = true`.
- **Overlay does not match the camera picture:** open "Align the heat overlay" on the page and adjust Size and Left / right. The thermal sensor sees about 60 degrees; the camera lens usually sees a different angle.
- **Person not detected, or false alarms:** change `HEAT_MIN_DELTA_C` and `HEAT_MIN_ABS_C`. On a hot day the room can be close to body temperature, which an 8x8 sensor cannot separate.
- **Straight-ahead zone too narrow or wide:** change `CENTRE_DEADBAND_DEG`.

## About the CAT6 tether

The ESP32-S3 has no built-in Ethernet. Video over the tether needs a W5500 SPI Ethernet module, which this change does not include. For now the viewer uses Wi-Fi.
