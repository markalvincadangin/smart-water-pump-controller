#include "device_shadow.h"
#include "../state/state.h"
#include "../utils/app_logger.h"
#include <ArduinoJson.h>

void DeviceShadow::init() {
    LOG(APP_LOG_LEVEL_INFO, "SHADOW", "Device Shadow initialized");
}

void DeviceShadow::evaluateDesired(const String& desiredMode, bool manualDesiredInt, bool countdownStart, int countdownDurationMin, bool emergencyStop, bool resetStop, bool clearError, bool bypassLevelSensor, bool bypassFlowSensor) {
    if (resetStop) {
        if (emergencyStopLatched) {
            emergencyStopLatched = false;
            LOG(APP_LOG_LEVEL_INFO, "SHADOW", "Emergency stop reset.");
        }
    }

    if (emergencyStop) {
        if (!emergencyStopLatched) {
            emergencyStopLatched = true;
            LOG(APP_LOG_LEVEL_WARN, "SHADOW", "EMERGENCY STOP activated from cloud.");
        }
    }

    // Map bypasses
    if (cfgBypassLevelSensor != bypassLevelSensor) {
        cfgBypassLevelSensor = bypassLevelSensor;
        LOG(APP_LOG_LEVEL_INFO, "SHADOW", "Level sensor bypass -> %s", bypassLevelSensor ? "ON" : "OFF");
    }
    if (cfgBypassFlowSensor != bypassFlowSensor) {
        cfgBypassFlowSensor = bypassFlowSensor;
        LOG(APP_LOG_LEVEL_INFO, "SHADOW", "Flow sensor bypass -> %s", bypassFlowSensor ? "ON" : "OFF");
    }

    if (clearError) {
        if (isDryRunError || isOverflowError || isLevelSensorError || isFlowSensorError) {
            isDryRunError = false;
            isOverflowError = false;
            isLevelSensorError = false;
            isFlowSensorError = false;
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
        if (manualDesired != manualDesiredInt) {
            manualDesired = manualDesiredInt;
            LOG(APP_LOG_LEVEL_INFO, "SHADOW", "Manual intent changed -> %s", manualDesired ? "ON" : "OFF");
        }
    }
    
    // Evaluate countdown start
    if (pumpMode == "COUNTDOWN" && countdownStart) {
        if (!isCountdownActive) {
            isCountdownActive = true;
            int dur = countdownDurationMin > 0 ? countdownDurationMin : cfgLastCountdownDurationMin;
            countdownEndMs = millis() + (dur * 60000UL);
            LOG(APP_LOG_LEVEL_INFO, "SHADOW", "Countdown started for %d min", dur);
        }
    }
}

String DeviceShadow::getReportedJson() {
    StaticJsonDocument<256> doc;
    doc["run_mode"] = runMode;
    doc["is_running"] = isRunning;
    doc["is_error"] = (isDryRunError || isOverflowError || isLevelSensorError || isFlowSensorError);
    doc["is_overflow_error"] = isOverflowError;
    doc["emergency_stop_latched"] = emergencyStopLatched;
    
    uint32_t now = millis();
    int remainSec = 0;
    if (isCountdownActive && countdownEndMs > now) {
        remainSec = (int)((countdownEndMs - now) / 1000);
    }
    doc["countdown_remaining_sec"] = remainSec;
    
    String output;
    serializeJson(doc, output);
    return output;
}
