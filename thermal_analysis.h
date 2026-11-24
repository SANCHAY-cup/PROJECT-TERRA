#pragma once
// T.E.R.R.A. - thermal grid analysis.
// Pure C++ with NO Arduino dependency, so it can be unit-tested on a PC.
#include <stdint.h>

namespace terra {

constexpr uint8_t THERMAL_W = 8;
constexpr uint8_t THERMAL_H = 8;
constexpr uint8_t THERMAL_N = THERMAL_W * THERMAL_H;

enum class Direction : uint8_t { None, Left, Centre, Right };

struct ThermalResult {
  float     minC;       // coldest cell
  float     maxC;       // hottest cell
  float     baselineC;  // median of all cells = "room temperature"
  uint8_t   hotIndex;   // index (row*8+col) of the hottest cell
  bool      found;      // a warm target stands out from the background
  float     angleDeg;   // target bearing: negative = left, positive = right
  Direction dir;
};

inline float medianOf(const float *v, uint8_t n) {
  float s[THERMAL_N];
  for (uint8_t i = 0; i < n; i++) s[i] = v[i];
  for (uint8_t i = 1; i < n; i++) {          // insertion sort, n is tiny
    float k = s[i];
    int8_t j = (int8_t)i - 1;
    while (j >= 0 && s[j] > k) { s[j + 1] = s[j]; j--; }
    s[j + 1] = k;
  }
  return (n % 2) ? s[n / 2] : 0.5f * (s[n / 2 - 1] + s[n / 2]);
}

// grid      : 64 temperatures in degC, row-major, column 0 = left of the picture
// fovDeg    : horizontal field of view of the thermal sensor (AMG8833 = 60)
// minDeltaC : how much hotter than the background a target must be
// minAbsC   : a target must also be at least this hot in absolute terms
// deadband  : |angle| <= deadband counts as "Centre"
inline ThermalResult analyseThermal(const float *grid, float fovDeg, float minDeltaC,
                                    float minAbsC, float deadbandDeg) {
  ThermalResult r{};
  r.minC = r.maxC = grid[0];
  r.hotIndex = 0;
  for (uint8_t i = 1; i < THERMAL_N; i++) {
    if (grid[i] < r.minC) r.minC = grid[i];
    if (grid[i] > r.maxC) { r.maxC = grid[i]; r.hotIndex = i; }
  }
  r.baselineC = medianOf(grid, THERMAL_N);
  r.found = (r.maxC - r.baselineC >= minDeltaC) && (r.maxC >= minAbsC);
  r.dir = Direction::None;
  r.angleDeg = 0.0f;
  if (!r.found) return r;

  // Heat-weighted column centroid of every cell that is clearly above background.
  const float cut = r.baselineC + 0.5f * minDeltaC;
  float sum = 0.0f, sumX = 0.0f;
  for (uint8_t y = 0; y < THERMAL_H; y++) {
    for (uint8_t x = 0; x < THERMAL_W; x++) {
      float t = grid[y * THERMAL_W + x];
      if (t >= cut) { float w = t - r.baselineC; sum += w; sumX += w * x; }
    }
  }
  float cx = sumX / sum;                                   // sum > 0: hottest cell is >= cut
  r.angleDeg = (cx - (THERMAL_W - 1) * 0.5f) * (fovDeg / THERMAL_W);   // pixel-centre bearing
  float a = r.angleDeg < 0 ? -r.angleDeg : r.angleDeg;
  r.dir = (a <= deadbandDeg) ? Direction::Centre
                             : (r.angleDeg < 0 ? Direction::Left : Direction::Right);
  return r;
}

inline const char *directionName(Direction d) {
  switch (d) {
    case Direction::Left:   return "Left";
    case Direction::Centre: return "Centre";
    case Direction::Right:  return "Right";
    default:                return "No target";
  }
}

inline const char *actionFor(const ThermalResult &r) {
  switch (r.dir) {
    case Direction::Left:   return "Turn left";
    case Direction::Centre: return "Go ahead";
    case Direction::Right:  return "Turn right";
    default:                return "Searching";
  }
}

}  // namespace terra
