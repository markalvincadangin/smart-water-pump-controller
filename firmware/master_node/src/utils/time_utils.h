/**
 * @file time_utils.h
 * @brief Wrap-safe time utilities for millis().
 * 
 * Wrap-safe helpers for millis() (uint32_t on ESP32 Arduino).
 * Elapsed: (now - start) is correct in unsigned math for intervals < ~49 days.
 * Deadlines: use signed difference so "now past deadline" works across one millis() wrap.
 */
#pragma once

#include <Arduino.h>
#include <stdint.h>

/**
 * @brief Calculate elapsed milliseconds safely across wrap-arounds.
 * @param now Current millis().
 * @param start Start millis().
 * @return Elapsed milliseconds.
 */
inline uint32_t elapsedMillis32(uint32_t now, uint32_t start) {
  return (uint32_t)(now - start);
}

/**
 * @brief Check if a deadline has been reached safely across wrap-arounds.
 * @param now Current millis().
 * @param deadline Deadline millis().
 * @return true if the deadline has been reached, false otherwise.
 */
inline bool millisDeadlineReached(uint32_t now, uint32_t deadline) {
  return (int32_t)(now - deadline) >= 0;
}

/**
 * @brief Add milliseconds to a base time with saturation at UINT32_MAX.
 * @param a Base time.
 * @param b Milliseconds to add.
 * @return The sum, saturated at UINT32_MAX.
 */
inline uint32_t addMillisSaturated(uint32_t a, uint32_t b) {
  uint32_t s = a + b;
  if (s < a) return UINT32_MAX;
  return s;
}
