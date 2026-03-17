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
    // In FORCE_ON, P0 owns the relay; P1 only sets flags for monitoring.
    if (pumpMode != "FORCE_ON") {
      setPump(false);
    }
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
        // In FORCE_ON, P0 owns the relay; P1 only sets flags for monitoring.
        if (pumpMode != "FORCE_ON") {
          setPump(false);
          Serial.println("[SAFETY][ERROR] DRY-RUN LOCKOUT. Pump stopped; waiting for acknowledge.");
        } else {
          Serial.println("[SAFETY][ERROR] DRY-RUN detected under FORCE_ON (relay held ON by override).");
        }
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

// ---- Pump state machine (v4.0 Hierarchical Priority Model) ----

/**
 * @brief Pump state machine — v4.0 six-level priority cascade.
 *   P0: FORCE_ON — absolute override, all safety bypassed at relay level.
 *   P1: Hard safety — dry-run lockout, overflow. Cannot be bypassed except by P0.
 *   P2: FORCE_OFF — persistent emergency stop.
 *   P3: MANUAL — operator-initiated, full safety (identical to AUTO).
 *   P4: COUNTDOWN — timed run, full safety.
 *   P5: AUTO — hysteresis with sleep, bypass, level error, and level checks.
 */
void executePumpLogic() {
  // Sync isManualRun — true only during MANUAL mode
  isManualRun = (pumpMode == "MANUAL");

  // ── runMode derivation (always before any return) ──────────────────────
  if (pumpMode == "FORCE_ON") {
    runMode = "FORCE_ON";
  } else if (isDryRunError || isOverflowError) {
    runMode = "OFF";
  } else if (pumpMode == "FORCE_OFF") {
    runMode = "OFF";
  } else if (pumpMode == "MANUAL" && isRunning) {
    runMode = "MANUAL";
  } else if (pumpMode == "MANUAL" && !isRunning) {
    runMode = "MANUAL_OFF"; // v5: MANUAL mode, pump off (operator or safety)
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

  // ── P0: ABSOLUTE OVERRIDE — FORCE_ON ───────────────────────────────────
  // checkSafetyCutoff() already ran. Error flags are SET for dashboard display.
  // The relay is NOT affected by those flags at this priority level.
  if (pumpMode == "FORCE_ON") {
    // R-02: FORCE_ON auto-timeout — revert to AUTO after cfgForceOnMaxMin minutes
    if (forceOnStartMs == 0) {
      forceOnStartMs = millis();
    }
    unsigned long forceOnElapsed = millis() - forceOnStartMs;
    if (cfgForceOnMaxMin > 0 && forceOnElapsed >= (unsigned long)cfgForceOnMaxMin * 60000UL) {
      Serial.printf("[FORCE_ON] Auto-expired after %d min. Reverting to AUTO.\n", cfgForceOnMaxMin);
      pumpMode = "AUTO";
      pendingModeWriteback = true;
      pendingModeWritebackSentMs = 0;
      forceOnStartMs = 0;
      // Do NOT setPump here — let P5 evaluate level state on next cycle
      return;
    }
    setPump(true);
    return;
  } else {
    forceOnStartMs = 0;  // Reset whenever not in FORCE_ON
  }

  // ── P1: HARD SAFETY ────────────────────────────────────────────────────
  if (isDryRunError || isOverflowError) {
    lastFaultCode    = isDryRunError ? "DRY_RUN" : "OVERFLOW";
    lastFaultMessage = isDryRunError
      ? "Dry-run lockout: low flow while pump was running."
      : "Overflow protection: max runtime exceeded.";
    setPump(false);
    // COUNTDOWN: cancel and revert to AUTO
    if (isCountdownActive) {
      isCountdownActive = false; countdownEndMs = 0;
      pumpMode = "AUTO"; pendingModeWriteback = true; pendingModeWritebackSentMs = 0;
    }
    // MANUAL: do NOT revert mode. Pump off, mode stays MANUAL.
    // After clear_error: P3 will restart the pump automatically.
    return;
  }

  // ── P2: FORCE_OFF ──────────────────────────────────────────────────────
  if (pumpMode == "FORCE_OFF") {
    setPump(false);
    return;
  }

  // ── P3: MANUAL RUN (full safety — same as AUTO) ───────────────────────
  if (pumpMode == "MANUAL") {
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

  // P5c: Level sensor error in AUTO: fail-safe pump OFF
  if (isLevelSensorError) {
    if (isRunning) {
      Serial.println("[AUTO] Level sensor error — stopping pump (fail-safe).");
      lastFaultCode = "LEVEL_SENSOR";
      lastFaultMessage = "Level sensor offline: controller stopped pump in AUTO (fail-safe).";
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


