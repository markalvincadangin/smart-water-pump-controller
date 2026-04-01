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
      LOG(LOG_WARN, "SENSOR", "Level sensor failure: %d consecutive timeouts.", levelSensorFailCount);
    }
    // Auto-bypass after sustained failure (optional, default off)
    if (isLevelSensorError && cfgAutoBypassOnSensorFail && !cfgBypassLevelSensor) {
      if (levelSensorFailStartMs == 0) levelSensorFailStartMs = millis();
      if ((millis() - levelSensorFailStartMs) >= (unsigned long)cfgAutoBypassDelaySec * 1000UL) {
        cfgBypassLevelSensor = true;
        autoBypassWasEngaged = true;
        autoBypassActive = true;
        LOG(LOG_WARN, "SENSOR", "Auto-bypass enabled after sustained sensor failure.");
      }
    }
  } else {
    // REFACTOR [M-01]: levelLastUpdateMs (updated by pollRemoteSensorNode) is the sole
    // freshness-gate timestamp. levelLastValidMs is kept only for the dashboard health metric.
    if (isLevelSensorError) {
      LOG(LOG_INFO, "SENSOR", "Level sensor recovered. Error cleared.");
    }
    levelSensorFailCount = 0;
    isLevelSensorError = false;
    levelSensorFailStartMs = 0;

    if (prevLevelError) {
      levelAnchorPct = sensorReading;
      flowVolumeAddedL = 0.0f;
      estimatedLevelPct = (float)sensorReading;
      LOG(LOG_DEBUG, "SENSOR", "Level anchor reset to recovered reading: %d%%.", sensorReading);
    } else {
      levelAnchorPct = sensorReading;  // Keep anchor current for next estimate
    }
    if (autoBypassWasEngaged) {
      cfgBypassLevelSensor = false;
      autoBypassWasEngaged = false;
      autoBypassActive = false;
      LOG(LOG_INFO, "SENSOR", "Auto-bypass disabled: sensor recovered.");
    }
  }
}

/**
 * @brief Detects flow sensor stuck-high condition.
 *        If pump is OFF but flow > FLOW_STUCK_THRESHOLD_LPM for > FLOW_STUCK_TIMEOUT_MS,
 *        flags isFlowSensorError.
 */
void checkFlowSensorStuck() {
  if (cfgBypassFlowSensor) {
    if (isFlowSensorError) {
      LOG(LOG_INFO, "SENSOR", "Flow bypass active. Clearing flow-stuck error.");
      isFlowSensorError = false;
    }
    flowStuckTimerActive = false;
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
        LOG(LOG_WARN, "SENSOR", "Flow stuck-high: %.1f LPM while pump OFF >%ds.",
                      flowRateLpm, (int)(FLOW_STUCK_TIMEOUT_MS / 1000));
      }
    }
  } else {
    // Flow is normal (pump OFF with low flow, or pump ON with any flow)
    if (isFlowSensorError) {
      LOG(LOG_INFO, "SENSOR", "Flow sensor recovered. Error cleared.");
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
    if (pumpMode == "MANUAL") {
      // REFACTOR [H-05]: non-latching warning in MANUAL; pump continues
      manualRuntimeWarning = true;
      LOG(LOG_WARN, "SAFETY", "Manual runtime exceeded %d min. Supervisor recommended.", cfgMaxPumpRuntimeMin);
    } else {
      isOverflowError = true;
      setPump(false);
      pumpAutoStartTracking = false;
      LOG(LOG_ERROR, "SAFETY", "Max runtime exceeded (%d min). Pump stopped.", cfgMaxPumpRuntimeMin);
    }
  } else {
    manualRuntimeWarning = false;
  }
}

/**
 * @brief Monitors flow rate while pump is active (dry-run detection).
 *        Triggers isDryRunError = true and kills relay after cfgDryRunTimeoutSec
 *        of sustained low-flow. Reset only via Firebase clear_error signal.
 */
void checkDryRunProtection() {
  if (!isRunning || cfgBypassFlowSensor) {
    dryRunTimerActive = false;
    dryRunStartMs = 0;
    if (isRunning && cfgBypassFlowSensor && isDryRunError) {
       LOG(LOG_INFO, "SAFETY", "Flow bypass active. Clearing dry-run error.");
       isDryRunError = false;
    }
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
      LOG(LOG_WARN, "SAFETY", "Dry-run condition detected. Timer started.");
    } else {
      unsigned long elapsed = millis() - dryRunStartMs;
      if (elapsed >= dryRunTimeoutMs) {
        isDryRunError = true;
        // HARD SAFETY: always stop the pump regardless of mode.
        setPump(false);
        LOG(LOG_ERROR, "SAFETY", "DRY-RUN LOCKOUT. flow=%.2fLPM < %.1fLPM. Pump stopped.",
                       flowRateLpm, cfgDryRunThresholdLpm);
      }
    }
  } else {
    if (dryRunTimerActive) {
      LOG(LOG_INFO, "SAFETY", "Flow restored. Dry-run timer reset.");
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
  // REFACTOR [C-02]: suspend logic if level not yet valid
  if (waterLevelPct < 0) {
    return;
  }
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

  // REFACTOR [H-07]: set cooldown runMode when off-timer is active
  if (!isRunning && pumpOffStartMs > 0 && (millis() - pumpOffStartMs) < MIN_PUMP_OFF_TIME_MS) {
    offTimerActive = true;
    offTimerEndMs = pumpOffStartMs + MIN_PUMP_OFF_TIME_MS;
    uint32_t remainMs = (offTimerEndMs > millis()) ? (offTimerEndMs - millis()) : 0;
    pumpCooldownRemainingSec = (int)(remainMs / 1000UL);
    runMode = (pumpMode == "MANUAL") ? "MANUAL_COOLDOWN" : (pumpMode == "COUNTDOWN" ? "COUNTDOWN" : "AUTO_COOLDOWN");
  } else {
    offTimerActive = false;
    offTimerEndMs = 0;
    pumpCooldownRemainingSec = 0;

    // Derive runMode early so status stays coherent even when returning early.
    if (pumpMode == "MANUAL" && isRunning && manualDesired) {
      runMode = "MANUAL_ON";
    } else if (pumpMode == "MANUAL") {
      runMode = "MANUAL_OFF";
    } else if (pumpMode == "COUNTDOWN" && isCountdownActive) {
      runMode = "COUNTDOWN";
    } else if (pumpMode == "COUNTDOWN" && !isCountdownActive) {
      runMode = "AUTO_STANDBY";
    } else if (pumpMode == "AUTO" && isRunning) {
      runMode = "AUTO";
    } else if (pumpMode == "AUTO" && !isRunning) {
      runMode = "AUTO_STANDBY";
    } else {
      runMode = isRunning ? "AUTO" : "AUTO_STANDBY";
    }
  }

  // MANUAL policy (intent-based)
  if (pumpMode == "MANUAL") {
    // manualDesired=false keeps the pump OFF (mode stays MANUAL).
    if (!manualDesired) {
      setPump(false);
      return;
    }

    // In MANUAL, still require fresh/stable level data unless bypass is ON.
    if (!cfgBypassLevelSensor && !allowStartFromSensors) {
      if (isRunning) {
        bool isLevelFresh = (levelLastUpdateMs > 0) && ((millis() - levelLastUpdateMs) <= LEVEL_STALE_TIMEOUT_MS);
        lastFaultCode    = (!remoteSensorStable || !isLevelFresh) ? "COMM_LOSS" : "STALE_LEVEL";
        lastFaultMessage = "No fresh/stable level data. Pump stopped (failsafe).";
        LOG(LOG_ERROR, "SAFETY", "MANUAL: no fresh/stable level data — pump stopped failsafe.");
        setPump(false);
      }
      return;
    }
    if (isLevelSensorError && !cfgBypassLevelSensor) {
      if (isRunning) {
        LOG(LOG_WARN, "SAFETY", "MANUAL: level sensor error — pump stopped failsafe.");
        lastFaultCode    = "LEVEL_SENSOR";
        lastFaultMessage = "Level sensor offline: pump stopped in MANUAL (fail-safe).";
        setPump(false);
      }
      return;
    }
    // Tank-full stop — v5: stay in MANUAL (sticky). Operator exits explicitly.
    if (!cfgBypassLevelSensor && waterLevelPct >= cfgPumpStopLevel) {
      LOG(LOG_INFO, "SAFETY", "MANUAL: tank full (%d%%). Pump stopped.", waterLevelPct);
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
        LOG(LOG_INFO, "SAFETY", "COUNTDOWN: tank full (%d%%). Stopping early.", waterLevelPct);
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
      // REFACTOR [3.1]: was flat Serial.printf
      LOG(LOG_INFO, "PUMP", "AUTO: sleeping — level %d%% >= stop %d%%. Stopping pump.",
          waterLevelPct, cfgPumpStopLevel);
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
      LOG(LOG_ERROR, "SAFETY", "AUTO: no fresh/stable level data — pump stopped failsafe.");
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
    LOG(LOG_INFO, "PUMP", "AUTO: level %d%% <= start threshold %d%%. Starting pump.", waterLevelPct, cfgPumpStartLevel);
    setPump(true);
  } else if (isRunning && waterLevelPct >= cfgPumpStopLevel) {
    LOG(LOG_INFO, "PUMP", "AUTO: level %d%% >= stop threshold %d%%. Stopping pump.", waterLevelPct, cfgPumpStopLevel);
    setPump(false);
  }
}


