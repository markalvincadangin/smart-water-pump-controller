// =============================================================================
// SmartFlow — ESP32 Master Firmware (Arduino multi-tab)
//
// Entry points: setup() and loop().
// All includes, globals, and helpers live in the prefixed tabs:
//   01_config.ino, 02_rs485_comm.ino, 03_safety_pump.ino,
//   04_persistence.ino, 05_connectivity_cloud.ino
// =============================================================================

#include <smart_water_pump_controller_shared.h>

// REFACTOR [C-01]: explicit setup() declaration was missing. Fixed.
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) delay(10);  // Short wait for USB Serial attach

  // Print boot banner before LOG() is available (gLogLevel not yet loaded)
  Serial.println("\n====================================");
  Serial.println("         SmartFlow (ESP32)");
  Serial.println("====================================");


  // Boot reason logging
  bootReasonStr = getBootReasonString();
  LOG(LOG_INFO, "BOOT", "Reset reason: %s", bootReasonStr.c_str());

  // String heap-fragmentation mitigation (reserve once at boot)
  pumpMode.reserve(12);
  runMode.reserve(16);
  runPrevPumpMode.reserve(16);
  lastFaultCode.reserve(24);
  lastFaultMessage.reserve(160);
  firebaseLastError.reserve(200);
  bootReasonStr.reserve(32);
  lastPersistedMode.reserve(12);

  // --- GPIO Setup ---
  pinMode(RELAY_PIN,        OUTPUT);
  pinMode(RS485_DE_RE_PIN,  OUTPUT);

  // Safety: ensure pump is OFF on boot
  setPump(false);
  digitalWrite(RS485_DE_RE_PIN, LOW);  // RX mode by default
  LOG(LOG_INFO, "BOOT", "GPIO configured. Relay pin %d. Pump OFF.", RELAY_PIN);

  // --- RS-485 UART2 init (tank sensor node) ---
  Serial2.begin(RS485_UART_BAUD, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
  LOG(LOG_INFO, "BOOT", "RS-485 UART2 init: baud=%d TX=%d RX=%d DE/RE=%d", RS485_UART_BAUD, RS485_TX_PIN, RS485_RX_PIN, RS485_DE_RE_PIN);

  // Crash loop detection
  checkCrashLoop();
  if (inSafeMode) {
    // Safe mode: skip everything except Serial output
    LOG(LOG_ERROR, "BOOT", "SAFE MODE active. Skipping WiFi, Firebase, sensor init.");
    LOG(LOG_WARN,  "BOOT", "Pump OFF. Auto-clear after 1 hour or full power cycle.");
    return;  // Exit setup() — loop() handles safe mode
  }

  // --- Load device config from NVS ---
  loadDeviceConfigFromNVS();

  // Load last known state from NVS
  loadStateFromNVS();

  // Startup stabilization delay
  if (STARTUP_STABILIZE_MS > 0) {
    LOG(LOG_DEBUG, "BOOT", "Stabilization delay (%lums)...", (unsigned long)STARTUP_STABILIZE_MS);
    unsigned long t0 = millis();
    while ((millis() - t0) < (unsigned long)STARTUP_STABILIZE_MS) {
      delay(1);
    }
  }

  // --- WiFi ---
  connectWiFi();

  // NTP time sync — Philippine Standard Time (GMT+8)
  if (WiFi.status() == WL_CONNECTED) {
    configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 5000)) {
      ntpSynced = true;
      LOG(LOG_INFO, "BOOT", "NTP synced: %04d-%02d-%02d %02d:%02d PHT",
                    timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                    timeinfo.tm_hour, timeinfo.tm_min);
    } else {
      LOG(LOG_WARN, "BOOT", "NTP sync failed. Sleep mode disabled until next WiFi connect.");
    }
  } else {
    LOG(LOG_WARN, "BOOT", "No WiFi at boot. NTP and sleep mode disabled.");
  }

  // --- Firebase ---
  if (WiFi.status() == WL_CONNECTED) {
    initFirebase();
  } else {
    LOG(LOG_WARN, "FIREBASE", "Init skipped — no WiFi. Will init when WiFi connects.");
  }

  // Hardware watchdog
  // Core may already init TWDT with a short default (e.g. 5s). We need >= 30s for light sleep.
  // Deinit then reinit with our timeout so 30s sleep does not trigger a reset.
  esp_task_wdt_deinit();
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = (uint32_t)(WDT_TIMEOUT_SEC * 1000),
    .idle_core_mask = 0,
    .trigger_panic = true
  };
  esp_task_wdt_init(&wdt_config);
#else
  esp_task_wdt_init(WDT_TIMEOUT_SEC, true);
#endif
  esp_task_wdt_add(NULL);
  LOG(LOG_INFO, "BOOT", "Watchdog: %ds timeout, task registered.", WDT_TIMEOUT_SEC);

  // --- Initialize Timing ---
  unsigned long nowInit = millis();
  lastSensorMs        = nowInit;
  lastFirebaseMs      = nowInit;
  lastDeviceConfigMs  = 0;
  lastWifiRetryMs     = 0;
  lastRssiLogMs       = nowInit;
  lastLevelWriteMs    = nowInit;
  lastUptimeWriteMs   = nowInit;
  lastHeapDiagMs      = nowInit;
  minFreeHeapObserved = ESP.getFreeHeap();

  LOG(LOG_INFO, "BOOT", "Boot complete. mode=%s runMode=%s dryRun=%s",
      pumpMode.c_str(), runMode.c_str(), isDryRunError ? "ERR" : "OK");
}

// =============================================================================
// MAIN LOOP
// Non-blocking design using millis() timers.
// =============================================================================

void loop() {
  unsigned long now = millis();

  // Watchdog reset
  esp_task_wdt_reset();

  // Safe mode handling
  if (inSafeMode) {
    // If we have wall-clock time (NTP), prefer a true 1-hour "real time" latch.
    // Otherwise fall back to 1-hour continuous uptime in safe mode.
    uint32_t safeModeEpochSec = 0;
    if (prefs.begin(NVS_STATE_NAMESPACE, true)) {
      safeModeEpochSec = prefs.getUInt("safe_mode_epoch_sec", 0);
      prefs.end();
    }

    if (ntpSynced && safeModeEpochSec == 0) {
      struct tm ti;
      if (getLocalTime(&ti, 1000)) {
        time_t nowEpoch = mktime(&ti);
        if (nowEpoch > 0) {
          if (prefs.begin(NVS_STATE_NAMESPACE, false)) {
            prefs.putUInt("safe_mode_epoch_sec", (uint32_t)nowEpoch);
            prefs.end();
          }
          safeModeEpochSec = (uint32_t)nowEpoch;
          LOG(LOG_INFO, "BOOT", "Safe mode: wall-clock epoch latched for auto-clear.");
        }
      }
    }

    bool shouldClear = false;
    if (ntpSynced && safeModeEpochSec > 0) {
      struct tm ti;
      if (getLocalTime(&ti, 1000)) {
        time_t nowEpoch = mktime(&ti);
        if (nowEpoch > 0 && (uint32_t)nowEpoch >= safeModeEpochSec) {
          uint32_t age = (uint32_t)nowEpoch - safeModeEpochSec;
          shouldClear = (age >= (SAFE_MODE_TIMEOUT_MS / 1000UL));
        }
      }
    } else {
      shouldClear = (now - safeModeEnteredMs >= SAFE_MODE_TIMEOUT_MS);
    }

    if (shouldClear) {
      LOG(LOG_WARN, "BOOT", "Safe mode timeout reached. Clearing latch and restarting...");
      if (prefs.begin(NVS_STATE_NAMESPACE, false)) {
        prefs.putULong("safe_mode_ms", 0);
        prefs.putUInt("safe_mode_epoch_sec", 0);
        prefs.putInt("boot_count", 0);
        prefs.end();
      }
      ESP.restart();  // Clean restart
    }
    // In safe mode: just print heartbeat every 30s, pump stays OFF
    static unsigned long lastSafeModeLog = 0;
    if (now - lastSafeModeLog >= 30000) {
      lastSafeModeLog = now;
      unsigned long remaining = (SAFE_MODE_TIMEOUT_MS - min(SAFE_MODE_TIMEOUT_MS, (now - safeModeEnteredMs))) / 60000UL;
      LOG(LOG_WARN, "BOOT", "SAFE MODE: pump OFF, %lu min until auto-clear.", remaining);
    }
    delay(100);
    return;
  }

  // REFACTOR [H-06]: 180s fallback clear for crash-loop (if Firebase fails)
  if (!crashCounterCleared && now > 180000UL) {
    if (prefs.begin(NVS_STATE_NAMESPACE, false)) {
      prefs.putInt("boot_count", 0);
      prefs.end();
    }
    crashCounterCleared = true;
    LOG(LOG_INFO, "BOOT", "Crash loop counter cleared by 180s fallback (no Firebase).");
  }

  // WiFi recovery (exponential backoff with jitter)
  if (WiFi.status() != WL_CONNECTED) {
    if (wifiWasConnected) {
      wifiWasConnected = false;
      wifiBackoffMs = WIFI_BACKOFF_INITIAL_MS;
      LOG(LOG_WARN, "WIFI", "Connection lost.");
    }
    if (now - lastWifiRetryMs >= wifiBackoffMs) {
      lastWifiRetryMs = now;
      LOG(LOG_INFO, "WIFI", "Reconnecting (backoff: %lums)...", wifiBackoffMs);
      WiFi.disconnect(false);
      WiFi.mode(WIFI_STA);
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

      wifiBackoffMs = min(wifiBackoffMs * 2, (unsigned long)WIFI_BACKOFF_MAX_MS);
      long jitter = (long)random(-WIFI_JITTER_MS, WIFI_JITTER_MS);
      wifiBackoffMs = max((unsigned long)WIFI_BACKOFF_INITIAL_MS,
                          (unsigned long)((long)wifiBackoffMs + jitter));
    }
  } else {
    if (!wifiWasConnected) {
      wifiWasConnected = true;
      wifiBackoffMs = WIFI_BACKOFF_INITIAL_MS;
      wifiRssi = WiFi.RSSI();
      LOG(LOG_INFO, "WIFI", "Reconnected! IP: %s RSSI: %d dBm",
                    WiFi.localIP().toString().c_str(), wifiRssi);

      if (!firebaseInitialized) {
        LOG(LOG_INFO, "FIREBASE", "Late init — WiFi was unavailable at boot.");
        initFirebase();
      } else {
        LOG(LOG_INFO, "FIREBASE", "Refreshing auth token after WiFi recovery.");
        Firebase.refreshToken(&config);
        firebaseConsecutiveFailCount = 0;
        firebaseCooldownUntilMs = millis() + 10000UL;
      }

      configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");
      struct tm timeinfo;
      if (getLocalTime(&timeinfo, 5000)) {
        ntpSynced = true;
        LOG(LOG_INFO, "WIFI", "NTP re-synced: %02d:%02d PHT", timeinfo.tm_hour, timeinfo.tm_min);
      } else {
        LOG(LOG_WARN, "WIFI", "NTP re-sync failed. Will retry on next reconnect.");
      }
    }
    lastWifiRetryMs = 0;
  }

  // RSSI telemetry (60s)
  if (WiFi.status() == WL_CONNECTED && now - lastRssiLogMs >= 60000) {
    lastRssiLogMs = now;
    wifiRssi = WiFi.RSSI();
    LOG(LOG_DEBUG, "WIFI", "RSSI: %d dBm", wifiRssi);
  }

  // --- Long-runtime diagnostics (heap; every 10 minutes) ---
  if (now - lastHeapDiagMs >= 600000UL) {
    lastHeapDiagMs = now;
    uint32_t freeHeap = ESP.getFreeHeap();
    if (minFreeHeapObserved == 0 || freeHeap < minFreeHeapObserved) {
      minFreeHeapObserved = freeHeap;
    }
    LOG(LOG_DEBUG, "HEAP", "free=%lu min_obs=%lu bytes", (unsigned long)freeHeap, (unsigned long)minFreeHeapObserved);
  }

  // Sleep/idle state + dynamic intervals
  int currentHour = -1;
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 100)) {
    currentHour = timeinfo.tm_hour;
    if (!ntpSynced) { ntpSynced = true; LOG(LOG_INFO, "BOOT", "NTP synced post-reconnect."); }
  }
  bool emergencyOverride = (waterLevelPct <= cfgSleepEmergencyLevel);
  if (emergencyOverride && cfgSleepEnabled && ntpSynced) {
    static unsigned long lastEmergLog = 0;
    if (now - lastEmergLog >= 60000) {
      lastEmergLog = now;
      LOG(LOG_WARN, "SLEEP", "Emergency override: level %d%% <= threshold %d%%",
                    waterLevelPct, cfgSleepEmergencyLevel);
    }
  }
  bool wasSleeping = isSleeping;
  isSleeping = cfgSleepEnabled && ntpSynced && (currentHour >= 0) &&
               isInSleepWindow(currentHour) && !emergencyOverride;

  // Idle slow-poll: pump OFF + level >= 90% for 5 min
  if (!isSleeping && !isRunning && waterLevelPct >= IDLE_LEVEL_THRESHOLD) {
    if (!isIdleMode) {
      if (idleStartMs == 0) idleStartMs = now;
      else if (now - idleStartMs >= IDLE_STABLE_TIME_MS) {
        isIdleMode = true;
        LOG(LOG_INFO, "IDLE", "Tank >=90%%, pump OFF 5min — entering slow-poll mode.");
      }
    }
  } else {
    if (isIdleMode) LOG(LOG_INFO, "IDLE", "Exiting slow-poll — resuming normal intervals.");
    isIdleMode = false;
    idleStartMs = 0;
  }

  // Dynamic intervals (idle intervals configurable via Firebase)
  unsigned long sensorInterval = isSleeping ? SLEEP_WAKE_INTERVAL_MS :
    (isIdleMode ? (unsigned long)cfgIdleSensorIntervalMs : SENSOR_INTERVAL_MS);
  unsigned long firebaseInterval = isSleeping ? SLEEP_WAKE_INTERVAL_MS :
    (isIdleMode ? (unsigned long)cfgIdleFirebaseIntervalMs : FIREBASE_INTERVAL_MS);

  // Sleep transition logging
  if (isSleeping && !wasSleeping && now - lastSleepLogMs >= 10000) {
    lastSleepLogMs = now;
    LOG(LOG_INFO, "SLEEP", "Entering scheduled sleep — 30s poll interval.");
  } else if (!isSleeping && wasSleeping) {
    lastSleepLogMs = now;
    LOG(LOG_INFO, "SLEEP", "Waking up — resuming normal operation.");
  }

  // Sensors
  if (now - lastSensorMs >= sensorInterval) {
    lastSensorMs = now;

    // 1. Poll remote tank sensor node over RS-485 (LVL + FLOW + ERR)
    bool gotFrame = pollRemoteSensorNode();

    // 2. Convert remote node health into our existing failure model
    //    (pass -1 when remote reports ultrasonic failure or goes offline)
    int levelForFailureLogic = gotFrame ? waterLevelPct : -1;
    if (gotFrame && (remoteSensorLastErrCode == 1 || remoteSensorLastErrCode == 3)) {
      levelForFailureLogic = -1;
    }
    checkLevelSensorFailure(levelForFailureLogic);

    // 3. Flow-based estimate (only meaningful while pump runs)
    updateFlowBasedEstimate();

    // REFACTOR [H-01]: moved to LOG_DEBUG — suppressed in production LOG_INFO mode
    LOG(LOG_DEBUG, "SENSOR", "lvl=%d%% flow=%.2fLPM node=%s err=%d lvlErr=%s flwErr=%s ovfErr=%s slp=%s",
                  waterLevelPct, flowRateLpm,
                  remoteSensorOnline ? "ONLINE" : "OFFLINE",
                  remoteSensorLastErrCode,
                  isLevelSensorError ? "Y" : "N",
                  isFlowSensorError ? "Y" : "N",
                  isOverflowError ? "Y" : "N",
                  isSleeping ? "Y" : "N");

    // 4. Run all safety checks (dry-run, flow stuck, overflow)
    checkSafetyCutoff();

    // Check countdown expiry (revert to AUTO when timer ends)
    checkCountdownExpiry();

    // 5. Execute pump state machine
    executePumpLogic();
  }

  // Firebase sync (with short retry after push failure)
  bool normalIntervalDue = (now - lastFirebaseMs >= firebaseInterval);
  bool statusRetryDue = (statusPushRetryCount > 0 && statusPushRetryCount < STATUS_PUSH_RETRY_MAX &&
                         now - statusPushRetryMs >= STATUS_PUSH_RETRY_MS);
  if (normalIntervalDue || statusRetryDue) {
    if (normalIntervalDue) lastFirebaseMs = now;

    if (firebaseCooldownUntilMs != 0 && now < firebaseCooldownUntilMs) {
      // Cooling down after token/auth errors
      if (now - firebaseLastErrorLogMs >= 60000) {
        firebaseLastErrorLogMs = now;
        unsigned long remaining = (firebaseCooldownUntilMs - now) / 1000UL;
        LOG(LOG_WARN, "FIREBASE", "Cooling down (%lus left). LastErr: %s",
                      remaining, firebaseLastError.c_str());
      }
    } else if (firebaseCooldownUntilMs != 0 && now >= firebaseCooldownUntilMs) {
      firebaseCooldownUntilMs = 0;
    }

    if (firebaseCooldownUntilMs == 0 && WiFi.status() == WL_CONNECTED && Firebase.ready()) {
      // Device config: read every 30s; first time run immediately
      if (lastDeviceConfigMs == 0 || (now - lastDeviceConfigMs >= DEVICE_CONFIG_INTERVAL_MS)) {
        lastDeviceConfigMs = now;
        readDeviceConfigFromFirebase();
      }
      readFirebaseControl();
      pushFirebaseStatus();
    } else {
      if (WiFi.status() != WL_CONNECTED) {
        // Connection-lost case: do not refresh token; just wait for WiFi recovery.
      } else if (firebaseCooldownUntilMs == 0) {
        LOG(LOG_DEBUG, "FIREBASE", "Not ready. Skipping sync.");
      }
    }
  }

  // NVS state persistence (on change + wear-reduced level)
  persistStateToNVS();

  // --- SENSOR NOISE TELEMETRY (rate-limited; 60s window) ---
  if (now - lastSensorTelemetryLogMs >= 60000) {
    lastSensorTelemetryLogMs = now;
    if (ultrasonicCycleOkCountWin || ultrasonicCycleTimeoutCountWin || flowDiscardMaxSaneCountWin || flowStuckHighEventCountWin) {
      LOG(LOG_DEBUG, "TELEM", "US ok/timeout 60s: %lu/%lu | flow_disc: %lu | flow_stuck: %lu | last_cm=%.1f",
                    (unsigned long)ultrasonicCycleOkCountWin,
                    (unsigned long)ultrasonicCycleTimeoutCountWin,
                    (unsigned long)flowDiscardMaxSaneCountWin,
                    (unsigned long)flowStuckHighEventCountWin,
                    (float)ultrasonicLastGoodCmX10 / 10.0f);
    }
    ultrasonicCycleOkCountWin = 0;
    ultrasonicCycleTimeoutCountWin = 0;
    flowDiscardMaxSaneCountWin = 0;
    flowStuckHighEventCountWin = 0;
  }

  // Light sleep during scheduled sleep window
  if (isSleeping) {
    esp_task_wdt_reset();
    unsigned long nextWake = lastSensorMs + SLEEP_WAKE_INTERVAL_MS;
    unsigned long remainingMs = (nextWake > now) ? (nextWake - now) : 1000;
    uint64_t sleepUs = (uint64_t)remainingMs * 1000ULL;
    if (sleepUs < 100000ULL) sleepUs = 100000ULL;  // Min 100ms
    esp_sleep_enable_timer_wakeup(sleepUs);
    esp_light_sleep_start();
    // Execution resumes here after wake
    esp_task_wdt_reset();
  }

  // FreeRTOS: delay(1) yields to scheduler
  delay(1);
}
