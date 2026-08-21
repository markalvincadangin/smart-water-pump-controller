/**
 * @file cloud_manager.h
 * @brief Manages cloud connectivity and data synchronization (Firebase RTDB).
 */
#pragma once

#include <Arduino.h>

class CloudManager {
public:
  /**
   * @brief Initialize the cloud subsystem.
   */
  static void init();

  /**
   * @brief Perform periodic synchronization with the cloud.
   */
  static void sync();

  /**
   * @brief Check if the device is authenticated with the cloud.
   * @return true if authenticated, false otherwise.
   */
  [[nodiscard]] static bool isAuthenticated();

  /**
   * @brief Check if the current pairing verifier has been successfully published.
   * 
   * True only after the verifier queued for the current BLE pairing session
   * has been accepted by RTDB. The raw proof remains RAM-only.
   * 
   * @return true if the verifier is published, false otherwise.
   */
  [[nodiscard]] static bool isPairingVerifierPublished();

  /**
   * @brief Queue a pairing verifier for publication.
   * 
   * Queues a raw BLE-local proof for hashing and device-authenticated 
   * verifier publication. The raw proof never enters RTDB or logs.
   * 
   * @param rawProof The raw proof string.
   * @param purpose The purpose of the pairing (e.g., "claim").
   * @param lifetimeMs Lifetime of the verifier in milliseconds.
   */
  static void queuePairingVerifier(const String& rawProof, const String& purpose, uint32_t lifetimeMs);
    
  /**
   * @brief Push an event log to the cloud.
   * @param level The log level (e.g., "INFO", "WARN").
   * @param component The component generating the event.
   * @param code The event code.
   * @param details Human-readable event details.
   */
  static void pushCloudEvent(const String& level, const String& component, const String& code, const String& details);

  /**
   * @brief Clear the countdown desired state in the cloud.
   */
  static void clearCountdownDesiredState();

  /**
   * @brief Clear the reboot desired state in the cloud.
   * @return true if successfully cleared, false otherwise.
   */
  [[nodiscard]] static bool clearRebootDesiredState();

  /**
   * @brief Clear the error desired state in the cloud.
   */
  static void clearErrorDesiredState();

  /**
   * @brief Set the error fallback desired state in the cloud.
   */
  static void setErrorFallbackDesiredState();

private:
  /**
   * @brief Pushes current telemetry (water level, flow rate, ultrasonic).
   */
  static void pushTelemetry();

  /**
   * @brief Pushes device operational status and lifecycle state.
   */
  static void pushStatus();

  /**
   * @brief Pushes the local reported state of the device shadow.
   */
  static void pushShadow();

  /**
   * @brief Reads the desired state of the device shadow and applies it.
   */
  static void readShadow();

  /**
   * @brief Publishes device metadata (firmware version, hardware info).
   * @return true if metadata was successfully published, false otherwise.
   */
  [[nodiscard]] static bool pushMetadata();

  /**
   * @brief Fetches configuration settings from the cloud.
   */
  static void readSettings();

  /**
   * @brief Processes a pending Wi-Fi reprovisioning request.
   */
  static void processWifiReprovisionRequest();

  /**
   * @brief Processes a pending ownership pairing/transfer request.
   */
  static void processOwnershipPairingRequest();

  /**
   * @brief Pushes periodic diagnostic data (heap, WiFi RSSI, boot reason).
   */
  static void pushDiagnostics();

  /**
   * @brief Uses the bootstrap secret to authenticate via custom token.
   * @return true if successfully authenticated, false otherwise.
   */
  [[nodiscard]] static bool bootstrapAndStartFirebase();
};
