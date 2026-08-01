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
  static bool isAuthenticated();

  /**
   * @brief Check if the current pairing verifier has been successfully published.
   * 
   * True only after the verifier queued for the current BLE pairing session
   * has been accepted by RTDB. The raw proof remains RAM-only.
   * 
   * @return true if the verifier is published, false otherwise.
   */
  static bool isPairingVerifierPublished();

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
  static bool clearRebootDesiredState();

  /**
   * @brief Set the error fallback desired state in the cloud.
   */
  static void setErrorFallbackDesiredState();

private:
  static void pushTelemetry();
  static void pushStatus();
  static void pushShadow();
  static void readShadow();
  static bool pushMetadata();
  static void readSettings();
  static void processWifiReprovisionRequest();
  static void processOwnershipPairingRequest();
  static void pushDiagnostics();
  static bool bootstrapAndStartFirebase();
};
