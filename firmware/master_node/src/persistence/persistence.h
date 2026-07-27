#pragma once

#include "../config/config.h"

void loadDeviceConfigFromNVS();
void saveDeviceConfigToNVS();

bool isInSleepWindow(int currentHour);

void checkCrashLoop();
void loadStateFromNVS();
void persistStateToNVS();

