#pragma once
#include <Arduino.h>

// ---------- Pin map (edit to match your wiring) ----------
#if defined(TERRA_NANO)
  constexpr uint8_t PIN_GAS     = A0;  // MQ-series gas sensor (analog)
  constexpr uint8_t PIN_SOUND   = A1;  // microphone / infrasonic front-end (analog)
  constexpr uint8_t PIN_HEAT    = 2;   // PIR / thermal presence output (digital)
  constexpr uint8_t PIN_HAZARD  = 4;   // -> CAT6 Pin 8 (hazard alert line)
  constexpr uint8_t PIN_STATUS  = 6;   // WS2812 status LED data
  constexpr uint16_t STACK_SMALL = 160;
  constexpr uint16_t STACK_LARGE = 256;

#elif defined(TERRA_ESP32S3)
  // ESP32-S3 WROOM N16R8 (16 MB flash, 8 MB octal PSRAM).
  // GPIO 26-37 are used by the flash/PSRAM and must NOT be used. GPIO 0, 3, 45, 46 are strapping pins.
  constexpr uint8_t PIN_GAS     = 1;   // MQ gas sensor, ADC1 (scale 5 V output down to 3.3 V!)
  constexpr uint8_t PIN_SOUND   = 2;   // microphone front-end, ADC1
  constexpr uint8_t PIN_HEAT    = 255; // not used: heat comes from the AMG8833 thermal grid
  constexpr uint8_t PIN_HAZARD  = 14;  // -> CAT6 Pin 8 (hazard alert line)
  constexpr uint8_t PIN_STATUS  = 38;  // WS2812 status LED data
  constexpr uint16_t STACK_SMALL = 2048;
  constexpr uint16_t STACK_LARGE = 4096;

  // Thermal sensor (AMG8833, I2C) on its own bus - the camera uses its own SCCB bus.
  constexpr uint8_t PIN_I2C_SDA = 21;
  constexpr uint8_t PIN_I2C_SCL = 47;
  constexpr uint8_t AMG_I2C_ADDR = 0x69;  // 0x69 if the AD_SELECT pad is high, 0x68 if low (both are tried)

  // Camera (OV2640 / OV5640, DVP). This is the Freenove ESP32-S3-WROOM-CAM wiring.
  // If you buy a different camera board, change ONLY these lines.
  constexpr int CAM_PIN_PWDN  = -1;
  constexpr int CAM_PIN_RESET = -1;
  constexpr int CAM_PIN_XCLK  = 15;
  constexpr int CAM_PIN_SIOD  = 4;
  constexpr int CAM_PIN_SIOC  = 5;
  constexpr int CAM_PIN_Y9    = 16;
  constexpr int CAM_PIN_Y8    = 17;
  constexpr int CAM_PIN_Y7    = 18;
  constexpr int CAM_PIN_Y6    = 12;
  constexpr int CAM_PIN_Y5    = 10;
  constexpr int CAM_PIN_Y4    = 8;
  constexpr int CAM_PIN_Y3    = 9;
  constexpr int CAM_PIN_Y2    = 11;
  constexpr int CAM_PIN_VSYNC = 6;
  constexpr int CAM_PIN_HREF  = 7;
  constexpr int CAM_PIN_PCLK  = 13;
  constexpr bool CAM_HFLIP = false;   // mirror the picture left/right
  constexpr bool CAM_VFLIP = false;   // flip the picture upside down (camera mounted upside down)

#else  // plain ESP32 (esp32dev)
  constexpr uint8_t PIN_GAS     = 34;
  constexpr uint8_t PIN_SOUND   = 35;
  constexpr uint8_t PIN_HEAT    = 27;
  constexpr uint8_t PIN_HAZARD  = 26;
  constexpr uint8_t PIN_STATUS  = 5;
  constexpr uint16_t STACK_SMALL = 2048;
  constexpr uint16_t STACK_LARGE = 4096;
#endif

// ---------- Thresholds (calibrate on the real sensors!) ----------
#if defined(TERRA_NANO)
  constexpr uint16_t ADC_MAX = 1023;
#else 
  constexpr uint16_t ADC_MAX = 4095;
#endif
constexpr uint16_t GAS_ALERT_RAW = (uint32_t)ADC_MAX * 40/100; // 40% of full scale
constexpr uint16_t SOUND_ALERT_RAW = (uint32_t)ADC_MAX * 30/100; // 30% of full scale
constexpr uint16_t SOUND_WINDOW_MS = 50;

// ---------- Timing ----------
constexpr uint16_t SENSE_PERIOD_MS     = 100;
constexpr uint16_t TELEMETRY_PERIOD_MS = 1000;
constexpr uint16_t STATUS_PERIOD_MS    = 100;

// ---------- ESP32-S3 thermal camera + live viewer ----------
#if defined(TERRA_ESP32S3)
  // Thermal target detection (calibrate in the real room - body heat looks weaker with distance)
  constexpr float    THERMAL_FOV_DEG        = 60.0f;  // AMG8833 horizontal field of view
  constexpr float    HEAT_MIN_DELTA_C       = 3.0f;   // target must be this much hotter than the room
  constexpr float    HEAT_MIN_ABS_C         = 28.0f;  // ...and at least this hot
  constexpr float    CENTRE_DEADBAND_DEG    = 8.0f;   // +/- this counts as straight ahead
  constexpr float    THERMAL_SMOOTH         = 0.5f;   // 0 = no smoothing, 0.9 = very slow/smooth
  constexpr uint16_t THERMAL_PERIOD_MS      = 100;    // AMG8833 runs at 10 frames per second
  constexpr bool     THERMAL_FLIP_X         = false;  // set true if left/right are swapped
  constexpr bool     THERMAL_FLIP_Y         = false;  // set true if up/down are swapped

  // Wi-Fi. Leave WIFI_STA_SSID empty to make the rover its own hotspot (works anywhere, no router).
  constexpr const char *WIFI_AP_SSID  = "TERRA-ROVER";
  constexpr const char *WIFI_AP_PASS  = "terra1234";  // at least 8 characters - CHANGE THIS
  constexpr const char *WIFI_STA_SSID = "";           // optional: join your router instead
  constexpr const char *WIFI_STA_PASS = "";

  // Camera picture
  constexpr int CAM_JPEG_QUALITY = 12;  // 10 (best) .. 63 (smallest)
#endif
