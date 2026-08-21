/**
 * @file ultrasonic_hal.h
 * @brief Hardware Abstraction Layer for the ultrasonic water level sensor.
 */
#pragma once

class UltrasonicHal {
public:
  /**
   * @brief Initialize the ultrasonic hardware (or its communication bus).
   */
  static void init();

  /**
   * @brief Read the current water level.
   * @return Water level percentage (0-100).
   */
  static int readLevelPercent();

  /**
   * @brief Check if the ultrasonic sensor is responding.
   * @return true if online, false otherwise.
   */
  static bool isOnline();
};
