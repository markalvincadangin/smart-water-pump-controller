#include "pump_app.h"

#include "../../state/state.h"
#include "../../safety/safety_pump.h"
#include "../../utils/time_utils.h"
#include "../../config/config.h"

void PumpApp::checkCountdownExpiry() {
  if (!isCountdownActive || pumpMode != "COUNTDOWN") return;
  uint32_t now = millis();
  if (countdownEndMs != 0 && millisDeadlineReached(now, countdownEndMs)) {
    LOG(LOG_LEVEL_INFO, "COUNTDOWN", "Timer expired. Pump stopped. Mode stays COUNTDOWN.");
    isCountdownActive = false;
    countdownEndMs    = 0;
    // Keep COUNTDOWN mode active; user must explicitly start a new timer.
  }
}

void PumpApp::executeLogic() {
  // Suspend control decisions until the first valid level sample.
  if (waterLevelPct < 0) return;

  isManualRun = (pumpMode == "MANUAL");

  // Emergency stop latch: highest priority.
  if (emergencyStopLatched) {
    lastFaultCode    = "E_STOP";
    lastFaultMessage = "Emergency stop is latched. Reset stop to resume operation.";
    setPump(false);
    return;
  }

  // Hard safety lockouts: evaluate decisions.
  SafetyStatus status = checkSafetyCutoff();
  
  // Handle warning for MANUAL mode if near threshold
  static bool manualRuntimeWarnLogged = false;
  if (!isRunning || pumpMode != "MANUAL") {
    manualRuntimeWarning = false;
    manualRuntimeWarnLogged = false;
  } else {
    manualRuntimeWarning = status.overflowNearThreshold;
    if (manualRuntimeWarning && !manualRuntimeWarnLogged) {
      manualRuntimeWarnLogged = true;
      LOG(LOG_LEVEL_WARN, "SAFETY", "[WARN] Manual runtime reached 90%% of limit (%d min).", cfgMaxPumpRuntimeMin);
    }
  }

  if (status.decision == SafetyDecision::STOP_DRYRUN || isDryRunError) {
    lastFaultCode    = "DRY_RUN";
    lastFaultMessage = "Dry-run lockout: low flow while pump was running.";
    setPump(false);
    if (isCountdownActive) {
      isCountdownActive = false; countdownEndMs = 0;
      // Keep COUNTDOWN mode active; pump remains off until user starts again.
    }
    return;
  }
  
  if (status.decision == SafetyDecision::STOP_OVERFLOW || isOverflowError) {
    lastFaultCode    = "OVERFLOW";
    lastFaultMessage = "Overflow protection: max runtime exceeded.";
    setPump(false);
    if (isCountdownActive) {
      isCountdownActive = false; countdownEndMs = 0;
      // Keep COUNTDOWN mode active; pump remains off until user starts again.
    }
    return;
  }

  // Sensor validity gate: use one freshness snapshot per loop.
  uint32_t nowMsPump = millis();
  const bool levelFreshOk = (levelLastUpdateMs > 0) &&
    (elapsedMillis32(nowMsPump, levelLastUpdateMs) <= LEVEL_STALE_TIMEOUT_MS);
  bool allowStartFromSensors = remoteSensorStable && levelFreshOk && !isLevelSensorError && !cfgBypassLevelSensor;

  // Expose cooldown mode while off-timer is active.
  if (!isRunning && pumpOffStartMs > 0 && elapsedMillis32(nowMsPump, pumpOffStartMs) < MIN_PUMP_OFF_TIME_MS) {
    offTimerActive = true;
    offTimerEndMs = pumpOffStartMs + MIN_PUMP_OFF_TIME_MS;
    uint32_t remainMs = (offTimerEndMs > nowMsPump) ? (uint32_t)(offTimerEndMs - nowMsPump) : 0;
    pumpCooldownRemainingSec = (int)(remainMs / 1000UL);
    runMode = (pumpMode == "MANUAL") ? "MANUAL_COOLDOWN" : (pumpMode == "COUNTDOWN" ? "COUNTDOWN" : "AUTO_COOLDOWN");
  } else {
    offTimerActive = false;
    offTimerEndMs = 0;
    pumpCooldownRemainingSec = 0;

    if (pumpMode == "MANUAL" && isRunning && manualDesired) {
      runMode = "MANUAL_ON";
    } else if (pumpMode == "MANUAL") {
      runMode = "MANUAL_OFF";
    } else if (pumpMode == "COUNTDOWN" && isCountdownActive) {
      runMode = "COUNTDOWN";
    } else if (pumpMode == "COUNTDOWN" && !isCountdownActive) {
      runMode = "COUNTDOWN";
    } else if (pumpMode == "AUTO" && isRunning) {
      runMode = "AUTO";
    } else if (pumpMode == "AUTO" && !isRunning) {
      runMode = "AUTO_STANDBY";
    } else {
      runMode = isRunning ? "AUTO" : "AUTO_STANDBY";
    }
  }

  if (pumpMode == "MANUAL" || pumpMode == "COUNTDOWN") {
    bool isIntentActive = (pumpMode == "MANUAL") ? manualDesired : isCountdownActive;
    
    if (!isIntentActive) {
      setPump(false);
      return;
    }

    // Common safety checks for MANUAL and COUNTDOWN
    if (!cfgBypassLevelSensor && !allowStartFromSensors) {
      if (isRunning || pumpMode == "COUNTDOWN") {
        lastFaultCode = (!remoteSensorStable || !levelFreshOk) ? "COMM_LOSS" : "STALE_LEVEL";
        lastFaultMessage = "No fresh/stable level data. Pump stopped (failsafe).";
      }
      if (isRunning) {
        setPump(false);
      }
      if (pumpMode == "COUNTDOWN") {
        isCountdownActive = false; countdownEndMs = 0;
      }
      return;
    }

    if (isLevelSensorError && !cfgBypassLevelSensor) {
      if (isRunning || pumpMode == "COUNTDOWN") {
        LOG(LOG_LEVEL_ERROR, pumpMode.c_str(), "Level sensor error — stopping (fail-safe).");
        lastFaultCode    = "LEVEL_SENSOR";
        lastFaultMessage = "Level sensor offline: pump stopped (fail-safe).";
      }
      if (isRunning) {
        setPump(false);
      }
      if (pumpMode == "COUNTDOWN") {
        isCountdownActive = false; countdownEndMs = 0;
      }
      return;
    }

    if (!cfgBypassLevelSensor && waterLevelPct >= cfgPumpStopLevel) {
      LOG(LOG_LEVEL_INFO, pumpMode.c_str(), "Tank full (%d%%). Stopping pump.", waterLevelPct);
      setPump(false);
      if (pumpMode == "COUNTDOWN") {
        isCountdownActive = false; countdownEndMs = 0;
      }
      return;
    }

    // Minimum off-time check
    if (!isRunning && pumpOffStartMs > 0 && elapsedMillis32(nowMsPump, pumpOffStartMs) < MIN_PUMP_OFF_TIME_MS) {
      return;
    }
    
    setPump(true);
    return;
  }

  if (isSleeping) {
    if (isRunning && waterLevelPct >= cfgPumpStopLevel) {
      LOG(LOG_LEVEL_INFO, "AUTO", "Water at %d%%. Stopping pump.", waterLevelPct);
      setPump(false);
    }
    return;
  }

  if (cfgBypassLevelSensor) {
    return;
  }

  // HARD SAFETY: stale or unstable comm => pump OFF and block start.
  if (!allowStartFromSensors) {
    if (isRunning) {
      LOG(LOG_LEVEL_ERROR, "AUTO", "No fresh/stable level data — stopping pump (fail-safe).");
      lastFaultCode = (!remoteSensorStable || !levelFreshOk) ? "COMM_LOSS" : "STALE_LEVEL";
      lastFaultMessage = "No fresh/stable level data: controller stopped pump (failsafe).";
      setPump(false);
    }
    return;
  }

  if (!isRunning && waterLevelPct <= cfgPumpStartLevel) {
    if (pumpOffStartMs > 0 && elapsedMillis32(nowMsPump, pumpOffStartMs) < MIN_PUMP_OFF_TIME_MS) {
      return;
    }
    LOG(LOG_LEVEL_INFO, "AUTO", "Water at %d%%. Starting pump.", waterLevelPct);
    setPump(true);
  } else if (isRunning && waterLevelPct >= cfgPumpStopLevel) {
    LOG(LOG_LEVEL_INFO, "AUTO", "Water at %d%%. Stopping pump.", waterLevelPct);
    setPump(false);
  }
}
