#pragma once

#include "../config/config.h"

// Firebase config read/write
void readDeviceConfigFromFirebase();
bool readFirebaseControl();
bool pushFirebaseStatus();
void pushFirebaseErrorLog(const String& level, const String& component, const String& message);

// WiFi + Firebase init helpers
void connectWiFi();
void initFirebase();

// Boot diagnostics
String getBootReasonString();
