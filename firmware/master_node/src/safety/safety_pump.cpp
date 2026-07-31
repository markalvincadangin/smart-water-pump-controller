#include "safety_pump.h"

#include "../state/state.h"
#include "../drivers/pump_driver.h"
#include "../utils/time_utils.h"

void setPump(bool on) {
  if (on == isRunning) return;

  if (on && !isRunning) {
    totalPumpCycles++;
    pumpOnSinceMs = millis();
    app_logger.logCloudEvent(APP_LOG_LEVEL_INFO, LogCategory::PUMP, EventCode::EVT_PUMP_ON, "Relay ENERGIZED. Pump is now ON.");
  }
  if (!on && isRunning && pumpOnSinceMs > 0) {
    totalPumpRunSec += (millis() - pumpOnSinceMs) / 1000UL;
    pumpOnSinceMs = 0;
    app_logger.logCloudEvent(APP_LOG_LEVEL_INFO, LogCategory::PUMP, EventCode::EVT_PUMP_OFF, "Relay DE-ENERGIZED. Pump is now OFF.");
  }

  if (on) {
      PumpDriver::turnOn();
  } else {
      PumpDriver::turnOff();
  }
  isRunning = on;
  if (!on) {
    pumpOffStartMs = millis();
  } else {
    pumpOffStartMs = 0;
  }
}

void checkLevelSensorFailure(int sensorReading) {
  bool prevLevelError = isLevelSensorError;

  if (sensorReading == -1) {
    levelSensorFailCount++;
    if (levelSensorFailCount >= cfgLevelSensorFailureThreshold && !isLevelSensorError) {
      isLevelSensorError = true;
      app_logger.logCloudEvent(APP_LOG_LEVEL_ERROR, LogCategory::SENSOR, EventCode::EVT_SENSOR_LEVEL_FAIL, "Ultrasonic (level) failure: %d consecutive timeouts.", levelSensorFailCount);
    }
    if (isLevelSensorError && cfgAutoBypassOnSensorFail && !cfgBypassLevelSensor) {
      if (levelSensorFailStartMs == 0) levelSensorFailStartMs = millis();
      if (elapsedMillis32(millis(), levelSensorFailStartMs) >= (uint32_t)cfgAutoBypassDelaySec * 1000UL) {
        cfgBypassLevelSensor = true;
        autoBypassWasEngaged = true;
        autoBypassActive = true;
        app_logger.logCloudEvent(APP_LOG_LEVEL_ERROR, LogCategory::SAFETY, EventCode::EVT_AUTO_BYPASS_ENABLED, "Enabled after sustained sensor failure.");
      }
    }
  } else {
    levelLastValidMs = millis();
    if (isLevelSensorError) {
      isLevelSensorError = false;
      levelSensorFailCount = 0;
      app_logger.logCloudEvent(APP_LOG_LEVEL_INFO, LogCategory::SENSOR, EventCode::EVT_SENSOR_LEVEL_RECOVERED, "Ultrasonic (level) recovered. Error cleared.");
      levelSensorFailStartMs = 0;
    }

    if (prevLevelError) {
      levelAnchorPct = sensorReading;
      flowVolumeAddedL = 0.0f;
      estimatedLevelPct = (float)sensorReading;
      LOG(APP_LOG_LEVEL_INFO, "ESTIMATE", "Anchor reset to recovered sensor reading.");
    } else {
      levelAnchorPct = sensorReading;
    }
    if (autoBypassWasEngaged) {
      cfgBypassLevelSensor = false;
      autoBypassWasEngaged = false;
      autoBypassActive = false;
      LOG(APP_LOG_LEVEL_INFO, "AUTO-BYPASS", "Sensor recovered. Bypass auto-disabled.");
    }
  }
}

void checkFlowSensorStuck() {
  if (cfgBypassFlowSensor) {
    flowStuckTimerActive = false;
    flowStuckStartMs = 0;
    if (isFlowSensorError) {
      isFlowSensorError = false;
      app_logger.logCloudEvent(APP_LOG_LEVEL_INFO, LogCategory::SENSOR, EventCode::EVT_SENSOR_FLOW_RECOVERED, "Flow sensor recovered. Error cleared.");
    }
    return;
  }
  if (!isRunning && flowRateLpm > FLOW_STUCK_THRESHOLD_LPM) {
    if (!flowStuckTimerActive) {
      flowStuckTimerActive = true;
      flowStuckStartMs = millis();
    } else if (elapsedMillis32(millis(), flowStuckStartMs) >= FLOW_STUCK_TIMEOUT_MS) {
      if (!isFlowSensorError) {
        isFlowSensorError = true;
        lastFaultCode = "FLOW_SENSOR";
        lastFaultMessage = "Flow sensor reading abnormal (stuck-high while pump OFF).";
        flowStuckHighEventCount++;
        flowStuckHighEventCountWin++;
        app_logger.logCloudEvent(APP_LOG_LEVEL_ERROR, LogCategory::SENSOR, EventCode::EVT_SENSOR_FLOW_STUCK, "Flow stuck-high: %.1f LPM while pump OFF for >%ds.", flowRateLpm, (int)(FLOW_STUCK_TIMEOUT_MS / 1000));
      }
    }
  } else {
    if (isFlowSensorError) {
      app_logger.logCloudEvent(APP_LOG_LEVEL_INFO, LogCategory::SENSOR, EventCode::EVT_SENSOR_FLOW_RECOVERED, "Flow sensor recovered. Error cleared.");
      isFlowSensorError = false;
    }
    flowStuckTimerActive = false;
    flowStuckStartMs = 0;
  }
}


OverflowStatus checkOverflowProtection() {
  if (!isRunning) {
    pumpAutoStartTracking = false;
    pumpAutoStartMs = 0;
    return {SafetyDecision::OK, false};
  }

  if (!pumpAutoStartTracking) {
    pumpAutoStartTracking = true;
    pumpAutoStartMs = millis();
    return {SafetyDecision::OK, false};
  }

  uint32_t maxRuntimeMs = (uint32_t)cfgMaxPumpRuntimeMin * 60000UL;
  uint32_t elapsed = elapsedMillis32(millis(), pumpAutoStartMs);

  bool nearThreshold = elapsed >= (maxRuntimeMs * 9UL) / 10UL;

  if (elapsed >= maxRuntimeMs) {
    isOverflowError = true;
    pumpAutoStartTracking = false;
    app_logger.logCloudEvent(APP_LOG_LEVEL_ERROR, LogCategory::SAFETY, EventCode::EVT_MAX_RUNTIME_EXCEEDED, "Max runtime exceeded (%d min). Pump stopped.", cfgMaxPumpRuntimeMin);
    return {SafetyDecision::STOP_OVERFLOW, nearThreshold};
  }
  return {SafetyDecision::OK, nearThreshold};
}

SafetyDecision checkDryRunProtection() {
  // If flow sensor bypass is on, clear any existing dry-run lockout and skip the check.
  // Bypass must recover the controller as well as prevent new dry-run trips.
  if (cfgBypassFlowSensor) {
    dryRunTimerActive = false;
    dryRunStartMs = 0;
    if (isDryRunError) {
      isDryRunError = false;
      lastFaultCode = "";
      lastFaultMessage = "";
      LOG(APP_LOG_LEVEL_INFO, "SAFETY", "Flow bypass active. Clearing dry-run lockout.");
    }
    return SafetyDecision::OK;
  }

  if (!isRunning) {
    dryRunTimerActive = false;
    dryRunStartMs = 0;
    return SafetyDecision::OK;
  }

  // If remote sensor data is stale/unstable, do not advance a dry-run timer.
  // Comm loss must fail-safe by stopping the pump via freshness/stability gates,
  // not by misclassifying as DRY_RUN.
  bool levelFreshGate = (levelLastUpdateMs > 0) &&
                        (elapsedMillis32(millis(), levelLastUpdateMs) <= LEVEL_STALE_TIMEOUT_MS);
  if (!remoteSensorStable || !levelFreshGate) {
    dryRunTimerActive = false;
    dryRunStartMs = 0;
    return SafetyDecision::OK;
  }

  unsigned long dryRunTimeoutMs = (unsigned long)cfgDryRunTimeoutSec * 1000UL;
  if (flowRateLpm < cfgDryRunThresholdLpm) {
    if (!dryRunTimerActive) {
      dryRunTimerActive = true;
      dryRunStartMs = millis();
      app_logger.logCloudEvent(APP_LOG_LEVEL_WARN, LogCategory::SAFETY, EventCode::EVT_DRY_RUN_WARN, "Dry-run condition detected. Timer started.");
    } else {
      uint32_t elapsedDr = elapsedMillis32(millis(), dryRunStartMs);
      if (elapsedDr >= dryRunTimeoutMs) {
        isDryRunError = true;
        // HARD SAFETY: always stop the pump regardless of mode.
        app_logger.logCloudEvent(APP_LOG_LEVEL_ERROR, LogCategory::SAFETY, EventCode::EVT_DRY_RUN_LOCKOUT, "DRY-RUN LOCKOUT. Pump stopped; waiting for acknowledge.");
        return SafetyDecision::STOP_DRYRUN;
      }
    }
  } else {
    if (dryRunTimerActive) {
      app_logger.logCloudEvent(APP_LOG_LEVEL_INFO, LogCategory::SAFETY, EventCode::EVT_DRY_RUN_CLEARED, "Flow restored. Dry-run timer reset.");
    }
    dryRunTimerActive = false;
    dryRunStartMs = 0;
  }
  return SafetyDecision::OK;
}

SafetyStatus checkSafetyCutoff() {
  SafetyDecision dryRunDecision = checkDryRunProtection();
  checkFlowSensorStuck();
  OverflowStatus overflowStatus = checkOverflowProtection();

  if (dryRunDecision != SafetyDecision::OK) {
    return {dryRunDecision, overflowStatus.nearThreshold};
  }
  if (overflowStatus.decision != SafetyDecision::OK) {
    return {overflowStatus.decision, overflowStatus.nearThreshold};
  }
  return {SafetyDecision::OK, overflowStatus.nearThreshold};
}

