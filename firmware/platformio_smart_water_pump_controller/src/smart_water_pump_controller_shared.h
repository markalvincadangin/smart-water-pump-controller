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
// ESP32 UART2: TX2=GPIO17, RX2=GPIO16. Half-duplex DE/RE tied to one GPIO.
#define RS485_UART_BAUD     115200
#define RS485_TX_PIN        17
#define RS485_RX_PIN        16
#define RS485_DE_RE_PIN      5   // LOW=RX, HIGH=TX

// ---- Tank calibration ----
#define TANK_EMPTY_CM   122
#define TANK_FULL_CM     8

// AUTO mode hysteresis thresholds (percent)
#define PUMP_START_LEVEL  30
#define PUMP_STOP_LEVEL  100

// ---- Safety + timing ----
#define DRY_RUN_THRESHOLD_LPM  0.5f
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
#define RS485_RX_LINE_MAX          96
#define REMOTE_SENSOR_OFFLINE_MS   5000UL

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

extern float flowRateLpm;
extern int   waterLevelPct;
extern bool  isRunning;
extern int   prevWaterLevelPct;

extern String pumpMode;
extern bool   isDryRunError;
extern bool   isLevelSensorError;
extern bool   isFlowSensorError;
extern bool   isOverflowError;

extern int           levelSensorFailCount;
extern unsigned long levelLastValidMs;      // millis() of last valid level reading
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

// Remote sensor node telemetry over RS-485 (tank link)
extern unsigned long remoteSensorLastRxMs;
extern uint32_t      remoteSensorConsecutiveFailCount;
extern int           remoteSensorLastErrCode;   // protocol ERR:<code> from sensor node
extern bool          remoteSensorOnline;        // derived from lastRx age

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

// -----------------------------------------------------------------------------
// Runtime mode + operator intent
// -----------------------------------------------------------------------------
// runMode is derived from mode + state; it is not written by the dashboard.
extern String        runMode;                // "OFF" | "AUTO" | "AUTO_STANDBY" | "MANUAL_ON" | "MANUAL_OFF" | "COUNTDOWN" | "STOPPED"
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

