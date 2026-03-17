// =============================================================================
// Part IV: Configuration and state persistence — Sections 12b, 12b2, 16C, 16D
// =============================================================================

// =============================================================================
// SECTION 12b: DEVICE CONFIG — NVS (persist across reboot when offline)
// =============================================================================

void loadDeviceConfigFromNVS() {
  if (!prefs.begin(NVS_NAMESPACE, true)) {  // read-only
    Serial.println("[NVS] Namespace open failed. Using firmware defaults.");
    return;
  }
  if (prefs.getInt("tank_empty", -1) == -1) {
    prefs.end();
    Serial.println("[NVS] No saved config. Using firmware defaults.");
    return;
  }
  int te = prefs.getInt("tank_empty", TANK_EMPTY_CM);
  int tf = prefs.getInt("tank_full", TANK_FULL_CM);
  int ps = prefs.getInt("pump_start", PUMP_START_LEVEL);
  int po = prefs.getInt("pump_stop", PUMP_STOP_LEVEL);
  float drLpm = prefs.getFloat("dry_run_lpm", DRY_RUN_THRESHOLD_LPM);
  int drSec = prefs.getInt("dry_run_sec", (int)(DRY_RUN_TIMEOUT_MS / 1000UL));
  float flowCal = prefs.getFloat("flow_cal", FLOW_CALIBRATION_FACTOR);
  int maxRuntime = prefs.getInt("max_runtime", MAX_PUMP_RUNTIME_MIN);

  // Phase 3: Sleep config (optional keys — defaults if missing)
  bool slpEn = prefs.getBool("slp_en", SLEEP_DEFAULT_ENABLED);
  int slpStart = prefs.getInt("slp_start", SLEEP_DEFAULT_START_HOUR);
  int slpEnd = prefs.getInt("slp_end", SLEEP_DEFAULT_END_HOUR);
  int slpEmerg = prefs.getInt("slp_emerg", SLEEP_DEFAULT_EMERGENCY_LVL);

  // Phase 4: Advanced config (optional keys)
  int sensThresh = prefs.getInt("sens_thresh", SENSOR_FAILURE_THRESHOLD);
  int idleSens = prefs.getInt("idle_sens_ms", IDLE_SENSOR_INTERVAL_MS_DEF);
  int idleFb = prefs.getInt("idle_fb_ms", IDLE_FIREBASE_INTERVAL_MS_DEF);
  bool autoBypassEn = prefs.getBool("auto_bypass_en", false);
  int autoBypassSec = prefs.getInt("auto_bypass_sec", AUTO_BYPASS_FAILURE_SEC_DEF);

  int schemaVer = prefs.getInt("schema_ver", 0);
  prefs.end();

  if (schemaVer > NVS_SCHEMA_VERSION) {
    Serial.println("[NVS] Schema version from newer firmware. Using defaults.");
    return;
  }
  if (schemaVer < NVS_SCHEMA_VERSION) {
    Serial.printf("[NVS] Schema v%d loaded into firmware v%d — new fields use defaults.\n",
                  schemaVer, NVS_SCHEMA_VERSION);
  }
  // Validate: if NVS was corrupted, keep firmware defaults
  if (te < 5 || te > 200 || tf < 1 || tf >= te || ps < 0 || ps > 100 || po < 0 || po > 100 || po <= ps
      || drLpm < 0.1f || drLpm > 10.0f || drSec < 10 || drSec > 300
      || flowCal < 0.1f || flowCal > 20.0f || maxRuntime < 30 || maxRuntime > 480) {
    Serial.println("[NVS] Stored config invalid. Using firmware defaults.");
    return;
  }
  cfgTankEmptyCm = te;
  cfgTankFullCm = tf;
  cfgPumpStartLevel = ps;
  cfgPumpStopLevel = po;
  cfgDryRunThresholdLpm = drLpm;
  cfgDryRunTimeoutSec = drSec;
  cfgFlowCalibration = flowCal;
  cfgMaxPumpRuntimeMin = maxRuntime;

  // Phase 3: Apply sleep config (validate hours 0–23, emergency 0–100)
  if (slpStart >= 0 && slpStart <= 23) cfgSleepStartHour = slpStart;
  if (slpEnd >= 0 && slpEnd <= 23) cfgSleepEndHour = slpEnd;
  if (slpEmerg >= 0 && slpEmerg <= 100) cfgSleepEmergencyLevel = slpEmerg;
  cfgSleepEnabled = slpEn;

  // Phase 4: Apply advanced config (validate ranges)
  if (sensThresh >= 3 && sensThresh <= 20) cfgLevelSensorFailureThreshold = sensThresh;
  if (idleSens >= 5000 && idleSens <= 60000) cfgIdleSensorIntervalMs = idleSens;
  if (idleFb >= 10000 && idleFb <= 120000) cfgIdleFirebaseIntervalMs = idleFb;
  cfgAutoBypassOnSensorFail = autoBypassEn;
  if (autoBypassSec >= 10 && autoBypassSec <= 300) cfgAutoBypassDelaySec = autoBypassSec;

  Serial.println("[NVS] Device config loaded.");
}

void saveDeviceConfigToNVS() {
  if (!prefs.begin(NVS_NAMESPACE, false)) {  // read-write
    Serial.println("[NVS] Failed to open for write. Config not persisted.");
    return;
  }
  prefs.putInt("tank_empty", cfgTankEmptyCm);
  prefs.putInt("tank_full", cfgTankFullCm);
  prefs.putInt("pump_start", cfgPumpStartLevel);
  prefs.putInt("pump_stop", cfgPumpStopLevel);
  prefs.putFloat("dry_run_lpm", cfgDryRunThresholdLpm);
  prefs.putInt("dry_run_sec", cfgDryRunTimeoutSec);
  prefs.putFloat("flow_cal", cfgFlowCalibration);
  prefs.putInt("max_runtime", cfgMaxPumpRuntimeMin);
  // Phase 3: Sleep config
  prefs.putBool("slp_en", cfgSleepEnabled);
  prefs.putInt("slp_start", cfgSleepStartHour);
  prefs.putInt("slp_end", cfgSleepEndHour);
  prefs.putInt("slp_emerg", cfgSleepEmergencyLevel);
  // Phase 4: Advanced config
  prefs.putInt("sens_thresh", cfgLevelSensorFailureThreshold);
  prefs.putInt("idle_sens_ms", cfgIdleSensorIntervalMs);
  prefs.putInt("idle_fb_ms", cfgIdleFirebaseIntervalMs);
  prefs.putBool("auto_bypass_en", cfgAutoBypassOnSensorFail);
  prefs.putInt("auto_bypass_sec", cfgAutoBypassDelaySec);
  prefs.putInt("schema_ver", NVS_SCHEMA_VERSION);
  prefs.end();
  Serial.println("[NVS] Device config saved.");
}

// =============================================================================
// SECTION 12b2: SLEEP WINDOW HELPER (Phase 3)
// Handles overnight windows (e.g. 23–5). Returns true if currentHour is within sleep window.
// =============================================================================

bool isInSleepWindow(int currentHour) {
  if (cfgSleepStartHour <= cfgSleepEndHour) {
    // Same-day window: e.g. 9–17
    return (currentHour >= cfgSleepStartHour && currentHour < cfgSleepEndHour);
  } else {
    // Overnight window: e.g. 23–5
    return (currentHour >= cfgSleepStartHour) || (currentHour < cfgSleepEndHour);
  }
}

// =============================================================================
// SECTION 16C: CRASH LOOP DETECTION (Phase 2)
// Reads NVS boot counter and timestamp. If >5 reboots in 5 minutes, enters safe mode.
// Safe mode auto-clears after 1 hour.
// =============================================================================

void checkCrashLoop() {
  if (!prefs.begin(NVS_STATE_NAMESPACE, false)) {
    Serial.println("[BOOT] NVS state namespace open failed.");
    return;
  }

  unsigned long now = millis();  // ~0 at boot, but we use relative time
  unsigned long lastBootTime = prefs.getULong("last_boot_ms", 0);
  int bootCount = prefs.getInt("boot_count", 0);
  unsigned long safeModeStart = prefs.getULong("safe_mode_ms", 0);

  // Check if safe mode should be cleared (1 hour timeout)
  if (safeModeStart > 0) {
    // Safe mode was previously entered. If this is a fresh power cycle
    // (millis() near 0), clear safe mode since user likely did a full power cycle
    if (now < 5000) {
      // Fresh power cycle — clear safe mode
      prefs.putULong("safe_mode_ms", 0);
      prefs.putInt("boot_count", 0);
      Serial.println("[BOOT] Power cycle detected. Safe mode cleared.");
      prefs.end();
      return;
    }
  }

  // Use esp_timer for more reliable time (microseconds since boot)
  // For crash loop detection, we track boot_count and reset it if we
  // had a long uptime before the reboot (indicated by large lastBootTime)
  if (lastBootTime > (unsigned long)(CRASH_LOOP_WINDOW_SEC * 1000UL)) {
    // Previous run had uptime > 5 min — not a crash loop
    bootCount = 0;
  }

  bootCount++;
  prefs.putInt("boot_count", bootCount);
  prefs.putULong("last_boot_ms", 0);  // Will be updated to millis() periodically

  if (bootCount >= CRASH_LOOP_THRESHOLD) {
    inSafeMode = true;
    safeModeEnteredMs = millis();
    lastFaultCode = "SAFE_MODE";
    lastFaultMessage = "Crash loop detected. Controller in safe mode. Power cycle to recover.";
    prefs.putULong("safe_mode_ms", safeModeEnteredMs);
    Serial.printf("[ERROR] CRASH LOOP DETECTED: %d reboots. Entering SAFE MODE.\n", bootCount);
    Serial.println("[SAFE MODE] Pump OFF. Firebase disabled. Serial only.");
  } else {
    Serial.printf("[BOOT] Boot count: %d/%d (window: %ds)\n",
                  bootCount, CRASH_LOOP_THRESHOLD, CRASH_LOOP_WINDOW_SEC);
  }

  prefs.end();
}

// =============================================================================
// SECTION 16D: NVS STATE PERSISTENCE (Phase 2)
// Persists pumpMode, isDryRunError on change; waterLevelPct with wear reduction.
// =============================================================================

void loadStateFromNVS() {
  if (!prefs.begin(NVS_STATE_NAMESPACE, true)) return;

  String savedMode = prefs.getString("mode", "AUTO");
  bool savedDryRun = prefs.getBool("dry_run_err", false);
  bool savedBypass = prefs.getBool("bypass_lvl", false);
  int savedLevel = prefs.getInt("level_pct", -1);
  totalPumpCycles = prefs.getUInt("pump_cycles", 0);
  totalPumpRunSec = prefs.getULong("pump_run_sec", 0);
  lastPersistedPumpCycles = totalPumpCycles;
  lastPersistedPumpRunSec = totalPumpRunSec;
  lastRebootRequestId = prefs.getInt("last_reboot_id", 0);
  cfgLastCountdownDurationMin = prefs.getInt("cd_dur_min", 15);
  prefs.end();

  // v4.0: MANUAL is safe to restore (full safety active). FORCE_ON requires explicit re-activation.
  if (savedMode == "AUTO" || savedMode == "FORCE_OFF"
      || savedMode == "COUNTDOWN" || savedMode == "MANUAL") {
    pumpMode = savedMode;
    lastPersistedMode = savedMode;
    if (savedMode == "COUNTDOWN") {
      Serial.println("[BOOT] Restored COUNTDOWN mode from NVS. Timer will restart on Firebase sync.");
    } else if (savedMode == "MANUAL") {
      Serial.println("[BOOT] Restored MANUAL mode. Pump will start when sensor block runs.");
    }
  } else if (savedMode == "FORCE_ON") {
    pumpMode = "AUTO";
    lastPersistedMode = "AUTO";
    Serial.println("[BOOT] FORCE_ON not restored after reboot. Defaulting to AUTO.");
  }
  isDryRunError = savedDryRun;
  lastPersistedDryRun = savedDryRun;
  cfgBypassLevelSensor = savedBypass;
  lastPersistedBypass = savedBypass;

  Serial.printf("[BOOT] Last state: Level=%d%%, Mode=%s, DryRun=%s, Bypass=%s, Cycles=%lu, RunSec=%lu\n",
                savedLevel, pumpMode.c_str(), isDryRunError ? "YES" : "NO", savedBypass ? "YES" : "NO",
                (unsigned long)totalPumpCycles, (unsigned long)totalPumpRunSec);
}

void persistStateToNVS() {
  bool modeChanged = (pumpMode != lastPersistedMode);
  bool dryRunChanged = (isDryRunError != lastPersistedDryRun);
  bool bypassChanged = (cfgBypassLevelSensor != lastPersistedBypass);
  bool telemetryChanged = (totalPumpCycles != lastPersistedPumpCycles) || (totalPumpRunSec != lastPersistedPumpRunSec);
  int levelDelta = abs(waterLevelPct - lastPersistedLevel);
  unsigned long now = millis();
  bool levelNeedsWrite = (lastPersistedLevel == -1)
    || (levelDelta >= NVS_LEVEL_DELTA_THRESHOLD)
    || (now - lastLevelWriteMs >= NVS_LEVEL_INTERVAL_MS);
  bool uptimeNeedsWrite = (now - lastUptimeWriteMs >= NVS_UPTIME_INTERVAL_MS);

  if (!modeChanged && !dryRunChanged && !bypassChanged && !telemetryChanged && !levelNeedsWrite && !uptimeNeedsWrite) return;

  if (!prefs.begin(NVS_STATE_NAMESPACE, false)) return;

  if (modeChanged) {
    prefs.putString("mode", pumpMode);
    lastPersistedMode = pumpMode;
    Serial.printf("[NVS] Mode persisted: %s\n", pumpMode.c_str());
  }
  if (dryRunChanged) {
    prefs.putBool("dry_run_err", isDryRunError);
    lastPersistedDryRun = isDryRunError;
  }
  if (bypassChanged) {
    prefs.putBool("bypass_lvl", cfgBypassLevelSensor);
    lastPersistedBypass = cfgBypassLevelSensor;
    Serial.printf("[NVS] Bypass level sensor persisted: %s\n", cfgBypassLevelSensor ? "ON" : "OFF");
  }
  if (telemetryChanged) {
    prefs.putUInt("pump_cycles", totalPumpCycles);
    prefs.putULong("pump_run_sec", totalPumpRunSec);
    lastPersistedPumpCycles = totalPumpCycles;
    lastPersistedPumpRunSec = totalPumpRunSec;
  }
  if (levelNeedsWrite) {
    prefs.putInt("level_pct", waterLevelPct);
    lastPersistedLevel = waterLevelPct;
    lastLevelWriteMs = now;
  }
  if (uptimeNeedsWrite) {
    prefs.putULong("last_boot_ms", now);
    lastUptimeWriteMs = now;
  }

  prefs.end();
}

