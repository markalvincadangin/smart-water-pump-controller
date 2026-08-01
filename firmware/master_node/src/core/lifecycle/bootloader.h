#pragma once
#include <Arduino.h>

class Bootloader {
public:
  /**
   * @brief Executes the core bootloader sequence.
   * 
   * Initializes hardware, mitigates heap fragmentation, evaluates crash loops,
   * configures the watchdog timer, and transitions the device to the appropriate 
   * network and lifecycle state.
   */
  static void executeSetup();

  /**
   * @brief Retrieves a human-readable string of the ESP32 hardware reset reason.
   * 
   * @return const char* String representing the reset cause.
   */
  static const char* getBootReasonString();

  /**
   * @brief Applies a cloud-authorized Wi-Fi recovery request.
   * 
   * Preserves device identity, ownership, and safety state, then restarts the MCU 
   * into BLE onboarding mode.
   * 
   * @param requestId The unique identifier for the reprovision request.
   * @return true if successfully applied and restarted, false if the request was invalid or already applied.
   */
  static bool applyWifiReprovisionRequest(const char* requestId);
};
