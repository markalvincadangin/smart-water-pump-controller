#pragma once

#include "../config/config.h"

enum class DeviceLifecycle {
    UNCLAIMED,
    PROVISIONING,
    ONLINE
};

extern DeviceLifecycle deviceLifecycle;

// Firebase objects
extern FirebaseData   fbdo;
extern FirebaseAuth   auth;
extern FirebaseConfig config;
extern Preferences    prefs;
extern uint8_t        gLogLevel;

// Device config (mutable, persisted)
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
extern bool cfgBypassLevelSensor;
extern bool cfgBypassFlowSensor;   // bypass dry-run + flow-stuck checks

// Live telemetry
extern float flowRateLpm;
extern int   waterLevelPct;
extern bool  isRunning;
extern int   prevWaterLevelPct;

// Mode and faults
enum class PumpState {
    IDLE,
    MANUAL,
    COUNTDOWN,
    ERROR
};

extern PumpState currentState;
extern String pumpMode;
extern bool   isDryRunError;
extern bool   isLevelSensorError;
extern bool   isFlowSensorError;
extern bool   isOverflowError;
extern bool   manualRuntimeWarning;

extern String        runMode;
extern String        runPrevPumpMode;
extern unsigned long runStartMs;
extern bool          isManualRun;
extern String        lastFaultCode;
extern String        lastFaultMessage;

// Operator intent + stop latch
extern bool          manualDesired;
extern bool          emergencyStopLatched;

// COUNTDOWN
extern bool          isCountdownActive;
extern unsigned long countdownEndMs;
extern bool          pendingModeWriteback;
extern unsigned long pendingModeWritebackSentMs;
extern int           cfgLastCountdownDurationMin;
extern int           statusPushRetryCount;
extern unsigned long statusPushRetryMs;

// Sensor failure + estimate
extern int           levelSensorFailCount;
extern unsigned long levelLastValidMs;
extern float         estimatedLevelPct;
extern float         flowVolumeAddedL;
extern unsigned long lastFlowEstimateMs;
extern int           levelAnchorPct;
extern bool          cfgAutoBypassOnSensorFail;
extern int           cfgAutoBypassDelaySec;
extern bool          autoBypassWasEngaged;
extern bool          autoBypassActive;
extern unsigned long levelSensorFailStartMs;

// Flow stuck detection
extern unsigned long flowStuckStartMs;
extern bool          flowStuckTimerActive;
extern bool          offTimerActive;
extern unsigned long offTimerEndMs;
extern int           pumpCooldownRemainingSec;

// Pump runtime/cycles + min-off tracking
extern unsigned long totalPumpRunSec;
extern uint32_t      totalPumpCycles;
extern uint32_t      lastPersistedPumpCycles;
extern unsigned long lastPersistedPumpRunSec;
extern unsigned long pumpOnSinceMs;
extern unsigned long pumpOffStartMs;

// RS-485 remote sensor node status
extern unsigned long remoteSensorLastRxMs;
extern uint32_t      remoteSensorConsecutiveFailCount;
extern int           remoteSensorLastErrCode;
extern bool          remoteSensorOnline;
extern bool          remoteSensorStable;
extern uint32_t      remoteSensorOkStreak;
extern uint32_t      remoteSensorFailStreak;
extern uint32_t      remoteSensorLevelDiscardCount;

extern unsigned long levelLastUpdateMs;

// Telemetry counters
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

// Dry-run
extern unsigned long dryRunStartMs;
extern bool          dryRunTimerActive;

// Overflow tracking
extern unsigned long pumpAutoStartMs;
extern bool          pumpAutoStartTracking;

// Safe mode
extern bool          inSafeMode;
extern unsigned long safeModeEnteredMs;
extern String        bootReasonStr;

// Emergency stop state preservation
extern String        emergencyStopSavedMode;

// WiFi/Firebase runtime
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
extern uint32_t      firebaseTimeoutCount;
extern uint32_t      firebaseAuthErrorCount;
extern uint32_t      firebaseNotReadySkipCount;

extern uint32_t      cloudControlOkCount;
extern uint32_t      cloudControlFailCount;
extern uint32_t      cloudStatusOkCount;
extern uint32_t      cloudStatusFailCount;
extern unsigned long cloudLastControlOkMs;
extern uint32_t      cloudLastControlCallMs;
extern uint32_t      cloudLastStatusCallMs;
extern uint32_t      cloudLastCycleMs;

extern uint32_t      rs485LastCallMs;
extern uint32_t      loopMaxMs;

// Persisted-state bookkeeping
extern String        lastPersistedMode;
extern bool          lastPersistedDryRun;
extern bool          lastPersistedBypass;
extern bool          lastPersistedBypassFlow;
extern int           lastPersistedLevel;
extern unsigned long lastLevelWriteMs;
extern unsigned long lastUptimeWriteMs;
extern int           lastRebootRequestId;

// Sleep/idle flags
extern bool          isSleeping;
extern bool          ntpSynced;
extern uint32_t      ntpEpochSecAtLastSync;
extern unsigned long ntpLastSyncMs;
extern bool          isIdleMode;
extern unsigned long idleStartMs;
extern unsigned long lastSleepLogMs;

// Loop timing
extern unsigned long lastSensorMs;
extern unsigned long lastFirebaseMs;
extern unsigned long lastDeviceConfigMs;

// Heap diagnostics
extern unsigned long lastHeapDiagMs;
extern uint32_t      minFreeHeapObserved;

