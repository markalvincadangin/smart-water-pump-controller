/**
 * @file sensor_driver.h
 * @brief High-level abstraction for local and remote sensors.
 */
#pragma once

class SensorDriver {
public:
  /**
   * @brief Initialize all sensor hardware abstractions.
   */
  static void init();

  /**
   * @brief Get the current water level percent.
   * @return Water level (0-100), or -1 if invalid/error.
   */
  static int getWaterLevelPercent();

  /**
   * @brief Get the current flow rate in liters per minute.
   * @return Flow rate in LPM.
   */
  static float getFlowRateLpm();

  /**
   * @brief Check if the ultrasonic level sensor is currently reporting valid data.
   * @return true if online, false otherwise.
   */
  static bool isUltrasonicOnline();

  /**
   * @brief Check if the flow meter is currently reporting valid data.
   * @return true if online, false otherwise.
   */
  static bool isFlowMeterOnline();
};
