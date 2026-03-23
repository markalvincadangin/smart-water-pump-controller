#pragma once

#include "../config/config.h"

// Firebase config read/write
void readDeviceConfigFromFirebase();
void readFirebaseControl();
void pushFirebaseStatus();
void pushFirebaseErrorLog(const String& level, const String& component, const String& message);

// Countdown helper (invoked from loop prior to pump logic)
void checkCountdownExpiry();

// WiFi + Firebase init helpers
void connectWiFi();
void initFirebase();

// Boot diagnostics
String getBootReasonString();

