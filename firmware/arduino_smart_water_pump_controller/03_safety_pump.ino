// -----------------------------------------------------------------------------
// Pump control + safety
// -----------------------------------------------------------------------------

// ---- Relay control ----

// Relay module is active-low: LOW = pump ON, HIGH = pump OFF.
// v3.0: Track cycles and runtime for telemetry (persisted in NVS).
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

// ---- Safety checks ----

/**
 * @brief Checks for ultrasonic (level) sensor failure.
 *        After cfgLevelSensorFailureThreshold consecutive timeouts, flags isLevelSensorError.
 *        v3.0: Updates levelLastValidMs, anchor reset on recovery, optional auto-bypass.
 */
void checkLevelSensorFailure(int sensorReading) {
  bool prevLevelError = isLevelSensorError;

  if (sensorReading == -1) {
    levelSensorFailCount++;
    if (levelSensorFailCount >= cfgLevelSensorFailureThreshold && !isLevelSensorError) {
      isLevelSensorError = true;
      Serial.printf("[SENSOR][ERROR] Ultrasonic (level) failure: %d consecutive timeouts.\n", levelSensorFailCount);
    }
    // Auto-bypass after sustained failure (optional, default off)
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
      levelAnchorPct = sensorReading;  // Keep anchor current for next estimate
    }
    if (autoBypassWasEngaged) {
      cfgBypassLevelSensor = false;
      autoBypassWasEngaged = false;
      autoBypassActive = false;
      Serial.println("[AUTO-BYPASS] Sensor recovered. Bypass auto-disabled.");
    }
  }
}

/**
 * @brief Detects flow sensor stuck-high condition.
 *        If pump is OFF but flow > FLOW_STUCK_THRESHOLD_LPM for > FLOW_STUCK_TIMEOUT_MS,
 *        flags isFlowSensorError.
 */
void checkFlowSensorStuck() {
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
      }
    }
  } else {
    // Flow is normal (pump OFF with low flow, or pump ON with any flow)
    if (isFlowSensorError) {
      Serial.println("[SENSOR][INFO] Flow sensor recovered. Error cleared.");
      isFlowSensorError = false;
    }
    flowStuckTimerActive = false;
    flowStuckStartMs = 0;
  }
}

/**
 * @brief Checks for pump overflow (max runtime exceeded in AUTO mode).
 *        If pump has been running continuously > cfgMaxPumpRuntimeMin without
 *        reaching the stop level, flag overflow error and stop pump.
 */
void checkOverflowProtection() {
  if (!isRunning || !(pumpMode == "AUTO" || pumpMode == "COUNTDOWN" || pumpMode == "MANUAL")) {
    pumpAutoStartTracking = false;
    pumpAutoStartMs = 0;
    return;
  }

  if (!pumpAutoStartTracking) {
    // Pump just started in AUTO — begin tracking
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
  }
}

/**
 * @brief Monitors flow rate while pump is active (dry-run detection).
 *        Triggers isDryRunError = true and kills relay after cfgDryRunTimeoutSec
 *        of sustained low-flow. Reset only via Firebase clear_error signal.
 */
void checkDryRunProtection() {
  if (!isRunning) {
    dryRunTimerActive = false;
    dryRunStartMs = 0;
    return;
  }

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

/**
 * @brief Master safety check — runs all safety sub-checks.
 */
void checkSafetyCutoff() {
  checkDryRunProtection();
  checkFlowSensorStuck();
  checkOverflowProtection();
}

// ---- Pump state machine ----

/**
 * @brief Pump state machine priority cascade.
 * - Emergency stop latch: pump OFF until reset_stop.
 * - Hard safety lockouts: dry-run and overflow.
 * - Sensor validity gate: require fresh/stable remote data when bypass is OFF.
 * - Policy modes: MANUAL (intent ON/OFF), COUNTDOWN, AUTO.
 */
void executePumpLogic() {
  // Sync isManualRun — true only during MANUAL mode
  isManualRun = (pumpMode == "MANUAL");

  // Emergency stop latch: highest priority.
  if (emergencyStopLatched) {
    lastFaultCode    = "E_STOP";
    lastFaultMessage = "Emergency stop is latched. Reset stop to resume operation.";
    setPump(false);
    runMode = "STOPPED";
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
      pumpMode = "AUTO"; pendingModeWriteback = true; pendingModeWritebackSentMs = 0;
    }
    return;
  }

  bool isLevelFresh = (levelLastUpdateMs > 0) && ((millis() - levelLastUpdateMs) <= LEVEL_STALE_TIMEOUT_MS);
  bool allowStartFromSensors = remoteSensorStable && isLevelFresh && !isLevelSensorError && !cfgBypassLevelSensor;

  // Derive runMode early so status stays coherent even when returning early.
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

  // MANUAL policy (intent-based)
  if (pumpMode == "MANUAL") {
    // manualDesired=false keeps the pump OFF (mode stays MANUAL).
    if (!manualDesired) {
      setPump(false);
      return;
    }
    if (!cfgBypassLevelSensor && !allowStartFromSensors) {
      if (isRunning) {
        bool isLevelFresh = (levelLastUpdateMs > 0) && ((millis() - levelLastUpdateMs) <= LEVEL_STALE_TIMEOUT_MS);
        lastFaultCode    = (!remoteSensorStable || !isLevelFresh) ? "COMM_LOSS" : "STALE_LEVEL";
        lastFaultMessage = "No fresh/stable level data. Pump stopped (failsafe).";
        setPump(false);
      }
      return;
    }
    // Level sensor error fail-safe (only when bypass is OFF)
    if (isLevelSensorError && !cfgBypassLevelSensor) {
      if (isRunning) {
        Serial.println("[MANUAL] Level sensor error — stopping (fail-safe).");
        lastFaultCode    = "LEVEL_SENSOR";
        lastFaultMessage = "Level sensor offline: pump stopped in MANUAL (fail-safe).";
        setPump(false);
      }
      return;  // Mode stays MANUAL; pump off until sensor recovers or bypass enabled
    }
    // Tank-full stop — v5: stay in MANUAL (sticky). Operator exits explicitly.
    if (!cfgBypassLevelSensor && waterLevelPct >= cfgPumpStopLevel) {
      Serial.printf("[MANUAL] Tank full (%d%%). Stopping pump (mode stays MANUAL).\n", waterLevelPct);
      setPump(false);
      return;
    }
    // R-01: Minimum off-time check (motor protection)
    if (!isRunning && pumpOffStartMs > 0 && (millis() - pumpOffStartMs) < MIN_PUMP_OFF_TIME_MS) {
      return;  // Skip start, wait for minimum off-time
    }
    setPump(true);
    return;
  }

  // ── P4: COUNTDOWN ─────────────────────────────────────────────────────
  if (pumpMode == "COUNTDOWN") {
    if (isCountdownActive) {
      if (!cfgBypassLevelSensor && !allowStartFromSensors) {
        bool isLevelFresh = (levelLastUpdateMs > 0) && ((millis() - levelLastUpdateMs) <= LEVEL_STALE_TIMEOUT_MS);
        lastFaultCode    = (!remoteSensorStable || !isLevelFresh) ? "COMM_LOSS" : "STALE_LEVEL";
        lastFaultMessage = "No fresh/stable level data. Pump stopped (failsafe).";
        setPump(false); isCountdownActive = false; countdownEndMs = 0;
        pumpMode = "AUTO"; pendingModeWriteback = true; pendingModeWritebackSentMs = 0;
        return;
      }
      if (!cfgBypassLevelSensor && waterLevelPct >= cfgPumpStopLevel) {
        Serial.printf("[COUNTDOWN] Tank full (%d%%). Stopping early.\n", waterLevelPct);
        setPump(false); isCountdownActive = false; countdownEndMs = 0;
        pumpMode = "AUTO"; pendingModeWriteback = true; pendingModeWritebackSentMs = 0;
        return;
      }
      // R-01: Minimum off-time check (motor protection)
      if (!isRunning && pumpOffStartMs > 0 && (millis() - pumpOffStartMs) < MIN_PUMP_OFF_TIME_MS) {
        return;  // Skip start, wait for minimum off-time
      }
      setPump(true);
    }
    return;
  }

  // ── P5: AUTO ─────────────────────────────────────────────────────────
  if (isSleeping) {
    if (isRunning && waterLevelPct >= cfgPumpStopLevel) {
      Serial.printf("[AUTO] Water at %d%%. Stopping pump.\n", waterLevelPct);
      setPump(false);
    }
    return;
  }

  // P5b: Level sensor bypass — ignore level; flow guard (P1) is the only stop condition
  if (cfgBypassLevelSensor) {
    return;
  }

  // P5c: stale/unstable comm OR level error in AUTO: fail-safe pump OFF
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

  // P5d: Standard hysteresis control
  // R-01: Minimum off-time check before auto-start (motor protection)
  if (!isRunning && waterLevelPct <= cfgPumpStartLevel) {
    if (pumpOffStartMs > 0 && (millis() - pumpOffStartMs) < MIN_PUMP_OFF_TIME_MS) {
      return;  // Skip start, wait for minimum off-time
    }
    Serial.printf("[AUTO] Water at %d%%. Starting pump.\n", waterLevelPct);
    setPump(true);
  } else if (isRunning && waterLevelPct >= cfgPumpStopLevel) {
    Serial.printf("[AUTO] Water at %d%%. Stopping pump.\n", waterLevelPct);
    setPump(false);
  }
}


