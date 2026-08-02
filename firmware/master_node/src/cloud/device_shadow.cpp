/**
 * @file device_shadow.cpp
 * @brief Manages the local representation of the cloud device shadow.
 */
#include "device_shadow.h"
#include "../state/state.h"
#include "../utils/app_logger.h"
#include <ArduinoJson.h>

namespace {
  String shadowLastMode = "";
  bool shadowLastManualDesired = false;
  bool shadowLastCountdownStart = false;
  PumpCommand activeCommand(CommandType::NONE);

  // Cached state for getReportedJson to prevent heap fragmentation
  String prevRunMode = "";
  bool prevIsRunning = false;
  bool prevIsError = false;
  bool prevIsOverflowError = false;
  bool prevEmergencyStopLatched = false;
  int prevRemainSec = -1;
  String prevLastFaultMessage = "";
  String cachedReportedJson = "{}";
}

void DeviceShadow::init() {
  LOG(APP_LOG_LEVEL_INFO, "SHADOW", "Device Shadow initialized");
  cachedReportedJson.reserve(256);
}

/**
 * @brief Evaluates desired state fields from the cloud and maps them to local pump commands.
 * 
 * Side-effects:
 * - May mutate `cfgBypassLevelSensor` and `cfgBypassFlowSensor`.
 * - May latch or unlatch `emergencyStopLatched`.
 * - Populates the `activeCommand` variable for the main loop to process.
 * - Updates local shadow state cache (`shadowLastMode`, etc.) for edge detection.
 */
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

  String newMode = desiredMode;
  newMode.trim();
  newMode.toUpperCase();

  bool modeChanged = (shadowLastMode != newMode);
  bool manualEdge = (newMode == "MANUAL" && manualDesiredInt != shadowLastManualDesired);
  bool countdownEdge = (newMode == "COUNTDOWN" && countdownStart != shadowLastCountdownStart);

  if (clearError) {
    activeCommand.type = CommandType::CLEAR_ERROR;
    LOG(APP_LOG_LEVEL_INFO, "SHADOW", "Cloud requested error clear.");
    return;
  }

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
  bool isError = (currentState == PumpState::ERROR) || isDryRunError || isOverflowError || isLevelSensorError || isFlowSensorError;
  
  uint32_t now = millis();
  int remainSec = 0;
  if (isCountdownActive && countdownEndMs > now) {
    remainSec = (int)((countdownEndMs - now) / 1000);
  }

  if (runMode == prevRunMode &&
      isRunning == prevIsRunning &&
      isError == prevIsError &&
      isOverflowError == prevIsOverflowError &&
      emergencyStopLatched == prevEmergencyStopLatched &&
      remainSec == prevRemainSec &&
      lastFaultMessage == prevLastFaultMessage) {
    return cachedReportedJson;
  }

  prevRunMode = runMode;
  prevIsRunning = isRunning;
  prevIsError = isError;
  prevIsOverflowError = isOverflowError;
  prevEmergencyStopLatched = emergencyStopLatched;
  prevRemainSec = remainSec;
  prevLastFaultMessage = lastFaultMessage;

  /*
   * Construct the JSON structure for the reported state.
   * Expected schema for RTDB /shadow/reported node:
   * {
   *   "run_mode": string,                 // Current active logical mode
   *   "is_running": boolean,              // Hardware pump state (true=ON)
   *   "is_error": boolean,                // General fault indicator
   *   "is_overflow_error": boolean,       // Specific overflow fault
   *   "emergency_stop_latched": boolean,  // True if E-STOP is engaged
   *   "countdown_remaining_sec": int,     // Remaining seconds if countdown active, else 0
   *   "last_fault_message": string        // Human-readable fault description
   * }
   */
  StaticJsonDocument<256> doc;
  doc["run_mode"] = runMode;
  doc["is_running"] = isRunning;
  doc["is_error"] = isError;
  doc["is_overflow_error"] = isOverflowError;
  doc["emergency_stop_latched"] = emergencyStopLatched;
  doc["countdown_remaining_sec"] = remainSec;
  doc["last_fault_message"] = lastFaultMessage;
    
  String output;
  output.reserve(256);
  serializeJson(doc, output);
  cachedReportedJson = output;
  return output;
}
