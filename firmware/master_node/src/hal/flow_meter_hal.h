/**
 * @file flow_meter_hal.h
 * @brief Hardware Abstraction Layer for the Flow Meter.
 */
#pragma once

class FlowMeterHal {
public:
  /**
   * @brief Initialize the flow meter hardware (or its communication bus).
   */
  static void init();

  /**
   * @brief Read the current flow rate.
   * @return Current flow rate in Liters Per Minute (LPM).
   */
  static float readFlowRateLpm();

  /**
   * @brief Check if the flow meter sensor is responding.
   * @return true if online, false otherwise.
   */
  static bool isOnline();
};
