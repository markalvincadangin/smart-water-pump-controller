#include "state.h"

// Definitions (single translation unit)
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

int  cfgLevelSensorFailureThreshold = SENSOR_FAILURE_THRESHOLD;
int  cfgIdleSensorIntervalMs   = IDLE_SENSOR_INTERVAL_MS_DEF;
int  cfgIdleFirebaseIntervalMs = IDLE_FIREBASE_INTERVAL_MS_DEF;
bool cfgBypassLevelSensor      = false;
bool cfgBypassFlowSensor       = false;   // NEW: bypass dry-run + flow-stuck checks

float flowRateLpm             = 0.0f;
int   waterLevelPct           = 0;
bool  isRunning               = false;
int   prevWaterLevelPct       = 0;

String pumpMode          = "AUTO";
bool   isDryRunError     = false;
bool   isLevelSensorError = false;
bool   isFlowSensorError = false;
bool   isOverflowError   = false;

int           levelSensorFailCount     = 0;
unsigned long levelLastValidMs         = 0;
float         estimatedLevelPct        = -1.0f;
float         flowVolumeAddedL         = 0.0f;
unsigned long lastFlowEstimateMs       = 0;
int           levelAnchorPct           = -1;
unsigned long totalPumpRunSec          = 0;
uint32_t      totalPumpCycles          = 0;
uint32_t      lastPersistedPumpCycles  = 0;
unsigned long lastPersistedPumpRunSec  = 0;
unsigned long pumpOnSinceMs            = 0;
bool          cfgAutoBypassOnSensorFail = false;
int           cfgAutoBypassDelaySec    = AUTO_BYPASS_FAILURE_SEC_DEF;
bool          autoBypassWasEngaged     = false;
bool          autoBypassActive         = false;
unsigned long levelSensorFailStartMs   = 0;
unsigned long flowStuckStartMs         = 0;
bool          flowStuckTimerActive     = false;
unsigned long pumpOffStartMs           = 0;

unsigned long remoteSensorLastRxMs              = 0;
uint32_t      remoteSensorConsecutiveFailCount  = 0;
int           remoteSensorLastErrCode           = -1;
bool          remoteSensorOnline                = false;
bool          remoteSensorStable                = false;
uint32_t      remoteSensorOkStreak              = 0;
uint32_t      remoteSensorFailStreak            = 0;

unsigned long levelLastUpdateMs                 = 0;

uint32_t ultrasonicCycleOkCount       = 0;
uint32_t ultrasonicCycleTimeoutCount  = 0;
uint32_t ultrasonicLastGoodCmX10      = 0;
uint32_t flowDiscardMaxSaneCount      = 0;
uint32_t flowStuckHighEventCount      = 0;

unsigned long lastSensorTelemetryLogMs      = 0;
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
bool          firebaseInitialized = false;

int           wifiRssi                 = 0;
unsigned long lastSuccessfulFirebaseMs = 0;
unsigned long lastRssiLogMs            = 0;
unsigned long firebaseCooldownUntilMs  = 0;
uint32_t      firebaseConsecutiveFailCount = 0;
String        firebaseLastError        = "";
unsigned long firebaseLastErrorLogMs   = 0;

String        lastPersistedMode   = "AUTO";
bool          lastPersistedDryRun = false;
bool          lastPersistedBypass = false;
bool          lastPersistedBypassFlow = false;
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

// Runtime mode / operator intent
String        runMode          = "OFF";
String        runPrevPumpMode  = "AUTO";
unsigned long runStartMs       = 0;
bool          isManualRun      = false;
String        lastFaultCode    = "";
String        lastFaultMessage = "";

// Operator intent + stop latch
bool          manualDesired        = false;
bool          emergencyStopLatched = false;

bool          isCountdownActive = false;
unsigned long countdownEndMs   = 0;
bool          pendingModeWriteback = false;
unsigned long pendingModeWritebackSentMs = 0;
int           cfgLastCountdownDurationMin = 15;
int           statusPushRetryCount   = 0;
unsigned long statusPushRetryMs      = 0;

