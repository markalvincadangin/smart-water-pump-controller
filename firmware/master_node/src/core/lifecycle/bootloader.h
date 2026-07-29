#pragma once
#include <Arduino.h>

class Bootloader {
public:
    static void executeSetup();
    static String getBootReasonString();
    // Applies a cloud-authorized Wi-Fi recovery request. This preserves device
    // identity, ownership, and safety state, then restarts into BLE onboarding.
    static bool applyWifiReprovisionRequest(const String& requestId);
};
