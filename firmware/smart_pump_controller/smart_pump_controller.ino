// =============================================================================
// Smart Water Pump Controller - Firmware
// Platform  : ESP32 DevKit V1 (38-pin), Arduino Framework
// Author    : Mark Alvin Cadangin
// Version   : 2.4.0  (Phase 4 — Firebase Config Expansion)
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
#include <esp_task_wdt.h>  // Phase 2: Hardware watchdog timer
#include <esp_system.h>    // Phase 2: esp_reset_reason() for boot logging
#include <esp_sleep.h>     // Phase 3: Light Sleep
#include <time.h>          // Phase 3: NTP time sync

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

// YF-G1 Flow Sensor: Q (L/min) = F (Hz) / 7.5
// Datasheet: F = 7.5 * Q.  Variants may differ (some cite 4.8).
// Verify with bucket + stopwatch test. Tunable via Firebase flow_calibration_factor.
#define FLOW_CALIBRATION_FACTOR  7.5f

// Overflow Protection — max pump runtime in AUTO mode (minutes)
#define MAX_PUMP_RUNTIME_MIN     120   // 2 hours; configurable via Firebase

// Sensor Failure Detection
#define SENSOR_FAILURE_THRESHOLD  5    // Consecutive ultrasonic timeouts before error
#define FLOW_STUCK_THRESHOLD_LPM  2.0f // Flow > this when pump OFF = stuck sensor
#define FLOW_STUCK_TIMEOUT_MS     5000 // 5 seconds of stuck-high flow before flagging
#define FLOW_MAX_SANE_LPM         100.0f // Readings above this are discarded
#define LEVEL_RATE_OF_CHANGE_MAX  30   // Max % change per second before holding previous

// Ultrasonic median filter
#define ULTRASONIC_SAMPLES        5    // Number of readings for median filter
#define ULTRASONIC_SAMPLE_DELAY   60   // ms between samples (JSN-SR04T settling time)
#define ULTRASONIC_EMA_ALPHA      0.3f // Exponential moving average smoothing factor

// Timing intervals (kept in firmware only)
#define SENSOR_INTERVAL_MS       1000   // Sample sensors every 1 second
#define FIREBASE_INTERVAL_MS     3000   // Push to Firebase every 3 seconds
#define DEVICE_CONFIG_INTERVAL_MS 30000 // Read device config from Firebase every 30 seconds (reduces 7 RTDB reads)
#define ULTRASONIC_TIMEOUT_MS   50     // Max wait for echo pulse (ms) — increased for 40m CAT6 attenuation

// NVS namespaces
#define NVS_NAMESPACE        "pump_cfg"    // Device config
#define NVS_STATE_NAMESPACE  "pump_state"  // Runtime state persistence (Phase 2)

// Crash Loop Detection (Phase 2)
#define CRASH_LOOP_THRESHOLD    5     // Reboots in window = crash loop
#define CRASH_LOOP_WINDOW_SEC   300   // 5-minute window
#define SAFE_MODE_TIMEOUT_MS    3600000UL  // 1 hour auto-clear

// WiFi Exponential Backoff (Phase 2)
#define WIFI_BACKOFF_INITIAL_MS 5000   // First retry delay
#define WIFI_BACKOFF_MAX_MS     60000  // Max retry delay (cap)
#define WIFI_JITTER_MS          2000   // ±2s random jitter

// Hardware Watchdog (Phase 2)
#define WDT_TIMEOUT_SEC         15     // 15-second WDT timeout

// NVS State Persistence (Phase 2) — wear reduction
#define NVS_LEVEL_DELTA_THRESHOLD 5    // Only write level when changed by ≥5%
#define NVS_LEVEL_INTERVAL_MS   300000UL  // Or every 5 minutes (whichever first)

// Scheduled Sleep (Phase 3)
#define SLEEP_DEFAULT_ENABLED       false
#define SLEEP_DEFAULT_START_HOUR    23
#define SLEEP_DEFAULT_END_HOUR      5
#define SLEEP_DEFAULT_EMERGENCY_LVL 5
#define SLEEP_WAKE_INTERVAL_MS      30000UL  // Wake every 30s during sleep for sensor+Firebase

// Condition-Based Idle (Phase 3) — outside sleep hours; Phase 4: configurable via Firebase
#define IDLE_LEVEL_THRESHOLD        90    // Level ≥ 90% and pump OFF
#define IDLE_STABLE_TIME_MS         300000UL  // 5 minutes stable at ≥90%
#define IDLE_SENSOR_INTERVAL_MS_DEF 10000  // Default slow-poll: sensor every 10s
#define IDLE_FIREBASE_INTERVAL_MS_DEF 30000  // Default slow-poll: Firebase every 30s

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
int    cfgMaxPumpRuntimeMin   = MAX_PUMP_RUNTIME_MIN;

// --- Sleep config (Phase 3) ---
bool   cfgSleepEnabled        = SLEEP_DEFAULT_ENABLED;
int    cfgSleepStartHour      = SLEEP_DEFAULT_START_HOUR;
int    cfgSleepEndHour        = SLEEP_DEFAULT_END_HOUR;
int    cfgSleepEmergencyLevel = SLEEP_DEFAULT_EMERGENCY_LVL;

// --- Phase 4: Advanced config (Firebase-configurable) ---
int    cfgSensorFailureThreshold = SENSOR_FAILURE_THRESHOLD;  // 3–20, default 5
int    cfgIdleSensorIntervalMs   = IDLE_SENSOR_INTERVAL_MS_DEF;   // 5000–60000
int    cfgIdleFirebaseIntervalMs = IDLE_FIREBASE_INTERVAL_MS_DEF; // 10000–120000

// --- Sensor Data ---
volatile uint32_t pulseCount      = 0;   // ISR-incremented pulse counter
float             flowRateLpm     = 0.0f;
int               waterLevelPct   = 0;
float             waterLevelEma   = 0.0f; // EMA-smoothed level (float precision)
bool              isRunning       = false;
int               prevWaterLevelPct = 0;  // Previous cycle level for rate-of-change guard

// --- System State ---
String            pumpMode        = "AUTO"; // AUTO | FORCE_ON | FORCE_OFF
bool              isDryRunError   = false;
bool              isSensorError   = false;  // Ultrasonic sensor failure
bool              isFlowSensorError = false; // Flow sensor stuck-high
bool              isOverflowError = false;   // Max runtime exceeded

// --- Sensor Failure Tracking ---
int               sensorFailCount   = 0;    // Consecutive ultrasonic timeouts
unsigned long     flowStuckStartMs  = 0;    // When flow stuck-high was first detected
bool              flowStuckTimerActive = false;

// --- Dry-Run Timer ---
unsigned long     dryRunStartMs   = 0;
bool              dryRunTimerActive = false;

// --- Overflow Protection ---
unsigned long     pumpAutoStartMs  = 0;     // When pump last started in AUTO mode
bool              pumpAutoStartTracking = false;

// --- Phase 2: System Resilience ---
bool              inSafeMode        = false;  // Crash loop safe mode
unsigned long     safeModeEnteredMs = 0;     // When safe mode was entered
String            bootReasonStr     = "";    // Human-readable boot reason

// WiFi Exponential Backoff
unsigned long     wifiBackoffMs     = WIFI_BACKOFF_INITIAL_MS;  // Current backoff delay
bool              wifiWasConnected  = false;  // Track connection state transitions
bool              firebaseNeedsReinit = false; // Reinit Firebase after WiFi reconnect

// Connectivity Telemetry
int               wifiRssi          = 0;      // Current WiFi RSSI (dBm)
unsigned long     lastSuccessfulFirebaseMs = 0; // Last successful Firebase push
unsigned long     lastRssiLogMs     = 0;      // Last RSSI serial log

// NVS State Persistence — wear reduction
String            lastPersistedMode = "AUTO";
bool              lastPersistedDryRun = false;
int               lastPersistedLevel = -1;    // -1 = never written
unsigned long     lastLevelWriteMs   = 0;

// --- Phase 3: Scheduled Sleep & Idle ---
bool              isSleeping         = false;  // Currently in sleep window
bool              ntpSynced          = false;  // NTP time sync successful
bool              isIdleMode         = false;  // Condition-based idle (pump OFF + level ≥90%)
unsigned long     idleStartMs        = 0;      // When idle conditions were first met
unsigned long     lastSleepLogMs     = 0;      // Last sleep mode log

// --- Timing ---
unsigned long     lastSensorMs     = 0;
unsigned long     lastFirebaseMs   = 0;
unsigned long     lastDeviceConfigMs = 0;
unsigned long     lastWifiRetryMs    = 0;

// =============================================================================
// SECTION 7: INTERRUPT SERVICE ROUTINE (ISR)
// YF-G1 flow sensor generates pulses. The ISR counts them in real-time
// to ensure zero pulses are missed (from Software Documentation).
// =============================================================================

void IRAM_ATTR flowPulseISR() {
  pulseCount = pulseCount + 1;  // Avoid deprecated '++' on volatile
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
// Phase 1: 5-sample median filter + EMA smoothing + float percentage.
// =============================================================================

/**
 * @brief Takes a single JSN-SR04T distance reading.
 * @return float Distance in cm, or -1.0f on timeout.
 */
float readSingleUltrasonic() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, ULTRASONIC_TIMEOUT_MS * 1000UL);
  if (duration == 0) return -1.0f;

  float distanceCm = duration / 58.0f;

  // Distance validation: discard physically impossible readings
  if (distanceCm < 2.0f || distanceCm > 200.0f) return -1.0f;

  return distanceCm;
}

/**
 * @brief Reads JSN-SR04T with 5-sample median filter, float percentage,
 *        and EMA smoothing. Includes rate-of-change guard.
 * @return int Water level percentage (0-100), or -1 on total sensor failure.
 */
int readUltrasonicSensor() {
  // Take ULTRASONIC_SAMPLES readings
  float readings[ULTRASONIC_SAMPLES];
  int validCount = 0;

  for (int i = 0; i < ULTRASONIC_SAMPLES; i++) {
    float d = readSingleUltrasonic();
    if (d >= 0.0f) {
      readings[validCount++] = d;
    }
    if (i < ULTRASONIC_SAMPLES - 1) delay(ULTRASONIC_SAMPLE_DELAY);
  }

  // If no valid readings at all, sensor has failed this cycle
  if (validCount == 0) {
    Serial.println("[WARN] Ultrasonic: All 5 readings timed out.");
    return -1;
  }

  // Sort valid readings for median (simple insertion sort)
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

  // Clamp to calibrated range
  medianDist = constrain(medianDist, (float)cfgTankFullCm, (float)cfgTankEmptyCm);

  // Float-based percentage (more precise than integer map())
  float range = (float)(cfgTankEmptyCm - cfgTankFullCm);
  float levelFloat = 100.0f * ((float)cfgTankEmptyCm - medianDist) / range;
  levelFloat = constrain(levelFloat, 0.0f, 100.0f);

  // EMA smoothing
  if (waterLevelEma < 0.1f && waterLevelPct == 0) {
    // First reading after boot — initialize EMA directly
    waterLevelEma = levelFloat;
  } else {
    waterLevelEma = ULTRASONIC_EMA_ALPHA * levelFloat + (1.0f - ULTRASONIC_EMA_ALPHA) * waterLevelEma;
  }

  int newLevel = (int)(waterLevelEma + 0.5f); // Round to nearest int
  newLevel = constrain(newLevel, 0, 100);

  // Rate-of-change guard: if level jumps > LEVEL_RATE_OF_CHANGE_MAX in 1s, hold previous
  int delta = abs(newLevel - prevWaterLevelPct);
  if (prevWaterLevelPct > 0 && delta > LEVEL_RATE_OF_CHANGE_MAX) {
    Serial.printf("[WARN] Level changed %d%% in 1s (prev=%d%%, new=%d%%). Holding previous.\n",
                  delta, prevWaterLevelPct, newLevel);
    return prevWaterLevelPct;  // Return last known good value
  }

  return newLevel;
}

// =============================================================================
// SECTION 10: FLOW RATE CALCULATION (calculateFlowRate)
// =============================================================================

/**
 * @brief Calculates flow rate from ISR pulse count over a 1-second window.
 *        Atomically reads and resets pulseCount using noInterrupts().
 *        YF-G1: Q (L/min) = F (Hz) / 7.5  (datasheet: F = 7.5 * Q).
 *        Variants may differ — verify with bucket test.
 * @return float Flow rate in Litres Per Minute.
 */
float calculateFlowRate() {
  // Atomically read and reset the ISR pulse counter
  noInterrupts();
  uint32_t count = pulseCount;
  pulseCount = 0;
  interrupts();

  // Convert pulses/second to L/min using calibration factor
  float lpm = (float)count / cfgFlowCalibration;

  // Sanity check: discard physically impossible readings
  if (lpm > FLOW_MAX_SANE_LPM) {
    Serial.printf("[WARN] Flow reading %.1f LPM exceeds max sane (%.0f). Discarded.\n",
                  lpm, FLOW_MAX_SANE_LPM);
    return flowRateLpm;  // Keep previous value
  }

  return lpm;
}

// =============================================================================
// SECTION 11: SAFETY CHECKS
// Includes: dry-run protection, sensor failure detection, overflow protection,
// and flow sensor stuck-high detection.
// =============================================================================

/**
 * @brief Checks for ultrasonic sensor failure.
 *        After cfgSensorFailureThreshold consecutive timeouts, flags isSensorError.
 *        Auto-recovers when a valid reading is received. Threshold configurable via Firebase (Phase 4).
 */
void checkSensorFailure(int sensorReading) {
  if (sensorReading == -1) {
    sensorFailCount++;
    if (sensorFailCount >= cfgSensorFailureThreshold && !isSensorError) {
      isSensorError = true;
      Serial.printf("[ERROR] Ultrasonic sensor failure: %d consecutive timeouts. "
                    "Sensor error flagged.\n", sensorFailCount);
    }
  } else {
    if (isSensorError) {
      Serial.println("[INFO] Ultrasonic sensor recovered. Error cleared.");
    }
    sensorFailCount = 0;
    isSensorError = false;
  }
}

/**
 * @brief Detects flow sensor stuck-high condition.
 *        If pump is OFF but flow > FLOW_STUCK_THRESHOLD_LPM for > FLOW_STUCK_TIMEOUT_MS,
 *        flags isFlowSensorError.
 */
void checkFlowSensorStuck() {
  if (!isRunning && flowRateLpm > FLOW_STUCK_THRESHOLD_LPM) {
    if (!flowStuckTimerActive) {
      flowStuckTimerActive = true;
      flowStuckStartMs = millis();
    } else if (millis() - flowStuckStartMs >= FLOW_STUCK_TIMEOUT_MS) {
      if (!isFlowSensorError) {
        isFlowSensorError = true;
        Serial.printf("[ERROR] Flow sensor stuck-high: %.1f LPM while pump OFF for >%ds.\n",
                      flowRateLpm, (int)(FLOW_STUCK_TIMEOUT_MS / 1000));
      }
    }
  } else {
    if (isFlowSensorError && isRunning) {
      // Only clear stuck-high error when pump starts (flow is expected)
    } else if (!isRunning && flowRateLpm <= FLOW_STUCK_THRESHOLD_LPM) {
      if (isFlowSensorError) {
        Serial.println("[INFO] Flow sensor recovered. Stuck-high error cleared.");
      }
      isFlowSensorError = false;
    }
    flowStuckTimerActive = false;
    flowStuckStartMs = 0;
  }
}

/**
 * @brief Checks for pump overflow (max runtime exceeded in AUTO mode).
 *        If pump has been running continuously > cfgMaxPumpRuntimeMin without
 *        reaching the stop level, flag overflow error and stop pump.
 */
void checkOverflowProtection() {
  if (!isRunning || pumpMode != "AUTO") {
    // Not running or not in AUTO — reset tracking
    pumpAutoStartTracking = false;
    pumpAutoStartMs = 0;
    return;
  }

  if (!pumpAutoStartTracking) {
    // Pump just started in AUTO — begin tracking
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
    Serial.printf("[ERROR] Max runtime exceeded (%d min). Pump stopped. "
                  "Possible overflow or sensor failure.\n", cfgMaxPumpRuntimeMin);
  }
}

/**
 * @brief Monitors flow rate while pump is active (dry-run detection).
 *        Triggers isDryRunError = true and kills relay after cfgDryRunTimeoutSec
 *        of sustained low-flow. Reset only via Firebase clear_error signal.
 */
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
      Serial.println("[WARN] Dry-run condition detected. Timer started.");
    } else {
      unsigned long elapsed = millis() - dryRunStartMs;
      if (elapsed >= dryRunTimeoutMs) {
        isDryRunError = true;
        setPump(false);
        Serial.println("[ERROR] DRY-RUN LOCKOUT. Pump killed. Awaiting acknowledge.");
      }
    }
  } else {
    if (dryRunTimerActive) {
      Serial.println("[INFO] Flow restored. Dry-run timer reset.");
    }
    dryRunTimerActive = false;
    dryRunStartMs = 0;
  }
}

/**
 * @brief Master safety check — runs all safety sub-checks.
 */
void checkSafetyCutoff() {
  checkDryRunProtection();
  checkFlowSensorStuck();
  checkOverflowProtection();
}

// =============================================================================
// SECTION 12: PUMP STATE MACHINE (executePumpLogic)
// Three mutually exclusive states controlled via Firebase.
// =============================================================================

/**
 * @brief Executes the pump control state machine based on current mode.
 *        Error lockouts override everything (dry-run, overflow).
 *        Sensor error in AUTO mode → pump OFF (fail-safe).
 *        Phase 3: During sleep window, AUTO is suppressed (pump won't auto-start);
 *        FORCE_ON always works; emergency override handled in loop (bypasses sleep).
 *
 * AUTO:      Hysteresis control based on tank water level.
 * FORCE_ON:  Override - turns pump ON regardless of level.
 * FORCE_OFF: Override - turns pump OFF regardless of level.
 */
void executePumpLogic() {
  // Error lockouts override everything
  if (isDryRunError || isOverflowError) {
    setPump(false);
    return;
  }

  if (pumpMode == "FORCE_ON") {
    // Manual override — operator responsibility; dry-run still active
    setPump(true);

  } else if (pumpMode == "FORCE_OFF") {
    setPump(false);

  } else {
    // AUTO mode

    // Phase 3: During scheduled sleep, suppress AUTO start (prevent pump from auto-starting)
    if (isSleeping) {
      if (isRunning) {
        // Pump is running (from FORCE_ON or emergency) — keep it running until stop level
        if (waterLevelPct >= cfgPumpStopLevel) {
          Serial.printf("[AUTO] Water at %d%%. Stopping pump.\n", waterLevelPct);
          setPump(false);
        }
      }
      // Do NOT auto-start in sleep — return
      return;
    }

    // Sensor error in AUTO → fail-safe: pump OFF (prevent overflow on stale data)
    if (isSensorError) {
      if (isRunning) {
        Serial.println("[AUTO] Sensor error — stopping pump (fail-safe).");
        setPump(false);
      }
      return;
    }

    // Hysteresis control
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
  int maxRuntime = prefs.getInt("max_runtime", MAX_PUMP_RUNTIME_MIN);

  // Phase 3: Sleep config (optional keys — defaults if missing)
  bool slpEn = prefs.getBool("slp_en", SLEEP_DEFAULT_ENABLED);
  int slpStart = prefs.getInt("slp_start", SLEEP_DEFAULT_START_HOUR);
  int slpEnd = prefs.getInt("slp_end", SLEEP_DEFAULT_END_HOUR);
  int slpEmerg = prefs.getInt("slp_emerg", SLEEP_DEFAULT_EMERGENCY_LVL);

  // Phase 4: Advanced config (optional keys)
  int sensThresh = prefs.getInt("sens_thresh", SENSOR_FAILURE_THRESHOLD);
  int idleSens = prefs.getInt("idle_sens_ms", IDLE_SENSOR_INTERVAL_MS_DEF);
  int idleFb = prefs.getInt("idle_fb_ms", IDLE_FIREBASE_INTERVAL_MS_DEF);

  prefs.end();
  // Validate: if NVS was corrupted, keep firmware defaults
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

  // Phase 3: Apply sleep config (validate hours 0–23, emergency 0–100)
  if (slpStart >= 0 && slpStart <= 23) cfgSleepStartHour = slpStart;
  if (slpEnd >= 0 && slpEnd <= 23) cfgSleepEndHour = slpEnd;
  if (slpEmerg >= 0 && slpEmerg <= 100) cfgSleepEmergencyLevel = slpEmerg;
  cfgSleepEnabled = slpEn;

  // Phase 4: Apply advanced config (validate ranges)
  if (sensThresh >= 3 && sensThresh <= 20) cfgSensorFailureThreshold = sensThresh;
  if (idleSens >= 5000 && idleSens <= 60000) cfgIdleSensorIntervalMs = idleSens;
  if (idleFb >= 10000 && idleFb <= 120000) cfgIdleFirebaseIntervalMs = idleFb;

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
  prefs.putInt("max_runtime", cfgMaxPumpRuntimeMin);
  // Phase 3: Sleep config
  prefs.putBool("slp_en", cfgSleepEnabled);
  prefs.putInt("slp_start", cfgSleepStartHour);
  prefs.putInt("slp_end", cfgSleepEndHour);
  prefs.putInt("slp_emerg", cfgSleepEmergencyLevel);
  // Phase 4: Advanced config
  prefs.putInt("sens_thresh", cfgSensorFailureThreshold);
  prefs.putInt("idle_sens_ms", cfgIdleSensorIntervalMs);
  prefs.putInt("idle_fb_ms", cfgIdleFirebaseIntervalMs);
  prefs.end();
  Serial.println("[NVS] Device config saved.");
}

// =============================================================================
// SECTION 12b2: SLEEP WINDOW HELPER (Phase 3)
// Handles overnight windows (e.g. 23–5). Returns true if currentHour is within sleep window.
// =============================================================================

bool isInSleepWindow(int currentHour) {
  if (cfgSleepStartHour <= cfgSleepEndHour) {
    // Same-day window: e.g. 9–17
    return (currentHour >= cfgSleepStartHour && currentHour < cfgSleepEndHour);
  } else {
    // Overnight window: e.g. 23–5
    return (currentHour >= cfgSleepStartHour) || (currentHour < cfgSleepEndHour);
  }
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

  int te = 0, tf = 0, ps = 0, po = 0, drSec = 0, maxRun = 0;
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
    if (v >= 0.1f && v <= 20.0f) flowCal = v; else allOk = false; 
  } else allOk = false;

  // max_pump_runtime_min — optional; if missing, keep current value
  json.get(jsonData, "max_pump_runtime_min");
  if (jsonData.success) {
    int v = jsonData.intValue;
    if (v >= 30 && v <= 480) maxRun = v; else maxRun = cfgMaxPumpRuntimeMin;
  } else {
    maxRun = cfgMaxPumpRuntimeMin;  // Key not in Firebase yet — use current
  }

  // Phase 3: Sleep config (optional keys)
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

  // Phase 4: Advanced config (optional keys — keep current if invalid/missing)
  int sensThresh = cfgSensorFailureThreshold;
  int idleSens = cfgIdleSensorIntervalMs;
  int idleFb = cfgIdleFirebaseIntervalMs;
  json.get(jsonData, "sensor_failure_threshold");
  if (jsonData.success) { int v = jsonData.intValue; if (v >= 3 && v <= 20) sensThresh = v; }
  json.get(jsonData, "idle_sensor_interval_ms");
  if (jsonData.success) { int v = jsonData.intValue; if (v >= 5000 && v <= 60000) idleSens = v; }
  json.get(jsonData, "idle_firebase_interval_ms");
  if (jsonData.success) { int v = jsonData.intValue; if (v >= 10000 && v <= 120000) idleFb = v; }

  if (!allOk) return;  // Path missing or invalid — keep current config (no log spam)

  // Apply only if something changed (reduces NVS wear)
  // Use a small epsilon (0.01) for float comparisons to avoid false positives from rounding
  bool floatsChanged = (fabsf(drLpm - cfgDryRunThresholdLpm) > 0.01f) ||
                       (fabsf(flowCal - cfgFlowCalibration) > 0.01f);

  bool sleepChanged = (slpEn != cfgSleepEnabled || slpStart != cfgSleepStartHour ||
                       slpEnd != cfgSleepEndHour || slpEmerg != cfgSleepEmergencyLevel);
  bool advancedChanged = (sensThresh != cfgSensorFailureThreshold ||
                          idleSens != cfgIdleSensorIntervalMs || idleFb != cfgIdleFirebaseIntervalMs);

  bool changed = (te != cfgTankEmptyCm || tf != cfgTankFullCm || ps != cfgPumpStartLevel || po != cfgPumpStopLevel
                  || drSec != cfgDryRunTimeoutSec || maxRun != cfgMaxPumpRuntimeMin || floatsChanged || sleepChanged || advancedChanged);
  if (!changed) return;

  cfgTankEmptyCm        = te;
  cfgTankFullCm         = tf;
  cfgPumpStartLevel     = ps;
  cfgPumpStopLevel      = po;
  cfgDryRunThresholdLpm = drLpm;
  cfgDryRunTimeoutSec   = drSec;
  cfgFlowCalibration    = flowCal;
  cfgMaxPumpRuntimeMin  = maxRun;
  cfgSleepEnabled        = slpEn;
  cfgSleepStartHour      = slpStart;
  cfgSleepEndHour        = slpEnd;
  cfgSleepEmergencyLevel = slpEmerg;
  cfgSensorFailureThreshold = sensThresh;
  cfgIdleSensorIntervalMs   = idleSens;
  cfgIdleFirebaseIntervalMs = idleFb;
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

  // Read clear_error flag — clears ALL error types
  if (Firebase.RTDB.getBool(&fbdo, "/pump_system/control/clear_error")) {
    if (fbdo.boolData() == true) {
      bool hadError = isDryRunError || isOverflowError;
      if (hadError) {
        isDryRunError     = false;
        isOverflowError   = false;
        dryRunTimerActive = false;
        dryRunStartMs     = 0;
        pumpAutoStartTracking = false;
        pumpAutoStartMs   = 0;
        Serial.println("[FIREBASE] All errors acknowledged and cleared.");
        Firebase.RTDB.setBool(&fbdo, "/pump_system/control/clear_error", false);
      }
    }
  }
}

// =============================================================================
// SECTION 14: FIREBASE — PUSH STATUS NODE
// Pushes sensor data and system state to /pump_system/status/.
// Firebase Data Structure:
//   water_level_percent : int
//   is_running          : bool
//   flow_rate_lpm       : float
//   is_error            : bool     (dry-run lockout)
//   is_sensor_error     : bool     (Phase 1: ultrasonic failure)
//   is_overflow_error   : bool     (Phase 1: max runtime exceeded)
// =============================================================================

void pushFirebaseStatus() {
  FirebaseJson statusJson;
  // Phase 5: Uptime counter (using esp_timer to avoid 49-day millis() rollover)
  uint32_t uptimeMinutes = (uint32_t)(esp_timer_get_time() / 60000000ULL);

  statusJson.set("water_level_percent", waterLevelPct);
  statusJson.set("is_running",          isRunning);
  statusJson.set("flow_rate_lpm",       flowRateLpm);
  statusJson.set("is_error",            isDryRunError);
  statusJson.set("is_sensor_error",     isSensorError || isFlowSensorError);
  statusJson.set("is_overflow_error",   isOverflowError);
  statusJson.set("is_sleeping",         isSleeping);   // Phase 3: scheduled sleep active
  statusJson.set("wifi_rssi",           wifiRssi);
  statusJson.set("last_boot_reason",    bootReasonStr);
  statusJson.set("uptime_minutes",      uptimeMinutes);

  if (Firebase.RTDB.setJSON(&fbdo, "/pump_system/status", &statusJson)) {
    lastSuccessfulFirebaseMs = millis();
    Serial.printf("[FIREBASE] Status -> Level:%d%% | Flow:%.2f | Run:%s | Err:%s | RSSI:%d | Uptime:%um\n",
                  waterLevelPct, flowRateLpm,
                  isRunning       ? "Y" : "N",
                  isDryRunError   ? "Y" : "N",
                  wifiRssi,
                  uptimeMinutes);
  } else {
    Serial.printf("[FIREBASE] Push failed: %s\n", fbdo.errorReason().c_str());
  }
}

// =============================================================================
// SECTION 15: WiFi CONNECTION
// Phase 2: Initial connect only. Runtime reconnect uses exponential backoff in loop().
// =============================================================================

void connectWiFi() {
  Serial.printf("\n[WIFI] Connecting to: %s", WIFI_SSID);
  WiFi.setAutoReconnect(true);  // Phase 2: auto-reconnect at WiFi layer
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
// SECTION 16B: BOOT REASON (Phase 2)
// =============================================================================

String getBootReasonString() {
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

// =============================================================================
// SECTION 16C: CRASH LOOP DETECTION (Phase 2)
// Reads NVS boot counter and timestamp. If >5 reboots in 5 minutes, enters safe mode.
// Safe mode auto-clears after 1 hour.
// =============================================================================

void checkCrashLoop() {
  if (!prefs.begin(NVS_STATE_NAMESPACE, false)) {
    Serial.println("[BOOT] NVS state namespace open failed.");
    return;
  }

  unsigned long now = millis();  // ~0 at boot, but we use relative time
  unsigned long lastBootTime = prefs.getULong("last_boot_ms", 0);
  int bootCount = prefs.getInt("boot_count", 0);
  unsigned long safeModeStart = prefs.getULong("safe_mode_ms", 0);

  // Check if safe mode should be cleared (1 hour timeout)
  if (safeModeStart > 0) {
    // Safe mode was previously entered. If this is a fresh power cycle
    // (millis() near 0), clear safe mode since user likely did a full power cycle
    if (now < 5000) {
      // Fresh power cycle — clear safe mode
      prefs.putULong("safe_mode_ms", 0);
      prefs.putInt("boot_count", 0);
      Serial.println("[BOOT] Power cycle detected. Safe mode cleared.");
      prefs.end();
      return;
    }
  }

  // Use esp_timer for more reliable time (microseconds since boot)
  // For crash loop detection, we track boot_count and reset it if we
  // had a long uptime before the reboot (indicated by large lastBootTime)
  if (lastBootTime > (unsigned long)(CRASH_LOOP_WINDOW_SEC * 1000UL)) {
    // Previous run had uptime > 5 min — not a crash loop
    bootCount = 0;
  }

  bootCount++;
  prefs.putInt("boot_count", bootCount);
  prefs.putULong("last_boot_ms", 0);  // Will be updated to millis() periodically

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

// =============================================================================
// SECTION 16D: NVS STATE PERSISTENCE (Phase 2)
// Persists pumpMode, isDryRunError on change; waterLevelPct with wear reduction.
// =============================================================================

void loadStateFromNVS() {
  if (!prefs.begin(NVS_STATE_NAMESPACE, true)) return;

  String savedMode = prefs.getString("mode", "AUTO");
  bool savedDryRun = prefs.getBool("dry_run_err", false);
  int savedLevel = prefs.getInt("level_pct", -1);
  prefs.end();

  // Validate mode
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
  bool levelNeedsWrite = (lastPersistedLevel == -1)  // Never written
    || (levelDelta >= NVS_LEVEL_DELTA_THRESHOLD)
    || (now - lastLevelWriteMs >= NVS_LEVEL_INTERVAL_MS);

  if (!modeChanged && !dryRunChanged && !levelNeedsWrite) return;

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

  // Update uptime for crash loop detection
  prefs.putULong("last_boot_ms", now);

  prefs.end();
}

// =============================================================================
// SECTION 17: SETUP
// =============================================================================

void setup() {
  Serial.begin(115200);
  Serial.println("\n====================================");
  Serial.println(" Smart Water Pump Controller v2.4.0");
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
  }

  // --- Hardware Watchdog (Phase 2) ---
  // ESP32 Arduino 3.x uses esp_task_wdt_config_t; 2.x uses init(sec, panic)
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
  esp_task_wdt_add(NULL);  // Add current task (loopTask)
  Serial.printf("[INIT] Watchdog timer initialized: %ds timeout.\n", WDT_TIMEOUT_SEC);

  // --- Initialize Timing ---
  unsigned long nowInit = millis();
  lastSensorMs        = nowInit;
  lastFirebaseMs      = nowInit;
  lastDeviceConfigMs  = 0;
  lastWifiRetryMs     = 0;
  lastRssiLogMs       = nowInit;
  lastLevelWriteMs    = nowInit;

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
      // Just lost connection
      wifiWasConnected = false;
      wifiBackoffMs = WIFI_BACKOFF_INITIAL_MS;
      Serial.println("[WIFI] Connection lost.");
    }
    if (now - lastWifiRetryMs >= wifiBackoffMs) {
      lastWifiRetryMs = now;
      Serial.printf("[WIFI] Reconnecting (backoff: %lums)...\n", wifiBackoffMs);
      WiFi.disconnect(true);  // Full disconnect (clear stored config)
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

      // Increase backoff: 5s → 10s → 20s → 40s → 60s (cap)
      wifiBackoffMs = min(wifiBackoffMs * 2, (unsigned long)WIFI_BACKOFF_MAX_MS);
      // Add jitter: ±2s
      long jitter = (long)random(-WIFI_JITTER_MS, WIFI_JITTER_MS);
      wifiBackoffMs = max((unsigned long)WIFI_BACKOFF_INITIAL_MS,
                          (unsigned long)((long)wifiBackoffMs + jitter));
    }
  } else {
    if (!wifiWasConnected) {
      // Just reconnected
      wifiWasConnected = true;
      wifiBackoffMs = WIFI_BACKOFF_INITIAL_MS;
      wifiRssi = WiFi.RSSI();
      firebaseNeedsReinit = true;  // Stale token after reconnect
      Serial.printf("[WIFI] Reconnected! IP: %s | RSSI: %d dBm\n",
                    WiFi.localIP().toString().c_str(), wifiRssi);
    }
    lastWifiRetryMs = 0;
  }

  // --- FIREBASE REINIT after WiFi reconnect (Phase 2) ---
  if (firebaseNeedsReinit && WiFi.status() == WL_CONNECTED) {
    firebaseNeedsReinit = false;
    initFirebase();
    configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");  // Phase 3: Retry NTP on reconnect
    Serial.println("[FIREBASE] Reinitialized after WiFi reconnect.");
  }

  // --- RSSI TELEMETRY (Phase 2: update every 60s) ---
  if (WiFi.status() == WL_CONNECTED && now - lastRssiLogMs >= 60000) {
    lastRssiLogMs = now;
    wifiRssi = WiFi.RSSI();
    Serial.printf("[WIFI] RSSI: %d dBm\n", wifiRssi);
  }

  // --- Phase 3: Determine sleep/idle state and dynamic intervals ---
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

  // Condition-based idle (outside sleep): pump OFF, level >= 90% for 5 min
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

  // Dynamic intervals (Phase 3/4: idle intervals configurable via Firebase)
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

  // --- SENSOR SAMPLING (dynamic interval) ---
  if (now - lastSensorMs >= sensorInterval) {
    lastSensorMs = now;

    // 1. Read ultrasonic water level (5-sample median + EMA)
    int reading = readUltrasonicSensor();

    // 2. Check for sensor failure (consecutive timeouts)
    checkSensorFailure(reading);

    if (reading >= 0) {
      prevWaterLevelPct = waterLevelPct;
      waterLevelPct = reading;
    }

    // 3. Calculate flow rate from last 1-second pulse window
    flowRateLpm = calculateFlowRate();

    Serial.printf("[SENSOR] Level: %d%% | Flow: %.2f LPM | SensorErr:%s | OverflowErr:%s | Sleep:%s\n",
                  waterLevelPct, flowRateLpm,
                  isSensorError ? "Y" : "N",
                  isOverflowError ? "Y" : "N",
                  isSleeping ? "Y" : "N");

    // 4. Run all safety checks (dry-run, flow stuck, overflow)
    checkSafetyCutoff();

    // 5. Execute pump state machine
    executePumpLogic();
  }

  // --- FIREBASE SYNC (dynamic interval) ---
  if (now - lastFirebaseMs >= firebaseInterval) {
    lastFirebaseMs = now;

    if (Firebase.ready()) {
      // Device config: read every 30s; first time run immediately
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

  // --- NVS STATE PERSISTENCE (Phase 2: on change + wear-reduced level) ---
  persistStateToNVS();

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
