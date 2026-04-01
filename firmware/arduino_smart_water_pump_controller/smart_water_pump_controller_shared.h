#pragma once

// -----------------------------------------------------------------------------
// Shared definitions for Arduino multi-tab build
//
// IMPORTANT (Arduino build order):
// - Arduino concatenates the main sketch file (`smart_pump_controller.ino`) first.
// - Additional `.ino` tabs are appended afterward (alphabetically).
//
// To ensure `setup()`/`loop()` can see constants/globals, we place the shared
// includes, macros, and globals in this header and include it at the top of
// `smart_pump_controller.ino`.
// -----------------------------------------------------------------------------

// =============================================================================
// PHASE 1 — Structured Log System
// =============================================================================
// Five severity levels. Build-time floor strips calls below LOG_COMPILE_FLOOR
// entirely (zero binary + zero runtime overhead). Runtime ceiling (gLogLevel)
// is read from Firebase config — change verbosity in the field without reflash.
//
// Format: [L][MODULE][MS] message
// Example: [W][RS485][0046002] Frame timeout attempt 2/3. Retrying.
// =============================================================================

#define LOG_ERROR   0   // Safety trips, hardware failures, crash detection
#define LOG_WARN    1   // Degraded states, comm loss, approaching limits
#define LOG_INFO    2   // State transitions, mode changes, boot events
#define LOG_DEBUG   3   // Per-cycle sensor readings, RS-485 frame details
#define LOG_VERBOSE 4   // State machine internals, raw ISR counts, timer values

// Compile-time floor: calls below this level are removed by preprocessor.
// Development: LOG_DEBUG. Release-optimized: LOG_INFO.
#ifndef LOG_COMPILE_FLOOR
  #define LOG_COMPILE_FLOOR LOG_DEBUG
#endif

// Runtime ceiling (gLogLevel): set via Firebase config/device/debug_log_level.
// Initialized to LOG_INFO so production output is ERROR + WARN + INFO only.
extern uint8_t gLogLevel;

// Level → single-char code lookup
static const char LOG_LEVEL_CHAR[] = { 'E', 'W', 'I', 'D', 'V' };

// LOG() macro — compile-time + runtime gated. Thread-safe for single-core loop.
// REFACTOR [H-01]: replaces all flat Serial.printf/println calls.
#define LOG(level, module, fmt, ...) \
  do { \
    if ((level) <= LOG_COMPILE_FLOOR && (level) <= gLogLevel) { \
      Serial.printf("[%c][%s][%010lu] " fmt "\n", \
        LOG_LEVEL_CHAR[(level) <= 4 ? (level) : 4], (module), millis(), ##__VA_ARGS__); \
    } \
  } while(0)

// ---- Core libraries ----
#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>   // NVS config/state
#include <math.h>          // fabsf()
#include <esp_task_wdt.h>  // watchdog
#include <esp_system.h>    // esp_reset_reason()
#include <esp_sleep.h>     // light sleep
#include <time.h>          // NTP time
#include <esp_timer.h>     // ISR-safe microsecond timer

// ---- Firebase ----
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

// ---- Credentials ----
// Copy `secrets.h.example` → `secrets.h` and fill in WiFi + Firebase credentials.
// Never commit `secrets.h`.
#include "secrets.h"

// ---- GPIO mapping ----
#define RELAY_PIN        4

// RS-485 (Tank Link) — production pinout (see hardware/wiring_notes.md)
// ESP32 UART2: TX2=GPIO17, RX2=GPIO25. Half-duplex DE/RE tied to one GPIO.
#define RS485_UART_BAUD     115200
#define RS485_TX_PIN        17
#define RS485_RX_PIN        25
#define RS485_DE_RE_PIN      5   // LOW=RX, HIGH=TX

// ---- Tank calibration ----
#define TANK_EMPTY_CM   122
#define TANK_FULL_CM     8

// AUTO mode hysteresis thresholds (percent)
#define PUMP_START_LEVEL  30
#define PUMP_STOP_LEVEL  100

// ---- Safety + timing ----
#define DRY_RUN_THRESHOLD_LPM  1.0f
#define DRY_RUN_TIMEOUT_MS     30000

#define FLOW_CALIBRATION_FACTOR  7.5f
#define MAX_PUMP_RUNTIME_MIN     120

#define SENSOR_FAILURE_THRESHOLD  5
#define FLOW_STUCK_THRESHOLD_LPM  2.0f
#define FLOW_STUCK_TIMEOUT_MS     5000
#define FLOW_MAX_SANE_LPM         100.0f
#define FLOW_PUMP_OFF_ZERO_MS     3000
#define LEVEL_RATE_OF_CHANGE_MAX  15

#define ULTRASONIC_SAMPLES        5
#define ULTRASONIC_SAMPLE_DELAY   80
#define ULTRASONIC_EMA_ALPHA      0.5f

#define SENSOR_INTERVAL_MS         1000
#define FIREBASE_INTERVAL_MS       3000
#define FIREBASE_AUTH_COOLDOWN_MS  30000UL
#define DEVICE_CONFIG_INTERVAL_MS  30000
#define ULTRASONIC_TIMEOUT_MS      100

// RS-485 poll + framing
#define RS485_REQ_INTERVAL_MS      1000
#define RS485_FRAME_TIMEOUT_MS     250
#define RS485_MAX_RETRIES          3
#define RS485_RX_LINE_MAX          128  // enlarged for LDSC field (Phase 2)
#define REMOTE_SENSOR_OFFLINE_MS   5000UL
#define RS485_TX_TURNAROUND_US     80   // DE/RE settle time; tune for cable/transceiver

// Startup
#define STARTUP_STABILIZE_MS       1000UL

// Level freshness hard safety
#define LEVEL_STALE_TIMEOUT_MS     2500UL

// Remote sensor stability latch
#define REMOTE_STABLE_ONLINE_N     3
#define REMOTE_STABLE_OFFLINE_N    3

// NVS namespaces + schema
#define NVS_NAMESPACE        "pump_cfg"
#define NVS_STATE_NAMESPACE  "pump_state"
#define NVS_SCHEMA_VERSION   1

// Crash loop detection
#define CRASH_LOOP_THRESHOLD    5
#define CRASH_LOOP_WINDOW_SEC   300
#define SAFE_MODE_TIMEOUT_MS    3600000UL

// WiFi exponential backoff
#define WIFI_BACKOFF_INITIAL_MS 5000
#define WIFI_BACKOFF_MAX_MS     60000
#define WIFI_JITTER_MS          2000

// Watchdog
// Note: With weak WiFi, Firebase RTDB reads can still block long enough to starve loopTask.
// Keep this comfortably above worst-case network stalls to avoid crash-loop safe mode.
#define WDT_TIMEOUT_SEC         120

// Status push retry
#define STATUS_PUSH_RETRY_MAX   3
#define STATUS_PUSH_RETRY_MS    1000

// NVS wear reduction
#define NVS_LEVEL_DELTA_THRESHOLD 5
#define NVS_LEVEL_INTERVAL_MS     300000UL
#define NVS_UPTIME_INTERVAL_MS    60000UL

// Scheduled sleep (light sleep)
#define SLEEP_DEFAULT_ENABLED       false
#define SLEEP_DEFAULT_START_HOUR    23
#define SLEEP_DEFAULT_END_HOUR      5
#define SLEEP_DEFAULT_EMERGENCY_LVL 5
#define SLEEP_WAKE_INTERVAL_MS      30000UL

// Idle slow-poll mode (outside sleep hours)
#define IDLE_LEVEL_THRESHOLD          90
#define IDLE_STABLE_TIME_MS           300000UL
#define IDLE_SENSOR_INTERVAL_MS_DEF   10000
#define IDLE_FIREBASE_INTERVAL_MS_DEF 30000

// COUNTDOWN mode (semi-automatic timed run)
#define COUNTDOWN_ADD_TIME_MIN       5
#define COUNTDOWN_MAX_DURATION_MIN   120

// Sensor resilience
#define TANK_CAPACITY_L              660   // Bestank WT660
#define AUTO_BYPASS_FAILURE_SEC_DEF  60

// Minimum off-time between pump starts (motor protection)
#define MIN_PUMP_OFF_TIME_MS         30000

#ifndef APP_HOSTNAME
  #define APP_HOSTNAME "smartflow-controller"
#endif

// ---- Firebase objects ----
extern FirebaseData   fbdo;
extern FirebaseAuth   auth;
extern FirebaseConfig config;
extern Preferences    prefs;

// ---- Global state ----
extern int   cfgTankEmptyCm;
extern int   cfgTankFullCm;
extern int   cfgPumpStartLevel;
extern int   cfgPumpStopLevel;
extern float cfgDryRunThresholdLpm;
extern int   cfgDryRunTimeoutSec;
extern float cfgFlowCalibration;
extern int   cfgMaxPumpRuntimeMin;

extern bool cfgSleepEnabled;
extern int  cfgSleepStartHour;
extern int  cfgSleepEndHour;
extern int  cfgSleepEmergencyLevel;

extern int  cfgLevelSensorFailureThreshold;
extern int  cfgIdleSensorIntervalMs;
extern int  cfgIdleFirebaseIntervalMs;
/** When true, ignore level sensor for start/stop; flow guard is still active. */
extern bool cfgBypassLevelSensor;
/** When true, ignore flow sensor for dry-run and flow-stuck checks. */
extern bool cfgBypassFlowSensor;

extern float flowRateLpm;
extern int   waterLevelPct;
extern bool  isRunning;
extern int   prevWaterLevelPct;

extern String pumpMode;
extern bool   isDryRunError;
extern bool   isLevelSensorError;
extern bool   isFlowSensorError;
extern bool   isOverflowError;
extern bool   manualRuntimeWarning;

extern int           levelSensorFailCount;
extern unsigned long levelLastValidMs;  // dashboard health metric ONLY — NOT a freshness gate; see M-01
extern float         estimatedLevelPct;     // flow-based estimate (-1 = not set)
extern float         flowVolumeAddedL;      // liters added since anchor
extern unsigned long lastFlowEstimateMs;    // timestamp for dt in flow estimate
extern int           levelAnchorPct;        // last known good level for estimate
extern unsigned long totalPumpRunSec;       // accumulated runtime (persisted)
extern uint32_t      totalPumpCycles;       // cycle count (persisted)
extern uint32_t      lastPersistedPumpCycles;
extern unsigned long lastPersistedPumpRunSec;
extern unsigned long pumpOnSinceMs;         // millis() when current run started
extern bool          cfgAutoBypassOnSensorFail;
extern int           cfgAutoBypassDelaySec;
extern bool          autoBypassWasEngaged;
extern bool          autoBypassActive;
extern unsigned long levelSensorFailStartMs;
extern unsigned long flowStuckStartMs;
extern bool          flowStuckTimerActive;
extern unsigned long pumpOffStartMs;
extern bool          offTimerActive;
extern unsigned long offTimerEndMs;
extern int           pumpCooldownRemainingSec;

// Remote sensor node telemetry over RS-485 (tank link)
extern unsigned long remoteSensorLastRxMs;
extern uint32_t      remoteSensorConsecutiveFailCount;
extern int           remoteSensorLastErrCode;   // protocol ERR:<code> from sensor node
extern bool          remoteSensorOnline;        // derived from lastRx age
extern bool          remoteSensorStable;
extern uint32_t      remoteSensorOkStreak;
extern uint32_t      remoteSensorFailStreak;
extern uint32_t      remoteSensorLevelDiscardCount;  // REFACTOR [3.3]: LDSC field from Phase 2 NodeMCU frame

extern unsigned long levelLastUpdateMs;

extern uint32_t ultrasonicCycleOkCount;
extern uint32_t ultrasonicCycleTimeoutCount;
extern uint32_t ultrasonicLastGoodCmX10;
extern uint32_t flowDiscardMaxSaneCount;
extern uint32_t flowStuckHighEventCount;

extern unsigned long lastSensorTelemetryLogMs;
extern uint32_t      ultrasonicCycleOkCountWin;
extern uint32_t      ultrasonicCycleTimeoutCountWin;
extern uint32_t      flowDiscardMaxSaneCountWin;
extern uint32_t      flowStuckHighEventCountWin;

extern unsigned long dryRunStartMs;
extern bool          dryRunTimerActive;

extern unsigned long pumpAutoStartMs;
extern bool          pumpAutoStartTracking;

extern bool          inSafeMode;
extern unsigned long safeModeEnteredMs;
extern String        bootReasonStr;

extern unsigned long wifiBackoffMs;
extern bool          wifiWasConnected;
extern bool          firebaseInitialized;
extern bool          crashCounterCleared;

extern int           wifiRssi;
extern unsigned long lastSuccessfulFirebaseMs;
extern unsigned long lastRssiLogMs;
extern unsigned long firebaseCooldownUntilMs;
extern uint32_t      firebaseConsecutiveFailCount;
extern String        firebaseLastError;
extern unsigned long firebaseLastErrorLogMs;

extern String        lastPersistedMode;
extern bool          lastPersistedDryRun;
extern bool          lastPersistedBypass;
extern bool          lastPersistedBypassFlow;
extern int           lastPersistedLevel;
extern unsigned long lastLevelWriteMs;
extern unsigned long lastUptimeWriteMs;

extern bool          isSleeping;
extern bool          ntpSynced;
extern bool          isIdleMode;
extern unsigned long idleStartMs;
extern unsigned long lastSleepLogMs;
extern int           lastRebootRequestId;

extern unsigned long lastSensorMs;
extern unsigned long lastFirebaseMs;
extern unsigned long lastDeviceConfigMs;
extern unsigned long lastWifiRetryMs;

extern unsigned long lastHeapDiagMs;
extern uint32_t      minFreeHeapObserved;

// Phase 1: rate-limit timestamps for repeated WARN conditions
extern unsigned long lastRs485WarnMs;
extern unsigned long lastFbWarnMs;

// -----------------------------------------------------------------------------
// Runtime mode + operator intent
// -----------------------------------------------------------------------------
// runMode is derived from mode + state; it is not written by the dashboard.
extern String        runMode;                // "AUTO" | "AUTO_STANDBY" | "AUTO_COOLDOWN" | "MANUAL_ON" | "MANUAL_OFF" | "MANUAL_COOLDOWN" | "COUNTDOWN" | "STOPPED"
extern String        runPrevPumpMode;        // reserved for compatibility/debugging
extern unsigned long runStartMs;             // millis() when the current run was requested
extern bool          isManualRun;            // true when operating under MANUAL policy (legacy flag kept for compatibility)
extern String        lastFaultCode;          // short identifier (e.g., "DRY_RUN", "OVERFLOW", "E_STOP")
extern String        lastFaultMessage;       // human-readable detail

// Operator intent and stop latch
extern bool          manualDesired;
extern bool          emergencyStopLatched;

// Countdown state (semi-automatic timed run)
extern bool          isCountdownActive;
extern unsigned long countdownEndMs;         // millis() when countdown expires
extern bool          pendingModeWriteback;   // suppresses stale Firebase mode reads during write-back propagation
extern unsigned long pendingModeWritebackSentMs;   // rate-limits write-back retries (5s between attempts)
extern int           cfgLastCountdownDurationMin;  // NVS-persisted last duration for offline use
extern int           statusPushRetryCount;
extern unsigned long statusPushRetryMs;

void checkCountdownExpiry();       // called from loop() before executePumpLogic()
void updateFlowBasedEstimate();    // call after updating flowRateLpm

bool pollRemoteSensorNode();   // RS-485: updates waterLevelPct/flowRateLpm + error flags

