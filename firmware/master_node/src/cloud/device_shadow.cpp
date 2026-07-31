#include "device_shadow.h"
#include "../state/state.h"
#include "../utils/app_logger.h"
#include <ArduinoJson.h>

static String shadowLastMode = "";
static bool shadowLastManualDesired = false;
static bool shadowLastCountdownStart = false;
static PumpCommand activeCommand(CommandType::NONE);

void DeviceShadow::init() {
    LOG(APP_LOG_LEVEL_INFO, "SHADOW", "Device Shadow initialized");
}

void DeviceShadow::evaluateDesired(const String& desiredMode, bool manualDesiredInt, bool countdownStart, int countdownDurationMin, bool emergencyStop, bool resetStop, bool clearError, bool bypassLevelSensor, bool bypassFlowSensor, bool rebootDevice) {
    if (rebootDevice) {
        if (millis() > 15000) {
            activeCommand.type = CommandType::REBOOT_DEVICE;
            LOG(APP_LOG_LEVEL_INFO, "SHADOW", "Cloud requested device reboot.");
        } else {
            LOG(APP_LOG_LEVEL_WARN, "SHADOW", "Cloud requested reboot, but ignored due to early boot time guard.");
        }
        return; // Prioritize reboot above all other intent changes
    }

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
        // We emit CLEAR_ERROR immediately if it's true, but we should probably debounce it.
        // For simplicity, if we receive clearError and the state has errors, we can emit the command.
        if (isDryRunError || isOverflowError || isLevelSensorError || isFlowSensorError) {
            activeCommand.type = CommandType::CLEAR_ERROR;
            LOG(APP_LOG_LEVEL_INFO, "SHADOW", "Cloud requested error clear.");
            // We do NOT clear the state here anymore. The state machine will handle it.
        }
    }

    if (emergencyStopLatched) {
        LOG(APP_LOG_LEVEL_WARN, "SHADOW", "Mode/State change blocked by E-STOP.");
        return;
    }

    String newMode = desiredMode;
    newMode.trim();
    newMode.toUpperCase();

    // Edge detection for mode and intents
    bool modeChanged = (shadowLastMode != newMode);
    bool manualEdge = (newMode == "MANUAL" && manualDesiredInt != shadowLastManualDesired);
    bool countdownEdge = (newMode == "COUNTDOWN" && countdownStart != shadowLastCountdownStart);

    if (modeChanged || manualEdge || countdownEdge) {
        LOG(APP_LOG_LEVEL_INFO, "SHADOW", "Cloud intent change detected -> Mode: %s, Manual: %d, C/D: %d", newMode.c_str(), manualDesiredInt, countdownStart);
        
        if (newMode == "MANUAL") {
            if (manualDesiredInt) {
                activeCommand = PumpCommand(CommandType::START_MANUAL);
            } else {
                activeCommand = PumpCommand(CommandType::STOP_MANUAL);
            }
        } else if (newMode == "COUNTDOWN") {
            if (countdownStart) {
                uint32_t dur = countdownDurationMin > 0 ? countdownDurationMin : cfgLastCountdownDurationMin;
                activeCommand = PumpCommand(CommandType::START_COUNTDOWN, dur * 60);
            } else {
                activeCommand = PumpCommand(CommandType::STOP_COUNTDOWN);
            }
        } else {
            // "AUTO" or other
            activeCommand = PumpCommand(CommandType::STOP_AUTO);
        }

        shadowLastMode = newMode;
        shadowLastManualDesired = manualDesiredInt;
        shadowLastCountdownStart = countdownStart;
    }
}

PumpCommand DeviceShadow::getCommand() {
    return activeCommand;
}

void DeviceShadow::clearCommand() {
    activeCommand.type = CommandType::NONE;
}

String DeviceShadow::getReportedJson() {
    StaticJsonDocument<256> doc;
    doc["run_mode"] = runMode;
    doc["is_running"] = isRunning;
    doc["is_error"] = (currentState == PumpState::ERROR) || isDryRunError || isOverflowError || isLevelSensorError || isFlowSensorError;
    doc["is_overflow_error"] = isOverflowError;
    doc["emergency_stop_latched"] = emergencyStopLatched;
    
    uint32_t now = millis();
    int remainSec = 0;
    if (isCountdownActive && countdownEndMs > now) {
        remainSec = (int)((countdownEndMs - now) / 1000);
    }
    doc["countdown_remaining_sec"] = remainSec;
    doc["last_fault_message"] = lastFaultMessage;
    
    String output;
    serializeJson(doc, output);
    return output;
}
