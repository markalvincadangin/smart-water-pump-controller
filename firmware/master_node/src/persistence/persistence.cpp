#include "persistence.h"

#include "../state/state.h"

/**
 * @brief Loads device configuration from NVS and validates against firmware schema.
 *
 * Reads all hardware and policy parameters. Falls back to firmware defaults
 * if the schema version is outdated or stored values fail range checks.
 */
void loadDeviceConfigFromNVS() {
  if (!prefs.begin(NVS_NAMESPACE, true)) {
    LOG(APP_LOG_LEVEL_ERROR, "NVS", "Namespace open failed. Using firmware defaults.");
    return;
  }
  if (prefs.getInt("tank_empty", -1) == -1) {
    prefs.end();
    LOG(APP_LOG_LEVEL_INFO, "NVS", "No saved config. Using firmware defaults.");
    return;
  }
  int tankEmptyCm = prefs.getInt("tank_empty", TANK_EMPTY_CM);
  int tankFullCm = prefs.getInt("tank_full", TANK_FULL_CM);
  int pumpStartLevel = prefs.getInt("pump_start", PUMP_START_LEVEL);
  int pumpStopLevel = prefs.getInt("pump_stop", PUMP_STOP_LEVEL);
  float dryRunLpm = prefs.getFloat("dry_run_lpm", DRY_RUN_THRESHOLD_LPM);
  int dryRunSec = prefs.getInt("dry_run_sec", (int)(DRY_RUN_TIMEOUT_MS / 1000UL));
  float flowCal = prefs.getFloat("flow_cal", FLOW_CALIBRATION_FACTOR);
  int maxRuntime = prefs.getInt("max_runtime", MAX_PUMP_RUNTIME_MIN);

  bool sleepEnabled = prefs.getBool("slp_en", SLEEP_DEFAULT_ENABLED);
  int sleepStart = prefs.getInt("slp_start", SLEEP_DEFAULT_START_HOUR);
  int sleepEnd = prefs.getInt("slp_end", SLEEP_DEFAULT_END_HOUR);
  int sleepEmerg = prefs.getInt("slp_emerg", SLEEP_DEFAULT_EMERGENCY_LVL);

  int sensThresh = prefs.getInt("sens_thresh", SENSOR_FAILURE_THRESHOLD);
  int idleSens = prefs.getInt("idle_sens_ms", IDLE_SENSOR_INTERVAL_MS_DEF);
  int idleFb = prefs.getInt("idle_fb_ms", IDLE_FIREBASE_INTERVAL_MS_DEF);
  bool autoBypassEn = prefs.getBool("auto_bypass_en", false);
  int autoBypassSec = prefs.getInt("auto_bypass_sec", AUTO_BYPASS_FAILURE_SEC_DEF);

  int schemaVer = prefs.getInt("schema_ver", 0);
  prefs.end();

  if (schemaVer > NVS_SCHEMA_VERSION) {
    LOG(APP_LOG_LEVEL_INFO, "NVS", "Schema version from newer firmware. Using defaults.");
    return;
  }
  if (schemaVer < NVS_SCHEMA_VERSION) {
    LOG(APP_LOG_LEVEL_INFO, "NVS", "Schema v%d loaded into firmware v%d — new fields use defaults.", schemaVer, NVS_SCHEMA_VERSION);
  }
  if (tankEmptyCm < 5 || tankEmptyCm > 200 || tankFullCm < 1 || tankFullCm >= tankEmptyCm || pumpStartLevel < 0 || pumpStartLevel > 100 || pumpStopLevel < 0 || pumpStopLevel > 100 || pumpStopLevel <= pumpStartLevel
      || dryRunLpm < 0.1f || dryRunLpm > 10.0f || dryRunSec < 10 || dryRunSec > 300
      || flowCal < 0.1f || flowCal > 20.0f || maxRuntime < 30 || maxRuntime > 480) {
    LOG(APP_LOG_LEVEL_INFO, "NVS", "Stored config invalid. Using firmware defaults.");
    return;
  }
  cfgTankEmptyCm = tankEmptyCm;
  cfgTankFullCm = tankFullCm;
  cfgPumpStartLevel = pumpStartLevel;
  cfgPumpStopLevel = pumpStopLevel;
  cfgDryRunThresholdLpm = dryRunLpm;
  cfgDryRunTimeoutSec = dryRunSec;
  cfgFlowCalibration = flowCal;
  cfgMaxPumpRuntimeMin = maxRuntime;

  if (sleepStart >= 0 && sleepStart <= 23) {
    cfgSleepStartHour = sleepStart;
  }
  if (sleepEnd >= 0 && sleepEnd <= 23) {
    cfgSleepEndHour = sleepEnd;
  }
  if (sleepEmerg >= 0 && sleepEmerg <= 100) {
    cfgSleepEmergencyLevel = sleepEmerg;
  }
  cfgSleepEnabled = sleepEnabled;

  if (sensThresh >= 3 && sensThresh <= 20) {
    cfgLevelSensorFailureThreshold = sensThresh;
  }
  if (idleSens >= 5000 && idleSens <= 60000) {
    cfgIdleSensorIntervalMs = idleSens;
  }
  if (idleFb >= 10000 && idleFb <= 120000) {
    cfgIdleFirebaseIntervalMs = idleFb;
  }
  cfgAutoBypassOnSensorFail = autoBypassEn;
  if (autoBypassSec >= 10 && autoBypassSec <= 300) {
    cfgAutoBypassDelaySec = autoBypassSec;
  }

  app_logger.logCloudEvent(APP_LOG_LEVEL_INFO, LogCategory::SYSTEM, EventCode::EVT_CONFIG_RESTORED, "Device config loaded.");
}

/**
 * @brief Persists the current device configuration to NVS.
 */
void saveDeviceConfigToNVS() {
  if (!prefs.begin(NVS_NAMESPACE, false)) {
    LOG(APP_LOG_LEVEL_ERROR, "NVS", "Failed to open for write. Config not persisted.");
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
  prefs.putBool("slp_en", cfgSleepEnabled);
  prefs.putInt("slp_start", cfgSleepStartHour);
  prefs.putInt("slp_end", cfgSleepEndHour);
  prefs.putInt("slp_emerg", cfgSleepEmergencyLevel);
  prefs.putInt("sens_thresh", cfgLevelSensorFailureThreshold);
  prefs.putInt("idle_sens_ms", cfgIdleSensorIntervalMs);
  prefs.putInt("idle_fb_ms", cfgIdleFirebaseIntervalMs);
  prefs.putBool("auto_bypass_en", cfgAutoBypassOnSensorFail);
  prefs.putInt("auto_bypass_sec", cfgAutoBypassDelaySec);
  prefs.putInt("schema_ver", NVS_SCHEMA_VERSION);
  prefs.end();
  LOG(APP_LOG_LEVEL_INFO, "NVS", "Device config saved.");
}

/**
 * @brief Detects crash loops and latches Safe Mode when threshold is exceeded.
 *
 * Increments a boot counter each startup. If the firmware survives
 * CRASH_LOOP_THRESHOLD consecutive reboots without reaching stable uptime
 * (60 s), the device enters Safe Mode until a full power cycle or timeout.
 */
void checkCrashLoop() {
  if (!prefs.begin(NVS_STATE_NAMESPACE, false)) {
    LOG(APP_LOG_LEVEL_ERROR, "BOOT", "NVS state namespace open failed.");
    return;
  }

  // Clear crash counters and safe mode latch on a full power cycle or hardware reset
  if (esp_reset_reason() == ESP_RST_POWERON || esp_reset_reason() == ESP_RST_EXT) {
    prefs.putULong("safe_mode_ms", 0);
    prefs.putInt("boot_count", 0);
    prefs.putUInt("safe_epoch", 0);
  }

  int bootCount = prefs.getInt("boot_count", 0);
  unsigned long safeModeStart = prefs.getULong("safe_mode_ms", 0);

  if (safeModeStart > 0) {
    // Persisted safe mode latch across resets. Auto-clear is handled in main loop.
    inSafeMode = true;
    safeModeEnteredMs = millis();
    lastFaultCode = "SAFE_MODE";
    lastFaultMessage = "Crash loop detected. Controller in safe mode. Waiting for timeout or manual recovery.";
    prefs.end();
    return;
  }

  // Deterministic crash-loop detection without relying on cross-boot timestamps:
  // - Increment boot_count at each boot.
  // - If the firmware survives long enough, it clears boot_count (see persistStateToNVS below).
  bootCount++;
  prefs.putInt("boot_count", bootCount);

  if (bootCount >= CRASH_LOOP_THRESHOLD) {
    inSafeMode = true;
    safeModeEnteredMs = millis();
    lastFaultCode = "SAFE_MODE";
    lastFaultMessage = "Crash loop detected. Controller in safe mode. Power cycle to recover.";
    prefs.putULong("safe_mode_ms", safeModeEnteredMs);
    app_logger.logCloudEvent(APP_LOG_LEVEL_ERROR, LogCategory::SYSTEM, EventCode::EVT_CRASH_LOOP_SAFE_MODE, "CRASH LOOP DETECTED: %d boots without reaching stable uptime. Entering SAFE MODE.", bootCount);
    LOG(APP_LOG_LEVEL_INFO, "SAFE MODE", "Pump OFF. Firebase disabled. Serial only.");
  } else {
    LOG(APP_LOG_LEVEL_INFO, "BOOT", "Boot count (uncleared): %d/%d", bootCount, CRASH_LOOP_THRESHOLD);
  }

  prefs.end();
}

/**
 * @brief Restores pump mode, error flags, bypass settings, and telemetry from NVS.
 */
void loadStateFromNVS() {
  if (!prefs.begin(NVS_STATE_NAMESPACE, true)) {
    return;
  }

  char modeBuf[16];
  if (prefs.getString("mode", modeBuf, sizeof(modeBuf)) == 0) {
    strcpy(modeBuf, "AUTO");
  }
  String savedMode = String(modeBuf);
  bool savedDryRun = prefs.getBool("dry_run_err", false);
  bool savedBypass = prefs.getBool("bypass_lvl", false);
  bool savedBypassFlow = prefs.getBool("bypass_flow", false);
  int savedLevel = prefs.getInt("level_pct", -1);
  totalPumpCycles = prefs.getUInt("pump_cycles", 0);
  totalPumpRunSec = prefs.getULong("pump_run_sec", 0);
  lastPersistedPumpCycles = totalPumpCycles;
  lastPersistedPumpRunSec = totalPumpRunSec;
  lastRebootRequestId = prefs.getInt("last_reboot_id", 0);
  cfgLastCountdownDurationMin = prefs.getInt("cd_dur_min", 15);
  prefs.end();

  // Option B: Safest standard boot - force MANUAL mode on startup
  pumpMode = "MANUAL";
  manualDesired = false;
  forceCloudManualOverride = true;
  
  if (savedMode != "MANUAL") {
    LOG(APP_LOG_LEVEL_WARN, "BOOT", "Previous mode was %s. Forcing to MANUAL OFF for safety.", savedMode.c_str());
    // Leave lastPersistedMode as the old mode so persistStateToNVS() will see the change
    // and sync the new MANUAL mode back to NVS and Cloud on the first loop.
    lastPersistedMode = savedMode;
  } else {
    LOG(APP_LOG_LEVEL_INFO, "BOOT", "Restored MANUAL mode. Pump remains OFF.");
    lastPersistedMode = "MANUAL";
  }
  isDryRunError = savedDryRun;
  lastPersistedDryRun = savedDryRun;
  cfgBypassLevelSensor = savedBypass;
  lastPersistedBypass = savedBypass;
  cfgBypassFlowSensor = savedBypassFlow;
  lastPersistedBypassFlow = savedBypassFlow;

  LOG(APP_LOG_LEVEL_INFO, "BOOT", "Last state: Level=%d%%, Mode=%s, DryRun=%s, BypassLvl=%s, BypassFlow=%s, Cycles=%lu, RunSec=%lu", savedLevel, pumpMode.c_str(), isDryRunError ? "YES" : "NO", savedBypass ? "YES" : "NO", savedBypassFlow ? "YES" : "NO",
                (unsigned long)totalPumpCycles, (unsigned long)totalPumpRunSec);
}

/**
 * @brief Persists runtime state to NVS with delta-write optimization.
 *
 * Only writes parameters that have actually changed since the last persist
 * call, minimizing flash wear. Also clears the crash-loop counter once
 * the firmware reaches stable uptime (60 s).
 */
void persistStateToNVS() {
  bool modeChanged = (pumpMode != lastPersistedMode);
  bool dryRunChanged = (isDryRunError != lastPersistedDryRun);
  bool bypassChanged = (cfgBypassLevelSensor != lastPersistedBypass);
  bool bypassFlowChanged = (cfgBypassFlowSensor != lastPersistedBypassFlow);
  bool telemetryChanged = (totalPumpCycles != lastPersistedPumpCycles) || (totalPumpRunSec != lastPersistedPumpRunSec);
  int levelDelta = abs(waterLevelPct - lastPersistedLevel);
  unsigned long now = millis();
  bool levelNeedsWrite = (lastPersistedLevel == -1)
    || (levelDelta >= NVS_LEVEL_DELTA_THRESHOLD);
  // After the controller has been up for a while, clear crash-loop counter.
  // This is the "stable uptime" signal used by checkCrashLoop().
  static bool crashLoopClearedThisBoot = false;
  bool crashLoopClearDue = (!crashLoopClearedThisBoot) && (now >= 60000UL);

  if (!modeChanged && !dryRunChanged && !bypassChanged && !bypassFlowChanged
      && !telemetryChanged && !levelNeedsWrite && !crashLoopClearDue) {
    return;
  }

  if (!prefs.begin(NVS_STATE_NAMESPACE, false)) {
    return;
  }

  if (modeChanged) {
    prefs.putString("mode", pumpMode);
    lastPersistedMode = pumpMode;
    LOG(APP_LOG_LEVEL_INFO, "NVS", "Mode persisted: %s", pumpMode.c_str());
  }
  if (dryRunChanged) {
    prefs.putBool("dry_run_err", isDryRunError);
    lastPersistedDryRun = isDryRunError;
  }
  if (bypassChanged) {
    prefs.putBool("bypass_lvl", cfgBypassLevelSensor);
    lastPersistedBypass = cfgBypassLevelSensor;
    LOG(APP_LOG_LEVEL_INFO, "NVS", "Bypass level sensor persisted: %s", cfgBypassLevelSensor ? "ON" : "OFF");
  }
  if (bypassFlowChanged) {
    prefs.putBool("bypass_flow", cfgBypassFlowSensor);
    lastPersistedBypassFlow = cfgBypassFlowSensor;
    LOG(APP_LOG_LEVEL_INFO, "NVS", "Bypass flow sensor persisted: %s", cfgBypassFlowSensor ? "ON" : "OFF");
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

  if (crashLoopClearDue) {
    prefs.putInt("boot_count", 0);
    crashLoopClearedThisBoot = true;
    LOG(APP_LOG_LEVEL_INFO, "BOOT", "Stable uptime reached. Crash-loop counter cleared.");
  }

  prefs.end();
}

/**
 * @brief Clears local Wi-Fi and enrollment material from NVS.
 *
 * Preserves safety latches, pump state, counters, and crash-loop data
 * that must survive a reprovisioning event.
 *
 * @return true if successfully cleared, false if NVS could not be opened.
 */
bool clearNetworkEnrollment() {
  if (!prefs.begin(NVS_STATE_NAMESPACE, false)) {
    LOG(APP_LOG_LEVEL_ERROR, "NVS", "Unable to clear local enrollment state.");
    return false;
  }

  // Do not clear this namespace wholesale. It also holds safety latches, pump
  // state, counters, and crash-loop data that must survive reprovisioning.
  prefs.remove("wifi_ssid");
  prefs.remove("wifi_pass");
  prefs.remove("claim_token");
  // Remove only the obsolete anonymous-auth record written by earlier builds.
  prefs.remove("firmwareUid");
  prefs.end();

  LOG(APP_LOG_LEVEL_INFO, "NVS", "Local Wi-Fi and enrollment material cleared.");
  return true;
}
