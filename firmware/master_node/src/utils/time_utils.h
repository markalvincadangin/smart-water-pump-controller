#pragma once

#include <Arduino.h>
#include <stdint.h>

// Wrap-safe helpers for millis() (uint32_t on ESP32 Arduino).
// Elapsed: (now - start) is correct in unsigned math for intervals < ~49 days.
// Deadlines: use signed difference so "now past deadline" works across one millis() wrap.

inline uint32_t elapsedMillis32(uint32_t now, uint32_t start) {
  return (uint32_t)(now - start);
}

inline bool millisDeadlineReached(uint32_t now, uint32_t deadline) {
  return (int32_t)(now - deadline) >= 0;
}

inline uint32_t addMillisSaturated(uint32_t a, uint32_t b) {
  uint32_t s = a + b;
  if (s < a) return UINT32_MAX;
  return s;
}
