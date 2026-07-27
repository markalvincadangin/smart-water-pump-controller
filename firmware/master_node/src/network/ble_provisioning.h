#pragma once

#include <Arduino.h>

class BleProvisioning {
public:
    static void init();
    static void loop();
    static void stop();
    
    // Status
    static bool isProvisioned();
    static void updateStatus(const char* status);
};
