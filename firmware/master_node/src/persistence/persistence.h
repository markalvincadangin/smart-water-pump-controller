#pragma once

#include "../config/config.h"

void loadDeviceConfigFromNVS();
void saveDeviceConfigToNVS();

bool isInSleepWindow(int currentHour);

void checkCrashLoop();
void loadStateFromNVS();
void persistStateToNVS();

// Clears only local enrollment material. Safety state and configuration remain intact.
bool clearNetworkEnrollment();

