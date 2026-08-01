/**
 * @file safety_pump.h
 * @brief Pump and sensor safety logic.
 *
 * Implements hardware cutoff rules including dry-run protection,
 * sensor stuck detection, and overall fault mitigation.
 */
#pragma once

#include "../config/config.h"

enum class SafetyDecision {
  OK,
  STOP_DRYRUN,
  STOP_OVERFLOW
};

struct OverflowStatus {
  SafetyDecision decision;
  bool nearThreshold;
};

struct SafetyStatus {
  SafetyDecision decision;
  bool overflowNearThreshold;
};

// Relay + pump state
void setPump(bool on);

// Safety checks
void checkLevelSensorFailure(int sensorReading);
void checkFlowSensorStuck();
OverflowStatus checkOverflowProtection();
SafetyDecision checkDryRunProtection();
SafetyStatus checkSafetyCutoff();

