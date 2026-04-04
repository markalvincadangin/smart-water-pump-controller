#pragma once

#include "../config/config.h"

// Relay + pump state
void setPump(bool on);

// Safety checks
void checkLevelSensorFailure(int sensorReading);
void checkFlowSensorStuck();
void checkOverflowProtection();
void checkDryRunProtection();
void checkSafetyCutoff();

// Main pump state machine (P0–P5)
void executePumpLogic();

