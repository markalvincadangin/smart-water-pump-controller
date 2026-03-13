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

  // Advanced tuning (optional keys)
  int sensThresh = cfgSensorFailureThreshold;
  int idleSens = cfgIdleSensorIntervalMs;
  int idleFb = cfgIdleFirebaseIntervalMs;
  json.get(jsonData, "sensor_failure_threshold");
  if (jsonData.success) { int v = jsonData.intValue; if (v >= 3 && v <= 20) sensThresh = v; }
  json.get(jsonData, "idle_sensor_interval_ms");
  if (jsonData.success) { int v = jsonData.intValue; if (v >= 5000 && v <= 60000) idleSens = v; }
  json.get(jsonData, "idle_firebase_interval_ms");
  if (jsonData.success) { int v = jsonData.intValue; if (v >= 10000 && v <= 120000) idleFb = v; }

  if (!allOk) return;  // Missing/invalid keys → keep current config

  // Apply only if something changed (reduces NVS wear)
  // Use a small epsilon (0.01) for float comparisons to avoid false positives from rounding
  bool floatsChanged = (fabsf(drLpm - cfgDryRunThresholdLpm) > 0.01f) ||
                       (fabsf(flowCal - cfgFlowCalibration) > 0.01f);

  bool sleepChanged = (slpEn != cfgSleepEnabled || slpStart != cfgSleepStartHour ||
                       slpEnd != cfgSleepEndHour || slpEmerg != cfgSleepEmergencyLevel);
  bool advancedChanged = (sensThresh != cfgSensorFailureThreshold ||
                          idleSens != cfgIdleSensorIntervalMs || idleFb != cfgIdleFirebaseIntervalMs);

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
  cfgSensorFailureThreshold = sensThresh;
  cfgIdleSensorIntervalMs   = idleSens;
  cfgIdleFirebaseIntervalMs = idleFb;
  saveDeviceConfigToNVS();
  Serial.println("[FIREBASE] Device config updated.");
}

// Read `/pump_system/control/*` (mode + clear_error + reboot request).

void readFirebaseControl() {
  // Phase 7 commands are one-shot. We treat them as edge-triggered so the firmware
  // does not need to write back to /control (writes are admin-restricted).
  static bool lastManualStart = false;
  static bool lastManualStop  = false;
  static int  lastTimedStartSec = 0;

  // Read pump mode
  if (Firebase.RTDB.getString(&fbdo, "/pump_system/control/mode")) {
    firebaseConsecutiveFailCount = 0;
    String newMode = fbdo.stringData();
    newMode.trim();
    newMode.toUpperCase();
    if (newMode == "AUTO" || newMode == "FORCE_ON" || newMode == "FORCE_OFF") {
      // If a run is active, don't let /control/mode fight the run.
      // However, treat FORCE_OFF as a hard stop request.
      if (runMode == "MANUAL" || runMode == "TIMED") {
        if (newMode == "FORCE_OFF") {
          Serial.println("[FIREBASE] FORCE_OFF received during run — stopping.");
          setPump(false);
          runMode = "OFF";
          runDurationMs = 0;
          runStartMs = 0;
          runRemainingSec = 0;
          pumpMode = "FORCE_OFF";
        } else {
          // Remember desired mode to restore after timed run completes
          runPrevPumpMode = newMode;
        }
      } else {
        if (pumpMode != newMode) {
          Serial.printf("[FIREBASE] Mode changed: %s -> %s\n",
                        pumpMode.c_str(), newMode.c_str());
        }
        pumpMode = newMode;
      }
    }
  } else {
    String err = fbdo.errorReason();
    firebaseConsecutiveFailCount++;
    firebaseLastError = err;
    Serial.printf("[FIREBASE] Read mode failed: %s\n", err.c_str());
    // If token isn't ready/valid yet, back off and trigger a refresh attempt.
    // This prevents tight-loop RTDB calls that keep failing and can destabilize the client.
    if (err.indexOf("token is not ready") >= 0 || err.indexOf("revoked") >= 0 || err.indexOf("expired") >= 0) {
      unsigned long now = millis();
      firebaseCooldownUntilMs = max(firebaseCooldownUntilMs, now + FIREBASE_AUTH_COOLDOWN_MS);
      Firebase.refreshToken(&config);
      Serial.println("[FIREBASE] Auth not ready/expired; backing off and requesting token refresh.");
    } else if (err.indexOf("payload read timed out") >= 0 || err.indexOf("read timed out") >= 0) {
      // Network is too weak/unstable. Cool down longer to avoid starving loopTask and hitting WDT.
      unsigned long now = millis();
      firebaseCooldownUntilMs = max(firebaseCooldownUntilMs, now + 120000UL); // 2 minutes
      Serial.println("[FIREBASE] Network timeout; cooling down 120s to protect main loop.");
    }
  }

  // Phase 7: Smart manual/timed run controls (additive; safe to ignore for older dashboards)
  // manual_stop: immediate stop request (one-shot edge)
  if (Firebase.RTDB.getBool(&fbdo, "/pump_system/control/manual_stop")) {
    bool v = fbdo.boolData();
    if (v && !lastManualStop) {
      Serial.println("[FIREBASE] Manual stop requested.");
      // Stop pump immediately; clear run state
      setPump(false);
      runMode = "OFF";
      runDurationMs = 0;
      runStartMs = 0;
      runRemainingSec = 0;
      // Keep pump OFF after a manual stop (prevents immediate AUTO restart at low level)
      pumpMode = "FORCE_OFF";
    }
    lastManualStop = v;
  }

  // timed_start_sec: start a timed run when the value changes to a non-zero number
  if (Firebase.RTDB.getInt(&fbdo, "/pump_system/control/timed_start_sec")) {
    int sec = fbdo.intData();
    if (sec > 0 && sec != lastTimedStartSec) {
      // Guardrails: minimum 30s; maximum limited by cfgMaxPumpRuntimeMin (overflow safety envelope)
      int minSec = 30;
      int maxSec = max(60, cfgMaxPumpRuntimeMin * 60);
      int durSec = constrain(sec, minSec, maxSec);

      // Do not start if hard lockouts are active
      if (isDryRunError || isOverflowError) {
        Serial.println("[FIREBASE] Timed run rejected: error lockout active.");
      } else {
        runPrevPumpMode = pumpMode;  // remember current policy
        pumpMode = "FORCE_ON";
        runMode = "TIMED";
        runStartMs = millis();
        runDurationMs = (unsigned long)durSec * 1000UL;
        runRemainingSec = (uint32_t)durSec;
        Serial.printf("[FIREBASE] Timed run started: %ds (requested %ds).\n", durSec, sec);
      }
    }
    lastTimedStartSec = sec;
  }

  // manual_start: start manual-until-stop run (one-shot edge)
  if (Firebase.RTDB.getBool(&fbdo, "/pump_system/control/manual_start")) {
    bool v = fbdo.boolData();
    if (v && !lastManualStart) {
      // Do not start if hard lockouts are active
      if (isDryRunError || isOverflowError) {
        Serial.println("[FIREBASE] Manual run rejected: error lockout active.");
      } else {
        runPrevPumpMode = pumpMode;
        pumpMode = "FORCE_ON";
        runMode = "MANUAL";
        runStartMs = millis();
        runDurationMs = 0;
        runRemainingSec = 0;
        Serial.println("[FIREBASE] Manual run started.");
      }
    }
    lastManualStart = v;
  }

  // Read clear_error flag — clears ALL error types
  if (Firebase.RTDB.getBool(&fbdo, "/pump_system/control/clear_error")) {
    if (fbdo.boolData() == true) {
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
  }

  // Read reboot request ID — triggers a soft restart when changed
  if (Firebase.RTDB.getInt(&fbdo, "/pump_system/control/reboot_request_id")) {
    int requestedId = fbdo.intData();
    if (requestedId > 0 && requestedId != lastRebootRequestId) {
      Serial.printf("[FIREBASE] Reboot requested (id=%d).\n", requestedId);
      lastRebootRequestId = requestedId;
      // Acknowledge before restarting so dashboard sees progress
      Firebase.RTDB.setInt(&fbdo, "/pump_system/status/last_reboot_request_id", lastRebootRequestId);
      Firebase.RTDB.setInt(&fbdo, "/pump_system/status/last_reboot_at", (int)(esp_timer_get_time() / 1000000ULL)); // seconds since boot
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

  statusJson.set("water_level_percent", waterLevelPct);
  statusJson.set("is_running",          isRunning);
  statusJson.set("flow_rate_lpm",       flowRateLpm);
  statusJson.set("is_error",            isDryRunError);
  statusJson.set("is_sensor_error",     isSensorError || isFlowSensorError);
  statusJson.set("is_overflow_error",   isOverflowError);
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

  // Phase 7: smart run status (additive)
  statusJson.set("run_mode", runMode);
  statusJson.set("run_remaining_sec", (int)runRemainingSec);
  if (lastFaultCode.length() > 0) statusJson.set("last_fault_code", lastFaultCode);
  if (lastFaultMessage.length() > 0) statusJson.set("last_fault_message", lastFaultMessage);

  if (Firebase.RTDB.setJSON(&fbdo, "/pump_system/status", &statusJson)) {
    lastSuccessfulFirebaseMs = millis();
    firebaseConsecutiveFailCount = 0;
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
    Serial.printf("[FIREBASE] Push failed: %s\n", err.c_str());
    if (err.indexOf("token is not ready") >= 0 || err.indexOf("revoked") >= 0 || err.indexOf("expired") >= 0) {
      unsigned long now = millis();
      firebaseCooldownUntilMs = max(firebaseCooldownUntilMs, now + FIREBASE_AUTH_COOLDOWN_MS);
      Firebase.refreshToken(&config);
      Serial.println("[FIREBASE] Auth not ready/expired; backing off and requesting token refresh.");
    } else if (err.indexOf("payload read timed out") >= 0 || err.indexOf("read timed out") >= 0) {
      unsigned long now = millis();
      firebaseCooldownUntilMs = max(firebaseCooldownUntilMs, now + 120000UL); // 2 minutes
      Serial.println("[FIREBASE] Network timeout; cooling down 120s to protect main loop.");
    }
  }
}

// Initial WiFi connect. Runtime reconnect is handled in `loop()` (backoff + jitter).

void connectWiFi() {
  Serial.printf("\n[WIFI] Connecting to: %s", WIFI_SSID);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
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
    Serial.println("\n[WIFI] Connection failed. Operating in offline mode.");
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

  // SSL buffer (recommended for Firebase-ESP-Client v4.4.x)
  fbdo.setBSSLBufferSize(4096, 1024);

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);  // Auto-reconnect if router reboots

  // Prevent long blocking reads from starving the loop watchdog.
  // With weak WiFi (e.g. RSSI -85 to -95 dBm), RTDB reads can otherwise hang long enough
  // to trigger Task WDT and crash-loop into safe mode.
  Firebase.RTDB.setReadTimeout(&fbdo, 10000);     // 10s max per GET
  Firebase.RTDB.setwriteSizeLimit(&fbdo, "medium"); // keep default-ish write budget
  fbdo.setResponseSize(1024);                    // tolerate larger responses without retries

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

