#include "bootloader.h"
#include <Arduino.h>
#include <esp_task_wdt.h>

#include "../../config/config.h"
#include "../../state/state.h"
#include "../../rs485/rs485_comm.h"
#include "../../drivers/pump_driver.h"
#include "../../drivers/sensor_driver.h"
#include "../../persistence/persistence.h"
#include "../../connectivity/connectivity_cloud.h"
#include "../../utils/time_utils.h"

void Bootloader::executeSetup() {
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

  PumpDriver::init();
  Rs485Comm::init();
  SensorDriver::init();
  
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
      time_t nowEpoch = mktime(&timeinfo);
      if (nowEpoch > 0) {
        ntpEpochSecAtLastSync = (uint32_t)nowEpoch;
        ntpLastSyncMs = millis();
      }
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
