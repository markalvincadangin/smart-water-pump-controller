#include <Arduino.h>
#include <esp_task_wdt.h>

#include "config/config.h"
#include "state/state.h"
#include "rs485/rs485_comm.h"
#include "safety/safety_pump.h"
#include "core/app/pump_app.h"
#include "persistence/persistence.h"
#include "network/wifi_manager.h"
#include "network/ble_provisioning.h"
#include "cloud/cloud_manager.h"

#include "utils/time_utils.h"
#include "core/lifecycle/bootloader.h"

// Forward declare local helper (moved later into utils if desired)
static void updateFlowBasedEstimate();

void setup() {
  Bootloader::executeSetup();
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
          LOG(APP_LOG_LEVEL_INFO, "SAFE MODE", "Epoch latched for wall-clock auto-clear.");
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
      LOG(APP_LOG_LEVEL_ERROR, "SAFE MODE", "Timeout reached. Clearing latch and restarting...");
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
      LOG(APP_LOG_LEVEL_INFO, "SAFE MODE", "Pump OFF. %lu min until auto-clear.", remaining);
    }
    delay(100);
    return;
  }

  if (deviceLifecycle == DeviceLifecycle::PROVISIONING) {
      BleProvisioning::loop();
  }
  WifiManager::loop();

  if (WifiManager::isConnected()) {
      if (!wifiWasConnected) {
          wifiWasConnected = true;
          // Trigger NTP sync on connect
          configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");
          struct tm timeinfo;
          if (getLocalTime(&timeinfo, 5000)) {
              ntpSynced = true;
              time_t nowEpoch = mktime(&timeinfo);
              if (nowEpoch > 0) {
                  ntpEpochSecAtLastSync = (uint32_t)nowEpoch;
                  ntpLastSyncMs = millis();
              }
              LOG(APP_LOG_LEVEL_INFO, "NTP", "Time synced (post-reconnect).");
          }
      }
      
      // Update RSSI occasionally
      if (now - lastRssiLogMs >= 60000) {
          lastRssiLogMs = now;
          LOG(APP_LOG_LEVEL_INFO, "WIFI", "RSSI: %d dBm", WifiManager::getRssi());
      }
  } else {
      if (wifiWasConnected) {
          wifiWasConnected = false;
      }
  }

  if (now - lastHeapDiagMs >= 600000UL) {
    lastHeapDiagMs = now;
    uint32_t freeHeap = ESP.getFreeHeap();
    if (minFreeHeapObserved == 0 || freeHeap < minFreeHeapObserved) {
      minFreeHeapObserved = freeHeap;
    }
    LOG(APP_LOG_LEVEL_INFO, "HEAP", "free=%lu bytes | min_observed=%lu bytes", (unsigned long)freeHeap, (unsigned long)minFreeHeapObserved);
  }

  int currentHour = -1;
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 100)) {
    currentHour = timeinfo.tm_hour;
    if (!ntpSynced) {
      ntpSynced = true;
      time_t nowEpoch = mktime(&timeinfo);
      if (nowEpoch > 0) {
        ntpEpochSecAtLastSync = (uint32_t)nowEpoch;
        ntpLastSyncMs = millis();
      }
      LOG(APP_LOG_LEVEL_INFO, "NTP", "Time synced (post-reconnect).");
    }
  }
  bool emergencyOverride = (waterLevelPct <= cfgSleepEmergencyLevel);
  if (emergencyOverride && cfgSleepEnabled && ntpSynced) {
    static unsigned long lastEmergLog = 0;
    if (now - lastEmergLog >= 60000) {
      lastEmergLog = now;
      LOG(APP_LOG_LEVEL_ERROR, "SLEEP", "Emergency override: level at %d%% (<= %d%%)", waterLevelPct, cfgSleepEmergencyLevel);
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
        LOG(APP_LOG_LEVEL_INFO, "IDLE", "Tank ≥90%, pump OFF for 5 min — entering slow-poll mode.");
      }
    }
  } else {
    if (isIdleMode) LOG(APP_LOG_LEVEL_INFO, "IDLE", "Exiting slow-poll — resuming normal intervals.");
    isIdleMode = false;
    idleStartMs = 0;
  }

  unsigned long sensorInterval = isSleeping ? SLEEP_WAKE_INTERVAL_MS :
    (isIdleMode ? (unsigned long)cfgIdleSensorIntervalMs : SENSOR_INTERVAL_MS);
  unsigned long firebaseInterval = isSleeping ? SLEEP_WAKE_INTERVAL_MS :
    (isIdleMode ? (unsigned long)cfgIdleFirebaseIntervalMs : FIREBASE_INTERVAL_MS);

  if (isSleeping && !wasSleeping && now - lastSleepLogMs >= 10000) {
    lastSleepLogMs = now;
    LOG(APP_LOG_LEVEL_INFO, "SLEEP", "Entering scheduled sleep — 30s poll interval.");
  } else if (!isSleeping && wasSleeping) {
    lastSleepLogMs = now;
    LOG(APP_LOG_LEVEL_INFO, "SLEEP", "Waking up — resuming normal operation.");
  }

  if (now - lastSensorMs >= sensorInterval) {
    lastSensorMs = now;

    unsigned long rs485CallStart = millis();
    // M-01: prevent RS-485 transport stalls from delaying Firebase work.
    bool firebaseDueNow = (now - lastFirebaseMs >= firebaseInterval);
    bool statusRetryDue = (statusPushRetryCount > 0 && statusPushRetryCount < STATUS_PUSH_RETRY_MAX &&
                            now - statusPushRetryMs >= STATUS_PUSH_RETRY_MS);
    uint32_t rs485BudgetMs = (firebaseDueNow || statusRetryDue) ? 150UL : 0UL; // cap blocking when cloud work is due
    bool gotFrame = Rs485Comm::requestData(rs485BudgetMs);
    rs485LastCallMs = (uint32_t)(millis() - rs485CallStart);

    int levelForFailureLogic = gotFrame ? waterLevelPct : -1;
    if (gotFrame && (remoteSensorLastErrCode == 1 || remoteSensorLastErrCode == 3)) {
      levelForFailureLogic = -1;
    }
    checkLevelSensorFailure(levelForFailureLogic);

    updateFlowBasedEstimate();

    LOG(APP_LOG_LEVEL_INFO, "SENSOR", "Level:%d%% | Flow:%.2f LPM | Node:%s | ERR:%d | LevelErr:%s | FlowErr:%s | OverflowErr:%s | Sleep:%s", waterLevelPct, flowRateLpm,
                  remoteSensorOnline ? "ONLINE" : "OFFLINE",
                  remoteSensorLastErrCode,
                  isLevelSensorError ? "Y" : "N",
                  isFlowSensorError ? "Y" : "N",
                  isOverflowError ? "Y" : "N",
                  isSleeping ? "Y" : "N");

    checkSafetyCutoff();
    PumpApp::checkCountdownExpiry();
    PumpApp::executeLogic();
  }

  CloudManager::sync();

  persistStateToNVS();

  if (now - lastSensorTelemetryLogMs >= 60000) {
    lastSensorTelemetryLogMs = now;
    if (ultrasonicCycleOkCountWin || ultrasonicCycleTimeoutCountWin || flowDiscardMaxSaneCountWin || flowStuckHighEventCountWin) {
      LOG(APP_LOG_LEVEL_INFO, "TELEM", "Ultrasonic ok/timeout (60s): %lu/%lu | Flow discards (60s): %lu | Flow stuck events (60s): %lu | last_us_cm=%.1f", (unsigned long)ultrasonicCycleOkCountWin,
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
    unsigned long nextWake = addMillisSaturated(lastSensorMs, SLEEP_WAKE_INTERVAL_MS);
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

