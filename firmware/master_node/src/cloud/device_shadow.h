#pragma once

#include <Arduino.h>

class DeviceShadow {
public:
    static void init();
    static void evaluateDesired(const String& desiredMode, bool manualDesired, bool countdownStart, int countdownDurationMin, bool emergencyStop, bool resetStop, bool clearError, bool bypassLevelSensor, bool bypassFlowSensor);
    static String getReportedJson();
};
