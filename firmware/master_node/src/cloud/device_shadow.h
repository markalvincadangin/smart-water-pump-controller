#pragma once

#include <Arduino.h>

class DeviceShadow {
public:
    static void init();
    static void evaluateDesired(bool desiredPumpState, const String& desiredMode, bool clearError);
    static String getReportedJson();
};
