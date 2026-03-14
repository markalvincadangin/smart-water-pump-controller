// -----------------------------------------------------------------------------
// Pump control + safety
// -----------------------------------------------------------------------------

// ---- Relay control ----

// Relay module is active-low: LOW = pump ON, HIGH = pump OFF.
void setPump(bool on) {
  if (on && !isRunning) { totalPumpCycles++; pumpOnSinceMs = millis(); }
  if (!on && isRunning && pumpOnSinceMs > 0) {
    totalPumpRunSec += (millis() - pumpOnSinceMs) / 1000UL;
    pumpOnSinceMs = 0;
  }
  digitalWrite(RELAY_PIN, on ? LOW : HIGH);
  isRunning = on;
  if (!on) pumpOffStartMs = millis(); else pumpOffStartMs = 0;
}

// ---- Safety checks ----

void checkLevelSensorFailure(int sensorReading) {
  bool prevLevelError = isLevelSensorError;
  if (sensorReading == -1) {
    levelSensorFailCount++;
    if (levelSensorFailCount >= cfgLevelSensorFailureThreshold && !isLevelSensorError) {
      isLevelSensorError = true;
      Serial.printf("[SENSOR][ERROR] Ultrasonic (level) failure: %d consecutive timeouts.\n", levelSensorFailCount);
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
    if (isLevelSensorError) Serial.println("[SENSOR][INFO] Ultrasonic (level) recovered. Error cleared.");
    levelSensorFailCount = 0;
    isLevelSensorError = false;
    levelSensorFailStartMs = 0;
    if (prevLevelError) {
      levelAnchorPct = sensorReading;
      flowVolumeAddedL = 0.0f;
      estimatedLevelPct = (float)sensorReading;
      Serial.println("[ESTIMATE] Anchor reset to recovered sensor reading.");
    } else levelAnchorPct = sensorReading;
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
  if (!isRunning || pumpMode != "AUTO") {
    // Not running or not in AUTO — reset tracking
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

// ---- Pump state machine (v3.0 Hierarchical Priority Model) ----

/**
 * @brief Pump state machine — P1 Hard Safety, P2 Bypass, P3 Manual, P4 Countdown, P5 Automation.
 */
void executePumpLogic() {
  // Derive runMode first — must execute before any early return so the
  // dashboard always sees the correct state.
  if (isDryRunError || isOverflowError) {
    runMode = "OFF";
  } else if (pumpMode == "FORCE_OFF") {
    runMode = "OFF";
  } else if (pumpMode == "FORCE_ON") {
    runMode = "MANUAL";
  } else if (pumpMode == "COUNTDOWN" && isCountdownActive) {
    runMode = "COUNTDOWN";
  } else if (pumpMode == "AUTO" && isRunning) {
    runMode = "AUTO";
  } else if (pumpMode == "AUTO" && !isRunning) {
    runMode = "AUTO_STANDBY";
  } else if (!isRunning) {
    runMode = "OFF";
  } else {
    runMode = "AUTO";
  }

  // P1: HARD SAFETY — dry-run lockout and overflow (cannot be bypassed)
  if (isDryRunError || isOverflowError) {
    if (isDryRunError) {
      lastFaultCode = "DRY_RUN";
      lastFaultMessage = "Dry-run lockout: low flow while pump was running.";
    } else {
      lastFaultCode = "OVERFLOW";
      lastFaultMessage = "Overflow protection: max runtime exceeded in AUTO.";
    }
    setPump(false);
    if (isCountdownActive) {
      isCountdownActive = false;
      countdownEndMs = 0;
    }
    return;
  }

  // P3: EMERGENCY STOP — FORCE_OFF always wins
  if (pumpMode == "FORCE_OFF") {
    setPump(false);
    return;
  }

  // P3: MANUAL RUN — FORCE_ON (P1 still guards above)
  if (pumpMode == "FORCE_ON") {
    setPump(true);
    return;
  }

  // P4: COUNTDOWN — run until timer expires or tank 100% (unless bypass)
  if (pumpMode == "COUNTDOWN") {
    if (isCountdownActive) {
      if (!cfgBypassLevelSensor && waterLevelPct >= cfgPumpStopLevel) {
        Serial.printf("[COUNTDOWN] Tank full (%d%%). Stopping pump early.\n", waterLevelPct);
        setPump(false);
        isCountdownActive = false;
        countdownEndMs = 0;
        pumpMode = "AUTO";
        Firebase.RTDB.setString(&fbdo, "/pump_system/control/mode", "AUTO");
        return;
      }
      setPump(true);
    }
    return;
  }

  // P5: AUTO (and sleep / P2 bypass / level error / hysteresis)
  if (isSleeping) {
    if (isRunning && waterLevelPct >= cfgPumpStopLevel) {
      Serial.printf("[AUTO] Water at %d%%. Stopping pump.\n", waterLevelPct);
      setPump(false);
    }
    return;
  }

  // P2: Level sensor bypass — ignore level; flow guard (P1) is the only stop condition
  if (cfgBypassLevelSensor) {
    return;
  }

  // Level sensor error in AUTO: fail-safe pump OFF
  if (isLevelSensorError) {
    if (isRunning) {
      Serial.println("[AUTO] Level sensor error — stopping pump (fail-safe).");
      lastFaultCode = "LEVEL_SENSOR";
      lastFaultMessage = "Level sensor offline: controller stopped pump in AUTO (fail-safe).";
      setPump(false);
    }
    return;
  }

  // Standard hysteresis control
  if (!isRunning && waterLevelPct <= cfgPumpStartLevel) {
    Serial.printf("[AUTO] Water at %d%%. Starting pump.\n", waterLevelPct);
    setPump(true);
  } else if (isRunning && waterLevelPct >= cfgPumpStopLevel) {
    Serial.printf("[AUTO] Water at %d%%. Stopping pump.\n", waterLevelPct);
    setPump(false);
  }
}

