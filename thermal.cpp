// AMG8833 8x8 thermal sensor driver + target detection (ESP32-S3 only).
#if defined(TERRA_ESP32S3)
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "config.h"
#include "thermal.h"

namespace {

constexpr uint8_t REG_PCTL  = 0x00;  // power control
constexpr uint8_t REG_RST   = 0x01;  // reset
constexpr uint8_t REG_FPSC  = 0x02;  // frame rate (0 = 10 fps)
constexpr uint8_t REG_INTC  = 0x03;  // interrupt control
constexpr uint8_t REG_PIXEL = 0x80;  // 64 pixels x 2 bytes

uint8_t           g_addr  = AMG_I2C_ADDR;
SemaphoreHandle_t g_mtx   = nullptr;
ThermalFrame      g_frame = {};
volatile bool     g_heat  = false;

bool writeReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(g_addr);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

bool probe(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}

bool initSensor() {
  const uint8_t candidates[2] = {AMG_I2C_ADDR, (uint8_t)(AMG_I2C_ADDR ^ 0x01)};  // 0x69 <-> 0x68
  bool found = false;
  for (uint8_t a : candidates) {
    if (probe(a)) { g_addr = a; found = true; break; }
  }
  if (!found) return false;
  bool ok = writeReg(REG_PCTL, 0x00);   // normal mode
  vTaskDelay(pdMS_TO_TICKS(50));
  ok &= writeReg(REG_RST, 0x3F);        // initial reset
  ok &= writeReg(REG_INTC, 0x00);       // no interrupts
  ok &= writeReg(REG_FPSC, 0x00);       // 10 fps
  vTaskDelay(pdMS_TO_TICKS(100));
  return ok;
}

// Reads the 8x8 grid. Pixel format: 12-bit sign-magnitude, 0.25 degC per step.
bool readPixels(float *out) {
  uint8_t raw[128];
  for (uint8_t off = 0; off < 128; off += 32) {       // 32-byte chunks fit any I2C buffer
    Wire.beginTransmission(g_addr);
    Wire.write((uint8_t)(REG_PIXEL + off));
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom((uint8_t)g_addr, (uint8_t)32) != 32) return false;
    for (uint8_t i = 0; i < 32; i++) raw[off + i] = Wire.read();
  }
  for (uint8_t i = 0; i < terra::THERMAL_N; i++) {
    uint16_t v   = (uint16_t)raw[2 * i] | ((uint16_t)raw[2 * i + 1] << 8);
    float    mag = (float)(v & 0x7FF);
    float    c   = ((v & 0x800) ? -mag : mag) * 0.25f;
    if (c < -20.0f || c > 120.0f) return false;      // garbage read
    out[i] = c;
  }
  return true;
}

}  // namespace

bool thermalBegin() {
  g_mtx = xSemaphoreCreateMutex();
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);
  Wire.setTimeOut(50);
  bool ok = initSensor();
  Serial.println(ok ? "# thermal: AMG8833 found" : "# thermal: AMG8833 NOT found - check SDA/SCL wiring");
  return ok;
}

void thermalTask(void *) {
  float smooth[terra::THERMAL_N] = {};
  bool  haveSmooth = false;
  uint8_t fails = 0;

  for (;;) {
    float raw[terra::THERMAL_N];
    if (readPixels(raw)) {
      fails = 0;
      float oriented[terra::THERMAL_N];
      for (uint8_t y = 0; y < terra::THERMAL_H; y++) {
        for (uint8_t x = 0; x < terra::THERMAL_W; x++) {
          uint8_t sx = THERMAL_FLIP_X ? (terra::THERMAL_W - 1 - x) : x;
          uint8_t sy = THERMAL_FLIP_Y ? (terra::THERMAL_H - 1 - y) : y;
          oriented[y * terra::THERMAL_W + x] = raw[sy * terra::THERMAL_W + sx];
        }
      }
      for (uint8_t i = 0; i < terra::THERMAL_N; i++) {
        smooth[i] = haveSmooth ? THERMAL_SMOOTH * smooth[i] + (1.0f - THERMAL_SMOOTH) * oriented[i]
                               : oriented[i];
      }
      haveSmooth = true;

      ThermalFrame f;
      for (uint8_t i = 0; i < terra::THERMAL_N; i++) f.c[i] = smooth[i];
      f.r = terra::analyseThermal(f.c, THERMAL_FOV_DEG, HEAT_MIN_DELTA_C, HEAT_MIN_ABS_C,
                                  CENTRE_DEADBAND_DEG);
      f.ms = millis();
      f.valid = true;
      if (xSemaphoreTake(g_mtx, pdMS_TO_TICKS(20)) == pdTRUE) {
        g_frame = f;
        xSemaphoreGive(g_mtx);
      }
      g_heat = f.r.found;
    } else if (++fails >= 5) {
      g_heat = false;
      haveSmooth = false;
      if (xSemaphoreTake(g_mtx, pdMS_TO_TICKS(20)) == pdTRUE) {
        g_frame.valid = false;
        xSemaphoreGive(g_mtx);
      }
      initSensor();       // try to bring the sensor back
      fails = 0;
      vTaskDelay(pdMS_TO_TICKS(400));
    }
    vTaskDelay(pdMS_TO_TICKS(THERMAL_PERIOD_MS));
  }
}

bool thermalLatest(ThermalFrame &out) {
  if (!g_mtx || xSemaphoreTake(g_mtx, pdMS_TO_TICKS(20)) != pdTRUE) return false;
  out = g_frame;
  xSemaphoreGive(g_mtx);
  return out.valid;
}

bool thermalHeatDetected() { return g_heat; }

#endif  // TERRA_ESP32S3
