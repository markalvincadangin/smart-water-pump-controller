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

// Forward declare local helpers
#if FEATURE_SENSOR_SERVICE
static void updateFlowBasedEstimate();
static void handleSensors(unsigned long now);
#endif
static void handlePhysicalControls(unsigned long now);
static void handleSafeMode(unsigned long now);
static void handleNetworkState();
static void handleCloudCommands();
static void handleStateTransitions();

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

  // Physical Controls
  pinMode(PIN_RESET_BUTTON, INPUT_PULLUP);

  Bootloader::executeSetup();
  app_logger.initSinks();
  app_logger.beginSinks();
}

void loop() {
  unsigned long now = millis();
  unsigned long loopStartMs = now;

  handlePhysicalControls(now);

#ifdef ENABLE_OTA
  serviceOta();
#endif

  app_logger.handle();
  esp_task_wdt_reset();

  if (inSafeMode) {
    handleSafeMode(now);
    delay(100);
    return;
  }

  handleNetworkState();
  CloudManager::sync();
  handleCloudCommands();
  handleStateTransitions();
  
  PumpApp::executeLogic();

#if FEATURE_SENSOR_SERVICE
  handleSensors(now);
#endif

  persistStateToNVS();
  
  uint32_t loopMs = (uint32_t)(millis() - loopStartMs);
  if (loopMs > loopMaxMs) {
    loopMaxMs = loopMs;
  }

  delay(1);
}

// -----------------------------------------------------------------------------
// Orchestration Helpers
// -----------------------------------------------------------------------------

/**
 * @brief Checks the hardware reset button state and executes Soft Reboot or Factory Reset.
 *
 * @param now Current uptime in milliseconds.
 */
static void handlePhysicalControls(unsigned long now) {
  static uint32_t resetBtnPressStartMs = 0;
  static bool resetBtnWasPressed = false;
  if (digitalRead(PIN_RESET_BUTTON) == LOW) {
    if (!resetBtnWasPressed) {
      resetBtnWasPressed = true;
      resetBtnPressStartMs = now;
      LOG(APP_LOG_LEVEL_INFO, "SYSTEM", "Reset button pressed. 3s = Reboot, 10s = Factory Reset.");
    } else {
      if (elapsedMillis32(now, resetBtnPressStartMs) >= 10000UL) {
        LOG(APP_LOG_LEVEL_WARN, "SYSTEM", "10-second hold met! Executing Factory Reset.");
        char reqId[24];
        snprintf(reqId, sizeof(reqId), "hw-btn-%lu", now);
        Bootloader::applyWifiReprovisionRequest(reqId);
        resetBtnWasPressed = false; // Reset in case apply fails
      }
    }
  } else {
    if (resetBtnWasPressed) {
      uint32_t heldFor = elapsedMillis32(now, resetBtnPressStartMs);
      resetBtnWasPressed = false;
      
      if (heldFor >= 3000UL) {
        LOG(APP_LOG_LEVEL_WARN, "SYSTEM", "Button released after %lu ms. Executing Soft Reboot.", (unsigned long)heldFor);
        
        // Safety: Turn off pump
        setPump(false);
        
        // Clear boot count and safe mode latch before restart
        Preferences prefs;
        if (prefs.begin(NVS_STATE_NAMESPACE, false)) {
          prefs.putInt("boot_count", 0);
          prefs.putULong("safe_mode_ms", 0);
          prefs.putUInt("safe_epoch", 0);
          prefs.end();
        }
        
        delay(500); // Small delay to let log flush
        ESP.restart();
      } else {
        LOG(APP_LOG_LEVEL_INFO, "SYSTEM", "Reset button released after %lu ms. Ignored (needs 3s).", (unsigned long)heldFor);
      }
    }
  }
}

/**
 * @brief Manages the device state when booted into Safe Mode.
 *
 * Prevents logic execution and attempts to clear Safe Mode automatically 
 * after the timeout elapses.
 *
 * @param now Current uptime in milliseconds.
 */
static void handleSafeMode(unsigned long now) {
  static uint32_t safeModeEpochSec = 0;
  static bool epochLoaded = false;
  
  if (!epochLoaded) {
    Preferences prefs;
    if (prefs.begin(NVS_STATE_NAMESPACE, true)) {
      safeModeEpochSec = prefs.getUInt("safe_epoch", 0);
      prefs.end();
    }
    epochLoaded = true;
  }
  
  if (ntpSynced && safeModeEpochSec == 0) {
    struct tm ti;
    if (getLocalTime(&ti, 1000)) {
      time_t nowEpoch = mktime(&ti);
      if (nowEpoch > 0) {
        Preferences prefs;
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
    Preferences prefs;
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
}

/**
 * @brief Processes network connections (BLE provisioning and Wi-Fi).
 *
 * Also syncs system time with NTP upon successful Wi-Fi connection.
 */
static void handleNetworkState() {
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
        static bool bootEventSent = false;
        if (!bootEventSent) {
          bootEventSent = true;
          LOG_CLOUD(APP_LOG_LEVEL_INFO, LogCategory::SYSTEM, EventCode::EVT_BOOT, "SmartFlow Controller Booted and Connected");
        }
      }
    }
  } else {
    wifiWasConnected = false;
  }
}

/**
 * @brief Processes incoming commands from the Cloud Device Shadow.
 */
static void handleCloudCommands() {
  PumpCommand cmd = DeviceShadow::getCommand();
  if (cmd.type == CommandType::NONE) {
    return;
  }

  if (cmd.type == CommandType::CLEAR_ERROR) {
    isOverflowError = false;
    isDryRunError = false;
    isLevelSensorError = false;
    isFlowSensorError = false;
    if (currentState == PumpState::ERROR) {
      currentState = PumpState::IDLE;
    }
    LOG(APP_LOG_LEVEL_INFO, "STATE", "ERROR cleared");
    CloudManager::clearErrorDesiredState();
  } else if (cmd.type == CommandType::STOP_AUTO) {
    pumpMode = "AUTO";
    currentState = PumpState::IDLE;
    isCountdownActive = false;
    manualDesired = false;
    LOG(APP_LOG_LEVEL_INFO, "STATE", "STOP_AUTO received, reverting to AUTO standby");
  } else if (cmd.type == CommandType::STOP_MANUAL) {
    pumpMode = "MANUAL";
    currentState = PumpState::IDLE;
    isCountdownActive = false;
    manualDesired = false;
    LOG(APP_LOG_LEVEL_INFO, "STATE", "STOP_MANUAL received, entering MANUAL OFF");
  } else if (cmd.type == CommandType::STOP_COUNTDOWN) {
    pumpMode = "COUNTDOWN";
    currentState = PumpState::IDLE;
    isCountdownActive = false;
    manualDesired = false;
    LOG(APP_LOG_LEVEL_INFO, "STATE", "STOP_COUNTDOWN received, entering COUNTDOWN standby");
  } else if (cmd.type == CommandType::START_MANUAL && currentState != PumpState::ERROR) {
    pumpMode = "MANUAL";
    currentState = PumpState::MANUAL;
    manualDesired = true;
    isCountdownActive = false;
    runStartMs = millis();
    LOG(APP_LOG_LEVEL_INFO, "STATE", "START_MANUAL received, executing Manual mode");
  } else if (cmd.type == CommandType::START_COUNTDOWN && currentState != PumpState::ERROR) {
    pumpMode = "COUNTDOWN";
    currentState = PumpState::COUNTDOWN;
    isCountdownActive = true;
    manualDesired = false;
    runStartMs = millis(); // Track this to guard against extreme countdowns
    countdownEndMs = millis() + (cmd.durationSeconds * 1000UL);
    LOG(APP_LOG_LEVEL_INFO, "STATE", "START_COUNTDOWN received, executing Countdown mode");
  } else if (cmd.type == CommandType::REBOOT_DEVICE) {
    LOG(APP_LOG_LEVEL_WARN, "SYSTEM", "Remote REBOOT requested from Cloud!");
    int retry = 0;
    while (!CloudManager::clearRebootDesiredState() && retry < 3) {
      delay(1000);
      retry++;
    }
    Preferences prefs;
    if (prefs.begin(NVS_STATE_NAMESPACE, false)) {
      prefs.putInt("boot_count", 0);
      prefs.putULong("safe_mode_ms", 0);
      prefs.putUInt("safe_epoch", 0);
      prefs.end();
    }
    delay(1000); // Give time for clear state to hit network queue
    ESP.restart();
  }
  DeviceShadow::clearCommand();
}

/**
 * @brief Handles transitions between high-level states (e.g. entering ERROR state or finishing COUNTDOWN).
 */
static void handleStateTransitions() {
  if (isDryRunError || isOverflowError || isLevelSensorError || isFlowSensorError) {
    currentState = PumpState::ERROR;
  } else if (pumpMode == "AUTO" && currentState != PumpState::ERROR) {
    currentState = PumpState::IDLE;
  }

  // Handle countdown expiry: timer reached zero naturally.
  if (isCountdownActive && countdownEndMs != 0 && (int32_t)(millis() - countdownEndMs) >= 0) {
    LOG(APP_LOG_LEVEL_INFO, "STATE", "Countdown finished. Reverting to MANUAL OFF.");

    // First clear countdown_start so it doesn't re-trigger on reboot.
    CloudManager::clearCountdownDesiredState();
    delay(100);
    // Then update the cloud mode to MANUAL so the app reflects the finished state.
    CloudManager::setErrorFallbackDesiredState();

    pumpMode = "MANUAL";
    manualDesired = false;
    currentState = PumpState::IDLE;
    isCountdownActive = false;
    countdownEndMs = 0;
  }
}

#if FEATURE_SENSOR_SERVICE
/**
 * @brief Periodically polls hardware sensors (e.g. Ultrasonic via RS485).
 *
 * @param now Current uptime in milliseconds.
 */
static void handleSensors(unsigned long now) {
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
}

/**
 * @brief Approximates the current tank volume based on pump flow rate when ultrasonic data is unavailable.
 */
static void updateFlowBasedEstimate() {
  if (!isRunning || flowRateLpm < cfgDryRunThresholdLpm) {
    lastFlowEstimateMs = millis();
    return;
  }
  unsigned long now = millis();
  float dtSec = (now - lastFlowEstimateMs) / 1000.0f;
  lastFlowEstimateMs = now;
  if (dtSec > 5.0f) {
    return;
  }

  flowVolumeAddedL += flowRateLpm * (dtSec / 60.0f);
  if (levelAnchorPct >= 0) {
    float added = (flowVolumeAddedL / (float)TANK_CAPACITY_L) * 100.0f;
    estimatedLevelPct = constrain((float)levelAnchorPct + added, 0.0f, 100.0f);
  }
}
#endif
