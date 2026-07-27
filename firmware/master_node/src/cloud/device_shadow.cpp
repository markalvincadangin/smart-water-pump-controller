#include "device_shadow.h"
#include "../state/state.h"
#include "../utils/app_logger.h"
#include <ArduinoJson.h>

void DeviceShadow::init() {
    LOG(APP_LOG_LEVEL_INFO, "SHADOW", "Device Shadow initialized");
}

void DeviceShadow::evaluateDesired(bool desiredPumpState, const String& desiredMode, bool clearError) {
    if (clearError) {
        if (isDryRunError || isOverflowError) {
            isDryRunError = false;
            isOverflowError = false;
            dryRunTimerActive = false;
            dryRunStartMs = 0;
            pumpAutoStartTracking = false;
            pumpAutoStartMs = 0;
            LOG(APP_LOG_LEVEL_INFO, "SHADOW", "Errors cleared via Device Shadow.");
            lastFaultCode = "";
            lastFaultMessage = "";
        }
    }

    if (emergencyStopLatched) {
        LOG(APP_LOG_LEVEL_WARN, "SHADOW", "Mode/State change blocked by E-STOP.");
        return;
    }

    String newMode = desiredMode;
    newMode.trim();
    newMode.toUpperCase();

    if (newMode == "AUTO" || newMode == "MANUAL" || newMode == "COUNTDOWN") {
        if (pumpMode != newMode) {
            LOG(APP_LOG_LEVEL_INFO, "SHADOW", "Mode changed: %s -> %s", pumpMode.c_str(), newMode.c_str());
            pumpMode = newMode;
        }
    }

    // Evaluate manual intent
    if (pumpMode == "MANUAL") {
        if (manualDesired != desiredPumpState) {
            manualDesired = desiredPumpState;
            LOG(APP_LOG_LEVEL_INFO, "SHADOW", "Manual intent changed -> %s", manualDesired ? "ON" : "OFF");
        }
    }
}

String DeviceShadow::getReportedJson() {
    StaticJsonDocument<200> doc;
    doc["pumpState"] = isRunning;
    doc["mode"] = pumpMode;
    doc["clearError"] = false; // Always report false for one-shot clear
    
    String output;
    serializeJson(doc, output);
    return output;
}
