#include "bootloader.h"
#include <Arduino.h>
#include <esp_task_wdt.h>

#include "../../config/config.h"
#include "../../state/state.h"
#include "../../rs485/rs485_comm.h"
#include "../../drivers/pump_driver.h"
#include "../../drivers/sensor_driver.h"
#include "../../persistence/persistence.h"
#include "../../network/wifi_manager.h"
#include "../../network/ble_provisioning.h"
#include "../../cloud/cloud_manager.h"
#include "../../safety/safety_pump.h"
#include "../../utils/time_utils.h"
#include "../../config/feature_config.h"

const char* Bootloader::getBootReasonString() {
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

namespace {
bool applyScopedWifiReprovisionRequest(const char* requestId) {
  if (requestId == nullptr || requestId[0] == '\0') {
    return false;
  }
  if (!prefs.begin(NVS_STATE_NAMESPACE, false)) {
    LOG(APP_LOG_LEVEL_ERROR, "REPROVISION", "Unable to inspect Wi-Fi recovery request.");
    return false;
  }
  
  char buf[32];
  bool alreadyApplied = false;
  if (prefs.getString("reprov_req_id", buf, sizeof(buf)) > 0) {
    alreadyApplied = (strcmp(buf, requestId) == 0);
  }
  prefs.end();
  
  if (alreadyApplied) {
    return false;
  }

  // This is deliberately the first state-changing action. Do not replace it
  // with a direct relay call: setPump() keeps safety/accounting state coherent.
  setPump(false);
  LOG(APP_LOG_LEVEL_WARN, "REPROVISION", "Applying scoped Wi-Fi recovery request.");
  if (!clearNetworkEnrollment()) {
    return false;
  }

  if (!prefs.begin(NVS_STATE_NAMESPACE, false)) {
    LOG(APP_LOG_LEVEL_ERROR, "REPROVISION", "Enrollment cleared but request marker could not be saved.");
    // Enrollment is already gone, so the caller must still reboot into BLE.
    return true;
  }
  prefs.putString("reprov_req_id", requestId);
  prefs.end();
  
  // Also clear device configuration to ensure a complete factory reset
  if (prefs.begin(NVS_NAMESPACE, false)) {
    prefs.clear();
    prefs.end();
    LOG(APP_LOG_LEVEL_INFO, "REPROVISION", "Configuration namespace cleared to factory defaults.");
  }

  LOG(APP_LOG_LEVEL_INFO, "REPROVISION", "Enrollment cleared; BLE provisioning will start.");
  return true;
}

void applyOneTimeReprovisionRequest() {
  const char* requestId = SMARTFLOW_REPROVISION_REQUEST_ID;
  if (requestId[0] == '\0') {
    return;
  }
  applyScopedWifiReprovisionRequest(requestId);
}
} // namespace

bool Bootloader::applyWifiReprovisionRequest(const char* requestId) {
  if (!applyScopedWifiReprovisionRequest(requestId)) {
    return false;
  }
  LOG(APP_LOG_LEVEL_WARN, "REPROVISION", "Restarting into BLE provisioning after authorized recovery.");
  delay(50);
  esp_restart();
  return true; // Unreachable, keeps the API testable on non-ESP targets.
}

void Bootloader::executeSetup() {
  Serial.begin(115200);
  Serial.println("\n====================================");
  LOG(APP_LOG_LEVEL_INFO, "SYS", " SmartFlow");
  Serial.println("====================================");

  // String heap-fragmentation mitigation (reserve once at boot)
  pumpMode.reserve(12);
  runMode.reserve(16);
  runPrevPumpMode.reserve(16);
  lastFaultCode.reserve(24);
  lastFaultMessage.reserve(160);
  firebaseLastError.reserve(200);
  bootReasonStr.reserve(32);
  lastPersistedMode.reserve(12);

  bootReasonStr = String(getBootReasonString());
  LOG(APP_LOG_LEVEL_INFO, "BOOT", "Reset reason: %s", bootReasonStr.c_str());

  PumpDriver::init();
#if FEATURE_SENSOR_SERVICE
  Rs485Comm::init();
  SensorDriver::init();
#else
  LOG(APP_LOG_LEVEL_INFO, "BOOT", "Sensor Service (RS-485) is disabled via config.");
#endif
  
  LOG(APP_LOG_LEVEL_INFO, "INIT", "GPIO configured. Pump OFF.");
  LOG(APP_LOG_LEVEL_INFO, "INIT", "RS-485 UART2 initialized (115200 8N1).");

  applyOneTimeReprovisionRequest();

  checkCrashLoop();
  if (inSafeMode) {
    LOG(APP_LOG_LEVEL_INFO, "SAFE MODE", "Skipping WiFi, Firebase, and sensor init.");
    LOG(APP_LOG_LEVEL_INFO, "SAFE MODE", "Will auto-clear after 1 hour or full power cycle.");
#ifdef ENABLE_OTA
    // Keep the pump fail-off, but allow an authenticated local recovery update
    // using already-stored credentials. The regular application, sensors, and
    // cloud services remain disabled while Safe Mode is latched.
    WifiManager::init();
    WifiManager::connect();
    LOG(APP_LOG_LEVEL_INFO, "SAFE MODE", "Wi-Fi recovery path enabled for OTA.");
#endif
    return;
  }

  loadDeviceConfigFromNVS();
  loadStateFromNVS();

  if (STARTUP_STABILIZE_MS > 0) {
    LOG(APP_LOG_LEVEL_INFO, "INIT", "Stabilization delay (%lums)...", (unsigned long)STARTUP_STABILIZE_MS);
    unsigned long t0 = millis();
    while ((millis() - t0) < (unsigned long)STARTUP_STABILIZE_MS) {
      delay(1);
    }
  }

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
  LOG(APP_LOG_LEVEL_ERROR, "INIT", "Watchdog: %ds timeout, task registered (before WiFi).", WDT_TIMEOUT_SEC);

  WifiManager::init();

  if (!BleProvisioning::isProvisioned()) {
    deviceLifecycle = DeviceLifecycle::PROVISIONING;
    BleProvisioning::init();
    LOG(APP_LOG_LEVEL_INFO, "INIT", "Device UNCLAIMED. Entered PROVISIONING mode.");
  } else {
    deviceLifecycle = DeviceLifecycle::ONLINE;
    LOG(APP_LOG_LEVEL_INFO, "INIT", "Device CLAIMED. Entered ONLINE mode.");
    WifiManager::connect();
  }

  if (WifiManager::isConnected()) {
    configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 5000)) {
      ntpSynced = true;
      time_t nowEpoch = mktime(&timeinfo);
      if (nowEpoch > 0) {
        ntpEpochSecAtLastSync = (uint32_t)nowEpoch;
        ntpLastSyncMs = millis();
      }
      LOG(APP_LOG_LEVEL_INFO, "NTP", "Time synced: %04d-%02d-%02d %02d:%02d (PHT)", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                    timeinfo.tm_hour, timeinfo.tm_min);
    } else {
      LOG(APP_LOG_LEVEL_ERROR, "NTP", "Sync failed. Sleep mode disabled until next WiFi connect.");
    }
  } else {
    LOG(APP_LOG_LEVEL_INFO, "NTP", "No WiFi. Sleep mode disabled.");
  }

  if (deviceLifecycle == DeviceLifecycle::ONLINE) {
    CloudManager::init();
  }

  unsigned long nowInit = millis();
  lastSensorMs        = nowInit;
  lastFirebaseMs      = nowInit;
  lastDeviceConfigMs  = 0;
  lastRssiLogMs       = nowInit;
  lastLevelWriteMs    = nowInit;
  lastUptimeWriteMs   = nowInit;
  lastHeapDiagMs      = nowInit;
  minFreeHeapObserved = ESP.getFreeHeap();

  LOG(APP_LOG_LEVEL_INFO, "INIT", "Boot complete. Entering main loop.");
}
