#include "connectivity_cloud.h"
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include <esp_task_wdt.h>

#include "../state/state.h"
#include "../persistence/persistence.h"
#include "../safety/safety_pump.h"
#include "../config/config.h"
#include "../utils/time_utils.h"

void readDeviceConfigFromFirebase() {
  if (!Firebase.RTDB.getJSON(&fbdo, "/pump_system/config/device")) return;

  FirebaseJson json = fbdo.to<FirebaseJson>();
  FirebaseJsonData jsonData;

  int te = 0, tf = 0, ps = 0, po = 0, drSec = 0, maxRun = 0;
  float drLpm = 0.0f, flowCal = 0.0f;
  bool allOk = true;
  static uint32_t lastBadDeviceConfigLogMs = 0;

  json.get(jsonData, "tank_empty_cm");
  if (jsonData.success) {
    int v = jsonData.intValue;
    if (v >= 5 && v <= 200) te = v; else allOk = false;
  } else {
    allOk = false;
  }

  json.get(jsonData, "tank_full_cm");
  if (allOk && jsonData.success) {
    int v = jsonData.intValue;
    if (v >= 1 && v < te) tf = v; else allOk = false;
  } else {
    allOk = false;
  }

  json.get(jsonData, "pump_start_level");
  if (allOk && jsonData.success) {
    int v = jsonData.intValue;
    if (v >= 0 && v <= 100) ps = v; else allOk = false;
  } else {
    allOk = false;
  }

  json.get(jsonData, "pump_stop_level");
  if (allOk && jsonData.success) {
    int v = jsonData.intValue;
    if (v >= 0 && v <= 100 && v > ps) po = v; else allOk = false;
  } else {
    allOk = false;
  }

  json.get(jsonData, "dry_run_threshold_lpm");
  if (allOk && jsonData.success) {
    float v = (jsonData.typeNum == FirebaseJson::JSON_INT) ? (float)jsonData.intValue : (float)jsonData.doubleValue;
    if (v >= 0.1f && v <= 10.0f) drLpm = v; else allOk = false;
  } else {
    allOk = false;
  }

  json.get(jsonData, "dry_run_timeout_sec");
  if (allOk && jsonData.success) {
    int v = jsonData.intValue;
    if (v >= 10 && v <= 300) drSec = v; else allOk = false;
  } else {
    allOk = false;
  }

  json.get(jsonData, "flow_calibration_factor");
  if (allOk && jsonData.success) {
    float v = (jsonData.typeNum == FirebaseJson::JSON_INT) ? (float)jsonData.intValue : (float)jsonData.doubleValue;
    if (v >= 0.1f && v <= 20.0f) flowCal = v; else allOk = false;
  } else {
    allOk = false;
  }

  json.get(jsonData, "max_pump_runtime_min");
  if (jsonData.success) {
    int v = jsonData.intValue;
    if (v >= 30 && v <= 480) maxRun = v; else maxRun = cfgMaxPumpRuntimeMin;
  } else {
    maxRun = cfgMaxPumpRuntimeMin;
  }

  bool slpEn = cfgSleepEnabled;
  int slpStart = cfgSleepStartHour;
  int slpEnd = cfgSleepEndHour;
  int slpEmerg = cfgSleepEmergencyLevel;
  json.get(jsonData, "sleep_enabled");
  if (jsonData.success) slpEn = jsonData.boolValue;
  json.get(jsonData, "sleep_start_hour");
  if (jsonData.success) { int v = jsonData.intValue; if (v >= 0 && v <= 23) slpStart = v; }
  json.get(jsonData, "sleep_end_hour");
  if (jsonData.success) { int v = jsonData.intValue; if (v >= 0 && v <= 23) slpEnd = v; }
  json.get(jsonData, "sleep_emergency_level");
  if (jsonData.success) { int v = jsonData.intValue; if (v >= 0 && v <= 100) slpEmerg = v; }

  int sensThresh = cfgLevelSensorFailureThreshold;
  int idleSens = cfgIdleSensorIntervalMs;
  int idleFb = cfgIdleFirebaseIntervalMs;
  json.get(jsonData, "level_sensor_failure_threshold");
  if (jsonData.success) { int v = jsonData.intValue; if (v >= 3 && v <= 20) sensThresh = v; }
  if (sensThresh == cfgLevelSensorFailureThreshold) {
    json.get(jsonData, "sensor_failure_threshold");
    if (jsonData.success) { int v = jsonData.intValue; if (v >= 3 && v <= 20) sensThresh = v; }
  }
  json.get(jsonData, "idle_sensor_interval_ms");
  if (jsonData.success) { int v = jsonData.intValue; if (v >= 5000 && v <= 60000) idleSens = v; }
  json.get(jsonData, "idle_firebase_interval_ms");
  if (jsonData.success) { int v = jsonData.intValue; if (v >= 10000 && v <= 120000) idleFb = v; }
  bool autoBypassEn = cfgAutoBypassOnSensorFail;
  int autoBypassSec = cfgAutoBypassDelaySec;
  json.get(jsonData, "auto_bypass_on_sensor_fail");
  if (jsonData.success) autoBypassEn = jsonData.boolValue;
  json.get(jsonData, "auto_bypass_delay_sec");
  if (jsonData.success) { int v = jsonData.intValue; if (v >= 10 && v <= 300) autoBypassSec = v; }

  // REFACTOR [H-01]: runtime debug log level (0-4), optional.
  json.get(jsonData, "debug_log_level");
  if (jsonData.success) {
    int v = jsonData.intValue;
    if (v >= 0 && v <= 4) {
      gLogLevel = (uint8_t)v;
    }
  }

  if (!allOk) {
    uint32_t now = millis();
    // Avoid spamming logs if the dashboard is sending partial/invalid config.
    if (elapsedMillis32(now, lastBadDeviceConfigLogMs) >= 60000UL) {
      lastBadDeviceConfigLogMs = now;
      LOG(LOG_LEVEL_WARN, "FIREBASE", "Device config JSON missing/invalid required fields; keeping existing config.");
    }
    return;
  }

  bool floatsChanged = (fabsf(drLpm - cfgDryRunThresholdLpm) > 0.01f) ||
                       (fabsf(flowCal - cfgFlowCalibration) > 0.01f);

  bool sleepChanged = (slpEn != cfgSleepEnabled || slpStart != cfgSleepStartHour ||
                       slpEnd != cfgSleepEndHour || slpEmerg != cfgSleepEmergencyLevel);
  bool advancedChanged = (sensThresh != cfgLevelSensorFailureThreshold ||
                          idleSens != cfgIdleSensorIntervalMs || idleFb != cfgIdleFirebaseIntervalMs ||
                          autoBypassEn != cfgAutoBypassOnSensorFail || autoBypassSec != cfgAutoBypassDelaySec);

  bool changed = (te != cfgTankEmptyCm || tf != cfgTankFullCm || ps != cfgPumpStartLevel || po != cfgPumpStopLevel
                  || drSec != cfgDryRunTimeoutSec || maxRun != cfgMaxPumpRuntimeMin || floatsChanged || sleepChanged || advancedChanged);
  if (!changed) return;

  cfgTankEmptyCm        = te;
  cfgTankFullCm         = tf;
  cfgPumpStartLevel     = ps;
  cfgPumpStopLevel      = po;
  cfgDryRunThresholdLpm = drLpm;
  cfgDryRunTimeoutSec   = drSec;
  cfgFlowCalibration    = flowCal;
  cfgMaxPumpRuntimeMin  = maxRun;
  cfgSleepEnabled        = slpEn;
  cfgSleepStartHour      = slpStart;
  cfgSleepEndHour        = slpEnd;
  cfgSleepEmergencyLevel = slpEmerg;
  cfgLevelSensorFailureThreshold = sensThresh;
  cfgIdleSensorIntervalMs   = idleSens;
  cfgIdleFirebaseIntervalMs = idleFb;
  cfgAutoBypassOnSensorFail = autoBypassEn;
  cfgAutoBypassDelaySec     = (autoBypassSec >= 10 && autoBypassSec <= 300) ? autoBypassSec : cfgAutoBypassDelaySec;
  saveDeviceConfigToNVS();
  LOG(LOG_LEVEL_INFO, "FIREBASE", "Device config updated.");
}

void checkCountdownExpiry() {
  if (!isCountdownActive || pumpMode != "COUNTDOWN") return;
  uint32_t now = millis();
  if (countdownEndMs != 0 && millisDeadlineReached(now, countdownEndMs)) {
    LOG(LOG_LEVEL_INFO, "COUNTDOWN", "Timer expired. Pump stopped. Mode stays COUNTDOWN.");
    isCountdownActive = false;
    countdownEndMs    = 0;
    // Option B: pumpMode stays "COUNTDOWN" — pump is idle, user must start a new timer.
  }
}

bool readFirebaseControl() {
  unsigned long t0 = millis();
  static bool lastAddTime = false;
  static bool lastCountdownStart = false;

  if (!Firebase.RTDB.getJSON(&fbdo, "/pump_system/control")) {
    String err = fbdo.errorReason();
    firebaseConsecutiveFailCount++;
    cloudControlFailCount++;
    firebaseLastError = err;
    LOG(LOG_LEVEL_ERROR, "FIREBASE", "Control read failed: %s", err.c_str());
    if (err.indexOf("token is not ready") >= 0 || err.indexOf("revoked") >= 0 || err.indexOf("expired") >= 0) {
      firebaseAuthErrorCount++;
      unsigned long now = millis();
      firebaseCooldownUntilMs = max(firebaseCooldownUntilMs, (unsigned long)addMillisSaturated(now, FIREBASE_AUTH_COOLDOWN_MS));
      Firebase.refreshToken(&config);
      LOG(LOG_LEVEL_INFO, "FIREBASE", "Auth not ready/expired; backing off and requesting token refresh.");
    } else if (err.indexOf("payload read timed out") >= 0 || err.indexOf("read timed out") >= 0) {
      firebaseTimeoutCount++;
      fbdo.stopWiFiClient(); // Force teardown of dead TCP socket to escape the failure loop
      if (firebaseConsecutiveFailCount >= STATUS_PUSH_RETRY_MAX) {
        unsigned long now = millis();
        firebaseCooldownUntilMs = max(firebaseCooldownUntilMs, (unsigned long)addMillisSaturated(now, 30000UL));
        LOG(LOG_LEVEL_ERROR, "FIREBASE", "Control read timeout; cooling down 30s.");
      } else {
        LOG(LOG_LEVEL_ERROR, "FIREBASE", "Control read timeout; retrying (%d/%d).", (int)firebaseConsecutiveFailCount, STATUS_PUSH_RETRY_MAX);
      }
    }
    cloudLastControlCallMs = (uint32_t)(millis() - t0);
    return false;
  }

  firebaseConsecutiveFailCount = 0;
  cloudControlOkCount++;
  cloudLastControlOkMs = millis();
  FirebaseJson controlJson = fbdo.to<FirebaseJson>();
  FirebaseJsonData jd;

  String firebaseReadMode = "";
  controlJson.get(jd, "mode");
  if (jd.success) {
    String newMode = jd.stringValue;
    newMode.trim();
    newMode.toUpperCase();
    firebaseReadMode = newMode;
    // SAFETY: Block mode *application* while latched; still process reset_stop / clear_error below (M-27).
    if (emergencyStopLatched) {
      LOG(LOG_LEVEL_WARN, "E-STOP", "Mode change blocked by emergency stop latch. Current mode: %s", pumpMode.c_str());
    } else {
      // Only AUTO / MANUAL / COUNTDOWN are valid policy modes.
      if (newMode == "AUTO" || newMode == "COUNTDOWN" || newMode == "MANUAL") {
        if (pendingModeWriteback) {
          if (newMode == pumpMode) {
            pendingModeWriteback = false;
            pendingModeWritebackSentMs = 0;
            LOG(LOG_LEVEL_INFO, "FIREBASE", "Mode write-back confirmed.");
          } else if (pendingModeWritebackSentMs == 0 ||
                     elapsedMillis32(millis(), pendingModeWritebackSentMs) >= 5000UL) {
            Firebase.RTDB.setString(&fbdo, "/pump_system/control/mode", pumpMode);
            pendingModeWritebackSentMs = millis();
            LOG(LOG_LEVEL_INFO, "FIREBASE", "Mode write-back: %s (dashboard sync).", pumpMode.c_str());
          }
        } else {
          if (pumpMode != newMode) {
            LOG(LOG_LEVEL_INFO, "FIREBASE", "Mode changed: %s -> %s", pumpMode.c_str(), newMode.c_str());
          }
          pumpMode = newMode;
        }
      } else if (newMode == "FORCE_ON" || newMode == "FORCE_OFF") {
        // Backward compatibility: map deprecated FORCE modes to AUTO and write back.
        LOG(LOG_LEVEL_INFO, "FIREBASE", "Deprecated mode '%s' received. Mapping to AUTO.", newMode.c_str());
        pumpMode = "AUTO";
        pendingModeWriteback = true;
        pendingModeWritebackSentMs = 0;
      } else {
        LOG(LOG_LEVEL_INFO, "FIREBASE", "Unknown mode received: '%s'. Ignoring.", newMode.c_str());
      }
    }
  }

  // MANUAL intent (persistent)
  controlJson.get(jd, "manual_desired");
  if (jd.success) {
    manualDesired = jd.boolValue;
  }

  // Emergency stop (one-shot).
  // FIX [M-16]: Use Firebase self-clear as idempotency guard instead of the static
  // edge-detection flag. The flag is kept for the falling-edge no-op path only.
  // This ensures a soft-reset that sees emergency_stop=true still latches correctly
  // rather than silently skipping it because lastEmergencyStop was stale-true.
  controlJson.get(jd, "emergency_stop");
  if (jd.success) {
    bool v = jd.boolValue;
    if (v) {
      // Process the latch every time the flag is true (Firebase clear is the one-shot guard).
      if (!emergencyStopLatched) {
        LOG(LOG_LEVEL_INFO, "E-STOP", "Emergency stop requested. Saving mode: %s", pumpMode.c_str());
        emergencyStopLatched = true;
        emergencyStopSavedMode = pumpMode;  // Preserve current mode
        manualDesired = false;
        setPump(false);
        if (isCountdownActive) { isCountdownActive = false; countdownEndMs = 0; }
      }
      Firebase.RTDB.setBool(&fbdo, "/pump_system/control/emergency_stop", false);
    }
  }

  // Reset stop latch (one-shot)
  controlJson.get(jd, "reset_stop");
  if (jd.success) {
    bool v = jd.boolValue;
    // M-16 robustness: do not rely on a potentially stale edge-detection flag for operator-critical reset_stop.
    if (v) {
      if (isDryRunError || isOverflowError) {
        LOG(LOG_LEVEL_INFO, "E-STOP", "Reset requested but hard lockout active; ignoring.");
      } else {
        LOG(LOG_LEVEL_INFO, "E-STOP", "Reset stop requested. Restoring mode: %s", emergencyStopSavedMode.c_str());
        emergencyStopLatched = false;
        pumpMode = emergencyStopSavedMode;  // Restore saved mode
      }
      Firebase.RTDB.setBool(&fbdo, "/pump_system/control/reset_stop", false);
    }
  }

  // COUNTDOWN start (one-shot): Firebase self-clear is the idempotency guard.
  bool startCountdown = false;
  controlJson.get(jd, "countdown_start");
  if (jd.success) {
    bool v = jd.boolValue;
    if (v && !lastCountdownStart) {
      startCountdown = true;
      Firebase.RTDB.setBool(&fbdo, "/pump_system/control/countdown_start", false);
    }
    lastCountdownStart = v;
  }

  if (pumpMode == "COUNTDOWN" && !isCountdownActive && startCountdown) {
    int durationMin = cfgLastCountdownDurationMin;
    controlJson.get(jd, "countdown_duration_min");
    if (jd.success) {
      int v = (jd.typeNum == FirebaseJson::JSON_INT) ? jd.intValue : (int)jd.doubleValue;
      durationMin = constrain(v, 1, COUNTDOWN_MAX_DURATION_MIN);
      if (durationMin != cfgLastCountdownDurationMin) {
        cfgLastCountdownDurationMin = durationMin;
        if (prefs.begin(NVS_STATE_NAMESPACE, false)) {
          prefs.putInt("cd_dur_min", cfgLastCountdownDurationMin);
          prefs.end();
        }
      }
    } else {
      durationMin = constrain(durationMin, 1, COUNTDOWN_MAX_DURATION_MIN);
    }
    countdownEndMs = addMillisSaturated(millis(), (uint32_t)durationMin * 60000UL);
    isCountdownActive = true;
    lastAddTime = false;
    LOG(LOG_LEVEL_INFO, "COUNTDOWN", "Started: %d min.%s", durationMin,
                  Firebase.ready() ? "" : " (offline — using last known duration)");
  }

  if (pumpMode == "COUNTDOWN") {
    controlJson.get(jd, "countdown_add_time");
    if (jd.success) {
      bool v = jd.boolValue;
      if (v && !lastAddTime) {
        int addMin = COUNTDOWN_ADD_TIME_MIN;
        controlJson.get(jd, "countdown_add_min");
        if (jd.success) {
          int n = (jd.typeNum == FirebaseJson::JSON_INT) ? jd.intValue : (int)jd.doubleValue;
          if (n >= 1 && n <= COUNTDOWN_MAX_DURATION_MIN) addMin = n;
        }
        uint32_t nowMs = millis();
        if (isCountdownActive && countdownEndMs != 0 && !millisDeadlineReached(nowMs, countdownEndMs)) {
          uint32_t maxEnd = addMillisSaturated(nowMs, (uint32_t)COUNTDOWN_MAX_DURATION_MIN * 60000UL);
          uint32_t addMs = (uint32_t)addMin * 60000UL;
          uint32_t candidate = addMillisSaturated(countdownEndMs, addMs);
          countdownEndMs = min(candidate, maxEnd);
          LOG(LOG_LEVEL_INFO, "COUNTDOWN", "+%d min added.", addMin);
        }
      }
      // One-shot clear is always attempted when true, even if edge-detected as duplicate.
      if (v) Firebase.RTDB.setBool(&fbdo, "/pump_system/control/countdown_add_time", false);
      lastAddTime = v;
    }
  }

  if (pumpMode != "COUNTDOWN" && isCountdownActive) {
    isCountdownActive = false;
    countdownEndMs = 0;
    LOG(LOG_LEVEL_INFO, "COUNTDOWN", "Mode exited. Countdown cleared.");
  }

  // countdown_stop one-shot (Option B): stop active timer without changing mode
  controlJson.get(jd, "countdown_stop");
  if (jd.success && jd.boolValue) {
    if (isCountdownActive && pumpMode == "COUNTDOWN") {
      LOG(LOG_LEVEL_INFO, "COUNTDOWN", "Stop requested. Timer cleared. Mode stays COUNTDOWN.");
      isCountdownActive = false;
      countdownEndMs = 0;
    }
    Firebase.RTDB.setBool(&fbdo, "/pump_system/control/countdown_stop", false);
  }

  // bypass_flow_sensor
  controlJson.get(jd, "bypass_flow_sensor");
  if (jd.success) {
    bool v = jd.boolValue;
    if (v != cfgBypassFlowSensor) {
      cfgBypassFlowSensor = v;
      LOG(LOG_LEVEL_INFO, "FIREBASE", "Bypass flow sensor: %s", v ? "ON" : "OFF");
    }
  }

  controlJson.get(jd, "bypass_level_sensor");
  if (jd.success) {
    bool v = jd.boolValue;
    if (v != cfgBypassLevelSensor) {
      cfgBypassLevelSensor = v;
      if (!v) {
        autoBypassActive = false;
        autoBypassWasEngaged = false;
      }
      LOG(LOG_LEVEL_INFO, "FIREBASE", "Bypass level sensor: %s", v ? "ON" : "OFF");
    }
  }

  controlJson.get(jd, "clear_error");
  if (jd.success && jd.boolValue == true) {
    bool hadError = isDryRunError || isOverflowError;
    if (hadError) {
      isDryRunError     = false;
      isOverflowError   = false;
      dryRunTimerActive = false;
      dryRunStartMs     = 0;
      pumpAutoStartTracking = false;
      pumpAutoStartMs   = 0;
      LOG(LOG_LEVEL_ERROR, "FIREBASE", "Errors cleared.");
      lastFaultCode = "";
      lastFaultMessage = "";
    }
    // Always clear one-shot command to avoid a stuck true flag on benign calls.
    Firebase.RTDB.setBool(&fbdo, "/pump_system/control/clear_error", false);
  }

  controlJson.get(jd, "reboot_request_id");
  if (jd.success) {
    int requestedId = (jd.typeNum == FirebaseJson::JSON_INT) ? jd.intValue : (int)jd.doubleValue;
    if (requestedId > 0 && requestedId != lastRebootRequestId) {
      LOG(LOG_LEVEL_INFO, "FIREBASE", "Reboot requested (id=%d).", requestedId);
      lastRebootRequestId = requestedId;
      if (prefs.begin(NVS_STATE_NAMESPACE, false)) {
        prefs.putInt("last_reboot_id", lastRebootRequestId);
        prefs.end();
      }
      Firebase.RTDB.setInt(&fbdo, "/pump_system/status/last_reboot_request_id", lastRebootRequestId);
      Firebase.RTDB.setInt(&fbdo, "/pump_system/status/last_reboot_at", (int)(esp_timer_get_time() / 1000000ULL));
      delay(100);
      ESP.restart();
    }
  }

  static unsigned long lastHeartbeatMs = 0;
  if (elapsedMillis32(millis(), lastHeartbeatMs) >= 15000UL) {
    LOG(LOG_LEVEL_INFO, "FIREBASE", "Poll OK | Mode: %s | Relay: %s", pumpMode.c_str(), isRunning ? "ON" : "OFF");
    lastHeartbeatMs = millis();
  }

  cloudLastControlCallMs = (uint32_t)(millis() - t0);
  return true;
}

bool pushFirebaseStatus() {
  unsigned long t0 = millis();
  FirebaseJson statusJson;
  uint32_t uptimeMinutes = (uint32_t)(esp_timer_get_time() / 60000000ULL);
  // Prefer the actual sensor freshness clock for UI reporting so the dashboard reflects
  // the latest recovered level reading even if the RS-485 transport briefly blips.
  bool levelFresh = (levelLastValidMs > 0) &&
                    (elapsedMillis32(millis(), levelLastValidMs) <= LEVEL_STALE_TIMEOUT_MS);

  // REFACTOR [C-02]: omit level until first valid frame.
  if (waterLevelPct >= 0) {
    statusJson.set("water_level_percent", waterLevelPct);
  }
  statusJson.set("is_running",          isRunning);
  statusJson.set("flow_rate_lpm",       flowRateLpm);
  statusJson.set("is_error",            isDryRunError);
  statusJson.set("is_level_sensor_error", isLevelSensorError);
  statusJson.set("is_flow_sensor_error",  isFlowSensorError);
  statusJson.set("is_overflow_error",   isOverflowError);
  statusJson.set("manual_runtime_warning", manualRuntimeWarning);
  statusJson.set("bypass_level_sensor", cfgBypassLevelSensor);
  statusJson.set("bypass_flow_sensor",  cfgBypassFlowSensor);
  statusJson.set("auto_bypass_active",  autoBypassActive);
  statusJson.set("is_sleeping",         isSleeping);
  statusJson.set("is_idle_mode",        isIdleMode);
  statusJson.set("wifi_rssi",           wifiRssi);
  statusJson.set("last_boot_reason",    bootReasonStr);
  statusJson.set("debug_log_level",     (int)gLogLevel);
  statusJson.set("uptime_minutes",      uptimeMinutes);
  statusJson.set("free_heap_bytes",     (int)ESP.getFreeHeap());
#if defined(ESP32)
  statusJson.set("min_free_heap_bytes", (int)ESP.getMinFreeHeap());
  statusJson.set("max_alloc_heap_bytes",(int)ESP.getMaxAllocHeap());
#endif
  statusJson.set("min_free_heap_observed_bytes", (int)minFreeHeapObserved);
  statusJson.set("firebase_consecutive_failures", (int)firebaseConsecutiveFailCount);
  statusJson.set("firebase_last_error", firebaseLastError);
  statusJson.set("firebase_timeout_count", (int)firebaseTimeoutCount);
  statusJson.set("firebase_auth_error_count", (int)firebaseAuthErrorCount);
  statusJson.set("firebase_not_ready_skip_count", (int)firebaseNotReadySkipCount);
  statusJson.set("cloud_control_ok_count", (int)cloudControlOkCount);
  statusJson.set("cloud_control_fail_count", (int)cloudControlFailCount);
  statusJson.set("cloud_status_ok_count", (int)cloudStatusOkCount);
  statusJson.set("cloud_status_fail_count", (int)cloudStatusFailCount);
  uint32_t controlOkAgeSec = (cloudLastControlOkMs == 0)
    ? 0
    : (elapsedMillis32(millis(), cloudLastControlOkMs) / 1000UL);
  bool controlPollStale = (cloudLastControlOkMs == 0) || (controlOkAgeSec > 20UL);
  statusJson.set("cloud_last_control_ok_age_sec", (int)controlOkAgeSec);
  statusJson.set("cloud_control_poll_stale", controlPollStale);
  statusJson.set("cloud_last_control_call_ms", (int)cloudLastControlCallMs);
  statusJson.set("cloud_last_status_call_ms", (int)cloudLastStatusCallMs);
  statusJson.set("cloud_last_cycle_ms", (int)cloudLastCycleMs);
  statusJson.set("rs485_last_call_ms", (int)rs485LastCallMs);
  statusJson.set("loop_max_ms", (int)loopMaxMs);
  statusJson.set("ultrasonic_cycles_ok",         (int)ultrasonicCycleOkCount);
  statusJson.set("ultrasonic_cycles_timeout",    (int)ultrasonicCycleTimeoutCount);
  statusJson.set("ultrasonic_last_good_cm",      (float)ultrasonicLastGoodCmX10 / 10.0f);
  statusJson.set("flow_discard_max_sane",        (int)flowDiscardMaxSaneCount);
  statusJson.set("flow_stuck_high_events",       (int)flowStuckHighEventCount);
  statusJson.set("remote_level_discard_count",   (int)remoteSensorLevelDiscardCount);

  // UI truth signals
  statusJson.set("manual_desired",        manualDesired);
  statusJson.set("emergency_stop_latched", emergencyStopLatched);
  statusJson.set("remote_sensor_stable",  remoteSensorStable);
  statusJson.set("level_fresh",           levelFresh);

  statusJson.set("run_mode", runMode);
  // FIX [M-19]: push countdown_active so dashboard can distinguish "timer expired/stopped"
  // (isCountdownActive=false, countdownRemainSec=0) from "timer never started" (same values).
  statusJson.set("countdown_active", isCountdownActive);
  int32_t countdownRemainSec = 0;
  if (isCountdownActive && pumpMode == "COUNTDOWN" && countdownEndMs != 0) {
    uint32_t nowMs = millis();
    countdownRemainSec = millisDeadlineReached(nowMs, countdownEndMs)
      ? 0
      : (int32_t)((uint32_t)(countdownEndMs - nowMs) / 1000UL);
  }
  statusJson.set("countdown_remaining_sec", countdownRemainSec);
  statusJson.set("pump_cooldown_remaining_sec", pumpCooldownRemainingSec);
  if (lastFaultCode.length() > 0) statusJson.set("last_fault_code", lastFaultCode);
  if (lastFaultMessage.length() > 0) statusJson.set("last_fault_message", lastFaultMessage);

  if (estimatedLevelPct >= 0.0f) {
    statusJson.set("estimated_level_pct", (int)estimatedLevelPct);
  }
  statusJson.set("level_estimate_active", (estimatedLevelPct >= 0.0f && cfgBypassLevelSensor));
  statusJson.set("flow_volume_added_l", flowVolumeAddedL);
  uint32_t levelAgeS = (levelLastValidMs > 0) ? (elapsedMillis32(millis(), levelLastValidMs) / 1000UL) : 0;
  statusJson.set("level_last_valid_age_sec", (int)levelAgeS);
  int levelSensorHealthPct = 100;
  levelSensorHealthPct -= min(levelSensorFailCount * 20, 80);
  if (levelLastValidMs > 0 && elapsedMillis32(millis(), levelLastValidMs) > 30000UL) levelSensorHealthPct -= 20;
  levelSensorHealthPct = constrain(levelSensorHealthPct, 0, 100);
  statusJson.set("level_sensor_health_pct", levelSensorHealthPct);
  statusJson.set("total_pump_cycles", (int)totalPumpCycles);
  // Round to nearest minute to reduce silent truncation drift.
  statusJson.set("total_pump_run_min", (int)((totalPumpRunSec + 30UL) / 60UL));

  if (Firebase.RTDB.setJSON(&fbdo, "/pump_system/status", &statusJson)) {
    lastSuccessfulFirebaseMs = millis();
    firebaseConsecutiveFailCount = 0;
    cloudStatusOkCount++;
    statusPushRetryCount = 0;
    LOG(LOG_LEVEL_INFO, "FIREBASE", "Status -> Level:%d%% | Flow:%.2f | Run:%s | Err:%s | RSSI:%d | Uptime:%um", waterLevelPct, flowRateLpm,
                  isRunning       ? "Y" : "N",
                  isDryRunError   ? "Y" : "N",
                  wifiRssi,
                  uptimeMinutes);
    cloudLastStatusCallMs = (uint32_t)(millis() - t0);
    return true;
  } else {
    String err = fbdo.errorReason();
    firebaseConsecutiveFailCount++;
    cloudStatusFailCount++;
    firebaseLastError = err;
    statusPushRetryCount++;
    statusPushRetryMs = millis();
    LOG(LOG_LEVEL_ERROR, "FIREBASE", "Push failed: %s", err.c_str());
    if (err.indexOf("token is not ready") >= 0 || err.indexOf("revoked") >= 0 || err.indexOf("expired") >= 0) {
      firebaseAuthErrorCount++;
      unsigned long now = millis();
      firebaseCooldownUntilMs = max(firebaseCooldownUntilMs, (unsigned long)addMillisSaturated(now, FIREBASE_AUTH_COOLDOWN_MS));
      Firebase.refreshToken(&config);
      LOG(LOG_LEVEL_INFO, "FIREBASE", "Auth not ready/expired; backing off and requesting token refresh.");
    } else if (err.indexOf("payload read timed out") >= 0 || err.indexOf("read timed out") >= 0) {
      firebaseTimeoutCount++;
      fbdo.stopWiFiClient(); // Force teardown of dead TCP socket
      if (firebaseConsecutiveFailCount >= STATUS_PUSH_RETRY_MAX) {
        unsigned long now = millis();
        firebaseCooldownUntilMs = max(firebaseCooldownUntilMs, (unsigned long)addMillisSaturated(now, 30000UL));
        LOG(LOG_LEVEL_ERROR, "FIREBASE", "Network timeout; cooling down 30s.");
      } else {
        LOG(LOG_LEVEL_ERROR, "FIREBASE", "Network timeout; retrying (%d/%d).", (int)firebaseConsecutiveFailCount, STATUS_PUSH_RETRY_MAX);
      }
    }
    cloudLastStatusCallMs = (uint32_t)(millis() - t0);
    return false;
  }
}

void connectWiFi() {
  LOG(LOG_LEVEL_INFO, "WIFI", "Connecting to: %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    esp_task_wdt_reset();
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    wifiWasConnected = true;
    wifiRssi = WiFi.RSSI();
    LOG(LOG_LEVEL_INFO, "WIFI", "Connected! IP: %s | RSSI: %d dBm", WiFi.localIP().toString().c_str(), wifiRssi);
  } else {
    LOG(LOG_LEVEL_ERROR, "WIFI", "Connection failed after 20s. Will retry in main loop.");
  }
}

void initFirebase() {
  config.api_key      = API_KEY;
  config.database_url = DATABASE_URL;

  auth.user.email    = FIREBASE_EMAIL;
  auth.user.password = FIREBASE_PASSWORD;

  config.token_status_callback = tokenStatusCallback;

  // Maximize buffers to handle JSON payloads without fragmentation timeouts
  fbdo.setBSSLBufferSize(4096, 2048);
  fbdo.setResponseSize(4096);

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Firebase.RTDB.setReadTimeout(&fbdo, 15000);
  Firebase.RTDB.setwriteSizeLimit(&fbdo, "medium");  // library spelling (typo in upstream API)
  fbdo.keepAlive(30, 30, 2);

  firebaseInitialized = true;
  firebaseConsecutiveFailCount = 0;
  firebaseCooldownUntilMs = 0;

  LOG(LOG_LEVEL_INFO, "FIREBASE", "Initialized. Waiting for token...");
}

String getBootReasonString() {
  esp_reset_reason_t reason = esp_reset_reason();
  switch (reason) {
    case ESP_RST_POWERON:  return "Power-on";
    case ESP_RST_EXT:      return "External reset";
    case ESP_RST_SW:       return "Software reset";
    case ESP_RST_PANIC:    return "Exception/panic";
    case ESP_RST_INT_WDT:  return "Interrupt watchdog";
    case ESP_RST_TASK_WDT: return "Task watchdog";
    case ESP_RST_WDT:      return "Other watchdog";
    case ESP_RST_DEEPSLEEP:return "Deep sleep wake";
    case ESP_RST_BROWNOUT: return "Brownout";
    case ESP_RST_SDIO:     return "SDIO reset";
    default:               return "Unknown";
  }
}

void pushFirebaseErrorLog(const String& level, const String& component, const String& message) {
  if (!Firebase.ready()) return;
  
  FirebaseJson logJson;
  // Prefer wall-clock time (NTP) when available; otherwise fall back to monotonic boot-seconds.
  uint32_t epochSec = 0;
  if (ntpSynced && ntpEpochSecAtLastSync != 0 && ntpLastSyncMs != 0) {
    uint32_t deltaSec = (elapsedMillis32(millis(), ntpLastSyncMs) / 1000UL);
    epochSec = ntpEpochSecAtLastSync + deltaSec;
  }
  logJson.set("timestamp", (int)(epochSec != 0 ? epochSec : (esp_timer_get_time() / 1000000ULL)));
  logJson.set("level", level);
  logJson.set("component", component);
  logJson.set("message", message);
  
  if (Firebase.RTDB.pushJSON(&fbdo, "/pump_system/logs/errors", &logJson)) {
    LOG(LOG_LEVEL_ERROR, "FIREBASE", "Error log pushed successfully.");
  } else {
    LOG(LOG_LEVEL_ERROR, "FIREBASE", "Error log push failed: %s", fbdo.errorReason().c_str());
  }
}

