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


void checkOverflowProtection() {
  static bool manualRuntimeWarnLogged = false;

  if (!isRunning || !(pumpMode == "AUTO" || pumpMode == "COUNTDOWN" || pumpMode == "MANUAL")) {
    pumpAutoStartTracking = false;
    pumpAutoStartMs = 0;
    manualRuntimeWarning = false;
    manualRuntimeWarnLogged = false;
    return;
  }

  if (!pumpAutoStartTracking) {
    pumpAutoStartTracking = true;
    pumpAutoStartMs = millis();
    return;
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
    setPump(false);
    pumpAutoStartTracking = false;
    LOG(LOG_LEVEL_ERROR, "SAFETY", "[ERROR] Max runtime exceeded (%d min). Pump stopped.", cfgMaxPumpRuntimeMin);
    pushFirebaseErrorLog("CRITICAL", "SAFETY_OVERFLOW", "Max runtime exceeded. Pump stopped.");
  }
}

void checkDryRunProtection() {
  if (!isRunning) {
    dryRunTimerActive = false;
    dryRunStartMs = 0;
    return;
  }

  // If flow sensor bypass is on, skip dry-run entirely.
  if (cfgBypassFlowSensor) { dryRunTimerActive = false; dryRunStartMs = 0; return; }

  // If remote sensor data is stale/unstable, do not advance a dry-run timer.
  // Comm loss must fail-safe by stopping the pump via freshness/stability gates,
  // not by misclassifying as DRY_RUN.
  bool levelFreshGate = (levelLastUpdateMs > 0) &&
                        (elapsedMillis32(millis(), levelLastUpdateMs) <= LEVEL_STALE_TIMEOUT_MS);
  if (!remoteSensorStable || !levelFreshGate) {
    dryRunTimerActive = false;
    dryRunStartMs = 0;
    return;
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
        setPump(false);
        LOG(LOG_LEVEL_ERROR, "SAFETY", "[ERROR] DRY-RUN LOCKOUT. Pump stopped; waiting for acknowledge.");
        pushFirebaseErrorLog("CRITICAL", "SAFETY_DRY_RUN", "DRY-RUN LOCKOUT. Pump stopped.");
      }
    }
  } else {
    if (dryRunTimerActive) {
      LOG(LOG_LEVEL_INFO, "SAFETY", "[INFO] Flow restored. Dry-run timer reset.");
    }
    dryRunTimerActive = false;
    dryRunStartMs = 0;
  }
}

void checkSafetyCutoff() {
  checkDryRunProtection();
  checkFlowSensorStuck();
  checkOverflowProtection();
}

void executePumpLogic() {
  // REFACTOR [C-02]: suspend logic until first valid level.
  if (waterLevelPct < 0) return;

  isManualRun = (pumpMode == "MANUAL");

  // Emergency stop latch: highest priority.
  if (emergencyStopLatched) {
    lastFaultCode    = "E_STOP";
    lastFaultMessage = "Emergency stop is latched. Reset stop to resume operation.";
    setPump(false);
    return;
  }

  // Hard safety lockouts: always stop.
  if (isDryRunError || isOverflowError) {
    lastFaultCode    = isDryRunError ? "DRY_RUN" : "OVERFLOW";
    lastFaultMessage = isDryRunError
      ? "Dry-run lockout: low flow while pump was running."
      : "Overflow protection: max runtime exceeded.";
    setPump(false);
    if (isCountdownActive) {
      isCountdownActive = false; countdownEndMs = 0;
      // Option B: don't flip pumpMode — stays COUNTDOWN (pump stays off idle)
    }
    return;
  }

  // Sensor validity gate: single freshness snapshot for this loop (M-26).
  uint32_t nowMsPump = millis();
  const bool levelFreshOk = (levelLastUpdateMs > 0) &&
    (elapsedMillis32(nowMsPump, levelLastUpdateMs) <= LEVEL_STALE_TIMEOUT_MS);
  bool allowStartFromSensors = remoteSensorStable && levelFreshOk && !isLevelSensorError && !cfgBypassLevelSensor;

  // REFACTOR [H-07]: expose cooldown mode while off-timer is active.
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

  if (pumpMode == "MANUAL") {
    // MANUAL is intent-based; manualDesired=false keeps the pump OFF (mode stays MANUAL).
    if (!manualDesired) {
      setPump(false);
      return;
    }

    // In MANUAL, we still require fresh/stable level data when bypass is OFF.
    if (!cfgBypassLevelSensor && !allowStartFromSensors) {
      if (isRunning) {
        lastFaultCode = (!remoteSensorStable || !levelFreshOk) ? "COMM_LOSS" : "STALE_LEVEL";
        lastFaultMessage = "No fresh/stable level data. Pump stopped (failsafe).";
        setPump(false);
      }
      return;
    }
    if (isLevelSensorError && !cfgBypassLevelSensor) {
      if (isRunning) {
        LOG(LOG_LEVEL_ERROR, "MANUAL", "Level sensor error — stopping (fail-safe).");
        lastFaultCode    = "LEVEL_SENSOR";
        lastFaultMessage = "Level sensor offline: pump stopped in MANUAL (fail-safe).";
        setPump(false);
      }
      return;
    }
    if (!cfgBypassLevelSensor && waterLevelPct >= cfgPumpStopLevel) {
      LOG(LOG_LEVEL_INFO, "MANUAL", "Tank full (%d%%). Stopping pump (mode stays MANUAL).", waterLevelPct);
      setPump(false);
      return;
    }
    if (!isRunning && pumpOffStartMs > 0 && elapsedMillis32(nowMsPump, pumpOffStartMs) < MIN_PUMP_OFF_TIME_MS) {
      return;
    }
    setPump(true);
    return;
  }

  if (pumpMode == "COUNTDOWN") {
    if (isCountdownActive) {
      if (!cfgBypassLevelSensor && !allowStartFromSensors) {
        lastFaultCode = (!remoteSensorStable || !levelFreshOk) ? "COMM_LOSS" : "STALE_LEVEL";
        lastFaultMessage = "No fresh/stable level data. Pump stopped (failsafe).";
        setPump(false);
        isCountdownActive = false; countdownEndMs = 0;
        // Option B: mode stays COUNTDOWN (pump idle, user must start new timer)
        return;
      }
      if (!cfgBypassLevelSensor && waterLevelPct >= cfgPumpStopLevel) {
        LOG(LOG_LEVEL_INFO, "COUNTDOWN", "Tank full (%d%%). Stopping early.", waterLevelPct);
        setPump(false); isCountdownActive = false; countdownEndMs = 0;
        // Option B: mode stays COUNTDOWN
        return;
      }
      if (!isRunning && pumpOffStartMs > 0 && elapsedMillis32(nowMsPump, pumpOffStartMs) < MIN_PUMP_OFF_TIME_MS) {
        return;
      }
      setPump(true);
    } else {
      setPump(false);
    }
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

