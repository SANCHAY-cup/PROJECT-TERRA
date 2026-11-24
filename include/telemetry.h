#pragma once
#include <Arduino.h>

struct Telemetry {
  uint16_t gas;       // raw ADC
  uint16_t soundPP;   // peak-to-peak sound amplitude
  bool     heat;      // body-heat / thermal presence
  bool     gasAlert;
  bool     soundAlert;
  uint32_t uptimeMs;
};

inline bool anyHazard(const Telemetry &t) { return t.gasAlert || t.soundAlert || t.heat; }

// Latest sensor snapshot (defined in main.cpp). Used by the web viewer.
bool terraLatestTelemetry(Telemetry &t);
