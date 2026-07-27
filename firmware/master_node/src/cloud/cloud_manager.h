#pragma once

#include <Arduino.h>

class CloudManager {
public:
    static void init();
    static void sync();
    
    static void pushEventLog(const String& level, const String& component, const String& message);

private:
    static void pushTelemetry();
    static void pushStatus();
    static void pushShadow();
    static void readShadow();
    static void pushMetadata();
    static void readSettings();
    static void pushDiagnostics();
};
