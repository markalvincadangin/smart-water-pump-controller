#pragma once

#include "../config/config.h"

// Check for countdown expiry
void app_checkCountdownExpiry();

// Main application state machine (P0–P5)
void app_executePumpLogic();
