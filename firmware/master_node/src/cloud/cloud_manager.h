#pragma once

#include <Arduino.h>

class CloudManager {
public:
    static void init();
    static void sync();
    static bool isAuthenticated();
    // True only after the verifier queued for the current BLE pairing session
    // has been accepted by RTDB. The raw proof remains RAM-only.
    static bool isPairingVerifierPublished();
    // Queues a raw BLE-local proof for hashing and device-authenticated verifier publication.
    // The raw proof never enters RTDB or logs.
    static void queuePairingVerifier(const String& rawProof, const String& purpose, uint32_t lifetimeMs);
    
    static void pushCloudEvent(const String& level, const String& component, const String& code, const String& details);
    static void clearCountdownDesiredState();
    static bool clearRebootDesiredState();
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
