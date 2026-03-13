# 1 "C:\\Users\\markc\\AppData\\Local\\Temp\\tmpfvx69oxo"
#include <Arduino.h>
# 1 "C:/Users/markc/_Projects/micro-controller/smart-water-pump-controller/firmware/platformio_smart_water_pump_controller/src/smart_water_pump_controller.ino"
# 10 "C:/Users/markc/_Projects/micro-controller/smart-water-pump-controller/firmware/platformio_smart_water_pump_controller/src/smart_water_pump_controller.ino"
#include <smart_water_pump_controller_shared.h>
void setup();
void loop();
void IRAM_ATTR flowPulseISR();
float readSingleUltrasonic();
int readUltrasonicSensor();
float calculateFlowRate();
void setPump(bool on);
void checkSensorFailure(int sensorReading);
void checkFlowSensorStuck();
void checkOverflowProtection();
void checkDryRunProtection();
void checkSafetyCutoff();
void executePumpLogic();
void loadDeviceConfigFromNVS();
void saveDeviceConfigToNVS();
bool isInSleepWindow(int currentHour);
void checkCrashLoop();
void loadStateFromNVS();
void persistStateToNVS();
void readDeviceConfigFromFirebase();
void readFirebaseControl();
void pushFirebaseStatus();
void connectWiFi();
void initFirebase();
String getBootReasonString();
#line 12 "C:/Users/markc/_Projects/micro-controller/smart-water-pump-controller/firmware/platformio_smart_water_pump_controller/src/smart_water_pump_controller.ino"
void setup() {
  Serial.begin(115200);
  Serial.println("\n====================================");
  Serial.println(" Smart Water Pump Controller v2.4.0");
  Serial.println("====================================");


  bootReasonStr = getBootReasonString();
  Serial.printf("[BOOT] Reset reason: %s\n", bootReasonStr.c_str());


  pinMode(RELAY_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(FLOW_SENSOR_PIN, INPUT);


  setPump(false);
  digitalWrite(TRIG_PIN, LOW);
  Serial.println("[INIT] GPIO configured. Pump OFF.");


  checkCrashLoop();
  if (inSafeMode) {

    Serial.println("[SAFE MODE] Skipping WiFi, Firebase, and sensor init.");
    Serial.println("[SAFE MODE] Will auto-clear after 1 hour or full power cycle.");
    return;
  }


  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN),
                  flowPulseISR,
                  RISING);
  Serial.println("[INIT] Flow sensor interrupt attached on GPIO 34.");


  loadDeviceConfigFromNVS();


  loadStateFromNVS();


  Serial.println("[INIT] Stabilization delay (5s) — sensors settling...");
  delay(5000);


  connectWiFi();


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


  if (WiFi.status() == WL_CONNECTED) {
    initFirebase();
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
  Serial.printf("[INIT] Watchdog: %ds timeout, task registered.\n", WDT_TIMEOUT_SEC);


  unsigned long nowInit = millis();
  lastSensorMs = nowInit;
  lastFirebaseMs = nowInit;
  lastDeviceConfigMs = 0;
  lastWifiRetryMs = 0;
  lastRssiLogMs = nowInit;
  lastLevelWriteMs = nowInit;
  lastUptimeWriteMs = nowInit;
  lastHeapDiagMs = nowInit;
  minFreeHeapObserved = ESP.getFreeHeap();

  Serial.println("[INIT] Boot complete. Entering main loop.\n");
}
# 121 "C:/Users/markc/_Projects/micro-controller/smart-water-pump-controller/firmware/platformio_smart_water_pump_controller/src/smart_water_pump_controller.ino"
void loop() {
  unsigned long now = millis();


  esp_task_wdt_reset();


  if (inSafeMode) {

    if (now - safeModeEnteredMs >= SAFE_MODE_TIMEOUT_MS) {
      Serial.println("[SAFE MODE] 1-hour timeout reached. Restarting...");

      if (prefs.begin(NVS_STATE_NAMESPACE, false)) {
        prefs.putULong("safe_mode_ms", 0);
        prefs.putInt("boot_count", 0);
        prefs.end();
      }
      ESP.restart();
    }

    static unsigned long lastSafeModeLog = 0;
    if (now - lastSafeModeLog >= 30000) {
      lastSafeModeLog = now;
      unsigned long remaining = (SAFE_MODE_TIMEOUT_MS - (now - safeModeEnteredMs)) / 60000UL;
      Serial.printf("[SAFE MODE] Pump OFF. %lu min until auto-clear.\n", remaining);
    }
    delay(100);
    return;
  }


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
    }
    lastWifiRetryMs = 0;
  }


  if (WiFi.status() == WL_CONNECTED && now - lastRssiLogMs >= 60000) {
    lastRssiLogMs = now;
    wifiRssi = WiFi.RSSI();
    Serial.printf("[WIFI] RSSI: %d dBm\n", wifiRssi);
  }


  if (now - lastHeapDiagMs >= 600000UL) {
    lastHeapDiagMs = now;
    uint32_t freeHeap = ESP.getFreeHeap();
    if (minFreeHeapObserved == 0 || freeHeap < minFreeHeapObserved) {
      minFreeHeapObserved = freeHeap;
    }
    Serial.printf("[HEAP] free=%lu bytes | min_observed=%lu bytes\n",
                  (unsigned long)freeHeap, (unsigned long)minFreeHeapObserved);
  }


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


  unsigned long sensorInterval = isSleeping ? SLEEP_WAKE_INTERVAL_MS :
    (isIdleMode ? (unsigned long)cfgIdleSensorIntervalMs : SENSOR_INTERVAL_MS);
  unsigned long firebaseInterval = isSleeping ? SLEEP_WAKE_INTERVAL_MS :
    (isIdleMode ? (unsigned long)cfgIdleFirebaseIntervalMs : FIREBASE_INTERVAL_MS);


  if (isSleeping && !wasSleeping && now - lastSleepLogMs >= 10000) {
    lastSleepLogMs = now;
    Serial.println("[SLEEP] Entering scheduled sleep — 30s poll interval.");
  } else if (!isSleeping && wasSleeping) {
    lastSleepLogMs = now;
    Serial.println("[SLEEP] Waking up — resuming normal operation.");
  }


  if (now - lastSensorMs >= sensorInterval) {
    lastSensorMs = now;


    int reading = readUltrasonicSensor();


    checkSensorFailure(reading);

    if (reading >= 0) {
      prevWaterLevelPct = waterLevelPct;
      waterLevelPct = reading;
    }


    flowRateLpm = calculateFlowRate();

    Serial.printf("[SENSOR] Level:%d%% | Flow:%.2f LPM | SensorErr:%s | OverflowErr:%s | Sleep:%s\n",
                  waterLevelPct, flowRateLpm,
                  isSensorError ? "Y" : "N",
                  isOverflowError ? "Y" : "N",
                  isSleeping ? "Y" : "N");


    checkSafetyCutoff();


    executePumpLogic();
  }


  if (now - lastFirebaseMs >= firebaseInterval) {
    lastFirebaseMs = now;

    if (firebaseCooldownUntilMs != 0 && now < firebaseCooldownUntilMs) {

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

      if (lastDeviceConfigMs == 0 || (now - lastDeviceConfigMs >= DEVICE_CONFIG_INTERVAL_MS)) {
        lastDeviceConfigMs = now;
        readDeviceConfigFromFirebase();
      }
      readFirebaseControl();
      pushFirebaseStatus();
    } else {
      if (WiFi.status() != WL_CONNECTED) {

      } else if (firebaseCooldownUntilMs == 0) {
        Serial.println("[FIREBASE] Not ready. Skipping sync.");
      }
    }
  }


  persistStateToNVS();


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
# 1 "C:/Users/markc/_Projects/micro-controller/smart-water-pump-controller/firmware/platformio_smart_water_pump_controller/src/01_config.ino"
# 10 "C:/Users/markc/_Projects/micro-controller/smart-water-pump-controller/firmware/platformio_smart_water_pump_controller/src/01_config.ino"
#include <smart_water_pump_controller_shared.h>


FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
Preferences prefs;

int cfgTankEmptyCm = TANK_EMPTY_CM;
int cfgTankFullCm = TANK_FULL_CM;
int cfgPumpStartLevel = PUMP_START_LEVEL;
int cfgPumpStopLevel = PUMP_STOP_LEVEL;
float cfgDryRunThresholdLpm = DRY_RUN_THRESHOLD_LPM;
int cfgDryRunTimeoutSec = (int)(DRY_RUN_TIMEOUT_MS / 1000UL);
float cfgFlowCalibration = FLOW_CALIBRATION_FACTOR;
int cfgMaxPumpRuntimeMin = MAX_PUMP_RUNTIME_MIN;

bool cfgSleepEnabled = SLEEP_DEFAULT_ENABLED;
int cfgSleepStartHour = SLEEP_DEFAULT_START_HOUR;
int cfgSleepEndHour = SLEEP_DEFAULT_END_HOUR;
int cfgSleepEmergencyLevel = SLEEP_DEFAULT_EMERGENCY_LVL;

int cfgSensorFailureThreshold = SENSOR_FAILURE_THRESHOLD;
int cfgIdleSensorIntervalMs = IDLE_SENSOR_INTERVAL_MS_DEF;
int cfgIdleFirebaseIntervalMs = IDLE_FIREBASE_INTERVAL_MS_DEF;

volatile uint32_t pulseCount = 0;
volatile uint64_t lastPulseUs = 0;
float flowRateLpm = 0.0f;
int waterLevelPct = 0;
float waterLevelEma = 0.0f;
bool isRunning = false;
int prevWaterLevelPct = 0;

String pumpMode = "AUTO";
bool isDryRunError = false;
bool isSensorError = false;
bool isFlowSensorError = false;
bool isOverflowError = false;

int sensorFailCount = 0;
unsigned long flowStuckStartMs = 0;
bool flowStuckTimerActive = false;
unsigned long pumpOffStartMs = 0;

uint32_t ultrasonicCycleOkCount = 0;
uint32_t ultrasonicCycleTimeoutCount = 0;
uint32_t ultrasonicLastGoodCmX10 = 0;
uint32_t flowDiscardMaxSaneCount = 0;
uint32_t flowStuckHighEventCount = 0;

unsigned long lastSensorTelemetryLogMs = 0;
uint32_t ultrasonicCycleOkCountWin = 0;
uint32_t ultrasonicCycleTimeoutCountWin = 0;
uint32_t flowDiscardMaxSaneCountWin = 0;
uint32_t flowStuckHighEventCountWin = 0;

unsigned long dryRunStartMs = 0;
bool dryRunTimerActive = false;

unsigned long pumpAutoStartMs = 0;
bool pumpAutoStartTracking = false;

bool inSafeMode = false;
unsigned long safeModeEnteredMs = 0;
String bootReasonStr = "";

unsigned long wifiBackoffMs = WIFI_BACKOFF_INITIAL_MS;
bool wifiWasConnected = false;

int wifiRssi = 0;
unsigned long lastSuccessfulFirebaseMs = 0;
unsigned long lastRssiLogMs = 0;
unsigned long firebaseCooldownUntilMs = 0;
uint32_t firebaseConsecutiveFailCount = 0;
String firebaseLastError = "";
unsigned long firebaseLastErrorLogMs = 0;

String lastPersistedMode = "AUTO";
bool lastPersistedDryRun = false;
int lastPersistedLevel = -1;
unsigned long lastLevelWriteMs = 0;
unsigned long lastUptimeWriteMs = 0;

bool isSleeping = false;
bool ntpSynced = false;
bool isIdleMode = false;
unsigned long idleStartMs = 0;
unsigned long lastSleepLogMs = 0;
int lastRebootRequestId = 0;

unsigned long lastSensorMs = 0;
unsigned long lastFirebaseMs = 0;
unsigned long lastDeviceConfigMs = 0;
unsigned long lastWifiRetryMs = 0;

unsigned long lastHeapDiagMs = 0;
uint32_t minFreeHeapObserved = 0;


String runMode = "AUTO";
String runPrevPumpMode = "AUTO";
unsigned long runStartMs = 0;
unsigned long runDurationMs = 0;
uint32_t runRemainingSec = 0;
String lastFaultCode = "";
String lastFaultMessage = "";
# 1 "C:/Users/markc/_Projects/micro-controller/smart-water-pump-controller/firmware/platformio_smart_water_pump_controller/src/02_sensors.ino"
# 9 "C:/Users/markc/_Projects/micro-controller/smart-water-pump-controller/firmware/platformio_smart_water_pump_controller/src/02_sensors.ino"
#define FLOW_DEBOUNCE_US 2000ULL

void IRAM_ATTR flowPulseISR() {
  uint64_t now = esp_timer_get_time();
  if (now - lastPulseUs > FLOW_DEBOUNCE_US) {
    lastPulseUs = now;
    pulseCount = pulseCount + 1;
  }
}







float readSingleUltrasonic() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, ULTRASONIC_TIMEOUT_MS * 1000UL);
  if (duration == 0) return -1.0f;

  float distanceCm = duration / 58.0f;


  if (distanceCm < 2.0f || distanceCm > 200.0f) return -1.0f;

  return distanceCm;
}






int readUltrasonicSensor() {

  float readings[ULTRASONIC_SAMPLES];
  int validCount = 0;

  for (int i = 0; i < ULTRASONIC_SAMPLES; i++) {
    float d = readSingleUltrasonic();
    if (d >= 0.0f) {
      readings[validCount++] = d;
    }
    if (i < ULTRASONIC_SAMPLES - 1) delay(ULTRASONIC_SAMPLE_DELAY);
  }


  if (validCount == 0) {
    ultrasonicCycleTimeoutCount++;
    ultrasonicCycleTimeoutCountWin++;
    return -1;
  }

  ultrasonicCycleOkCount++;
  ultrasonicCycleOkCountWin++;


  for (int i = 1; i < validCount; i++) {
    float key = readings[i];
    int j = i - 1;
    while (j >= 0 && readings[j] > key) {
      readings[j + 1] = readings[j];
      j--;
    }
    readings[j + 1] = key;
  }
  float medianDist = readings[validCount / 2];

  ultrasonicLastGoodCmX10 = (uint32_t)(medianDist * 10.0f + 0.5f);


  medianDist = constrain(medianDist, (float)cfgTankFullCm, (float)cfgTankEmptyCm);


  float range = (float)(cfgTankEmptyCm - cfgTankFullCm);
  float levelFloat = 100.0f * ((float)cfgTankEmptyCm - medianDist) / range;
  levelFloat = constrain(levelFloat, 0.0f, 100.0f);


  if (waterLevelEma < 0.1f && waterLevelPct == 0) {

    waterLevelEma = levelFloat;
  } else {
    waterLevelEma = ULTRASONIC_EMA_ALPHA * levelFloat + (1.0f - ULTRASONIC_EMA_ALPHA) * waterLevelEma;
  }

  int newLevel = (int)(waterLevelEma + 0.5f);
  newLevel = constrain(newLevel, 0, 100);


  int delta = abs(newLevel - prevWaterLevelPct);
  if (prevWaterLevelPct > 0 && delta > LEVEL_RATE_OF_CHANGE_MAX) {
    Serial.printf("[SENSOR][WARN] Level jumped %d%% in 1s (prev=%d%%, new=%d%%). Holding previous.\n",
                  delta, prevWaterLevelPct, newLevel);
    return prevWaterLevelPct;
  }

  return newLevel;
}
# 124 "C:/Users/markc/_Projects/micro-controller/smart-water-pump-controller/firmware/platformio_smart_water_pump_controller/src/02_sensors.ino"
float calculateFlowRate() {

  noInterrupts();
  uint32_t count = pulseCount;
  pulseCount = 0;
  interrupts();



  if (!isRunning && pumpOffStartMs > 0 && (millis() - pumpOffStartMs) > FLOW_PUMP_OFF_ZERO_MS) {
    return 0.0f;
  }


  float lpm = (float)count / cfgFlowCalibration;


  if (lpm > FLOW_MAX_SANE_LPM) {
    flowDiscardMaxSaneCount++;
    flowDiscardMaxSaneCountWin++;
    Serial.printf("[SENSOR][WARN] Flow %.1f LPM exceeds max sane (%.0f). Discarded.\n",
                  lpm, FLOW_MAX_SANE_LPM);
    return flowRateLpm;
  }

  return lpm;
}
# 1 "C:/Users/markc/_Projects/micro-controller/smart-water-pump-controller/firmware/platformio_smart_water_pump_controller/src/03_safety_pump.ino"







void setPump(bool on) {
  digitalWrite(RELAY_PIN, on ? LOW : HIGH);
  isRunning = on;
  if (!on) {
    pumpOffStartMs = millis();
  } else {
    pumpOffStartMs = 0;
  }
}
# 25 "C:/Users/markc/_Projects/micro-controller/smart-water-pump-controller/firmware/platformio_smart_water_pump_controller/src/03_safety_pump.ino"
void checkSensorFailure(int sensorReading) {
  if (sensorReading == -1) {
    sensorFailCount++;
    if (sensorFailCount >= cfgSensorFailureThreshold && !isSensorError) {
      isSensorError = true;
      Serial.printf("[SENSOR][ERROR] Ultrasonic failure: %d consecutive timeouts.\n", sensorFailCount);
    }
  } else {
    if (isSensorError) {
      Serial.println("[SENSOR][INFO] Ultrasonic recovered. Error cleared.");
    }
    sensorFailCount = 0;
    isSensorError = false;
  }
}






void checkFlowSensorStuck() {
  if (!isRunning && flowRateLpm > FLOW_STUCK_THRESHOLD_LPM) {
    if (!flowStuckTimerActive) {
      flowStuckTimerActive = true;
      flowStuckStartMs = millis();
    } else if (millis() - flowStuckStartMs >= FLOW_STUCK_TIMEOUT_MS) {
      if (!isFlowSensorError) {
        isFlowSensorError = true;
        flowStuckHighEventCount++;
        flowStuckHighEventCountWin++;
        Serial.printf("[SENSOR][ERROR] Flow stuck-high: %.1f LPM while pump OFF for >%ds.\n",
                      flowRateLpm, (int)(FLOW_STUCK_TIMEOUT_MS / 1000));
      }
    }
  } else {

    if (isFlowSensorError) {
      Serial.println("[SENSOR][INFO] Flow sensor recovered. Error cleared.");
      isFlowSensorError = false;
    }
    flowStuckTimerActive = false;
    flowStuckStartMs = 0;
  }
}






void checkOverflowProtection() {
  if (!isRunning || pumpMode != "AUTO") {

    pumpAutoStartTracking = false;
    pumpAutoStartMs = 0;
    return;
  }

  if (!pumpAutoStartTracking) {

    pumpAutoStartTracking = true;
    pumpAutoStartMs = millis();
    return;
  }

  unsigned long maxRuntimeMs = (unsigned long)cfgMaxPumpRuntimeMin * 60000UL;
  unsigned long elapsed = millis() - pumpAutoStartMs;
  if (elapsed >= maxRuntimeMs) {
    isOverflowError = true;
    setPump(false);
    pumpAutoStartTracking = false;
    Serial.printf("[SAFETY][ERROR] Max runtime exceeded (%d min). Pump stopped.\n", cfgMaxPumpRuntimeMin);
  }
}






void checkDryRunProtection() {
  if (!isRunning) {
    dryRunTimerActive = false;
    dryRunStartMs = 0;
    return;
  }

  unsigned long dryRunTimeoutMs = (unsigned long)cfgDryRunTimeoutSec * 1000UL;
  if (flowRateLpm < cfgDryRunThresholdLpm) {
    if (!dryRunTimerActive) {
      dryRunTimerActive = true;
      dryRunStartMs = millis();
      Serial.println("[SAFETY][WARN] Dry-run condition detected. Timer started.");
    } else {
      unsigned long elapsed = millis() - dryRunStartMs;
      if (elapsed >= dryRunTimeoutMs) {
        isDryRunError = true;
        setPump(false);
        Serial.println("[SAFETY][ERROR] DRY-RUN LOCKOUT. Pump stopped; waiting for acknowledge.");
      }
    }
  } else {
    if (dryRunTimerActive) {
      Serial.println("[SAFETY][INFO] Flow restored. Dry-run timer reset.");
    }
    dryRunTimerActive = false;
    dryRunStartMs = 0;
  }
}




void checkSafetyCutoff() {
  checkDryRunProtection();
  checkFlowSensorStuck();
  checkOverflowProtection();
}
# 158 "C:/Users/markc/_Projects/micro-controller/smart-water-pump-controller/firmware/platformio_smart_water_pump_controller/src/03_safety_pump.ino"
void executePumpLogic() {



  if (runMode == "TIMED" && runDurationMs > 0 && runStartMs > 0) {
    unsigned long elapsed = millis() - runStartMs;
    if (elapsed >= runDurationMs) {
      Serial.println("[RUN] Timed run complete. Stopping pump.");
      setPump(false);
      runMode = "AUTO";
      runRemainingSec = 0;
      runStartMs = 0;
      runDurationMs = 0;
      pumpMode = runPrevPumpMode.length() > 0 ? runPrevPumpMode : "AUTO";
    } else {
      unsigned long remainingMs = runDurationMs - elapsed;
      runRemainingSec = (uint32_t)((remainingMs + 999UL) / 1000UL);
    }
  } else if (runMode != "TIMED") {
    runRemainingSec = 0;
  }


  if (isDryRunError || isOverflowError) {
    if (isDryRunError) {
      lastFaultCode = "DRY_RUN";
      lastFaultMessage = "Dry-run lockout: low flow while pump was running.";
    } else if (isOverflowError) {
      lastFaultCode = "OVERFLOW";
      lastFaultMessage = "Overflow protection: max runtime exceeded in AUTO.";
    }
    setPump(false);

    runMode = "OFF";
    runRemainingSec = 0;
    runStartMs = 0;
    runDurationMs = 0;
    return;
  }

  if (pumpMode == "FORCE_ON") {

    setPump(true);

  } else if (pumpMode == "FORCE_OFF") {
    setPump(false);

  } else {



    if (isSleeping) {
      if (isRunning) {

        if (waterLevelPct >= cfgPumpStopLevel) {
          Serial.printf("[AUTO] Water at %d%%. Stopping pump.\n", waterLevelPct);
          setPump(false);
        }
      }

      return;
    }


    if (isSensorError) {
      if (isRunning) {
        Serial.println("[AUTO] Sensor error — stopping pump (fail-safe).");
        lastFaultCode = "SENSOR";
        lastFaultMessage = "Sensor error: controller stopped pump in AUTO (fail-safe).";
        setPump(false);
      }
      return;
    }


    if (!isRunning && waterLevelPct <= cfgPumpStartLevel) {
      Serial.printf("[AUTO] Water at %d%%. Starting pump.\n", waterLevelPct);
      setPump(true);
    } else if (isRunning && waterLevelPct >= cfgPumpStopLevel) {
      Serial.printf("[AUTO] Water at %d%%. Stopping pump.\n", waterLevelPct);
      setPump(false);
    }

  }


  if (runMode == "MANUAL" || runMode == "TIMED") {

  } else if (!isRunning) {
    runMode = "OFF";
  } else if (pumpMode == "AUTO") {
    runMode = "AUTO";
  }
}
# 1 "C:/Users/markc/_Projects/micro-controller/smart-water-pump-controller/firmware/platformio_smart_water_pump_controller/src/04_persistence.ino"
# 9 "C:/Users/markc/_Projects/micro-controller/smart-water-pump-controller/firmware/platformio_smart_water_pump_controller/src/04_persistence.ino"
void loadDeviceConfigFromNVS() {
  if (!prefs.begin(NVS_NAMESPACE, true)) {
    Serial.println("[NVS] Namespace open failed. Using firmware defaults.");
    return;
  }
  if (prefs.getInt("tank_empty", -1) == -1) {
    prefs.end();
    Serial.println("[NVS] No saved config. Using firmware defaults.");
    return;
  }
  int te = prefs.getInt("tank_empty", TANK_EMPTY_CM);
  int tf = prefs.getInt("tank_full", TANK_FULL_CM);
  int ps = prefs.getInt("pump_start", PUMP_START_LEVEL);
  int po = prefs.getInt("pump_stop", PUMP_STOP_LEVEL);
  float drLpm = prefs.getFloat("dry_run_lpm", DRY_RUN_THRESHOLD_LPM);
  int drSec = prefs.getInt("dry_run_sec", (int)(DRY_RUN_TIMEOUT_MS / 1000UL));
  float flowCal = prefs.getFloat("flow_cal", FLOW_CALIBRATION_FACTOR);
  int maxRuntime = prefs.getInt("max_runtime", MAX_PUMP_RUNTIME_MIN);


  bool slpEn = prefs.getBool("slp_en", SLEEP_DEFAULT_ENABLED);
  int slpStart = prefs.getInt("slp_start", SLEEP_DEFAULT_START_HOUR);
  int slpEnd = prefs.getInt("slp_end", SLEEP_DEFAULT_END_HOUR);
  int slpEmerg = prefs.getInt("slp_emerg", SLEEP_DEFAULT_EMERGENCY_LVL);


  int sensThresh = prefs.getInt("sens_thresh", SENSOR_FAILURE_THRESHOLD);
  int idleSens = prefs.getInt("idle_sens_ms", IDLE_SENSOR_INTERVAL_MS_DEF);
  int idleFb = prefs.getInt("idle_fb_ms", IDLE_FIREBASE_INTERVAL_MS_DEF);

  prefs.end();

  if (te < 5 || te > 200 || tf < 1 || tf >= te || ps < 0 || ps > 100 || po < 0 || po > 100 || po <= ps
      || drLpm < 0.1f || drLpm > 10.0f || drSec < 10 || drSec > 300
      || flowCal < 0.1f || flowCal > 20.0f || maxRuntime < 30 || maxRuntime > 480) {
    Serial.println("[NVS] Stored config invalid. Using firmware defaults.");
    return;
  }
  cfgTankEmptyCm = te;
  cfgTankFullCm = tf;
  cfgPumpStartLevel = ps;
  cfgPumpStopLevel = po;
  cfgDryRunThresholdLpm = drLpm;
  cfgDryRunTimeoutSec = drSec;
  cfgFlowCalibration = flowCal;
  cfgMaxPumpRuntimeMin = maxRuntime;


  if (slpStart >= 0 && slpStart <= 23) cfgSleepStartHour = slpStart;
  if (slpEnd >= 0 && slpEnd <= 23) cfgSleepEndHour = slpEnd;
  if (slpEmerg >= 0 && slpEmerg <= 100) cfgSleepEmergencyLevel = slpEmerg;
  cfgSleepEnabled = slpEn;


  if (sensThresh >= 3 && sensThresh <= 20) cfgSensorFailureThreshold = sensThresh;
  if (idleSens >= 5000 && idleSens <= 60000) cfgIdleSensorIntervalMs = idleSens;
  if (idleFb >= 10000 && idleFb <= 120000) cfgIdleFirebaseIntervalMs = idleFb;

  Serial.println("[NVS] Device config loaded.");
}

void saveDeviceConfigToNVS() {
  if (!prefs.begin(NVS_NAMESPACE, false)) {
    Serial.println("[NVS] Failed to open for write. Config not persisted.");
    return;
  }
  prefs.putInt("tank_empty", cfgTankEmptyCm);
  prefs.putInt("tank_full", cfgTankFullCm);
  prefs.putInt("pump_start", cfgPumpStartLevel);
  prefs.putInt("pump_stop", cfgPumpStopLevel);
  prefs.putFloat("dry_run_lpm", cfgDryRunThresholdLpm);
  prefs.putInt("dry_run_sec", cfgDryRunTimeoutSec);
  prefs.putFloat("flow_cal", cfgFlowCalibration);
  prefs.putInt("max_runtime", cfgMaxPumpRuntimeMin);

  prefs.putBool("slp_en", cfgSleepEnabled);
  prefs.putInt("slp_start", cfgSleepStartHour);
  prefs.putInt("slp_end", cfgSleepEndHour);
  prefs.putInt("slp_emerg", cfgSleepEmergencyLevel);

  prefs.putInt("sens_thresh", cfgSensorFailureThreshold);
  prefs.putInt("idle_sens_ms", cfgIdleSensorIntervalMs);
  prefs.putInt("idle_fb_ms", cfgIdleFirebaseIntervalMs);
  prefs.end();
  Serial.println("[NVS] Device config saved.");
}






bool isInSleepWindow(int currentHour) {
  if (cfgSleepStartHour <= cfgSleepEndHour) {

    return (currentHour >= cfgSleepStartHour && currentHour < cfgSleepEndHour);
  } else {

    return (currentHour >= cfgSleepStartHour) || (currentHour < cfgSleepEndHour);
  }
}







void checkCrashLoop() {
  if (!prefs.begin(NVS_STATE_NAMESPACE, false)) {
    Serial.println("[BOOT] NVS state namespace open failed.");
    return;
  }

  unsigned long now = millis();
  unsigned long lastBootTime = prefs.getULong("last_boot_ms", 0);
  int bootCount = prefs.getInt("boot_count", 0);
  unsigned long safeModeStart = prefs.getULong("safe_mode_ms", 0);


  if (safeModeStart > 0) {


    if (now < 5000) {

      prefs.putULong("safe_mode_ms", 0);
      prefs.putInt("boot_count", 0);
      Serial.println("[BOOT] Power cycle detected. Safe mode cleared.");
      prefs.end();
      return;
    }
  }




  if (lastBootTime > (unsigned long)(CRASH_LOOP_WINDOW_SEC * 1000UL)) {

    bootCount = 0;
  }

  bootCount++;
  prefs.putInt("boot_count", bootCount);
  prefs.putULong("last_boot_ms", 0);

  if (bootCount >= CRASH_LOOP_THRESHOLD) {
    inSafeMode = true;
    safeModeEnteredMs = millis();
    prefs.putULong("safe_mode_ms", safeModeEnteredMs);
    Serial.printf("[ERROR] CRASH LOOP DETECTED: %d reboots. Entering SAFE MODE.\n", bootCount);
    Serial.println("[SAFE MODE] Pump OFF. Firebase disabled. Serial only.");
  } else {
    Serial.printf("[BOOT] Boot count: %d/%d (window: %ds)\n",
                  bootCount, CRASH_LOOP_THRESHOLD, CRASH_LOOP_WINDOW_SEC);
  }

  prefs.end();
}






void loadStateFromNVS() {
  if (!prefs.begin(NVS_STATE_NAMESPACE, true)) return;

  String savedMode = prefs.getString("mode", "AUTO");
  bool savedDryRun = prefs.getBool("dry_run_err", false);
  int savedLevel = prefs.getInt("level_pct", -1);
  prefs.end();


  if (savedMode == "AUTO" || savedMode == "FORCE_ON" || savedMode == "FORCE_OFF") {
    pumpMode = savedMode;
    lastPersistedMode = savedMode;
  }
  isDryRunError = savedDryRun;
  lastPersistedDryRun = savedDryRun;

  Serial.printf("[BOOT] Last state: Level=%d%%, Mode=%s, DryRun=%s\n",
                savedLevel, pumpMode.c_str(), isDryRunError ? "YES" : "NO");
}

void persistStateToNVS() {
  bool modeChanged = (pumpMode != lastPersistedMode);
  bool dryRunChanged = (isDryRunError != lastPersistedDryRun);
  int levelDelta = abs(waterLevelPct - lastPersistedLevel);
  unsigned long now = millis();
  bool levelNeedsWrite = (lastPersistedLevel == -1)
    || (levelDelta >= NVS_LEVEL_DELTA_THRESHOLD)
    || (now - lastLevelWriteMs >= NVS_LEVEL_INTERVAL_MS);
  bool uptimeNeedsWrite = (now - lastUptimeWriteMs >= NVS_UPTIME_INTERVAL_MS);

  if (!modeChanged && !dryRunChanged && !levelNeedsWrite && !uptimeNeedsWrite) return;

  if (!prefs.begin(NVS_STATE_NAMESPACE, false)) return;

  if (modeChanged) {
    prefs.putString("mode", pumpMode);
    lastPersistedMode = pumpMode;
    Serial.printf("[NVS] Mode persisted: %s\n", pumpMode.c_str());
  }
  if (dryRunChanged) {
    prefs.putBool("dry_run_err", isDryRunError);
    lastPersistedDryRun = isDryRunError;
  }
  if (levelNeedsWrite) {
    prefs.putInt("level_pct", waterLevelPct);
    lastPersistedLevel = waterLevelPct;
    lastLevelWriteMs = now;
  }
  if (uptimeNeedsWrite) {
    prefs.putULong("last_boot_ms", now);
    lastUptimeWriteMs = now;
  }

  prefs.end();
}
# 1 "C:/Users/markc/_Projects/micro-controller/smart-water-pump-controller/firmware/platformio_smart_water_pump_controller/src/05_connectivity_cloud.ino"






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


  json.get(jsonData, "max_pump_runtime_min");
  if (jsonData.success) {
    int v = jsonData.intValue;
    if (v >= 30 && v <= 480) maxRun = v; else maxRun = cfgMaxPumpRuntimeMin;
  } else {
    maxRun = cfgMaxPumpRuntimeMin;
  }


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


  int sensThresh = cfgSensorFailureThreshold;
  int idleSens = cfgIdleSensorIntervalMs;
  int idleFb = cfgIdleFirebaseIntervalMs;
  json.get(jsonData, "sensor_failure_threshold");
  if (jsonData.success) { int v = jsonData.intValue; if (v >= 3 && v <= 20) sensThresh = v; }
  json.get(jsonData, "idle_sensor_interval_ms");
  if (jsonData.success) { int v = jsonData.intValue; if (v >= 5000 && v <= 60000) idleSens = v; }
  json.get(jsonData, "idle_firebase_interval_ms");
  if (jsonData.success) { int v = jsonData.intValue; if (v >= 10000 && v <= 120000) idleFb = v; }

  if (!allOk) return;



  bool floatsChanged = (fabsf(drLpm - cfgDryRunThresholdLpm) > 0.01f) ||
                       (fabsf(flowCal - cfgFlowCalibration) > 0.01f);

  bool sleepChanged = (slpEn != cfgSleepEnabled || slpStart != cfgSleepStartHour ||
                       slpEnd != cfgSleepEndHour || slpEmerg != cfgSleepEmergencyLevel);
  bool advancedChanged = (sensThresh != cfgSensorFailureThreshold ||
                          idleSens != cfgIdleSensorIntervalMs || idleFb != cfgIdleFirebaseIntervalMs);

  bool changed = (te != cfgTankEmptyCm || tf != cfgTankFullCm || ps != cfgPumpStartLevel || po != cfgPumpStopLevel
                  || drSec != cfgDryRunTimeoutSec || maxRun != cfgMaxPumpRuntimeMin || floatsChanged || sleepChanged || advancedChanged);
  if (!changed) return;

  cfgTankEmptyCm = te;
  cfgTankFullCm = tf;
  cfgPumpStartLevel = ps;
  cfgPumpStopLevel = po;
  cfgDryRunThresholdLpm = drLpm;
  cfgDryRunTimeoutSec = drSec;
  cfgFlowCalibration = flowCal;
  cfgMaxPumpRuntimeMin = maxRun;
  cfgSleepEnabled = slpEn;
  cfgSleepStartHour = slpStart;
  cfgSleepEndHour = slpEnd;
  cfgSleepEmergencyLevel = slpEmerg;
  cfgSensorFailureThreshold = sensThresh;
  cfgIdleSensorIntervalMs = idleSens;
  cfgIdleFirebaseIntervalMs = idleFb;
  saveDeviceConfigToNVS();
  Serial.println("[FIREBASE] Device config updated.");
}



void readFirebaseControl() {


  static bool lastManualStart = false;
  static bool lastManualStop = false;
  static int lastTimedStartSec = 0;


  if (Firebase.RTDB.getString(&fbdo, "/pump_system/control/mode")) {
    firebaseConsecutiveFailCount = 0;
    String newMode = fbdo.stringData();
    newMode.trim();
    newMode.toUpperCase();
    if (newMode == "AUTO" || newMode == "FORCE_ON" || newMode == "FORCE_OFF") {


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


    if (err.indexOf("token is not ready") >= 0 || err.indexOf("revoked") >= 0 || err.indexOf("expired") >= 0) {
      unsigned long now = millis();
      firebaseCooldownUntilMs = max(firebaseCooldownUntilMs, now + FIREBASE_AUTH_COOLDOWN_MS);
      Firebase.refreshToken(&config);
      Serial.println("[FIREBASE] Auth not ready/expired; backing off and requesting token refresh.");
    } else if (err.indexOf("payload read timed out") >= 0 || err.indexOf("read timed out") >= 0) {

      unsigned long now = millis();
      firebaseCooldownUntilMs = max(firebaseCooldownUntilMs, now + 120000UL);
      Serial.println("[FIREBASE] Network timeout; cooling down 120s to protect main loop.");
    }
  }



  if (Firebase.RTDB.getBool(&fbdo, "/pump_system/control/manual_stop")) {
    bool v = fbdo.boolData();
    if (v && !lastManualStop) {
      Serial.println("[FIREBASE] Manual stop requested.");

      setPump(false);
      runMode = "OFF";
      runDurationMs = 0;
      runStartMs = 0;
      runRemainingSec = 0;

      pumpMode = "FORCE_OFF";
    }
    lastManualStop = v;
  }


  if (Firebase.RTDB.getInt(&fbdo, "/pump_system/control/timed_start_sec")) {
    int sec = fbdo.intData();
    if (sec > 0 && sec != lastTimedStartSec) {

      int minSec = 30;
      int maxSec = max(60, cfgMaxPumpRuntimeMin * 60);
      int durSec = constrain(sec, minSec, maxSec);


      if (isDryRunError || isOverflowError) {
        Serial.println("[FIREBASE] Timed run rejected: error lockout active.");
      } else {
        runPrevPumpMode = pumpMode;
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


  if (Firebase.RTDB.getBool(&fbdo, "/pump_system/control/manual_start")) {
    bool v = fbdo.boolData();
    if (v && !lastManualStart) {

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


  if (Firebase.RTDB.getBool(&fbdo, "/pump_system/control/clear_error")) {
    if (fbdo.boolData() == true) {
      bool hadError = isDryRunError || isOverflowError;
      if (hadError) {
        isDryRunError = false;
        isOverflowError = false;
        dryRunTimerActive = false;
        dryRunStartMs = 0;
        pumpAutoStartTracking = false;
        pumpAutoStartMs = 0;
        Serial.println("[FIREBASE] Errors cleared.");
        lastFaultCode = "";
        lastFaultMessage = "";
        Firebase.RTDB.setBool(&fbdo, "/pump_system/control/clear_error", false);
      }
    }
  }


  if (Firebase.RTDB.getInt(&fbdo, "/pump_system/control/reboot_request_id")) {
    int requestedId = fbdo.intData();
    if (requestedId > 0 && requestedId != lastRebootRequestId) {
      Serial.printf("[FIREBASE] Reboot requested (id=%d).\n", requestedId);
      lastRebootRequestId = requestedId;

      Firebase.RTDB.setInt(&fbdo, "/pump_system/status/last_reboot_request_id", lastRebootRequestId);
      Firebase.RTDB.setInt(&fbdo, "/pump_system/status/last_reboot_at", (int)(esp_timer_get_time() / 1000000ULL));
      delay(100);
      ESP.restart();
    }
  }
}



void pushFirebaseStatus() {
  FirebaseJson statusJson;

  uint32_t uptimeMinutes = (uint32_t)(esp_timer_get_time() / 60000000ULL);

  statusJson.set("water_level_percent", waterLevelPct);
  statusJson.set("is_running", isRunning);
  statusJson.set("flow_rate_lpm", flowRateLpm);
  statusJson.set("is_error", isDryRunError);
  statusJson.set("is_sensor_error", isSensorError || isFlowSensorError);
  statusJson.set("is_overflow_error", isOverflowError);
  statusJson.set("is_sleeping", isSleeping);
  statusJson.set("wifi_rssi", wifiRssi);
  statusJson.set("last_boot_reason", bootReasonStr);
  statusJson.set("uptime_minutes", uptimeMinutes);

  statusJson.set("free_heap_bytes", (int)ESP.getFreeHeap());
#if defined(ESP32)
  statusJson.set("min_free_heap_bytes", (int)ESP.getMinFreeHeap());
  statusJson.set("max_alloc_heap_bytes",(int)ESP.getMaxAllocHeap());
#endif
  statusJson.set("min_free_heap_observed_bytes", (int)minFreeHeapObserved);
  statusJson.set("firebase_consecutive_failures", (int)firebaseConsecutiveFailCount);
  statusJson.set("firebase_last_error", firebaseLastError);

  statusJson.set("ultrasonic_cycles_ok", (int)ultrasonicCycleOkCount);
  statusJson.set("ultrasonic_cycles_timeout", (int)ultrasonicCycleTimeoutCount);
  statusJson.set("ultrasonic_last_good_cm", (float)ultrasonicLastGoodCmX10 / 10.0f);
  statusJson.set("flow_discard_max_sane", (int)flowDiscardMaxSaneCount);
  statusJson.set("flow_stuck_high_events", (int)flowStuckHighEventCount);


  statusJson.set("run_mode", runMode);
  statusJson.set("run_remaining_sec", (int)runRemainingSec);
  if (lastFaultCode.length() > 0) statusJson.set("last_fault_code", lastFaultCode);
  if (lastFaultMessage.length() > 0) statusJson.set("last_fault_message", lastFaultMessage);

  if (Firebase.RTDB.setJSON(&fbdo, "/pump_system/status", &statusJson)) {
    lastSuccessfulFirebaseMs = millis();
    firebaseConsecutiveFailCount = 0;
    Serial.printf("[FIREBASE] Status -> Level:%d%% | Flow:%.2f | Run:%s | Err:%s | RSSI:%d | Uptime:%um\n",
                  waterLevelPct, flowRateLpm,
                  isRunning ? "Y" : "N",
                  isDryRunError ? "Y" : "N",
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
      firebaseCooldownUntilMs = max(firebaseCooldownUntilMs, now + 120000UL);
      Serial.println("[FIREBASE] Network timeout; cooling down 120s to protect main loop.");
    }
  }
}



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



void initFirebase() {
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;


  auth.user.email = FIREBASE_EMAIL;
  auth.user.password = FIREBASE_PASSWORD;


  config.token_status_callback = tokenStatusCallback;


  fbdo.setBSSLBufferSize(4096, 1024);

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);




  Firebase.RTDB.setReadTimeout(&fbdo, 10000);
  Firebase.RTDB.setwriteSizeLimit(&fbdo, "medium");
  fbdo.setResponseSize(1024);

  Serial.println("[FIREBASE] Initialized. Waiting for token...");
}



String getBootReasonString() {
  esp_reset_reason_t reason = esp_reset_reason();
  switch (reason) {
    case ESP_RST_POWERON: return "Power-on";
    case ESP_RST_EXT: return "External reset";
    case ESP_RST_SW: return "Software reset";
    case ESP_RST_PANIC: return "Exception/panic";
    case ESP_RST_INT_WDT: return "Interrupt watchdog";
    case ESP_RST_TASK_WDT: return "Task watchdog";
    case ESP_RST_WDT: return "Other watchdog";
    case ESP_RST_DEEPSLEEP:return "Deep sleep wake";
    case ESP_RST_BROWNOUT: return "Brownout";
    case ESP_RST_SDIO: return "SDIO reset";
    default: return "Unknown";
  }
}