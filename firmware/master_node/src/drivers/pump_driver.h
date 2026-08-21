/**
 * @file pump_driver.h
 * @brief High-level hardware abstraction for pump control.
 */
#pragma once

class PumpDriver {
public:
  /**
   * @brief Initialize the pump driver hardware.
   */
  static void init();

  /**
   * @brief Turn the pump on.
   */
  static void turnOn();

  /**
   * @brief Turn the pump off.
   */
  static void turnOff();

  /**
   * @brief Check if the pump is currently on.
   * @return true if on, false otherwise.
   */
  static bool isOn();
};
