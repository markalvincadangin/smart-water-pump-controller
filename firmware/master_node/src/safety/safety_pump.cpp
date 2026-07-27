#include "safety_pump.h"

#include "../state/state.h"
#include "../connectivity/connectivity_cloud.h"
#include "../utils/time_utils.h"

void setPump(bool on) {
  if (on == isRunning) return;

  if (on && !isRunning) {
    totalPumpCycles++;
    pumpOnSinceMs = millis();
    LOG(LOG_LEVEL_INFO, "PUMP", "Relay ENERGIZED. Pump is now ON.");
  }
  if (!on && isRunning && pumpOnSinceMs > 0) {
    totalPumpRunSec += (millis() - pumpOnSinceMs) / 1000UL;
    pumpOnSinceMs = 0;
    LOG(LOG_LEVEL_INFO, "PUMP", "Relay DE-ENERGIZED. Pump is now OFF.");
  }

  digitalWrite(RELAY_PIN, on ? LOW : HIGH);
  isRunning = on;
  if (!on) {
    pumpOffStartMs = millis();
  } else {
    pumpOffStartMs = 0;
  }
}

void checkLevelSensorFailure(int sensorReading) {
  bool prevLevelError = isLevelSensorError;

  if (sensorReading == -1) {
    levelSensorFailCount++;
    if (levelSensorFailCount >= cfgLevelSensorFailureThreshold && !isLevelSensorError) {
      isLevelSensorError = true;
      LOG(LOG_LEVEL_ERROR, "SENSOR", "[ERROR] Ultrasonic (level) failure: %d consecutive timeouts.", levelSensorFailCount);
      pushFirebaseErrorLog("ERROR", "SENSOR_ULTRASONIC", "Ultrasonic (level) failure: consecutive timeouts.");
    }
    if (isLevelSensorError && cfgAutoBypassOnSensorFail && !cfgBypassLevelSensor) {
      if (levelSensorFailStartMs == 0) levelSensorFailStartMs = millis();
      if (elapsedMillis32(millis(), levelSensorFailStartMs) >= (uint32_t)cfgAutoBypassDelaySec * 1000UL) {
        cfgBypassLevelSensor = true;
        autoBypassWasEngaged = true;
        autoBypassActive = true;
        LOG(LOG_LEVEL_ERROR, "AUTO-BYPASS", "Enabled after sustained sensor failure.");
      }
    }
  } else {
    levelLastValidMs = millis();
    if (isLevelSensorError) {
      LOG(LOG_LEVEL_ERROR, "SENSOR", "[INFO] Ultrasonic (level) recovered. Error cleared.");
    }
    levelSensorFailCount = 0;
    isLevelSensorError = false;
    levelSensorFailStartMs = 0;

    if (prevLevelError) {
      levelAnchorPct = sensorReading;
      flowVolumeAddedL = 0.0f;
      estimatedLevelPct = (float)sensorReading;
      LOG(LOG_LEVEL_INFO, "ESTIMATE", "Anchor reset to recovered sensor reading.");
    } else {
      levelAnchorPct = sensorReading;
    }
    if (autoBypassWasEngaged) {
      cfgBypassLevelSensor = false;
      autoBypassWasEngaged = false;
      autoBypassActive = false;
      LOG(LOG_LEVEL_INFO, "AUTO-BYPASS", "Sensor recovered. Bypass auto-disabled.");
    }
  }
}

void checkFlowSensorStuck() {
  if (cfgBypassFlowSensor) {
    flowStuckTimerActive = false;
    flowStuckStartMs = 0;
    if (isFlowSensorError) {
      isFlowSensorError = false;
      LOG(LOG_LEVEL_ERROR, "SENSOR", "[INFO] Flow sensor error cleared (bypass active).");
    }
    return;
  }
  if (!isRunning && flowRateLpm > FLOW_STUCK_THRESHOLD_LPM) {
    if (!flowStuckTimerActive) {
      flowStuckTimerActive = true;
      flowStuckStartMs = millis();
    } else if (elapsedMillis32(millis(), flowStuckStartMs) >= FLOW_STUCK_TIMEOUT_MS) {
      if (!isFlowSensorError) {
        isFlowSensorError = true;
        lastFaultCode = "FLOW_SENSOR";
        lastFaultMessage = "Flow sensor reading abnormal (stuck-high while pump OFF).";
        flowStuckHighEventCount++;
        flowStuckHighEventCountWin++;
        LOG(LOG_LEVEL_ERROR, "SENSOR", "[ERROR] Flow stuck-high: %.1f LPM while pump OFF for >%ds.", flowRateLpm, (int)(FLOW_STUCK_TIMEOUT_MS / 1000));
        pushFirebaseErrorLog("ERROR", "SENSOR_FLOW", "Flow stuck-high while pump OFF.");
      }
    }
  } else {
    if (isFlowSensorError) {
      LOG(LOG_LEVEL_ERROR, "SENSOR", "[INFO] Flow sensor recovered. Error cleared.");
      isFlowSensorError = false;
    }
    flowStuckTimerActive = false;
    flowStuckStartMs = 0;
  }
}


SafetyDecision checkOverflowProtection() {
  static bool manualRuntimeWarnLogged = false;

  if (!isRunning || !(pumpMode == "AUTO" || pumpMode == "COUNTDOWN" || pumpMode == "MANUAL")) {
    pumpAutoStartTracking = false;
    pumpAutoStartMs = 0;
    manualRuntimeWarning = false;
    manualRuntimeWarnLogged = false;
    return SafetyDecision::OK;
  }

  if (!pumpAutoStartTracking) {
    pumpAutoStartTracking = true;
    pumpAutoStartMs = millis();
    return SafetyDecision::OK;
  }

  uint32_t maxRuntimeMs = (uint32_t)cfgMaxPumpRuntimeMin * 60000UL;
  uint32_t elapsed = elapsedMillis32(millis(), pumpAutoStartMs);

  // Pre-warning for manual operation before hard cutoff.
  if (pumpMode == "MANUAL") {
    unsigned long warnThresholdMs = (maxRuntimeMs * 9UL) / 10UL;
    bool shouldWarn = elapsed >= warnThresholdMs;
    manualRuntimeWarning = shouldWarn;
    if (shouldWarn && !manualRuntimeWarnLogged) {
      manualRuntimeWarnLogged = true;
      LOG(LOG_LEVEL_WARN, "SAFETY", "[WARN] Manual runtime reached 90%% of limit (%d min).", cfgMaxPumpRuntimeMin);
    }
  } else {
    manualRuntimeWarning = false;
    manualRuntimeWarnLogged = false;
  }

  if (elapsed >= maxRuntimeMs) {
    isOverflowError = true;
    pumpAutoStartTracking = false;
    LOG(LOG_LEVEL_ERROR, "SAFETY", "[ERROR] Max runtime exceeded (%d min). Pump stopped.", cfgMaxPumpRuntimeMin);
    pushFirebaseErrorLog("CRITICAL", "SAFETY_OVERFLOW", "Max runtime exceeded. Pump stopped.");
    return SafetyDecision::STOP_OVERFLOW;
  }
  return SafetyDecision::OK;
}

SafetyDecision checkDryRunProtection() {
  // If flow sensor bypass is on, clear any existing dry-run lockout and skip the check.
  // Bypass must recover the controller as well as prevent new dry-run trips.
  if (cfgBypassFlowSensor) {
    dryRunTimerActive = false;
    dryRunStartMs = 0;
    if (isDryRunError) {
      isDryRunError = false;
      lastFaultCode = "";
      lastFaultMessage = "";
      LOG(LOG_LEVEL_INFO, "SAFETY", "Flow bypass active. Clearing dry-run lockout.");
    }
    return SafetyDecision::OK;
  }

  if (!isRunning) {
    dryRunTimerActive = false;
    dryRunStartMs = 0;
    return SafetyDecision::OK;
  }

  // If remote sensor data is stale/unstable, do not advance a dry-run timer.
  // Comm loss must fail-safe by stopping the pump via freshness/stability gates,
  // not by misclassifying as DRY_RUN.
  bool levelFreshGate = (levelLastUpdateMs > 0) &&
                        (elapsedMillis32(millis(), levelLastUpdateMs) <= LEVEL_STALE_TIMEOUT_MS);
  if (!remoteSensorStable || !levelFreshGate) {
    dryRunTimerActive = false;
    dryRunStartMs = 0;
    return SafetyDecision::OK;
  }

  unsigned long dryRunTimeoutMs = (unsigned long)cfgDryRunTimeoutSec * 1000UL;
  if (flowRateLpm < cfgDryRunThresholdLpm) {
    if (!dryRunTimerActive) {
      dryRunTimerActive = true;
      dryRunStartMs = millis();
      LOG(LOG_LEVEL_WARN, "SAFETY", "[WARN] Dry-run condition detected. Timer started.");
    } else {
      uint32_t elapsedDr = elapsedMillis32(millis(), dryRunStartMs);
      if (elapsedDr >= dryRunTimeoutMs) {
        isDryRunError = true;
        // HARD SAFETY: always stop the pump regardless of mode.
        LOG(LOG_LEVEL_ERROR, "SAFETY", "[ERROR] DRY-RUN LOCKOUT. Pump stopped; waiting for acknowledge.");
        pushFirebaseErrorLog("CRITICAL", "SAFETY_DRY_RUN", "DRY-RUN LOCKOUT. Pump stopped.");
        return SafetyDecision::STOP_DRYRUN;
      }
    }
  } else {
    if (dryRunTimerActive) {
      LOG(LOG_LEVEL_INFO, "SAFETY", "[INFO] Flow restored. Dry-run timer reset.");
    }
    dryRunTimerActive = false;
    dryRunStartMs = 0;
  }
  return SafetyDecision::OK;
}

SafetyDecision checkSafetyCutoff() {
  SafetyDecision dryRunDecision = checkDryRunProtection();
  checkFlowSensorStuck();
  SafetyDecision overflowDecision = checkOverflowProtection();

  if (dryRunDecision != SafetyDecision::OK) return dryRunDecision;
  if (overflowDecision != SafetyDecision::OK) return overflowDecision;
  return SafetyDecision::OK;
}

void executePumpLogic() {
  // Suspend control decisions until the first valid level sample.
  if (waterLevelPct < 0) return;

  isManualRun = (pumpMode == "MANUAL");

  // Emergency stop latch: highest priority.
  if (emergencyStopLatched) {
    lastFaultCode    = "E_STOP";
    lastFaultMessage = "Emergency stop is latched. Reset stop to resume operation.";
    setPump(false);
    return;
  }

  // Hard safety lockouts: evaluate decisions.
  SafetyDecision cutoffDecision = checkSafetyCutoff();
  if (cutoffDecision == SafetyDecision::STOP_DRYRUN || isDryRunError) {
    lastFaultCode    = "DRY_RUN";
    lastFaultMessage = "Dry-run lockout: low flow while pump was running.";
    setPump(false);
    if (isCountdownActive) {
      isCountdownActive = false; countdownEndMs = 0;
      // Keep COUNTDOWN mode active; pump remains off until user starts again.
    }
    return;
  }
  
  if (cutoffDecision == SafetyDecision::STOP_OVERFLOW || isOverflowError) {
    lastFaultCode    = "OVERFLOW";
    lastFaultMessage = "Overflow protection: max runtime exceeded.";
    setPump(false);
    if (isCountdownActive) {
      isCountdownActive = false; countdownEndMs = 0;
      // Keep COUNTDOWN mode active; pump remains off until user starts again.
    }
    return;
  }

  // Sensor validity gate: use one freshness snapshot per loop.
  uint32_t nowMsPump = millis();
  const bool levelFreshOk = (levelLastUpdateMs > 0) &&
    (elapsedMillis32(nowMsPump, levelLastUpdateMs) <= LEVEL_STALE_TIMEOUT_MS);
  bool allowStartFromSensors = remoteSensorStable && levelFreshOk && !isLevelSensorError && !cfgBypassLevelSensor;

  // Expose cooldown mode while off-timer is active.
  if (!isRunning && pumpOffStartMs > 0 && elapsedMillis32(nowMsPump, pumpOffStartMs) < MIN_PUMP_OFF_TIME_MS) {
    offTimerActive = true;
    offTimerEndMs = pumpOffStartMs + MIN_PUMP_OFF_TIME_MS;
    uint32_t remainMs = (offTimerEndMs > nowMsPump) ? (uint32_t)(offTimerEndMs - nowMsPump) : 0;
    pumpCooldownRemainingSec = (int)(remainMs / 1000UL);
    runMode = (pumpMode == "MANUAL") ? "MANUAL_COOLDOWN" : (pumpMode == "COUNTDOWN" ? "COUNTDOWN" : "AUTO_COOLDOWN");
  } else {
    offTimerActive = false;
    offTimerEndMs = 0;
    pumpCooldownRemainingSec = 0;

    if (pumpMode == "MANUAL" && isRunning && manualDesired) {
      runMode = "MANUAL_ON";
    } else if (pumpMode == "MANUAL") {
      runMode = "MANUAL_OFF";
    } else if (pumpMode == "COUNTDOWN" && isCountdownActive) {
      runMode = "COUNTDOWN";
    } else if (pumpMode == "COUNTDOWN" && !isCountdownActive) {
      runMode = "COUNTDOWN";
    } else if (pumpMode == "AUTO" && isRunning) {
      runMode = "AUTO";
    } else if (pumpMode == "AUTO" && !isRunning) {
      runMode = "AUTO_STANDBY";
    } else {
      runMode = isRunning ? "AUTO" : "AUTO_STANDBY";
    }
  }

  if (pumpMode == "MANUAL" || pumpMode == "COUNTDOWN") {
    bool isIntentActive = (pumpMode == "MANUAL") ? manualDesired : isCountdownActive;
    
    if (!isIntentActive) {
      setPump(false);
      return;
    }

    // Common safety checks for MANUAL and COUNTDOWN
    if (!cfgBypassLevelSensor && !allowStartFromSensors) {
      if (isRunning) {
        lastFaultCode = (!remoteSensorStable || !levelFreshOk) ? "COMM_LOSS" : "STALE_LEVEL";
        lastFaultMessage = "No fresh/stable level data. Pump stopped (failsafe).";
        setPump(false);
      }
      if (pumpMode == "COUNTDOWN") {
        isCountdownActive = false; countdownEndMs = 0;
      }
      return;
    }

    if (isLevelSensorError && !cfgBypassLevelSensor) {
      if (isRunning) {
        LOG(LOG_LEVEL_ERROR, pumpMode.c_str(), "Level sensor error — stopping (fail-safe).");
        lastFaultCode    = "LEVEL_SENSOR";
        lastFaultMessage = "Level sensor offline: pump stopped (fail-safe).";
        setPump(false);
      }
      if (pumpMode == "COUNTDOWN") {
        isCountdownActive = false; countdownEndMs = 0;
      }
      return;
    }

    if (!cfgBypassLevelSensor && waterLevelPct >= cfgPumpStopLevel) {
      LOG(LOG_LEVEL_INFO, pumpMode.c_str(), "Tank full (%d%%). Stopping pump.", waterLevelPct);
      setPump(false);
      if (pumpMode == "COUNTDOWN") {
        isCountdownActive = false; countdownEndMs = 0;
      }
      return;
    }

    // Minimum off-time check
    if (!isRunning && pumpOffStartMs > 0 && elapsedMillis32(nowMsPump, pumpOffStartMs) < MIN_PUMP_OFF_TIME_MS) {
      return;
    }
    
    setPump(true);
    return;
  }

  if (isSleeping) {
    if (isRunning && waterLevelPct >= cfgPumpStopLevel) {
      LOG(LOG_LEVEL_INFO, "AUTO", "Water at %d%%. Stopping pump.", waterLevelPct);
      setPump(false);
    }
    return;
  }

  if (cfgBypassLevelSensor) {
    return;
  }

  // HARD SAFETY: stale or unstable comm => pump OFF and block start.
  if (!allowStartFromSensors) {
    if (isRunning) {
      LOG(LOG_LEVEL_ERROR, "AUTO", "No fresh/stable level data — stopping pump (fail-safe).");
      lastFaultCode = (!remoteSensorStable || !levelFreshOk) ? "COMM_LOSS" : "STALE_LEVEL";
      lastFaultMessage = "No fresh/stable level data: controller stopped pump (failsafe).";
      setPump(false);
    }
    return;
  }

  if (!isRunning && waterLevelPct <= cfgPumpStartLevel) {
    if (pumpOffStartMs > 0 && elapsedMillis32(nowMsPump, pumpOffStartMs) < MIN_PUMP_OFF_TIME_MS) {
      return;
    }
    LOG(LOG_LEVEL_INFO, "AUTO", "Water at %d%%. Starting pump.", waterLevelPct);
    setPump(true);
  } else if (isRunning && waterLevelPct >= cfgPumpStopLevel) {
    LOG(LOG_LEVEL_INFO, "AUTO", "Water at %d%%. Stopping pump.", waterLevelPct);
    setPump(false);
  }
}

