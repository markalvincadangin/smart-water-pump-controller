// -----------------------------------------------------------------------------
// Connectivity: WiFi + Firebase helpers
// -----------------------------------------------------------------------------

// Read `/pump_system/config/device` as a single JSON payload and apply if changed.

void readDeviceConfigFromFirebase() {
  if (!Firebase.RTDB.getJSON(&fbdo, "/pump_system/config/device")) return;

  FirebaseJson json = fbdo.to<FirebaseJson>();
  FirebaseJsonData jsonData;

  int te = 0, tf = 0, ps = 0, po = 0, drSec = 0, maxRun = 0;
  float drLpm = 0.0f, flowCal = 0.0f;
  bool allOk = true;

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

  // max_pump_runtime_min — optional; if missing, keep current value
  json.get(jsonData, "max_pump_runtime_min");
  if (jsonData.success) {
    int v = jsonData.intValue;
    if (v >= 30 && v <= 480) maxRun = v; else maxRun = cfgMaxPumpRuntimeMin;
  } else {
    maxRun = cfgMaxPumpRuntimeMin;  // Key not in Firebase yet — use current
  }

  // Sleep schedule (optional keys)
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

  // Advanced tuning (optional keys) — prefer level_sensor_failure_threshold
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

  if (!allOk) return;  // Missing/invalid keys → keep current config

  // Apply only if something changed (reduces NVS wear)
  // Use a small epsilon (0.01) for float comparisons to avoid false positives from rounding
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
  Serial.println("[FIREBASE] Device config updated.");
}

// Called from loop() before executePumpLogic(). Reverts to AUTO when countdown expires.
// Does NOT call setPump() or Firebase writes — executePumpLogic() handles relay state,
// and pendingModeWriteback retry handles the Firebase write-back.
void checkCountdownExpiry() {
  if (!isCountdownActive || pumpMode != "COUNTDOWN") return;
  if (millis() >= countdownEndMs) {
    Serial.println("[COUNTDOWN] Timer expired. Reverting to AUTO mode.");
    isCountdownActive = false;
    countdownEndMs = 0;
    pumpMode = "AUTO";
    pendingModeWriteback = true;
    pendingModeWritebackSentMs = 0;
  }
}

// Read `/pump_system/control` as a single JSON — one round-trip for all control keys (offline-first reliability).
void readFirebaseControl() {
  static bool lastManualStart = false;
  static bool lastManualStop  = false;
  static bool countdownConsumed = false;
  static bool lastAddTime = false;
  static bool lastEmergencyStop = false;
  static bool lastResetStop = false;
  static bool lastCountdownStart = false;

  if (!Firebase.RTDB.getJSON(&fbdo, "/pump_system/control")) {
    String err = fbdo.errorReason();
    firebaseConsecutiveFailCount++;
    firebaseLastError = err;
    Serial.printf("[FIREBASE] Control read failed: %s\n", err.c_str());
    if (err.indexOf("token is not ready") >= 0 || err.indexOf("revoked") >= 0 || err.indexOf("expired") >= 0) {
      unsigned long now = millis();
      firebaseCooldownUntilMs = max(firebaseCooldownUntilMs, now + FIREBASE_AUTH_COOLDOWN_MS);
      Firebase.refreshToken(&config);
      Serial.println("[FIREBASE] Auth not ready/expired; backing off and requesting token refresh.");
    } else if (err.indexOf("payload read timed out") >= 0 || err.indexOf("read timed out") >= 0) {
      if (firebaseConsecutiveFailCount >= STATUS_PUSH_RETRY_MAX) {
        unsigned long now = millis();
        firebaseCooldownUntilMs = max(firebaseCooldownUntilMs, now + 30000UL);
        Serial.println("[FIREBASE] Control read timeout; cooling down 30s.");
      } else {
        Serial.printf("[FIREBASE] Control read timeout; retrying (%d/%d).\n",
                      (int)firebaseConsecutiveFailCount, STATUS_PUSH_RETRY_MAX);
      }
    }
    return;
  }

  firebaseConsecutiveFailCount = 0;
  FirebaseJson controlJson = fbdo.to<FirebaseJson>();
  FirebaseJsonData jd;

  String firebaseReadMode = "";
  controlJson.get(jd, "mode");
  if (jd.success) {
    String newMode = jd.stringValue;
    newMode.trim();
    newMode.toUpperCase();
    firebaseReadMode = newMode;
  // Only AUTO / MANUAL / COUNTDOWN are valid policy modes.
    if (newMode == "AUTO" || newMode == "COUNTDOWN" || newMode == "MANUAL") {
      if (pendingModeWriteback) {
        if (newMode == pumpMode) {
          pendingModeWriteback = false;
          pendingModeWritebackSentMs = 0;
          if (pumpMode == "AUTO") countdownConsumed = false;
          Serial.println("[FIREBASE] Mode write-back confirmed.");
        } else if (pendingModeWritebackSentMs == 0 ||
                   millis() - pendingModeWritebackSentMs >= 5000UL) {
          Firebase.RTDB.setString(&fbdo, "/pump_system/control/mode", pumpMode);
          pendingModeWritebackSentMs = millis();
          Serial.printf("[FIREBASE] Mode write-back: %s (dashboard sync).\n", pumpMode.c_str());
        }
      } else {
        if (pumpMode != newMode) {
          Serial.printf("[FIREBASE] Mode changed: %s -> %s\n",
                        pumpMode.c_str(), newMode.c_str());
        }
        pumpMode = newMode;
      }
    } else if (newMode == "FORCE_ON" || newMode == "FORCE_OFF") {
      // Backward compatibility: map deprecated FORCE modes to AUTO and write back.
      Serial.printf("[FIREBASE] Deprecated mode '%s' received. Mapping to AUTO.\n", newMode.c_str());
      pumpMode = "AUTO";
      pendingModeWriteback = true;
      pendingModeWritebackSentMs = 0;
    } else {
      Serial.printf("[FIREBASE] Unknown mode received: '%s'. Ignoring.\n", newMode.c_str());
    }
  }

  // MANUAL intent (persistent)
  controlJson.get(jd, "manual_desired");
  if (jd.success) {
    manualDesired = jd.boolValue;
  }

  // Emergency stop (one-shot)
  controlJson.get(jd, "emergency_stop");
  if (jd.success) {
    bool v = jd.boolValue;
    if (v && !lastEmergencyStop) {
      Serial.println("[E-STOP] Emergency stop requested.");
      emergencyStopLatched = true;
      manualDesired = false;
      setPump(false);
      if (isCountdownActive) { isCountdownActive = false; countdownEndMs = 0; }
      Firebase.RTDB.setBool(&fbdo, "/pump_system/control/emergency_stop", false);
    }
    lastEmergencyStop = v;
  }

  // Reset stop latch (one-shot)
  controlJson.get(jd, "reset_stop");
  if (jd.success) {
    bool v = jd.boolValue;
    if (v && !lastResetStop) {
      if (isDryRunError || isOverflowError) {
        Serial.println("[E-STOP] Reset requested but hard lockout active; ignoring.");
      } else {
        Serial.println("[E-STOP] Reset stop requested. Clearing latch.");
        emergencyStopLatched = false;
      }
      Firebase.RTDB.setBool(&fbdo, "/pump_system/control/reset_stop", false);
    }
    lastResetStop = v;
  }

  controlJson.get(jd, "manual_stop");
  if (jd.success) {
    bool v = jd.boolValue;
    if (v && !lastManualStop) {
      // Backward compatibility: legacy manual_stop => manual_desired=false
      Serial.println("[FIREBASE] manual_stop (legacy) received. Setting manual_desired=false.");
      manualDesired = false;
      setPump(false);
      Firebase.RTDB.setBool(&fbdo, "/pump_system/control/manual_stop", false);
    }
    lastManualStop = v;
  }

  if (firebaseReadMode.length() > 0 && firebaseReadMode != "COUNTDOWN") {
    countdownConsumed = false;
  }

  // COUNTDOWN start: preferred is countdown_start (one-shot). Back-compat: entering COUNTDOWN arms timer.
  bool startCountdown = false;
  controlJson.get(jd, "countdown_start");
  if (jd.success) {
    bool v = jd.boolValue;
    if (v && !lastCountdownStart) startCountdown = true;
    lastCountdownStart = v;
    if (v) Firebase.RTDB.setBool(&fbdo, "/pump_system/control/countdown_start", false);
  }

  if ((pumpMode == "COUNTDOWN" && !isCountdownActive && !countdownConsumed) || (pumpMode == "COUNTDOWN" && !isCountdownActive && startCountdown)) {
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
    countdownEndMs = millis() + (unsigned long)durationMin * 60000UL;
    isCountdownActive = true;
    countdownConsumed = true;
    lastAddTime = false;
    Serial.printf("[COUNTDOWN] Started: %d min.%s\n", durationMin,
                  Firebase.ready() ? "" : " (offline — using last known duration)");
  }

  // Process add_time when in COUNTDOWN. Only extend the timer when countdown is actually running.
  // While dry-run (or other) lockout is active the pump is off; adding time does nothing to pump state.
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
        unsigned long nowMs = millis();
        if (isCountdownActive && countdownEndMs > nowMs) {
          unsigned long maxEnd = nowMs + (unsigned long)COUNTDOWN_MAX_DURATION_MIN * 60000UL;
          countdownEndMs = min(countdownEndMs + (unsigned long)addMin * 60000UL, maxEnd);
          Serial.printf("[COUNTDOWN] +%d min added.\n", addMin);
        }
        Firebase.RTDB.setBool(&fbdo, "/pump_system/control/countdown_add_time", false);
      }
      lastAddTime = v;
    }
  }

  if (pumpMode != "COUNTDOWN" && isCountdownActive) {
    isCountdownActive = false;
    countdownEndMs = 0;
    Serial.println("[COUNTDOWN] Mode exited. Countdown cleared.");
  }

  controlJson.get(jd, "manual_start");
  if (jd.success) {
    bool v = jd.boolValue;
    if (v && !lastManualStart) {
      if (isDryRunError || isOverflowError) {
        Serial.println("[FIREBASE] Manual run rejected: error lockout active.");
      } else if (pumpOffStartMs > 0 && (millis() - pumpOffStartMs) < MIN_PUMP_OFF_TIME_MS) {
        Serial.println("[FIREBASE] Manual run rejected: minimum off-time not elapsed.");
      } else {
        // Backward compatibility: legacy manual_start => set MANUAL + manual_desired=true
        pumpMode = "MANUAL";
        manualDesired = true;
        runStartMs = millis();
        Serial.println("[FIREBASE] manual_start (legacy) received. Setting MANUAL + manual_desired=true.");
        Firebase.RTDB.setBool(&fbdo, "/pump_system/control/manual_start", false);
      }
    }
    lastManualStart = v;
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
      Serial.printf("[FIREBASE] Bypass level sensor: %s\n", v ? "ON" : "OFF");
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
      Serial.println("[FIREBASE] Errors cleared.");
      lastFaultCode = "";
      lastFaultMessage = "";
      Firebase.RTDB.setBool(&fbdo, "/pump_system/control/clear_error", false);
    }
  }

  controlJson.get(jd, "reboot_request_id");
  if (jd.success) {
    int requestedId = (jd.typeNum == FirebaseJson::JSON_INT) ? jd.intValue : (int)jd.doubleValue;
    if (requestedId > 0 && requestedId != lastRebootRequestId) {
      Serial.printf("[FIREBASE] Reboot requested (id=%d).\n", requestedId);
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
}

// Push `/pump_system/status` (single JSON write).

void pushFirebaseStatus() {
  FirebaseJson statusJson;
  // Uptime minutes (esp_timer avoids millis() rollover issues)
  uint32_t uptimeMinutes = (uint32_t)(esp_timer_get_time() / 60000000ULL);
  bool levelFresh = (levelLastUpdateMs > 0) && ((millis() - levelLastUpdateMs) <= LEVEL_STALE_TIMEOUT_MS);

  statusJson.set("water_level_percent", waterLevelPct);
  statusJson.set("is_running",          isRunning);
  statusJson.set("flow_rate_lpm",       flowRateLpm);
  statusJson.set("is_error",            isDryRunError);
  statusJson.set("is_level_sensor_error", isLevelSensorError);
  statusJson.set("is_flow_sensor_error",  isFlowSensorError);
  statusJson.set("is_overflow_error",   isOverflowError);
  statusJson.set("bypass_level_sensor", cfgBypassLevelSensor);
  statusJson.set("auto_bypass_active",  autoBypassActive);
  statusJson.set("is_sleeping",         isSleeping);
  statusJson.set("wifi_rssi",           wifiRssi);
  statusJson.set("last_boot_reason",    bootReasonStr);
  statusJson.set("uptime_minutes",      uptimeMinutes);
  // Long-runtime diagnostics
  statusJson.set("free_heap_bytes",     (int)ESP.getFreeHeap());
#if defined(ESP32)
  statusJson.set("min_free_heap_bytes", (int)ESP.getMinFreeHeap());
  statusJson.set("max_alloc_heap_bytes",(int)ESP.getMaxAllocHeap());
#endif
  statusJson.set("min_free_heap_observed_bytes", (int)minFreeHeapObserved);
  statusJson.set("firebase_consecutive_failures", (int)firebaseConsecutiveFailCount);
  statusJson.set("firebase_last_error", firebaseLastError);
  // Sensor noise telemetry (counters)
  statusJson.set("ultrasonic_cycles_ok",         (int)ultrasonicCycleOkCount);
  statusJson.set("ultrasonic_cycles_timeout",    (int)ultrasonicCycleTimeoutCount);
  statusJson.set("ultrasonic_last_good_cm",      (float)ultrasonicLastGoodCmX10 / 10.0f);
  statusJson.set("flow_discard_max_sane",        (int)flowDiscardMaxSaneCount);
  statusJson.set("flow_stuck_high_events",       (int)flowStuckHighEventCount);

  // UI truth signals
  statusJson.set("manual_desired",         manualDesired);
  statusJson.set("emergency_stop_latched", emergencyStopLatched);
  statusJson.set("remote_sensor_stable",   remoteSensorStable);
  statusJson.set("level_fresh",            levelFresh);

  // Run mode + countdown remaining time
  statusJson.set("run_mode", runMode);
  int32_t countdownRemainSec = 0;
  if (isCountdownActive && pumpMode == "COUNTDOWN") {
    unsigned long nowMs = millis();
    countdownRemainSec = (countdownEndMs > nowMs)
      ? (int32_t)((countdownEndMs - nowMs) / 1000UL)
      : 0;
  }
  statusJson.set("countdown_remaining_sec", countdownRemainSec);
  if (lastFaultCode.length() > 0) statusJson.set("last_fault_code", lastFaultCode);
  if (lastFaultMessage.length() > 0) statusJson.set("last_fault_message", lastFaultMessage);

  // Sensor resilience
  if (estimatedLevelPct >= 0.0f) {
    statusJson.set("estimated_level_pct", (int)estimatedLevelPct);
  }
  statusJson.set("level_estimate_active", (estimatedLevelPct >= 0.0f && cfgBypassLevelSensor));
  statusJson.set("flow_volume_added_l", flowVolumeAddedL);
  uint32_t levelAgeS = (levelLastValidMs > 0) ? (uint32_t)((millis() - levelLastValidMs) / 1000UL) : 0;
  statusJson.set("level_last_valid_age_sec", (int)levelAgeS);
  int levelSensorHealthPct = 100;
  levelSensorHealthPct -= min(levelSensorFailCount * 20, 80);
  if (levelLastValidMs > 0 && (millis() - levelLastValidMs) > 30000UL) levelSensorHealthPct -= 20;
  levelSensorHealthPct = constrain(levelSensorHealthPct, 0, 100);
  statusJson.set("level_sensor_health_pct", levelSensorHealthPct);
  statusJson.set("total_pump_cycles", (int)totalPumpCycles);
  statusJson.set("total_pump_run_min", (int)(totalPumpRunSec / 60));

  if (Firebase.RTDB.setJSON(&fbdo, "/pump_system/status", &statusJson)) {
    lastSuccessfulFirebaseMs = millis();
    firebaseConsecutiveFailCount = 0;
    statusPushRetryCount = 0;
    Serial.printf("[FIREBASE] Status -> Level:%d%% | Flow:%.2f | Run:%s | Err:%s | RSSI:%d | Uptime:%um\n",
                  waterLevelPct, flowRateLpm,
                  isRunning       ? "Y" : "N",
                  isDryRunError   ? "Y" : "N",
                  wifiRssi,
                  uptimeMinutes);
  } else {
    String err = fbdo.errorReason();
    firebaseConsecutiveFailCount++;
    firebaseLastError = err;
    statusPushRetryCount++;
    statusPushRetryMs = millis();
    Serial.printf("[FIREBASE] Push failed: %s\n", err.c_str());
    if (err.indexOf("token is not ready") >= 0 || err.indexOf("revoked") >= 0 || err.indexOf("expired") >= 0) {
      unsigned long now = millis();
      firebaseCooldownUntilMs = max(firebaseCooldownUntilMs, now + FIREBASE_AUTH_COOLDOWN_MS);
      Firebase.refreshToken(&config);
      Serial.println("[FIREBASE] Auth not ready/expired; backing off and requesting token refresh.");
    } else if (err.indexOf("payload read timed out") >= 0 || err.indexOf("read timed out") >= 0) {
      if (firebaseConsecutiveFailCount >= STATUS_PUSH_RETRY_MAX) {
        unsigned long now = millis();
        firebaseCooldownUntilMs = max(firebaseCooldownUntilMs, now + 30000UL);
        Serial.println("[FIREBASE] Network timeout; cooling down 30s.");
      } else {
        Serial.printf("[FIREBASE] Network timeout; retrying (%d/%d).\n",
                      (int)firebaseConsecutiveFailCount, STATUS_PUSH_RETRY_MAX);
      }
    }
  }
}

// Initial WiFi connect. Runtime reconnect is handled in `loop()` (backoff + jitter).

void connectWiFi() {
  Serial.printf("\n[WIFI] Connecting to: %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    wifiWasConnected = true;
    wifiRssi = WiFi.RSSI();
    Serial.printf("\n[WIFI] Connected! IP: %s | RSSI: %d dBm\n",
                  WiFi.localIP().toString().c_str(), wifiRssi);
  } else {
    Serial.println("\n[WIFI] Connection failed after 20s. Will retry in main loop.");
  }
}

// Initialize Firebase (Email/Password auth). Call once in `setup()`.

void initFirebase() {
  config.api_key      = API_KEY;
  config.database_url = DATABASE_URL;

  // Email/Password credentials (from secrets.h)
  auth.user.email    = FIREBASE_EMAIL;
  auth.user.password = FIREBASE_PASSWORD;

  // Token status callback (debug)
  config.token_status_callback = tokenStatusCallback;

  // SSL buffer (Firebase client stability)
  fbdo.setBSSLBufferSize(4096, 1024);

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Firebase.RTDB.setReadTimeout(&fbdo, 10000);
  Firebase.RTDB.setwriteSizeLimit(&fbdo, "medium");
  fbdo.setResponseSize(1024);

  firebaseInitialized = true;
  firebaseConsecutiveFailCount = 0;
  firebaseCooldownUntilMs = 0;

  Serial.println("[FIREBASE] Initialized. Waiting for token...");
}

// Boot reason string for status/diagnostics.

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

