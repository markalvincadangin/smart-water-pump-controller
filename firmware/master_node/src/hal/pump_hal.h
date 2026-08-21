/**
 * @file pump_hal.h
 * @brief Hardware Abstraction Layer for the water pump relay.
 */
#pragma once

#include <Arduino.h>

class PumpHal {
public:
  /**
   * @brief Initialize the GPIO pin for the pump relay.
   */
  static void init();

  /**
   * @brief Turn the pump on or off.
   * @param on true to enable, false to disable.
   */
  static void enable(bool on);

  /**
   * @brief Get the current physical state of the pump pin.
   * @return true if the pump is enabled, false otherwise.
   */
  static bool isEnabled();
};
