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
#else
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
constexpr uint16_t GAS_ALERT_RAW = (unit32_t)ADC_MAX * 40/100; // 40% of full scale
constexpr uint16_t SOUND_ALERT_RAW = (unit32_t)ADC_MAX * 30/100; // 30% of full scale
constexpr uint16_t SOUND_WINDOW_MS = 50;

// ---------- Timing ----------
constexpr uint16_t SENSE_PERIOD_MS     = 100;
constexpr uint16_t TELEMETRY_PERIOD_MS = 1000;
constexpr uint16_t STATUS_PERIOD_MS    = 100;
