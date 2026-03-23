#include "persistence.h"

#include "../state/state.h"

void loadDeviceConfigFromNVS() {
  if (!prefs.begin(NVS_NAMESPACE, true)) {
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

  bool slpEn = prefs.getBool("slp_en", SLEEP_DEFAULT_ENABLED);
  int slpStart = prefs.getInt("slp_start", SLEEP_DEFAULT_START_HOUR);
  int slpEnd = prefs.getInt("slp_end", SLEEP_DEFAULT_END_HOUR);
  int slpEmerg = prefs.getInt("slp_emerg", SLEEP_DEFAULT_EMERGENCY_LVL);

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

  if (slpStart >= 0 && slpStart <= 23) cfgSleepStartHour = slpStart;
  if (slpEnd >= 0 && slpEnd <= 23) cfgSleepEndHour = slpEnd;
  if (slpEmerg >= 0 && slpEmerg <= 100) cfgSleepEmergencyLevel = slpEmerg;
  cfgSleepEnabled = slpEn;

  if (sensThresh >= 3 && sensThresh <= 20) cfgLevelSensorFailureThreshold = sensThresh;
  if (idleSens >= 5000 && idleSens <= 60000) cfgIdleSensorIntervalMs = idleSens;
  if (idleFb >= 10000 && idleFb <= 120000) cfgIdleFirebaseIntervalMs = idleFb;
  cfgAutoBypassOnSensorFail = autoBypassEn;
  if (autoBypassSec >= 10 && autoBypassSec <= 300) cfgAutoBypassDelaySec = autoBypassSec;

  Serial.println("[NVS] Device config loaded.");
}

void saveDeviceConfigToNVS() {
  if (!prefs.begin(NVS_NAMESPACE, false)) {
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
  Serial.println("[NVS] Device config saved.");
}

bool isInSleepWindow(int currentHour) {
  if (cfgSleepStartHour <= cfgSleepEndHour) {
    return (currentHour >= cfgSleepStartHour && currentHour < cfgSleepEndHour);
  } else {
    return (currentHour >= cfgSleepStartHour) || (currentHour < cfgSleepEndHour);
  }
}

void checkCrashLoop() {
  if (!prefs.begin(NVS_STATE_NAMESPACE, false)) {
    Serial.println("[BOOT] NVS state namespace open failed.");
    return;
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
    Serial.printf("[ERROR] CRASH LOOP DETECTED: %d boots without reaching stable uptime. Entering SAFE MODE.\n", bootCount);
    Serial.println("[SAFE MODE] Pump OFF. Firebase disabled. Serial only.");
  } else {
    Serial.printf("[BOOT] Boot count (uncleared): %d/%d\n", bootCount, CRASH_LOOP_THRESHOLD);
  }

  prefs.end();
}

void loadStateFromNVS() {
  if (!prefs.begin(NVS_STATE_NAMESPACE, true)) return;

  String savedMode = prefs.getString("mode", "AUTO");
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

  // Only AUTO / MANUAL / COUNTDOWN are valid modes. Legacy values map to AUTO.
  savedMode.trim();
  savedMode.toUpperCase();
  if (savedMode == "AUTO" || savedMode == "COUNTDOWN" || savedMode == "MANUAL") {
    pumpMode = savedMode;
    lastPersistedMode = savedMode;
    if (savedMode == "COUNTDOWN") {
      Serial.println("[BOOT] Restored COUNTDOWN mode from NVS. Pump idle until user starts a new timer.");
    } else if (savedMode == "MANUAL") {
      manualDesired = false;  // never auto-start after reboot
      Serial.println("[BOOT] Restored MANUAL mode. Pump remains OFF until manual_desired is true.");
    }
  } else {
    pumpMode = "AUTO";
    lastPersistedMode = "AUTO";
    Serial.printf("[BOOT] Legacy/invalid mode '%s' not restored. Defaulting to AUTO.\n", savedMode.c_str());
  }
  isDryRunError = savedDryRun;
  lastPersistedDryRun = savedDryRun;
  cfgBypassLevelSensor = savedBypass;
  lastPersistedBypass = savedBypass;
  cfgBypassFlowSensor = savedBypassFlow;
  lastPersistedBypassFlow = savedBypassFlow;

  Serial.printf("[BOOT] Last state: Level=%d%%, Mode=%s, DryRun=%s, BypassLvl=%s, BypassFlow=%s, Cycles=%lu, RunSec=%lu\n",
                savedLevel, pumpMode.c_str(), isDryRunError ? "YES" : "NO", savedBypass ? "YES" : "NO", savedBypassFlow ? "YES" : "NO",
                (unsigned long)totalPumpCycles, (unsigned long)totalPumpRunSec);
}

void persistStateToNVS() {
  bool modeChanged = (pumpMode != lastPersistedMode);
  bool dryRunChanged = (isDryRunError != lastPersistedDryRun);
  bool bypassChanged = (cfgBypassLevelSensor != lastPersistedBypass);
  bool bypassFlowChanged = (cfgBypassFlowSensor != lastPersistedBypassFlow);
  bool telemetryChanged = (totalPumpCycles != lastPersistedPumpCycles) || (totalPumpRunSec != lastPersistedPumpRunSec);
  int levelDelta = abs(waterLevelPct - lastPersistedLevel);
  unsigned long now = millis();
  bool levelNeedsWrite = (lastPersistedLevel == -1)
    || (levelDelta >= NVS_LEVEL_DELTA_THRESHOLD)
    || (now - lastLevelWriteMs >= NVS_LEVEL_INTERVAL_MS);
  bool uptimeNeedsWrite = (now - lastUptimeWriteMs >= NVS_UPTIME_INTERVAL_MS);
  // After the controller has been up for a while, clear crash-loop counter.
  // This is the "stable uptime" signal used by checkCrashLoop().
  static bool crashLoopClearedThisBoot = false;
  bool crashLoopClearDue = (!crashLoopClearedThisBoot) && (now >= 60000UL);

  if (!modeChanged && !dryRunChanged && !bypassChanged && !bypassFlowChanged && !telemetryChanged && !levelNeedsWrite && !uptimeNeedsWrite && !crashLoopClearDue) return;

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
  if (bypassFlowChanged) {
    prefs.putBool("bypass_flow", cfgBypassFlowSensor);
    lastPersistedBypassFlow = cfgBypassFlowSensor;
    Serial.printf("[NVS] Bypass flow sensor persisted: %s\n", cfgBypassFlowSensor ? "ON" : "OFF");
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
    lastUptimeWriteMs = now;
  }

  if (crashLoopClearDue) {
    prefs.putInt("boot_count", 0);
    crashLoopClearedThisBoot = true;
    Serial.println("[BOOT] Stable uptime reached. Crash-loop counter cleared.");
  }

  prefs.end();
}

