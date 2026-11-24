#pragma once
#if defined(TERRA_ESP32S3)
#include <Arduino.h>
#include "thermal_analysis.h"

struct ThermalFrame {
  float                 c[terra::THERMAL_N];  // smoothed temperatures, degC, row-major
  terra::ThermalResult  r;                    // analysis of this frame
  uint32_t              ms;                   // millis() when captured
  bool                  valid;                // false = sensor not answering
};

bool thermalBegin();                      // start I2C + sensor; false if not found
void thermalTask(void *);                 // FreeRTOS task: read, smooth, analyse
bool thermalLatest(ThermalFrame &out);    // copy of the newest frame
bool thermalHeatDetected();               // true while a warm target is in view
#endif
