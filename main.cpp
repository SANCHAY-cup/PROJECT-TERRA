// PROJECT T.E.R.R.A. - robot-side firmware (FreeRTOS)
// Tasks: Sense -> (queue) -> Hazard / Status LED / Telemetry JSON
#include <Arduino.h>
#if defined(TERRA_NANO)
  #include <Arduino_FreeRTOS.h>
  #include <queue.h>
#else
  #include <freertos/FreeRTOS.h>
  #include <freertos/task.h>
  #include <freertos/queue.h>
#endif
#include <ArduinoJson.h>
#include <FastLED.h>
#include "config.h"
#include "telemetry.h"
#if defined(TERRA_ESP32S3)
  #include "thermal.h"
  #include "camera_web.h"
#endif

static QueueHandle_t telemetryQ;   // length-1 "latest value" mailbox
static CRGB statusLed[1];

static bool readLatest(Telemetry &t) { return xQueuePeek(telemetryQ, &t, 0) == pdTRUE; }

// Used by the web viewer (camera_web.cpp).
bool terraLatestTelemetry(Telemetry &t) { return readLatest(t); }

static void taskSense(void *) {
  for (;;) {
    Telemetry t{};
    t.gas = analogRead(PIN_GAS);
#if defined(TERRA_ESP32S3)
    t.heat = thermalHeatDetected();          // warm target seen by the AMG8833 grid
#else
    t.heat = digitalRead(PIN_HEAT) == HIGH;
#endif

    uint16_t lo = ADC_MAX, hi = 0;
    uint32_t start = millis();
    while (millis() - start < SOUND_WINDOW_MS) {
      uint16_t v = analogRead(PIN_SOUND);
      if (v < lo) lo = v;
      if (v > hi) hi = v;
    }
    t.soundPP = hi - lo;
    t.gasAlert = t.gas > GAS_ALERT_RAW;
    t.soundAlert = t.soundPP > SOUND_ALERT_RAW;
    t.uptimeMs = millis();
    xQueueOverwrite(telemetryQ, &t);
    vTaskDelay(pdMS_TO_TICKS(SENSE_PERIOD_MS));
  }
}

// Drives CAT6 Pin 8 so the operator's LED + buzzer fire.
static void taskHazard(void *) {
  for (;;) {
    Telemetry t;
    digitalWrite(PIN_HAZARD, (readLatest(t) && anyHazard(t)) ? HIGH : LOW);
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// Green = OK, Red = toxic gas, Blue = survivor sound, Orange = heat signature.
static void taskStatus(void *) {
  for (;;) {
    Telemetry t;
    CRGB c = CRGB::Black;
    if (readLatest(t)) {
      if (t.gasAlert)        c = CRGB::Red;
      else if (t.soundAlert) c = CRGB::Blue;
      else if (t.heat)       c = CRGB::Orange;
      else                   c = CRGB::Green;
    }
    statusLed[0] = c;
    FastLED.show();
    vTaskDelay(pdMS_TO_TICKS(STATUS_PERIOD_MS));
  }
}

static void taskTelemetry(void *) {
  for (;;) {
    Telemetry t;
    if (readLatest(t)) {
      JsonDocument doc;
      doc["v"] = TERRA_VERSION;
      doc["gas"] = t.gas;
      doc["snd"] = t.soundPP;
      doc["heat"] = t.heat;
      doc["haz"] = anyHazard(t);
      doc["ms"] = t.uptimeMs;
      serializeJson(doc, Serial);
      Serial.println();
    }
    vTaskDelay(pdMS_TO_TICKS(TELEMETRY_PERIOD_MS));
  }
}

void setup() {
  Serial.begin(115200);
#if !defined(TERRA_ESP32S3)
  pinMode(PIN_HEAT, INPUT);
#endif
  pinMode(PIN_HAZARD, OUTPUT);
  digitalWrite(PIN_HAZARD, LOW);
  FastLED.addLeds<WS2812B, PIN_STATUS, GRB>(statusLed, 1);
  FastLED.setBrightness(64);

  telemetryQ = xQueueCreate(1, sizeof(Telemetry));
  xTaskCreate(taskSense,     "sense",  STACK_LARGE, nullptr, 3, nullptr);
  xTaskCreate(taskHazard,    "hazard", STACK_SMALL, nullptr, 2, nullptr);
  xTaskCreate(taskStatus,    "status", STACK_SMALL, nullptr, 1, nullptr);
  xTaskCreate(taskTelemetry, "telem",  STACK_LARGE, nullptr, 1, nullptr);
#if defined(TERRA_ESP32S3)
  thermalBegin();                            // the task below retries if the sensor is not ready yet
  xTaskCreate(thermalTask, "thermal", 6144, nullptr, 2, nullptr);
  webBegin();                                // Wi-Fi + camera + live viewer
#endif
#if defined(TERRA_NANO)
  vTaskStartScheduler();
#endif
}

void loop() {}  // all work happens in FreeRTOS tasks
