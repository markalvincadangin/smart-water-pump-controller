#include <Arduino.h>
#include <esp_task_wdt.h>

#ifdef ENABLE_OTA
#include <ArduinoOTA.h>
#endif

#include "config/config.h"
#include "config/feature_config.h"
#include "config/hardware.h"
#include "state/state.h"
#include "rs485/rs485_comm.h"
#include "safety/safety_pump.h"
#include "core/app/pump_app.h"
#include "persistence/persistence.h"
#include "network/wifi_manager.h"
#include "network/ble_provisioning.h"
#include "cloud/cloud_manager.h"
#include "cloud/device_shadow.h"

#include "utils/time_utils.h"
#include "core/lifecycle/bootloader.h"

// Forward declare local helper
#if FEATURE_SENSOR_SERVICE
static void updateFlowBasedEstimate();
#endif

#ifdef ENABLE_OTA
static bool otaInitialized = false;
static void serviceOta() {
  if (!WifiManager::isConnected()) {
    otaInitialized = false;
    return;
  }

  if (!otaInitialized) {
    const char* otaPassword = SMARTFLOW_OTA_PASSWORD;
    if (otaPassword == nullptr || otaPassword[0] == '\0' ||
        strcmp(otaPassword, "replace_with_a_strong_local_password") == 0) {
      LOG(APP_LOG_LEVEL_ERROR, "OTA", "OTA disabled: SMARTFLOW_OTA_PASSWORD is not configured");
      return;
    }
    ArduinoOTA.setPassword(otaPassword);
    ArduinoOTA.begin();
    otaInitialized = true;
    LOG(APP_LOG_LEVEL_INFO, "OTA", "ArduinoOTA initialized and listening");
  }

  ArduinoOTA.handle();
}
#endif

void setup() {
  // Boot Safety: Ensure the relay is safely OFF immediately upon MCU boot
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_RELAY, HIGH); // Active-LOW: HIGH means OFF

  Bootloader::executeSetup();
  app_logger.initSinks();
  app_logger.beginSinks();
}

void loop() {
  unsigned long now = millis();
  unsigned long loopStartMs = now;

#ifdef ENABLE_OTA
  serviceOta();
#endif

  app_logger.handle();

  esp_task_wdt_reset();

  if (inSafeMode) {
    // ... Existing Safe Mode Logic ...
    // Note: Kept simplified for brevity if acceptable, but I should preserve it to not break things.
    // Let's preserve the existing safe mode handling
    uint32_t safeModeEpochSec = 0;
    if (prefs.begin(NVS_STATE_NAMESPACE, true)) {
      safeModeEpochSec = prefs.getUInt("safe_epoch", 0);
      prefs.end();
    }
    if (ntpSynced && safeModeEpochSec == 0) {
      struct tm ti;
      if (getLocalTime(&ti, 1000)) {
        time_t nowEpoch = mktime(&ti);
        if (nowEpoch > 0) {
          if (prefs.begin(NVS_STATE_NAMESPACE, false)) {
            prefs.putUInt("safe_epoch", (uint32_t)nowEpoch);
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
        prefs.putUInt("safe_epoch", 0);
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

  // BLE / WiFi Handlers
  if (BleProvisioning::isActive()) {
      BleProvisioning::loop();
  }
  WifiManager::loop();

  if (WifiManager::isConnected()) {
      if (!wifiWasConnected) {
          wifiWasConnected = true;
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
  } else {
      wifiWasConnected = false;
  }

  // Cloud Sync
  CloudManager::sync();

  // Extract Intent (Command)
  PumpCommand cmd = DeviceShadow::getCommand();
  if (cmd.type != CommandType::NONE) {
      // Process CLEAR_ERROR specifically
      if (cmd.type == CommandType::CLEAR_ERROR) {
          if (currentState == PumpState::ERROR) {
              isOverflowError = false;
              isDryRunError = false;
              isLevelSensorError = false;
              isFlowSensorError = false;
              currentState = PumpState::IDLE;
              setPump(false);
              runMode = "IDLE";
              LOG(APP_LOG_LEVEL_INFO, "STATE", "ERROR cleared, transitioning to IDLE");
          }
      } else if (cmd.type == CommandType::STOP) {
          currentState = PumpState::IDLE;
          setPump(false);
          isCountdownActive = false;
          runMode = "IDLE";
          LOG(APP_LOG_LEVEL_INFO, "STATE", "STOP received, transitioning to IDLE");
      } else if (cmd.type == CommandType::START_MANUAL && currentState != PumpState::ERROR) {
          currentState = PumpState::MANUAL;
          setPump(true);
          runStartMs = millis();
          runMode = "MANUAL";
          isCountdownActive = false;
          LOG(APP_LOG_LEVEL_INFO, "STATE", "START_MANUAL received, executing Manual mode");
      } else if (cmd.type == CommandType::START_COUNTDOWN && currentState != PumpState::ERROR) {
          currentState = PumpState::COUNTDOWN;
          setPump(true);
          runStartMs = millis(); // Track this to guard against extreme countdowns
          countdownEndMs = millis() + (cmd.durationSeconds * 1000UL);
          isCountdownActive = true;
          runMode = "COUNTDOWN";
          LOG(APP_LOG_LEVEL_INFO, "STATE", "START_COUNTDOWN received, executing Countdown mode");
      }
      DeviceShadow::clearCommand();
  }

  // Safety & Cutoff Execution State Machine
  switch (currentState) {
      case PumpState::IDLE:
          // Pump is off, wait for commands
          break;

      case PumpState::MANUAL:
          if (elapsedMillis32(millis(), runStartMs) >= ((uint32_t)cfgMaxPumpRuntimeMin * 60000UL)) {
              currentState = PumpState::ERROR;
              setPump(false);
              isOverflowError = true;
              lastFaultCode = "ERR_OVERFLOW";
              lastFaultMessage = "Max manual runtime exceeded";
              runMode = "ERROR";
              LOG(APP_LOG_LEVEL_ERROR, "STATE", "Max runtime breached, transitioning to ERROR");
          }
          break;

      case PumpState::COUNTDOWN:
          // Check countdown completion
          if ((int32_t)(millis() - countdownEndMs) >= 0) {
              currentState = PumpState::IDLE;
              setPump(false);
              isCountdownActive = false;
              runMode = "IDLE";
              LOG(APP_LOG_LEVEL_INFO, "STATE", "Countdown finished naturally");
          } else if (elapsedMillis32(millis(), runStartMs) >= ((uint32_t)cfgMaxPumpRuntimeMin * 60000UL)) {
              // Also guard against misconfigured extreme countdowns
              currentState = PumpState::ERROR;
              setPump(false);
              isOverflowError = true;
              lastFaultCode = "ERR_OVERFLOW";
              lastFaultMessage = "Max runtime exceeded during countdown";
              runMode = "ERROR";
              LOG(APP_LOG_LEVEL_ERROR, "STATE", "Max runtime breached during countdown");
          }
          break;

      case PumpState::ERROR:
          setPump(false); // Fallback enforce
          break;
  }

#if FEATURE_SENSOR_SERVICE
  // Optional Feature: Sensor Polling
  unsigned long sensorInterval = SENSOR_INTERVAL_MS;
  if (now - lastSensorMs >= sensorInterval) {
    lastSensorMs = now;
    unsigned long rs485CallStart = millis();
    bool gotFrame = Rs485Comm::requestData(0);
    rs485LastCallMs = (uint32_t)(millis() - rs485CallStart);

    int levelForFailureLogic = gotFrame ? waterLevelPct : -1;
    if (gotFrame && (remoteSensorLastErrCode == 1 || remoteSensorLastErrCode == 3)) {
      levelForFailureLogic = -1;
    }
    checkLevelSensorFailure(levelForFailureLogic);
    updateFlowBasedEstimate();
  }
#endif

  persistStateToNVS();
  
  uint32_t loopMs = (uint32_t)(millis() - loopStartMs);
  if (loopMs > loopMaxMs) {
    loopMaxMs = loopMs;
  }

  delay(1);
}

#if FEATURE_SENSOR_SERVICE
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
#endif
