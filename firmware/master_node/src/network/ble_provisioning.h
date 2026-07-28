#pragma once

#include <Arduino.h>

class BleProvisioning {
public:
    static void init();
    static void loop();
    static void stop();
    
    // Status
    static bool isProvisioned();
    static bool isActive();
    static bool isOwnershipPairingActive();
    static void updateStatus(const char* status);
    // Starts a restricted, non-destructive ownership-pairing session. Wi-Fi,
    // cloud ownership, and all safety state remain intact.
    static bool startOwnershipPairing(const String& purpose, uint32_t lifetimeMs);
    static void stopOwnershipPairing();
};

// Ownership pairing protocol (v1): get_token emits a fresh random raw proof
// only on the currently connected BLE session. The proof is sent to the cloud
// as a SHA-256 verifier, never persisted or logged, and is valid for one
// claim only for five minutes. Transfer and release reuse the same verifier
// format with their respective purpose when temporary ownership pairing is
// enabled by the owner-authorized maintenance flow.
