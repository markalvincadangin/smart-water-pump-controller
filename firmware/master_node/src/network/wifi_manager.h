/**
 * @file wifi_manager.h
 * @brief Manages Wi-Fi connectivity and lifecycle.
 */
#pragma once

class WifiManager {
public:
  /**
   * @brief Initialize the Wi-Fi subsystem.
   */
  static void init();

  /**
   * @brief Connect to the Wi-Fi network using credentials from NVS.
   */
  static void connect();

  /**
   * @brief Periodic loop to monitor and maintain the Wi-Fi connection.
   */
  static void loop();

  /**
   * @brief Check if the device is currently connected to Wi-Fi.
   * @return true if connected, false otherwise.
   */
  [[nodiscard]] static bool isConnected();

  /**
   * @brief Get the current Received Signal Strength Indicator (RSSI).
   * @return RSSI in dBm.
   */
  [[nodiscard]] static int getRssi();
};
