/**
 * @file device_shadow.h
 * @brief Manages the local representation of the cloud device shadow.
 * 
 * The Device Shadow pattern implements a robust device twin synchronization flow:
 * 
 * 1. **Reported State:** The device continuously publishes its true operational 
 *    state (e.g., actual run mode, fault codes, safety triggers) via `getReportedJson()`.
 * 
 * 2. **Desired State:** The cloud writes to the `desired` node. The device listens 
 *    to this stream, evaluates the requested state via `evaluateDesired()`, and 
 *    generates internal commands.
 * 
 * 3. **Convergence:** The device executes the commands. If successful, the new 
 *    true state is published back to `reported`, confirming the state convergence 
 *    to the cloud.
 */
#pragma once

#include <Arduino.h>
#include "../core/app/pump_command.h"

class DeviceShadow {
public:
  /**
   * @brief Initialize the device shadow.
   */
  static void init();

  /**
   * @brief Evaluate the desired state from the cloud and generate local commands.
   * @param desiredMode The desired operational mode ("auto", "manual", "countdown", "stop").
   * @param manualDesired True if manual mode is desired.
   * @param countdownStart True if a countdown start is requested.
   * @param countdownDurationMin Countdown duration in minutes.
   * @param emergencyStop True if an emergency stop is requested.
   * @param resetStop True if a reset of the emergency stop is requested.
   * @param clearError True if clearing errors is requested.
   * @param bypassLevelSensor True if level sensor bypass is requested.
   * @param bypassFlowSensor True if flow sensor bypass is requested.
   * @param rebootDevice True if a device reboot is requested.
   */
  static void evaluateDesired(const String& desiredMode, bool manualDesired, bool countdownStart, int countdownDurationMin, bool emergencyStop, bool resetStop, bool clearError, bool bypassLevelSensor, bool bypassFlowSensor, bool rebootDevice);

  /**
   * @brief Get the reported JSON string representing the current state.
   * @return A JSON string of the reported state.
   */
  [[nodiscard]] static String getReportedJson();

  /**
   * @brief Get the generated pump command based on the desired state evaluation.
   * @return The resulting PumpCommand.
   */
  [[nodiscard]] static PumpCommand getCommand();

  /**
   * @brief Clear the current generated pump command.
   */
  static void clearCommand();
};
