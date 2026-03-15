// =============================================================================
// Smart Water Pump Controller - Firmware (Arduino multi-tab)
//
// This file intentionally contains ONLY the entry points: `setup()` and `loop()`.
// All includes, globals, and helper functions live in the prefixed tabs:
//   `01_config.ino`, `02_sensors.ino`, `03_safety_pump.ino`,
//   `04_persistence.ino`, `05_connectivity_cloud.ino`
// =============================================================================

#include <smart_water_pump_controller_shared.h>

// Forward declaration for ISR so Arduino's preprocessor limitations don't bite.
void IRAM_ATTR flowPulseISR(void);

void setup() {
  Serial.begin(115200);
  Serial.println("\n====================================");
  Serial.println(" Smart Water Pump Controller v3.0.0");
  Serial.println("====================================");

  // --- Boot reason logging (Phase 2) ---
  bootReasonStr = getBootReasonString();
  Serial.printf("[BOOT] Reset reason: %s\n", bootReasonStr.c_str());

  // --- GPIO Setup ---
  pinMode(RELAY_PIN,        OUTPUT);
  pinMode(TRIG_PIN,         OUTPUT);
  pinMode(ECHO_PIN,         INPUT);
  pinMode(FLOW_SENSOR_PIN,  INPUT);

  // Safety: ensure pump is OFF on boot
  setPump(false);
  digitalWrite(TRIG_PIN, LOW);
  Serial.println("[INIT] GPIO configured. Pump OFF.");

  // --- Crash loop detection (Phase 2) ---
  checkCrashLoop();
  if (inSafeMode) {
    // Safe mode: skip everything except Serial output
    Serial.println("[SAFE MODE] Skipping WiFi, Firebase, and sensor init.");
    Serial.println("[SAFE MODE] Will auto-clear after 1 hour or full power cycle.");
    return;  // Exit setup() — loop() handles safe mode
  }

  // --- Attach Flow Sensor Interrupt ---
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN),
                  flowPulseISR,
                  RISING);
  Serial.println("[INIT] Flow sensor interrupt attached on GPIO 34.");

  // --- Load device config from NVS ---
  loadDeviceConfigFromNVS();

  // --- Load last known state from NVS (Phase 2) ---
  loadStateFromNVS();

  // --- Startup stabilization delay (Phase 2) ---
  Serial.println("[INIT] Stabilization delay (5s) — sensors settling...");
  delay(5000);

  // --- WiFi ---
  connectWiFi();

  // --- NTP time sync (Phase 3) — Philippine Standard Time (GMT+8)
  if (WiFi.status() == WL_CONNECTED) {
    configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 5000)) {
      ntpSynced = true;
      Serial.printf("[NTP] Time synced: %04d-%02d-%02d %02d:%02d (PHT)\n",
                    timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                    timeinfo.tm_hour, timeinfo.tm_min);
    } else {
      Serial.println("[NTP] Sync failed. Sleep mode disabled until next WiFi connect.");
    }
  } else {
    Serial.println("[NTP] No WiFi. Sleep mode disabled.");
  }

  // --- Firebase ---
  if (WiFi.status() == WL_CONNECTED) {
    initFirebase();
  } else {
    Serial.println("[FIREBASE] Skipped — no WiFi. Will init when WiFi connects.");
  }

  // --- Hardware Watchdog (Phase 2) ---
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
  Serial.printf("[INIT] Watchdog: %ds timeout, task registered.\n", WDT_TIMEOUT_SEC);

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

  Serial.println("[INIT] Boot complete. Entering main loop.\n");
}

// =============================================================================
// SECTION 18: MAIN LOOP
// Non-blocking design using millis() timers.
// Phase 2: WDT reset, exponential backoff WiFi, NVS state persistence,
//          safe mode handling, RSSI telemetry.
// =============================================================================

void loop() {
  unsigned long now = millis();

  // --- WATCHDOG RESET (Phase 2) ---
  esp_task_wdt_reset();

  // --- SAFE MODE HANDLING (Phase 2) ---
  if (inSafeMode) {
    // Check if safe mode timeout has elapsed (1 hour)
    if (now - safeModeEnteredMs >= SAFE_MODE_TIMEOUT_MS) {
      Serial.println("[SAFE MODE] 1-hour timeout reached. Restarting...");
      // Clear safe mode in NVS
      if (prefs.begin(NVS_STATE_NAMESPACE, false)) {
        prefs.putULong("safe_mode_ms", 0);
        prefs.putInt("boot_count", 0);
        prefs.end();
      }
      ESP.restart();  // Clean restart
    }
    // In safe mode: just print heartbeat every 30s, pump stays OFF
    static unsigned long lastSafeModeLog = 0;
    if (now - lastSafeModeLog >= 30000) {
      lastSafeModeLog = now;
      unsigned long remaining = (SAFE_MODE_TIMEOUT_MS - (now - safeModeEnteredMs)) / 60000UL;
      Serial.printf("[SAFE MODE] Pump OFF. %lu min until auto-clear.\n", remaining);
    }
    delay(100);
    return;
  }

  // --- WIFI RECOVERY (Phase 2: exponential backoff with jitter) ---
  if (WiFi.status() != WL_CONNECTED) {
    if (wifiWasConnected) {
      wifiWasConnected = false;
      wifiBackoffMs = WIFI_BACKOFF_INITIAL_MS;
      Serial.println("[WIFI] Connection lost.");
    }
    if (now - lastWifiRetryMs >= wifiBackoffMs) {
      lastWifiRetryMs = now;
      Serial.printf("[WIFI] Reconnecting (backoff: %lums)...\n", wifiBackoffMs);
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
      Serial.printf("[WIFI] Reconnected! IP: %s | RSSI: %d dBm\n",
                    WiFi.localIP().toString().c_str(), wifiRssi);

      if (!firebaseInitialized) {
        Serial.println("[FIREBASE] Late init — WiFi was unavailable at boot.");
        initFirebase();
      } else {
        Serial.println("[FIREBASE] Refreshing auth token after WiFi recovery.");
        Firebase.refreshToken(&config);
        firebaseConsecutiveFailCount = 0;
        firebaseCooldownUntilMs = millis() + 10000UL;
      }

      configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");
      struct tm timeinfo;
      if (getLocalTime(&timeinfo, 5000)) {
        ntpSynced = true;
        Serial.printf("[NTP] Re-synced: %02d:%02d (PHT)\n",
                      timeinfo.tm_hour, timeinfo.tm_min);
      } else {
        Serial.println("[NTP] Re-sync failed. Will retry on next reconnect.");
      }
    }
    lastWifiRetryMs = 0;
  }

  // RSSI telemetry (60s)
  if (WiFi.status() == WL_CONNECTED && now - lastRssiLogMs >= 60000) {
    lastRssiLogMs = now;
    wifiRssi = WiFi.RSSI();
    Serial.printf("[WIFI] RSSI: %d dBm\n", wifiRssi);
  }

  // --- Long-runtime diagnostics (heap; every 10 minutes) ---
  if (now - lastHeapDiagMs >= 600000UL) {
    lastHeapDiagMs = now;
    uint32_t freeHeap = ESP.getFreeHeap();
    if (minFreeHeapObserved == 0 || freeHeap < minFreeHeapObserved) {
      minFreeHeapObserved = freeHeap;
    }
    Serial.printf("[HEAP] free=%lu bytes | min_observed=%lu bytes\n",
                  (unsigned long)freeHeap, (unsigned long)minFreeHeapObserved);
  }

  // Sleep/idle state + dynamic intervals
  int currentHour = -1;
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 100)) {
    currentHour = timeinfo.tm_hour;
    if (!ntpSynced) { ntpSynced = true; Serial.println("[NTP] Time synced (post-reconnect)."); }
  }
  bool emergencyOverride = (waterLevelPct <= cfgSleepEmergencyLevel);
  if (emergencyOverride && cfgSleepEnabled && ntpSynced) {
    static unsigned long lastEmergLog = 0;
    if (now - lastEmergLog >= 60000) {
      lastEmergLog = now;
      Serial.printf("[SLEEP] Emergency override: level at %d%% (<= %d%%)\n",
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
        Serial.println("[IDLE] Tank ≥90%, pump OFF for 5 min — entering slow-poll mode.");
      }
    }
  } else {
    if (isIdleMode) Serial.println("[IDLE] Exiting slow-poll — resuming normal intervals.");
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
    Serial.println("[SLEEP] Entering scheduled sleep — 30s poll interval.");
  } else if (!isSleeping && wasSleeping) {
    lastSleepLogMs = now;
    Serial.println("[SLEEP] Waking up — resuming normal operation.");
  }

  // Sensors
  if (now - lastSensorMs >= sensorInterval) {
    lastSensorMs = now;

    // 1. Read ultrasonic water level (5-sample median + EMA)
    int reading = readUltrasonicSensor();

    // 2. Check for sensor failure (consecutive timeouts)
    checkLevelSensorFailure(reading);

    if (reading >= 0) {
      prevWaterLevelPct = waterLevelPct;
      waterLevelPct = reading;
    }

    // 3. Calculate flow rate from last 1-second pulse window
    flowRateLpm = calculateFlowRate();
    updateFlowBasedEstimate();

    Serial.printf("[SENSOR] Level:%d%% | Flow:%.2f LPM | SensorErr:%s | OverflowErr:%s | Sleep:%s\n",
                  waterLevelPct, flowRateLpm,
                  isLevelSensorError ? "Y" : "N",
                  isOverflowError ? "Y" : "N",
                  isSleeping ? "Y" : "N");

    // 4. Run all safety checks (dry-run, flow stuck, overflow)
    checkSafetyCutoff();

    // 4b. v3.0: Check countdown expiry (revert to AUTO when timer ends)
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
        Serial.printf("[FIREBASE] Cooling down (%lus left). LastErr: %s\n",
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
        Serial.println("[FIREBASE] Not ready. Skipping sync.");
      }
    }
  }

  // --- NVS STATE PERSISTENCE (Phase 2: on change + wear-reduced level) ---
  persistStateToNVS();

  // --- SENSOR NOISE TELEMETRY (rate-limited; 60s window) ---
  if (now - lastSensorTelemetryLogMs >= 60000) {
    lastSensorTelemetryLogMs = now;
    if (ultrasonicCycleOkCountWin || ultrasonicCycleTimeoutCountWin || flowDiscardMaxSaneCountWin || flowStuckHighEventCountWin) {
      Serial.printf("[TELEM] Ultrasonic ok/timeout (60s): %lu/%lu | Flow discards (60s): %lu | Flow stuck events (60s): %lu | last_us_cm=%.1f\n",
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

  // --- Phase 3: Light Sleep during scheduled sleep window ---
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
