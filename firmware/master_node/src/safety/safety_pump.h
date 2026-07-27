#pragma once

#include "../config/config.h"

enum class SafetyDecision {
  OK,
  STOP_DRYRUN,
  STOP_OVERFLOW
};

// Relay + pump state
void setPump(bool on);

// Safety checks
void checkLevelSensorFailure(int sensorReading);
void checkFlowSensorStuck();
SafetyDecision checkOverflowProtection();
SafetyDecision checkDryRunProtection();
SafetyDecision checkSafetyCutoff();

// Main pump state machine (P0–P5)
void executePumpLogic();

