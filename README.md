# PROJECT T.E.R.R.A.
**Tethered Emergency Reconnaissance & Rescue Assistant** - a  tethered rescue rover for collapsed-building search and rescue.

## Why
- Wireless fails inside rubble, so a 10 m CAT6 tether carries power, video, control and alerts.
- The rover assesses voids and detects survivors/hazards before rescuers go in.

## Repo layout
```
platformio.ini        build environments (nano, esp32, esp32s3)
src/main.cpp          FreeRTOS firmware: sense, hazard, status LED, JSON telemetry
include/config.h      pins and thresholds
include/telemetry.h   shared data structure
include/thermal_analysis.h  8x8 thermal grid -> direction, angle, robot action (ESP32-S3)
src/thermal.cpp       AMG8833 driver (ESP32-S3)
src/camera_web.cpp    Wi-Fi, camera stream and live viewer (ESP32-S3)
include/web_page.h    the viewer web page
docs/                 CAT6 pinout, remote build guide
.github/workflows/    CI that builds both environments
```

## Build
```bash
pip install platformio
pio run -e nano        # Arduino Nano (default)
pio run -e esp32       # ESP32
pio run -e esp32s3     # ESP32-S3 N16R8: camera + thermal grid + live viewer
pio run -e nano -t upload
pio device monitor
```

## ESP32-S3 live viewer (camera + thermal)
Join the Wi-Fi `TERRA-ROVER`, open `http://192.168.4.1/` and choose **Camera**, **Thermal**, **Overlay** or **Side by side**. The page shows the 8x8 thermal grid in degC, a heat overlay on the camera picture, the direction and angle of the warmest target, the hottest cell, and the suggested robot action. Wiring and calibration: `docs/esp32s3-wiring.md`.

## Dependencies
FreeRTOS (feilipu/FreeRTOS on Nano, built in on ESP32), ArduinoJson 7, FastLED.

## Status LED
Green OK, Red toxic gas, Blue survivor sound, Orange heat signature. Pin 8 on the tether also fires the operator's LED and buzzer.

## Note
Sensor pins and thresholds in `include/config.h` are placeholders; calibrate them against your real sensors.
