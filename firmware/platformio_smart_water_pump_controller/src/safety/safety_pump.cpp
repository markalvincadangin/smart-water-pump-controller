#include "safety_pump.h"

#include "../state/state.h"
#include "../connectivity/connectivity_cloud.h"

void setPump(bool on) {
  if (on && !isRunning) {
    totalPumpCycles++;
    pumpOnSinceMs = millis();
  }
  if (!on && isRunning && pumpOnSinceMs > 0) {
    totalPumpRunSec += (millis() - pumpOnSinceMs) / 1000UL;
    pumpOnSinceMs = 0;
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
      Serial.printf("[SENSOR][ERROR] Ultrasonic (level) failure: %d consecutive timeouts.\n", levelSensorFailCount);
      pushFirebaseErrorLog("ERROR", "SENSOR_ULTRASONIC", "Ultrasonic (level) failure: consecutive timeouts.");
    }
    if (isLevelSensorError && cfgAutoBypassOnSensorFail && !cfgBypassLevelSensor) {
      if (levelSensorFailStartMs == 0) levelSensorFailStartMs = millis();
      if ((millis() - levelSensorFailStartMs) >= (unsigned long)cfgAutoBypassDelaySec * 1000UL) {
        cfgBypassLevelSensor = true;
        autoBypassWasEngaged = true;
        autoBypassActive = true;
        Serial.println("[AUTO-BYPASS] Enabled after sustained sensor failure.");
      }
    }
  } else {
    levelLastValidMs = millis();
    if (isLevelSensorError) {
      Serial.println("[SENSOR][INFO] Ultrasonic (level) recovered. Error cleared.");
    }
    levelSensorFailCount = 0;
    isLevelSensorError = false;
    levelSensorFailStartMs = 0;

    if (prevLevelError) {
      levelAnchorPct = sensorReading;
      flowVolumeAddedL = 0.0f;
      estimatedLevelPct = (float)sensorReading;
      Serial.println("[ESTIMATE] Anchor reset to recovered sensor reading.");
    } else {
      levelAnchorPct = sensorReading;
    }
    if (autoBypassWasEngaged) {
      cfgBypassLevelSensor = false;
      autoBypassWasEngaged = false;
      autoBypassActive = false;
      Serial.println("[AUTO-BYPASS] Sensor recovered. Bypass auto-disabled.");
    }
  }
}

void checkFlowSensorStuck() {
  if (cfgBypassFlowSensor) {
    flowStuckTimerActive = false;
    flowStuckStartMs = 0;
    if (isFlowSensorError) {
      isFlowSensorError = false;
      Serial.println("[SENSOR][INFO] Flow sensor error cleared (bypass active).");
    }
    return;
  }
  if (!isRunning && flowRateLpm > FLOW_STUCK_THRESHOLD_LPM) {
    if (!flowStuckTimerActive) {
      flowStuckTimerActive = true;
      flowStuckStartMs = millis();
    } else if (millis() - flowStuckStartMs >= FLOW_STUCK_TIMEOUT_MS) {
      if (!isFlowSensorError) {
        isFlowSensorError = true;
        lastFaultCode = "FLOW_SENSOR";
        lastFaultMessage = "Flow sensor reading abnormal (stuck-high while pump OFF).";
        flowStuckHighEventCount++;
        flowStuckHighEventCountWin++;
        Serial.printf("[SENSOR][ERROR] Flow stuck-high: %.1f LPM while pump OFF for >%ds.\n",
                      flowRateLpm, (int)(FLOW_STUCK_TIMEOUT_MS / 1000));
        pushFirebaseErrorLog("ERROR", "SENSOR_FLOW", "Flow stuck-high while pump OFF.");
      }
    }
  } else {
    if (isFlowSensorError) {
      Serial.println("[SENSOR][INFO] Flow sensor recovered. Error cleared.");
      isFlowSensorError = false;
    }
    flowStuckTimerActive = false;
    flowStuckStartMs = 0;
  }
}


void checkOverflowProtection() {
  if (!isRunning || !(pumpMode == "AUTO" || pumpMode == "COUNTDOWN" || pumpMode == "MANUAL")) {
    pumpAutoStartTracking = false;
    pumpAutoStartMs = 0;
    return;
  }

  if (!pumpAutoStartTracking) {
    pumpAutoStartTracking = true;
    pumpAutoStartMs = millis();
    return;
  }

  unsigned long maxRuntimeMs = (unsigned long)cfgMaxPumpRuntimeMin * 60000UL;
  unsigned long elapsed = millis() - pumpAutoStartMs;
  if (elapsed >= maxRuntimeMs) {
    isOverflowError = true;
    setPump(false);
    pumpAutoStartTracking = false;
    Serial.printf("[SAFETY][ERROR] Max runtime exceeded (%d min). Pump stopped.\n", cfgMaxPumpRuntimeMin);
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
  bool isLevelFresh = (levelLastUpdateMs > 0) && ((millis() - levelLastUpdateMs) <= LEVEL_STALE_TIMEOUT_MS);
  if (!remoteSensorStable || !isLevelFresh) {
    dryRunTimerActive = false;
    dryRunStartMs = 0;
    return;
  }

  unsigned long dryRunTimeoutMs = (unsigned long)cfgDryRunTimeoutSec * 1000UL;
  if (flowRateLpm < cfgDryRunThresholdLpm) {
    if (!dryRunTimerActive) {
      dryRunTimerActive = true;
      dryRunStartMs = millis();
      Serial.println("[SAFETY][WARN] Dry-run condition detected. Timer started.");
    } else {
      unsigned long elapsed = millis() - dryRunStartMs;
      if (elapsed >= dryRunTimeoutMs) {
        isDryRunError = true;
        // HARD SAFETY: always stop the pump regardless of mode.
        setPump(false);
        Serial.println("[SAFETY][ERROR] DRY-RUN LOCKOUT. Pump stopped; waiting for acknowledge.");
        pushFirebaseErrorLog("CRITICAL", "SAFETY_DRY_RUN", "DRY-RUN LOCKOUT. Pump stopped.");
      }
    }
  } else {
    if (dryRunTimerActive) {
      Serial.println("[SAFETY][INFO] Flow restored. Dry-run timer reset.");
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

  // Sensor validity gate: require fresh/stable remote data unless bypass is enabled.
  bool isLevelFresh = (levelLastUpdateMs > 0) && ((millis() - levelLastUpdateMs) <= LEVEL_STALE_TIMEOUT_MS);
  bool allowStartFromSensors = remoteSensorStable && isLevelFresh && !isLevelSensorError && !cfgBypassLevelSensor;

  if (pumpMode == "MANUAL" && isRunning && manualDesired) {
    runMode = "MANUAL_ON";
  } else if (pumpMode == "MANUAL") {
    runMode = "MANUAL_OFF";
  } else if (pumpMode == "COUNTDOWN" && isCountdownActive) {
    runMode = "COUNTDOWN";
  } else if (pumpMode == "COUNTDOWN" && !isCountdownActive) {
    runMode = "OFF";
  } else if (pumpMode == "AUTO" && isRunning) {
    runMode = "AUTO";
  } else if (pumpMode == "AUTO" && !isRunning) {
    runMode = "AUTO_STANDBY";
  } else {
    runMode = isRunning ? "AUTO" : "OFF";
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
        bool isLevelFresh = (levelLastUpdateMs > 0) && ((millis() - levelLastUpdateMs) <= LEVEL_STALE_TIMEOUT_MS);
        lastFaultCode = (!remoteSensorStable || !isLevelFresh) ? "COMM_LOSS" : "STALE_LEVEL";
        lastFaultMessage = "No fresh/stable level data. Pump stopped (failsafe).";
        setPump(false);
      }
      return;
    }
    if (isLevelSensorError && !cfgBypassLevelSensor) {
      if (isRunning) {
        Serial.println("[MANUAL] Level sensor error — stopping (fail-safe).");
        lastFaultCode    = "LEVEL_SENSOR";
        lastFaultMessage = "Level sensor offline: pump stopped in MANUAL (fail-safe).";
        setPump(false);
      }
      return;
    }
    if (!cfgBypassLevelSensor && waterLevelPct >= cfgPumpStopLevel) {
      Serial.printf("[MANUAL] Tank full (%d%%). Stopping pump (mode stays MANUAL).\n", waterLevelPct);
      setPump(false);
      return;
    }
    if (!isRunning && pumpOffStartMs > 0 && (millis() - pumpOffStartMs) < MIN_PUMP_OFF_TIME_MS) {
      return;
    }
    setPump(true);
    return;
  }

  if (pumpMode == "COUNTDOWN") {
    if (isCountdownActive) {
      if (!cfgBypassLevelSensor && !allowStartFromSensors) {
        bool isLevelFresh = (levelLastUpdateMs > 0) && ((millis() - levelLastUpdateMs) <= LEVEL_STALE_TIMEOUT_MS);
        lastFaultCode = (!remoteSensorStable || !isLevelFresh) ? "COMM_LOSS" : "STALE_LEVEL";
        lastFaultMessage = "No fresh/stable level data. Pump stopped (failsafe).";
        setPump(false);
        isCountdownActive = false; countdownEndMs = 0;
        // Option B: mode stays COUNTDOWN (pump idle, user must start new timer)
        return;
      }
      if (!cfgBypassLevelSensor && waterLevelPct >= cfgPumpStopLevel) {
        Serial.printf("[COUNTDOWN] Tank full (%d%%). Stopping early.\n", waterLevelPct);
        setPump(false); isCountdownActive = false; countdownEndMs = 0;
        // Option B: mode stays COUNTDOWN
        return;
      }
      if (!isRunning && pumpOffStartMs > 0 && (millis() - pumpOffStartMs) < MIN_PUMP_OFF_TIME_MS) {
        return;
      }
      setPump(true);
    }
    return;
  }

  if (isSleeping) {
    if (isRunning && waterLevelPct >= cfgPumpStopLevel) {
      Serial.printf("[AUTO] Water at %d%%. Stopping pump.\n", waterLevelPct);
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
      Serial.println("[AUTO] No fresh/stable level data — stopping pump (fail-safe).");
      bool isLevelFresh = (levelLastUpdateMs > 0) && ((millis() - levelLastUpdateMs) <= LEVEL_STALE_TIMEOUT_MS);
      lastFaultCode = (!remoteSensorStable || !isLevelFresh) ? "COMM_LOSS" : "STALE_LEVEL";
      lastFaultMessage = "No fresh/stable level data: controller stopped pump (failsafe).";
      setPump(false);
    }
    return;
  }

  if (!isRunning && waterLevelPct <= cfgPumpStartLevel) {
    if (pumpOffStartMs > 0 && (millis() - pumpOffStartMs) < MIN_PUMP_OFF_TIME_MS) {
      return;
    }
    Serial.printf("[AUTO] Water at %d%%. Starting pump.\n", waterLevelPct);
    setPump(true);
  } else if (isRunning && waterLevelPct >= cfgPumpStopLevel) {
    Serial.printf("[AUTO] Water at %d%%. Stopping pump.\n", waterLevelPct);
    setPump(false);
  }
}

