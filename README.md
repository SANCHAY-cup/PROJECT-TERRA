# PROJECT T.E.R.R.A.
**Tethered Emergency Reconnaissance & Rescue Assistant** - a  tethered rescue rover for collapsed-building search and rescue.

## Why
- Wireless fails inside rubble, so a 10 m CAT6 tether carries power, video, control and alerts.
- The rover assesses voids and detects survivors/hazards before rescuers go in.

## Repo layout
```
platformio.ini        build environments (nano, esp32)
src/main.cpp          FreeRTOS firmware: sense, hazard, status LED, JSON telemetry
include/config.h      pins and thresholds
include/telemetry.h   shared data structure
docs/                 CAT6 pinout, remote build guide
.github/workflows/    CI that builds both environments
```

## Build
```bash
pip install platformio
pio run -e nano        # Arduino Nano (default)
pio run -e esp32       # ESP32
pio run -e nano -t upload
pio device monitor
```

## Dependencies
FreeRTOS (feilipu/FreeRTOS on Nano, built in on ESP32), ArduinoJson 7, FastLED.

## Status LED
Green OK, Red toxic gas, Blue survivor sound, Orange heat signature. Pin 8 on the tether also fires the operator's LED and buzzer.

## Note
Sensor pins and thresholds in `include/config.h` are placeholders; calibrate them against your real sensors.
