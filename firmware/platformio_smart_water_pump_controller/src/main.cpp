#include <Arduino.h>
#include <esp_task_wdt.h>

#include "config/config.h"
#include "state/state.h"

#include "rs485/rs485_comm.h"
#include "safety/safety_pump.h"
#include "persistence/persistence.h"
#include "connectivity/connectivity_cloud.h"

// Forward declare local helper (moved later into utils if desired)
static void updateFlowBasedEstimate();

void setup() {
  Serial.begin(115200);
  Serial.println("\n====================================");
  LOG(LOG_LEVEL_INFO, "SYS", " SmartFlow");
  Serial.println("====================================");

  bootReasonStr = getBootReasonString();
  LOG(LOG_LEVEL_INFO, "BOOT", "Reset reason: %s", bootReasonStr.c_str());

  // String heap-fragmentation mitigation (reserve once at boot)
  pumpMode.reserve(12);
  runMode.reserve(16);
  runPrevPumpMode.reserve(16);
  lastFaultCode.reserve(24);
  lastFaultMessage.reserve(160);
  firebaseLastError.reserve(200);
  bootReasonStr.reserve(32);
  lastPersistedMode.reserve(12);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Force relay OFF before state machine begins (Active-LOW)
  setPump(false);

  rs485_init();
  LOG(LOG_LEVEL_INFO, "INIT", "GPIO configured. Pump OFF.");
  LOG(LOG_LEVEL_INFO, "INIT", "RS-485 UART2 initialized (115200 8N1).");

  checkCrashLoop();
  if (inSafeMode) {
    LOG(LOG_LEVEL_INFO, "SAFE MODE", "Skipping WiFi, Firebase, and sensor init.");
    LOG(LOG_LEVEL_INFO, "SAFE MODE", "Will auto-clear after 1 hour or full power cycle.");
    return;
  }

  loadDeviceConfigFromNVS();
  loadStateFromNVS();

  if (STARTUP_STABILIZE_MS > 0) {
    LOG(LOG_LEVEL_INFO, "INIT", "Stabilization delay (%lums)...", (unsigned long)STARTUP_STABILIZE_MS);
    unsigned long t0 = millis();
    while ((millis() - t0) < (unsigned long)STARTUP_STABILIZE_MS) {
      delay(1);
    }
  }

  // M-31: register WDT before potentially long WiFi connect (WDT_TIMEOUT_SEC covers boot + WiFi).
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
  LOG(LOG_LEVEL_ERROR, "INIT", "Watchdog: %ds timeout, task registered (before WiFi).", WDT_TIMEOUT_SEC);

  connectWiFi();

  if (WiFi.status() == WL_CONNECTED) {
    configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 5000)) {
      ntpSynced = true;
      LOG(LOG_LEVEL_INFO, "NTP", "Time synced: %04d-%02d-%02d %02d:%02d (PHT)", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                    timeinfo.tm_hour, timeinfo.tm_min);
    } else {
      LOG(LOG_LEVEL_ERROR, "NTP", "Sync failed. Sleep mode disabled until next WiFi connect.");
    }
  } else {
    LOG(LOG_LEVEL_INFO, "NTP", "No WiFi. Sleep mode disabled.");
  }

  if (WiFi.status() == WL_CONNECTED) {
    initFirebase();
  } else {
    LOG(LOG_LEVEL_INFO, "FIREBASE", "Skipped — no WiFi. Will init when WiFi connects.");
  }

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

  LOG(LOG_LEVEL_INFO, "INIT", "Boot complete. Entering main loop.");
}

void loop() {
  unsigned long now = millis();
  unsigned long loopStartMs = now;

  esp_task_wdt_reset();

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
          LOG(LOG_LEVEL_INFO, "SAFE MODE", "Epoch latched for wall-clock auto-clear.");
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
      LOG(LOG_LEVEL_ERROR, "SAFE MODE", "Timeout reached. Clearing latch and restarting...");
      if (prefs.begin(NVS_STATE_NAMESPACE, false)) {
        prefs.putULong("safe_mode_ms", 0);
        prefs.putUInt("safe_mode_epoch_sec", 0);
        prefs.putInt("boot_count", 0);
        prefs.end();
      }
      ESP.restart();
    }
    static unsigned long lastSafeModeLog = 0;
    if (now - lastSafeModeLog >= 30000) {
      lastSafeModeLog = now;
      unsigned long remaining = (SAFE_MODE_TIMEOUT_MS - min(SAFE_MODE_TIMEOUT_MS, (now - safeModeEnteredMs))) / 60000UL;
      LOG(LOG_LEVEL_INFO, "SAFE MODE", "Pump OFF. %lu min until auto-clear.", remaining);
    }
    delay(100);
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    if (wifiWasConnected) {
      wifiWasConnected = false;
      wifiBackoffMs = WIFI_BACKOFF_INITIAL_MS;
      LOG(LOG_LEVEL_INFO, "WIFI", "Connection lost.");
    }
    if (now - lastWifiRetryMs >= wifiBackoffMs) {
      lastWifiRetryMs = now;
      LOG(LOG_LEVEL_INFO, "WIFI", "Reconnecting (backoff: %lums)...", wifiBackoffMs);
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
      LOG(LOG_LEVEL_INFO, "WIFI", "Reconnected! IP: %s | RSSI: %d dBm", WiFi.localIP().toString().c_str(), wifiRssi);

      if (!firebaseInitialized) {
        LOG(LOG_LEVEL_INFO, "FIREBASE", "Late init — WiFi was unavailable at boot.");
        initFirebase();
      } else {
        LOG(LOG_LEVEL_INFO, "FIREBASE", "Refreshing auth token after WiFi recovery.");
        Firebase.refreshToken(&config);
        firebaseConsecutiveFailCount = 0;
        firebaseCooldownUntilMs = millis() + 10000UL;
      }

      configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");
      struct tm timeinfo;
      if (getLocalTime(&timeinfo, 5000)) {
        ntpSynced = true;
        LOG(LOG_LEVEL_INFO, "NTP", "Re-synced: %02d:%02d (PHT)", timeinfo.tm_hour, timeinfo.tm_min);
      } else {
        LOG(LOG_LEVEL_ERROR, "NTP", "Re-sync failed. Will retry on next reconnect.");
      }
    }
    lastWifiRetryMs = 0;
  }

  if (WiFi.status() == WL_CONNECTED && now - lastRssiLogMs >= 60000) {
    lastRssiLogMs = now;
    wifiRssi = WiFi.RSSI();
    LOG(LOG_LEVEL_INFO, "WIFI", "RSSI: %d dBm", wifiRssi);
  }

  if (now - lastHeapDiagMs >= 600000UL) {
    lastHeapDiagMs = now;
    uint32_t freeHeap = ESP.getFreeHeap();
    if (minFreeHeapObserved == 0 || freeHeap < minFreeHeapObserved) {
      minFreeHeapObserved = freeHeap;
    }
    LOG(LOG_LEVEL_INFO, "HEAP", "free=%lu bytes | min_observed=%lu bytes", (unsigned long)freeHeap, (unsigned long)minFreeHeapObserved);
  }

  int currentHour = -1;
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 100)) {
    currentHour = timeinfo.tm_hour;
    if (!ntpSynced) { ntpSynced = true; LOG(LOG_LEVEL_INFO, "NTP", "Time synced (post-reconnect)."); }
  }
  bool emergencyOverride = (waterLevelPct <= cfgSleepEmergencyLevel);
  if (emergencyOverride && cfgSleepEnabled && ntpSynced) {
    static unsigned long lastEmergLog = 0;
    if (now - lastEmergLog >= 60000) {
      lastEmergLog = now;
      LOG(LOG_LEVEL_ERROR, "SLEEP", "Emergency override: level at %d%% (<= %d%%)", waterLevelPct, cfgSleepEmergencyLevel);
    }
  }
  bool wasSleeping = isSleeping;
  isSleeping = cfgSleepEnabled && ntpSynced && (currentHour >= 0) &&
               isInSleepWindow(currentHour) && !emergencyOverride;

  if (!isSleeping && !isRunning && waterLevelPct >= IDLE_LEVEL_THRESHOLD) {
    if (!isIdleMode) {
      if (idleStartMs == 0) idleStartMs = now;
      else if (now - idleStartMs >= IDLE_STABLE_TIME_MS) {
        isIdleMode = true;
        LOG(LOG_LEVEL_INFO, "IDLE", "Tank ≥90%, pump OFF for 5 min — entering slow-poll mode.");
      }
    }
  } else {
    if (isIdleMode) LOG(LOG_LEVEL_INFO, "IDLE", "Exiting slow-poll — resuming normal intervals.");
    isIdleMode = false;
    idleStartMs = 0;
  }

  unsigned long sensorInterval = isSleeping ? SLEEP_WAKE_INTERVAL_MS :
    (isIdleMode ? (unsigned long)cfgIdleSensorIntervalMs : SENSOR_INTERVAL_MS);
  unsigned long firebaseInterval = isSleeping ? SLEEP_WAKE_INTERVAL_MS :
    (isIdleMode ? (unsigned long)cfgIdleFirebaseIntervalMs : FIREBASE_INTERVAL_MS);

  if (isSleeping && !wasSleeping && now - lastSleepLogMs >= 10000) {
    lastSleepLogMs = now;
    LOG(LOG_LEVEL_INFO, "SLEEP", "Entering scheduled sleep — 30s poll interval.");
  } else if (!isSleeping && wasSleeping) {
    lastSleepLogMs = now;
    LOG(LOG_LEVEL_INFO, "SLEEP", "Waking up — resuming normal operation.");
  }

  if (now - lastSensorMs >= sensorInterval) {
    lastSensorMs = now;

    unsigned long rs485CallStart = millis();
    bool gotFrame = rs485_requestData();
    rs485LastCallMs = (uint32_t)(millis() - rs485CallStart);

    int levelForFailureLogic = gotFrame ? waterLevelPct : -1;
    if (gotFrame && (remoteSensorLastErrCode == 1 || remoteSensorLastErrCode == 3)) {
      levelForFailureLogic = -1;
    }
    checkLevelSensorFailure(levelForFailureLogic);

    updateFlowBasedEstimate();

    LOG(LOG_LEVEL_ERROR, "SENSOR", "Level:%d%% | Flow:%.2f LPM | Node:%s | ERR:%d | LevelErr:%s | FlowErr:%s | OverflowErr:%s | Sleep:%s", waterLevelPct, flowRateLpm,
                  remoteSensorOnline ? "ONLINE" : "OFFLINE",
                  remoteSensorLastErrCode,
                  isLevelSensorError ? "Y" : "N",
                  isFlowSensorError ? "Y" : "N",
                  isOverflowError ? "Y" : "N",
                  isSleeping ? "Y" : "N");

    checkSafetyCutoff();
    checkCountdownExpiry();
    executePumpLogic();
  }

  bool normalIntervalDue = (now - lastFirebaseMs >= firebaseInterval);
  bool statusRetryDue = (statusPushRetryCount > 0 && statusPushRetryCount < STATUS_PUSH_RETRY_MAX &&
                         now - statusPushRetryMs >= STATUS_PUSH_RETRY_MS);
  if (normalIntervalDue || statusRetryDue) {
    if (normalIntervalDue) lastFirebaseMs = now;
    unsigned long cloudCycleStart = millis();

    if (firebaseCooldownUntilMs != 0 && now < firebaseCooldownUntilMs) {
      if (now - firebaseLastErrorLogMs >= 60000) {
        firebaseLastErrorLogMs = now;
        unsigned long remaining = (firebaseCooldownUntilMs - now) / 1000UL;
        LOG(LOG_LEVEL_ERROR, "FIREBASE", "Cooling down (%lus left). LastErr: %s", remaining, firebaseLastError.c_str());
      }
    } else if (firebaseCooldownUntilMs != 0 && now >= firebaseCooldownUntilMs) {
      firebaseCooldownUntilMs = 0;
    }

    if (firebaseCooldownUntilMs == 0 && WiFi.status() == WL_CONNECTED && Firebase.ready()) {
      if (lastDeviceConfigMs == 0 || (now - lastDeviceConfigMs >= DEVICE_CONFIG_INTERVAL_MS)) {
        lastDeviceConfigMs = now;
        readDeviceConfigFromFirebase();
      }

      bool controlOk = readFirebaseControl();
      if (controlOk) {
        pushFirebaseStatus();
      } else {
        // Prevent back-to-back cloud calls in the same cycle when transport is already failing.
        if (now - firebaseLastErrorLogMs >= 5000UL) {
          firebaseLastErrorLogMs = now;
          LOG(LOG_LEVEL_ERROR, "FIREBASE", "Cloud cycle short-circuit after control failure.");
        }
      }
    } else {
      if (WiFi.status() != WL_CONNECTED) {
        // wait for WiFi recovery
      } else if (firebaseCooldownUntilMs == 0) {
        firebaseNotReadySkipCount++;
        LOG(LOG_LEVEL_INFO, "FIREBASE", "Not ready. Skipping sync.");
      }
    }

    cloudLastCycleMs = (uint32_t)(millis() - cloudCycleStart);
  }

  persistStateToNVS();

  if (now - lastSensorTelemetryLogMs >= 60000) {
    lastSensorTelemetryLogMs = now;
    if (ultrasonicCycleOkCountWin || ultrasonicCycleTimeoutCountWin || flowDiscardMaxSaneCountWin || flowStuckHighEventCountWin) {
      LOG(LOG_LEVEL_ERROR, "TELEM", "Ultrasonic ok/timeout (60s): %lu/%lu | Flow discards (60s): %lu | Flow stuck events (60s): %lu | last_us_cm=%.1f", (unsigned long)ultrasonicCycleOkCountWin,
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

  uint32_t loopMs = (uint32_t)(millis() - loopStartMs);
  if (loopMs > loopMaxMs) {
    loopMaxMs = loopMs;
  }

  if (isSleeping) {
    esp_task_wdt_reset();
    unsigned long nextWake = lastSensorMs + SLEEP_WAKE_INTERVAL_MS;
    unsigned long remainingMs = (nextWake > now) ? (nextWake - now) : 1000;
    uint64_t sleepUs = (uint64_t)remainingMs * 1000ULL;
    if (sleepUs < 100000ULL) sleepUs = 100000ULL;
    esp_sleep_enable_timer_wakeup(sleepUs);
    esp_light_sleep_start();
    esp_task_wdt_reset();
  }

  delay(1);
}

static void updateFlowBasedEstimate() {
  if (!isRunning || flowRateLpm < cfgDryRunThresholdLpm) {
    lastFlowEstimateMs = millis();
    return;
  }
  unsigned long now = millis();
  float dtSec = (now - lastFlowEstimateMs) / 1000.0f;
  lastFlowEstimateMs = now;
  if (dtSec > 5.0f) return;

  flowVolumeAddedL += flowRateLpm * (dtSec / 60.0f);
  if (levelAnchorPct >= 0) {
    float added = (flowVolumeAddedL / (float)TANK_CAPACITY_L) * 100.0f;
    estimatedLevelPct = constrain((float)levelAnchorPct + added, 0.0f, 100.0f);
  }
}

