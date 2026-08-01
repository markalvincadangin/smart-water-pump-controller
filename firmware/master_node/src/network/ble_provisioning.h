/**
 * @file ble_provisioning.h
 * @brief Manages BLE-based initial provisioning and ownership pairing.
 */
#pragma once

#include <Arduino.h>

class BleProvisioning {
public:
  /**
   * @brief Initialize and start the BLE provisioning service.
   */
  static void init();

  /**
   * @brief Periodic loop for processing queued BLE commands.
   */
  static void loop();

  /**
   * @brief Stop the BLE service and release resources.
   */
  static void stop();

  /**
   * @brief Status flags for provisioning and ownership pairing.
   */
  static bool isProvisioned();
  static bool isActive();
  static bool isOwnershipPairingActive();
  static void updateStatus(const char* status);

  /**
   * @brief Starts a restricted, non-destructive ownership-pairing session.
   * Wi-Fi, cloud ownership, and all safety state remain intact.
   * @param purpose The purpose of the pairing session (e.g. claim).
   * @param lifetimeMs The lifetime of the pairing session in milliseconds.
   * @return true if successfully started, false otherwise.
   */
  static bool startOwnershipPairing(const String& purpose, uint32_t lifetimeMs);
  static void stopOwnershipPairing();
};

/**
 * @brief Ownership pairing protocol (v1)
 *
 * get_token emits a fresh random raw proof only on the currently connected BLE session.
 * The proof is sent to the cloud as a SHA-256 verifier, never persisted or logged, and is
 * valid for one claim only for five minutes. Transfer and release reuse the same verifier
 * format with their respective purpose when temporary ownership pairing is enabled by the
 * owner-authorized maintenance flow.
 */
