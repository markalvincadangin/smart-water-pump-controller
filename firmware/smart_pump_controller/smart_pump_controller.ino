// =============================================================================
// Smart Water Pump Controller - Firmware
// Platform  : ESP32 DevKit V1 (38-pin), Arduino Framework
// Author    : Mark Alvin Cadangin
// Version   : 1.0.0
//
// PIN MAPPING (from Hardware Documentation):
//   GPIO 4  -> 5V Relay Module IN  (Output)
//   GPIO 5  -> JSN-SR04T TRIG      (Output)
//   GPIO 18 -> JSN-SR04T ECHO      (Input, via 1kΩ/2kΩ voltage divider)
//   GPIO 34 -> YF-G1 Flow Sensor   (Input, via 1kΩ/2kΩ voltage divider)
//
// SYSTEM STATES (controlled via Firebase /pump_system/control/mode):
//   AUTO      : Hysteresis automation (start ≤30%, stop ≥100%)
//   FORCE_ON  : Manual software override - pump ON
//   FORCE_OFF : Manual software shutdown - pump OFF
// =============================================================================

// --- Core Libraries ---
#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>   // NVS for persisting device config across reboot
#include <math.h>          // fabs() for float epsilon comparison

// --- Firebase Libraries ---
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

// =============================================================================
// SECTION 1: CONFIGURATION — Credentials in secrets.h (gitignored)
// =============================================================================
// Copy secrets.h.example to secrets.h and fill in your WiFi + Firebase credentials.
// Never commit secrets.h.
#include "secrets.h"

// =============================================================================
// SECTION 2: PIN DEFINITIONS (matches Hardware Documentation exactly)
// =============================================================================

#define RELAY_PIN        4    // Output -> 5V Relay Module -> Contactor A1
#define TRIG_PIN         5    // Output -> JSN-SR04T Ultrasonic Trigger
#define ECHO_PIN        18    // Input  -> JSN-SR04T Ultrasonic Echo (via divider)
#define FLOW_SENSOR_PIN 34    // Input  -> YF-G1 Flow Sensor Signal (via divider)

// =============================================================================
// SECTION 3: TANK & SENSOR CALIBRATION
// =============================================================================
//
// Tank: Bestank WT660 (660L vertical PE tank)
//   - Outside diameter: 91 cm | Total height: 134 cm
//   - Sensor: JSN-SR04T mounted on lid, facing down into tank
//
// Distance = sensor face to water surface. Shorter distance = more full.
// Values below calculated from WT660 specs. Fine-tune with a dipstick:
//   1. Empty tank: measure distance sensor→bottom (→ TANK_EMPTY_CM)
//   2. Full tank:  measure distance sensor→water  (→ TANK_FULL_CM)
//
#define TANK_EMPTY_CM   122   // Distance (cm) when tank is 0% full (sensor to bottom)
#define TANK_FULL_CM     8    // Distance (cm) when tank is 100% full (sensor to water)

// AUTO mode hysteresis thresholds (percent)
#define PUMP_START_LEVEL  30  // Start pump when water drops to or below 30%
#define PUMP_STOP_LEVEL  100  // Stop pump when water reaches 100%

// =============================================================================
// SECTION 4: SAFETY & TIMING CONSTANTS
// =============================================================================

// Dry-Run Protection (Software Safety - Level 2)
#define DRY_RUN_THRESHOLD_LPM  0.5f   // Flow below this = dry-run condition
#define DRY_RUN_TIMEOUT_MS     30000  // 30 seconds before lockout triggers

// YF-G1 Flow Sensor: F (Hz) = Q (L/min), i.e. pulses/second = L/min
#define FLOW_CALIBRATION_FACTOR  1.0f

// Timing intervals (kept in firmware only)
#define SENSOR_INTERVAL_MS       1000   // Sample sensors every 1 second
#define FIREBASE_INTERVAL_MS     3000   // Push to Firebase every 3 seconds
#define DEVICE_CONFIG_INTERVAL_MS 30000 // Read device config from Firebase every 30 seconds (reduces 7 RTDB reads)
#define ULTRASONIC_TIMEOUT_MS   30     // Max wait for echo pulse (ms)

// NVS namespace for device config
#define NVS_NAMESPACE  "pump_cfg"

// =============================================================================
// SECTION 5: FIREBASE OBJECTS
// =============================================================================

FirebaseData   fbdo;
FirebaseAuth   auth;
FirebaseConfig config;
Preferences    prefs;   // NVS

// =============================================================================
// SECTION 6: GLOBAL STATE VARIABLES
// =============================================================================

// --- Device config (runtime; loaded from NVS on boot, then from Firebase when online) ---
int    cfgTankEmptyCm         = TANK_EMPTY_CM;
int    cfgTankFullCm          = TANK_FULL_CM;
int    cfgPumpStartLevel      = PUMP_START_LEVEL;
int    cfgPumpStopLevel       = PUMP_STOP_LEVEL;
float  cfgDryRunThresholdLpm  = DRY_RUN_THRESHOLD_LPM;
int    cfgDryRunTimeoutSec    = (int)(DRY_RUN_TIMEOUT_MS / 1000UL);
float  cfgFlowCalibration     = FLOW_CALIBRATION_FACTOR;

// --- Sensor Data ---
volatile uint32_t pulseCount      = 0;   // ISR-incremented pulse counter
float             flowRateLpm     = 0.0f;
int               waterLevelPct   = 0;
bool              isRunning       = false;

// --- System State ---
String            pumpMode        = "AUTO"; // AUTO | FORCE_ON | FORCE_OFF
bool              isDryRunError   = false;

// --- Dry-Run Timer ---
unsigned long     dryRunStartMs   = 0;
bool              dryRunTimerActive = false;

// --- Timing ---
unsigned long     lastSensorMs     = 0;
unsigned long     lastFirebaseMs   = 0;
unsigned long     lastDeviceConfigMs = 0;  // When we last read config from Firebase
unsigned long     lastWifiRetryMs    = 0;

// =============================================================================
// SECTION 7: INTERRUPT SERVICE ROUTINE (ISR)
// YF-G1 flow sensor generates pulses. The ISR counts them in real-time
// to ensure zero pulses are missed (from Software Documentation).
// =============================================================================

void IRAM_ATTR flowPulseISR() {
  pulseCount++;
}

// =============================================================================
// SECTION 8: HARDWARE CONTROL
// =============================================================================

/**
 * @brief Energize or de-energize the relay.
 *        Relay IN = LOW  -> Relay NO closes -> Contactor coil energized -> Pump ON
 *        Relay IN = HIGH -> Relay NO opens  -> Contactor coil released  -> Pump OFF
 *        (Standard opto-isolated relay module: active LOW)
 */
void setPump(bool on) {
  digitalWrite(RELAY_PIN, on ? LOW : HIGH);
  isRunning = on;
}

// =============================================================================
// SECTION 9: ULTRASONIC LEVEL SENSOR (readUltrasonicSensor)
// =============================================================================

/**
 * @brief Reads the JSN-SR04T and returns water level as a percentage (0-100).
 *        Sends a 10µs TRIG pulse and measures ECHO pulse width.
 *        Distance (cm) = pulse duration (µs) / 58.
 *        Uses map() to normalize to 0-100% based on calibration constants.
 * @return int Water level percentage, or -1 on sensor timeout.
 */
int readUltrasonicSensor() {
  // Send trigger pulse
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Measure echo pulse duration
  long duration = pulseIn(ECHO_PIN, HIGH, ULTRASONIC_TIMEOUT_MS * 1000UL);

  if (duration == 0) {
    Serial.println("[WARN] Ultrasonic: No echo received (timeout).");
    return -1;
  }

  float distanceCm = duration / 58.0f;

  // Clamp distance to calibrated range before mapping
  distanceCm = constrain(distanceCm, (float)cfgTankFullCm, (float)cfgTankEmptyCm);

  // Map distance to percentage:
  // At cfgTankFullCm  -> 100%
  // At cfgTankEmptyCm -> 0%
  int levelPct = map(
    (long)distanceCm,
    (long)cfgTankFullCm,
    (long)cfgTankEmptyCm,
    100,
    0
  );

  return constrain(levelPct, 0, 100);
}

// =============================================================================
// SECTION 10: FLOW RATE CALCULATION (calculateFlowRate)
// =============================================================================

/**
 * @brief Calculates flow rate from ISR pulse count over a 1-second window.
 *        Atomically reads and resets pulseCount using noInterrupts().
 *        Q (L/min) ≈ Pulses / FLOW_CALIBRATION_FACTOR (from Software Documentation).
 * @return float Flow rate in Litres Per Minute.
 */
float calculateFlowRate() {
  // Atomically read and reset the ISR pulse counter
  noInterrupts();
  uint32_t count = pulseCount;
  pulseCount = 0;
  interrupts();

  // Convert pulses/second to L/min
  float lpm = (float)count / cfgFlowCalibration;
  return lpm;
}

// =============================================================================
// SECTION 11: DRY-RUN PROTECTION (checkSafetyCutoff)
// Software-level failsafe - Level 2 in the Safety Hierarchy.
// If pump is running but flow < 0.5 LPM for >30s, trigger lockout.
// =============================================================================

/**
 * @brief Monitors flow rate while pump is active.
 *        Triggers isDryRunError = true and kills relay after DRY_RUN_TIMEOUT_MS
 *        of sustained low-flow. Reset only via Firebase clear_error signal.
 */
void checkSafetyCutoff() {
  if (!isRunning) {
    // Pump is off — reset the dry-run timer
    dryRunTimerActive = false;
    dryRunStartMs = 0;
    return;
  }

  unsigned long dryRunTimeoutMs = (unsigned long)cfgDryRunTimeoutSec * 1000UL;
  if (flowRateLpm < cfgDryRunThresholdLpm) {
    // Flow is too low while pump is supposed to be running
    if (!dryRunTimerActive) {
      dryRunTimerActive = true;
      dryRunStartMs = millis();
      Serial.println("[WARN] Dry-run condition detected. Timer started.");
    } else {
      unsigned long elapsed = millis() - dryRunStartMs;
      if (elapsed >= dryRunTimeoutMs) {
        // LOCKOUT TRIGGERED
        isDryRunError = true;
        setPump(false);
        Serial.println("[ERROR] DRY-RUN LOCKOUT. Pump killed. Awaiting acknowledge.");
      }
    }
  } else {
    // Flow is healthy — reset timer
    if (dryRunTimerActive) {
      Serial.println("[INFO] Flow restored. Dry-run timer reset.");
    }
    dryRunTimerActive = false;
    dryRunStartMs = 0;
  }
}

// =============================================================================
// SECTION 12: PUMP STATE MACHINE (executePumpLogic)
// Three mutually exclusive states controlled via Firebase.
// =============================================================================

/**
 * @brief Executes the pump control state machine based on current mode.
 *        Does nothing if isDryRunError is active (lockout state).
 *
 * AUTO:      Hysteresis control based on tank water level.
 * FORCE_ON:  Override - turns pump ON regardless of level.
 * FORCE_OFF: Override - turns pump OFF regardless of level.
 */
void executePumpLogic() {
  // Dry-run lockout overrides everything
  if (isDryRunError) {
    setPump(false);
    return;
  }

  if (pumpMode == "FORCE_ON") {
    setPump(true);

  } else if (pumpMode == "FORCE_OFF") {
    setPump(false);

  } else {
    // AUTO mode — hysteresis control
    if (!isRunning && waterLevelPct <= cfgPumpStartLevel) {
      Serial.printf("[AUTO] Water at %d%%. Starting pump.\n", waterLevelPct);
      setPump(true);
    } else if (isRunning && waterLevelPct >= cfgPumpStopLevel) {
      Serial.printf("[AUTO] Water at %d%%. Stopping pump.\n", waterLevelPct);
      setPump(false);
    }
    // If between thresholds, maintain current state (hysteresis)
  }
}

// =============================================================================
// SECTION 12b: DEVICE CONFIG — NVS (persist across reboot when offline)
// =============================================================================

void loadDeviceConfigFromNVS() {
  if (!prefs.begin(NVS_NAMESPACE, true)) {  // read-only
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
  prefs.end();
  // Validate: if NVS was corrupted, keep firmware defaults
  if (te < 5 || te > 200 || tf < 1 || tf >= te || ps < 0 || ps > 100 || po < 0 || po > 100 || po <= ps
      || drLpm < 0.1f || drLpm > 10.0f || drSec < 10 || drSec > 300 || flowCal < 0.1f || flowCal > 10.0f) {
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
  Serial.println("[NVS] Device config loaded.");
}

void saveDeviceConfigToNVS() {
  if (!prefs.begin(NVS_NAMESPACE, false)) {  // read-write
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
  prefs.end();
  Serial.println("[NVS] Device config saved.");
}

// =============================================================================
// SECTION 12c: FIREBASE — READ DEVICE CONFIG (single getJSON = 1 round trip)
// Reads /pump_system/config/device. Validates, applies, saves to NVS only if changed.
// Called every DEVICE_CONFIG_INTERVAL_MS. If your library lacks to<FirebaseJson>(),
// use: FirebaseJson* json = fbdo.jsonObject(); if (!json) return; json->get(jsonData, "key");
// =============================================================================

void readDeviceConfigFromFirebase() {
  if (!Firebase.RTDB.getJSON(&fbdo, "/pump_system/config/device")) return;

  FirebaseJson json = fbdo.to<FirebaseJson>();
  FirebaseJsonData jsonData;

  int te = 0, tf = 0, ps = 0, po = 0, drSec = 0;
  float drLpm = 0.0f, flowCal = 0.0f;
  bool allOk = true;

  json.get(jsonData, "tank_empty_cm");
  if (jsonData.success) { int v = jsonData.intValue; if (v >= 5 && v <= 200) te = v; else allOk = false; } else allOk = false;

  json.get(jsonData, "tank_full_cm");
  if (allOk && jsonData.success) { int v = jsonData.intValue; if (v >= 1 && v < te) tf = v; else allOk = false; } else allOk = false;

  json.get(jsonData, "pump_start_level");
  if (allOk && jsonData.success) { int v = jsonData.intValue; if (v >= 0 && v <= 100) ps = v; else allOk = false; } else allOk = false;

  json.get(jsonData, "pump_stop_level");
  if (allOk && jsonData.success) { int v = jsonData.intValue; if (v >= 0 && v <= 100 && v > ps) po = v; else allOk = false; } else allOk = false;

  json.get(jsonData, "dry_run_threshold_lpm");
  if (allOk && jsonData.success) { 
    float v = (jsonData.typeNum == FirebaseJson::JSON_INT) ? (float)jsonData.intValue : (float)jsonData.doubleValue; 
    if (v >= 0.1f && v <= 10.0f) drLpm = v; else allOk = false; 
  } else allOk = false;

  json.get(jsonData, "dry_run_timeout_sec");
  if (allOk && jsonData.success) { int v = jsonData.intValue; if (v >= 10 && v <= 300) drSec = v; else allOk = false; } else allOk = false;

  json.get(jsonData, "flow_calibration_factor");
  if (allOk && jsonData.success) { 
    float v = (jsonData.typeNum == FirebaseJson::JSON_INT) ? (float)jsonData.intValue : (float)jsonData.doubleValue; 
    if (v >= 0.1f && v <= 10.0f) flowCal = v; else allOk = false; 
  } else allOk = false;

  if (!allOk) return;  // Path missing or invalid — keep current config (no log spam)

  // Apply only if something changed (reduces NVS wear)
  // Use a small epsilon (0.01) for float comparisons to avoid false positives from rounding
  bool floatsChanged = (fabsf(drLpm - cfgDryRunThresholdLpm) > 0.01f) ||
                       (fabsf(flowCal - cfgFlowCalibration) > 0.01f);

  bool changed = (te != cfgTankEmptyCm || tf != cfgTankFullCm || ps != cfgPumpStartLevel || po != cfgPumpStopLevel
                  || drSec != cfgDryRunTimeoutSec || floatsChanged);
  if (!changed) return;

  cfgTankEmptyCm        = te;
  cfgTankFullCm         = tf;
  cfgPumpStartLevel     = ps;
  cfgPumpStopLevel      = po;
  cfgDryRunThresholdLpm = drLpm;
  cfgDryRunTimeoutSec   = drSec;
  cfgFlowCalibration    = flowCal;
  saveDeviceConfigToNVS();
  Serial.println("[FIREBASE] Device config updated from database.");
}

// =============================================================================
// SECTION 13: FIREBASE — READ CONTROL NODE
// Reads /pump_system/control/ for mode and clear_error commands.
// =============================================================================

void readFirebaseControl() {
  // Read pump mode
  if (Firebase.RTDB.getString(&fbdo, "/pump_system/control/mode")) {
    String newMode = fbdo.stringData();
    newMode.trim();
    newMode.toUpperCase();
    if (newMode == "AUTO" || newMode == "FORCE_ON" || newMode == "FORCE_OFF") {
      if (pumpMode != newMode) {
        Serial.printf("[FIREBASE] Mode changed: %s -> %s\n",
                      pumpMode.c_str(), newMode.c_str());
      }
      pumpMode = newMode;
    }
  } else {
    Serial.printf("[FIREBASE] Read mode failed: %s\n",
                  fbdo.errorReason().c_str());
  }

  // Read clear_error flag
  if (Firebase.RTDB.getBool(&fbdo, "/pump_system/control/clear_error")) {
    if (fbdo.boolData() == true) {
      if (isDryRunError) {
        isDryRunError     = false;
        dryRunTimerActive = false;
        dryRunStartMs     = 0;
        Serial.println("[FIREBASE] Dry-run error acknowledged and cleared.");
        // Reset the flag in Firebase so it doesn't keep re-clearing
        Firebase.RTDB.setBool(&fbdo, "/pump_system/control/clear_error", false);
      }
    }
  }
}

// =============================================================================
// SECTION 14: FIREBASE — PUSH STATUS NODE
// Pushes sensor data and system state to /pump_system/status/.
// Firebase Data Structure (from Software Documentation):
//   water_level_percent : int
//   is_running          : bool
//   flow_rate_lpm       : float
//   is_error            : bool
// =============================================================================

void pushFirebaseStatus() {
  FirebaseJson statusJson;
  statusJson.set("water_level_percent", waterLevelPct);
  statusJson.set("is_running",          isRunning);
  statusJson.set("flow_rate_lpm",       flowRateLpm);
  statusJson.set("is_error",            isDryRunError);

  if (Firebase.RTDB.setJSON(&fbdo, "/pump_system/status", &statusJson)) {
    Serial.printf("[FIREBASE] Status pushed -> Level:%d%% | Flow:%.2f LPM | Running:%s | Error:%s\n",
                  waterLevelPct,
                  flowRateLpm,
                  isRunning ? "YES" : "NO",
                  isDryRunError ? "YES" : "NO");
  } else {
    Serial.printf("[FIREBASE] Push failed: %s\n", fbdo.errorReason().c_str());
  }
}

// =============================================================================
// SECTION 15: WiFi CONNECTION
// =============================================================================

void connectWiFi() {
  Serial.printf("\n[WIFI] Connecting to: %s", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[WIFI] Connected! IP: %s\n",
                  WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\n[WIFI] Connection failed. Operating in offline mode.");
  }
}

// =============================================================================
// SECTION 16: FIREBASE INITIALIZATION
// Uses Email/Password authentication. Create user in Firebase Console →
// Authentication → Users → Add user.
// =============================================================================

void initFirebase() {
  config.api_key      = API_KEY;
  config.database_url = DATABASE_URL;

  // Email/Password credentials (from secrets.h)
  auth.user.email    = FIREBASE_EMAIL;
  auth.user.password = FIREBASE_PASSWORD;

  // Token status callback for debugging
  config.token_status_callback = tokenStatusCallback;

  // SSL buffer (recommended for Firebase-ESP-Client v4.4.x)
  fbdo.setBSSLBufferSize(4096, 1024);

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);  // Auto-reconnect if router reboots

  Serial.println("[FIREBASE] Initialized (Email/Password). Waiting for token...");
}

// =============================================================================
// SECTION 17: SETUP
// =============================================================================

void setup() {
  Serial.begin(115200);
  Serial.println("\n====================================");
  Serial.println(" Smart Water Pump Controller v1.0");
  Serial.println("====================================");

  // --- GPIO Setup ---
  pinMode(RELAY_PIN,        OUTPUT);
  pinMode(TRIG_PIN,         OUTPUT);
  pinMode(ECHO_PIN,         INPUT);
  // GPIO 34 is input-only on ESP32, no pinMode needed for INPUT
  // but we set it explicitly for clarity
  pinMode(FLOW_SENSOR_PIN,  INPUT);

  // Safety: ensure pump is OFF on boot
  setPump(false);
  digitalWrite(TRIG_PIN, LOW);

  Serial.println("[INIT] GPIO configured. Pump OFF.");

  // --- Attach Flow Sensor Interrupt ---
  // Rising edge = one pulse from YF-G1 hall-effect sensor
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN),
                  flowPulseISR,
                  RISING);
  Serial.println("[INIT] Flow sensor interrupt attached on GPIO 34.");

  // --- WiFi ---
  connectWiFi();

  // --- Load device config from NVS (last known or defaults from firmware) ---
  loadDeviceConfigFromNVS();

  // --- Firebase ---
  if (WiFi.status() == WL_CONNECTED) {
    initFirebase();
  }

  // --- Initialize Timing ---
  unsigned long nowInit = millis();
  lastSensorMs        = nowInit;
  lastFirebaseMs      = nowInit;
  lastDeviceConfigMs  = 0;   // Read config on first Firebase-ready cycle
  lastWifiRetryMs     = 0;    // WiFi reconnect throttle (15s when disconnected)

  Serial.println("[INIT] Boot complete. Entering main loop.\n");
}

// =============================================================================
// SECTION 18: MAIN LOOP
// Non-blocking design using millis() timers.
// Sensors: sampled every 1s
// Firebase: synced every 3s
// =============================================================================

void loop() {
  unsigned long now = millis();

  // --- WIFI RECOVERY (non-blocking; 15s throttle to avoid reconnect spam) ---
  if (WiFi.status() != WL_CONNECTED) {
    if (now - lastWifiRetryMs >= 15000) {
      lastWifiRetryMs = now;
      Serial.println("[WIFI] Connection lost. Attempting reconnect...");
      WiFi.disconnect();
      WiFi.reconnect();
    }
  } else {
    lastWifiRetryMs = 0;  // Reset so first disconnect triggers retry after 15s
  }

  // --- SENSOR SAMPLING (every 1 second) ---
  if (now - lastSensorMs >= SENSOR_INTERVAL_MS) {
    lastSensorMs = now;

    // 1. Read ultrasonic water level
    int reading = readUltrasonicSensor();
    if (reading >= 0) {
      waterLevelPct = reading;
    }
    // If reading == -1 (timeout), keep last known value

    // 2. Calculate flow rate from last 1-second pulse window
    flowRateLpm = calculateFlowRate();

    Serial.printf("[SENSOR] Level: %d%% | Flow: %.2f LPM\n",
                  waterLevelPct, flowRateLpm);

    // 3. Run dry-run safety check
    checkSafetyCutoff();

    // 4. Execute pump state machine
    executePumpLogic();
  }

  // --- FIREBASE SYNC (every 3 seconds) ---
  if (now - lastFirebaseMs >= FIREBASE_INTERVAL_MS) {
    lastFirebaseMs = now;

    if (Firebase.ready()) {
      // Device config: read every 30s to reduce RTDB load (7 reads per run); first time (lastDeviceConfigMs==0) run immediately
      if (lastDeviceConfigMs == 0 || (now - lastDeviceConfigMs >= DEVICE_CONFIG_INTERVAL_MS)) {
        lastDeviceConfigMs = now;
        readDeviceConfigFromFirebase();
      }
      readFirebaseControl();
      pushFirebaseStatus();
    } else {
      Serial.println("[FIREBASE] Not ready. Skipping sync.");
    }
  }

  // FreeRTOS: delay(1) yields to scheduler and feeds watchdog (more reliable than yield() on ESP32)
  delay(1);
}
