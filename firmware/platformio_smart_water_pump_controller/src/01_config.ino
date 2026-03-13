// -----------------------------------------------------------------------------
// Configuration + globals live in `smart_pump_controller_shared.h`.
//
// This `.ino` file remains as an Arduino tab for discoverability, but the header
// is included from `smart_pump_controller.ino` so `setup()`/`loop()` always see
// the shared definitions (Arduino concatenates the main sketch file first).
// -----------------------------------------------------------------------------

// Shared definitions header (macros + externs).
#include <smart_water_pump_controller_shared.h>

// Definitions (not extern) — single translation unit
FirebaseData   fbdo;
FirebaseAuth   auth;
FirebaseConfig config;
Preferences    prefs;

int   cfgTankEmptyCm         = TANK_EMPTY_CM;
int   cfgTankFullCm          = TANK_FULL_CM;
int   cfgPumpStartLevel      = PUMP_START_LEVEL;
int   cfgPumpStopLevel       = PUMP_STOP_LEVEL;
float cfgDryRunThresholdLpm  = DRY_RUN_THRESHOLD_LPM;
int   cfgDryRunTimeoutSec    = (int)(DRY_RUN_TIMEOUT_MS / 1000UL);
float cfgFlowCalibration     = FLOW_CALIBRATION_FACTOR;
int   cfgMaxPumpRuntimeMin   = MAX_PUMP_RUNTIME_MIN;

bool cfgSleepEnabled         = SLEEP_DEFAULT_ENABLED;
int  cfgSleepStartHour       = SLEEP_DEFAULT_START_HOUR;
int  cfgSleepEndHour         = SLEEP_DEFAULT_END_HOUR;
int  cfgSleepEmergencyLevel  = SLEEP_DEFAULT_EMERGENCY_LVL;

int cfgSensorFailureThreshold = SENSOR_FAILURE_THRESHOLD;
int cfgIdleSensorIntervalMs   = IDLE_SENSOR_INTERVAL_MS_DEF;
int cfgIdleFirebaseIntervalMs = IDLE_FIREBASE_INTERVAL_MS_DEF;

volatile uint32_t pulseCount  = 0;
volatile uint64_t lastPulseUs = 0;
float flowRateLpm             = 0.0f;
int   waterLevelPct           = 0;
float waterLevelEma           = 0.0f;
bool  isRunning               = false;
int   prevWaterLevelPct       = 0;

String pumpMode          = "AUTO";
bool   isDryRunError     = false;
bool   isSensorError     = false;
bool   isFlowSensorError = false;
bool   isOverflowError   = false;

int           sensorFailCount       = 0;
unsigned long flowStuckStartMs      = 0;
bool          flowStuckTimerActive  = false;
unsigned long pumpOffStartMs        = 0;

uint32_t ultrasonicCycleOkCount       = 0;
uint32_t ultrasonicCycleTimeoutCount  = 0;
uint32_t ultrasonicLastGoodCmX10      = 0;
uint32_t flowDiscardMaxSaneCount      = 0;
uint32_t flowStuckHighEventCount      = 0;

unsigned long lastSensorTelemetryLogMs     = 0;
uint32_t      ultrasonicCycleOkCountWin      = 0;
uint32_t      ultrasonicCycleTimeoutCountWin = 0;
uint32_t      flowDiscardMaxSaneCountWin     = 0;
uint32_t      flowStuckHighEventCountWin     = 0;

unsigned long dryRunStartMs     = 0;
bool          dryRunTimerActive = false;

unsigned long pumpAutoStartMs       = 0;
bool          pumpAutoStartTracking = false;

bool          inSafeMode        = false;
unsigned long safeModeEnteredMs = 0;
String        bootReasonStr     = "";

unsigned long wifiBackoffMs    = WIFI_BACKOFF_INITIAL_MS;
bool          wifiWasConnected = false;

int           wifiRssi                 = 0;
unsigned long lastSuccessfulFirebaseMs = 0;
unsigned long lastRssiLogMs            = 0;
unsigned long firebaseCooldownUntilMs  = 0;
uint32_t      firebaseConsecutiveFailCount = 0;
String        firebaseLastError        = "";
unsigned long firebaseLastErrorLogMs   = 0;

String        lastPersistedMode   = "AUTO";
bool          lastPersistedDryRun = false;
int           lastPersistedLevel  = -1;
unsigned long lastLevelWriteMs    = 0;
unsigned long lastUptimeWriteMs   = 0;

bool          isSleeping     = false;
bool          ntpSynced      = false;
bool          isIdleMode     = false;
unsigned long idleStartMs    = 0;
unsigned long lastSleepLogMs = 0;
int           lastRebootRequestId = 0;

unsigned long lastSensorMs       = 0;
unsigned long lastFirebaseMs     = 0;
unsigned long lastDeviceConfigMs = 0;
unsigned long lastWifiRetryMs    = 0;

unsigned long lastHeapDiagMs      = 0;
uint32_t      minFreeHeapObserved = 0;

// Phase 7: Smart manual/timed run state (additive to existing pumpMode)
String        runMode         = "AUTO";
String        runPrevPumpMode = "AUTO";
unsigned long runStartMs      = 0;
unsigned long runDurationMs   = 0;
uint32_t      runRemainingSec = 0;
String        lastFaultCode   = "";
String        lastFaultMessage = "";
