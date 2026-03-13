// -----------------------------------------------------------------------------
// Pump control + safety
// -----------------------------------------------------------------------------

// ---- Relay control ----

// Relay module is active-low: LOW = pump ON, HIGH = pump OFF.
void setPump(bool on) {
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
 * @brief Checks for ultrasonic sensor failure.
 *        After cfgSensorFailureThreshold consecutive timeouts, flags isSensorError.
 *        Auto-recovers when a valid reading is received. Threshold configurable via Firebase (Phase 4).
 */
void checkSensorFailure(int sensorReading) {
  if (sensorReading == -1) {
    sensorFailCount++;
    if (sensorFailCount >= cfgSensorFailureThreshold && !isSensorError) {
      isSensorError = true;
      Serial.printf("[SENSOR][ERROR] Ultrasonic failure: %d consecutive timeouts.\n", sensorFailCount);
    }
  } else {
    if (isSensorError) {
      Serial.println("[SENSOR][INFO] Ultrasonic recovered. Error cleared.");
    }
    sensorFailCount = 0;
    isSensorError = false;
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

// ---- Pump state machine ----

/**
 * @brief Executes the pump control state machine based on current mode.
 *        Error lockouts override everything (dry-run, overflow).
 *        Sensor error in AUTO mode → pump OFF (fail-safe).
 *        Phase 3: During sleep window, AUTO is suppressed (pump won't auto-start);
 *        FORCE_ON always works; emergency override handled in loop (bypasses sleep).
 *
 * AUTO:      Hysteresis control based on tank water level.
 * FORCE_ON:  Override - turns pump ON regardless of level.
 * FORCE_OFF: Override - turns pump OFF regardless of level.
 */
void executePumpLogic() {
  // Phase 7: timed run auto-stop and countdown bookkeeping.
  // When in a timed run, we keep `pumpMode = FORCE_ON` but automatically stop when
  // duration elapses, then restore the previous mode.
  if (runMode == "TIMED" && runDurationMs > 0 && runStartMs > 0) {
    unsigned long elapsed = millis() - runStartMs;
    if (elapsed >= runDurationMs) {
      Serial.println("[RUN] Timed run complete. Stopping pump.");
      setPump(false);
      runMode = "AUTO";
      runRemainingSec = 0;
      runStartMs = 0;
      runDurationMs = 0;
      pumpMode = runPrevPumpMode.length() > 0 ? runPrevPumpMode : "AUTO";
    } else {
      unsigned long remainingMs = runDurationMs - elapsed;
      runRemainingSec = (uint32_t)((remainingMs + 999UL) / 1000UL);
    }
  } else if (runMode != "TIMED") {
    runRemainingSec = 0;
  }

  // Error lockouts override everything
  if (isDryRunError || isOverflowError) {
    if (isDryRunError) {
      lastFaultCode = "DRY_RUN";
      lastFaultMessage = "Dry-run lockout: low flow while pump was running.";
    } else if (isOverflowError) {
      lastFaultCode = "OVERFLOW";
      lastFaultMessage = "Overflow protection: max runtime exceeded in AUTO.";
    }
    setPump(false);
    // Cancel any active run so the dashboard shows a clear stopped state.
    runMode = "OFF";
    runRemainingSec = 0;
    runStartMs = 0;
    runDurationMs = 0;
    return;
  }

  if (pumpMode == "FORCE_ON") {
    // Manual override; dry-run protection still applies.
    setPump(true);

  } else if (pumpMode == "FORCE_OFF") {
    setPump(false);

  } else {
    // AUTO mode

    // During scheduled sleep, suppress AUTO start.
    if (isSleeping) {
      if (isRunning) {
        // Pump is running (from FORCE_ON or emergency) — keep it running until stop level
        if (waterLevelPct >= cfgPumpStopLevel) {
          Serial.printf("[AUTO] Water at %d%%. Stopping pump.\n", waterLevelPct);
          setPump(false);
        }
      }
      // Do not auto-start in sleep
      return;
    }

    // Sensor error in AUTO: fail-safe pump OFF (prevents overflow on stale data).
    if (isSensorError) {
      if (isRunning) {
        Serial.println("[AUTO] Sensor error — stopping pump (fail-safe).");
        lastFaultCode = "SENSOR";
        lastFaultMessage = "Sensor error: controller stopped pump in AUTO (fail-safe).";
        setPump(false);
      }
      return;
    }

    // Hysteresis control
    if (!isRunning && waterLevelPct <= cfgPumpStartLevel) {
      Serial.printf("[AUTO] Water at %d%%. Starting pump.\n", waterLevelPct);
      setPump(true);
    } else if (isRunning && waterLevelPct >= cfgPumpStopLevel) {
      Serial.printf("[AUTO] Water at %d%%. Stopping pump.\n", waterLevelPct);
      setPump(false);
    }
    // If between thresholds, maintain current state (hysteresis)
  }

  // Phase 7: maintain a stable run_mode value for the dashboard
  if (runMode == "MANUAL" || runMode == "TIMED") {
    // keep as-is
  } else if (!isRunning) {
    runMode = "OFF";
  } else if (pumpMode == "AUTO") {
    runMode = "AUTO";
  }
}

