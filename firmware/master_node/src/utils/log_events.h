/**
 * @file log_events.h
 * @brief Definitions for standard logging categories and event codes.
 */
#pragma once

#include <Arduino.h>

/**
 * Standard Log Categories used across the application.
 * These map to UI groupings in the frontend app.
 */
enum class LogCategory {
  PUMP,
  SENSOR,
  SAFETY,
  NETWORK,
  SYSTEM,
  PROVISIONING
};

/**
 * Standard Event Codes pushed to Firebase.
 * Ensures the Android app can accurately parse and localize events.
 */
enum class EventCode {
  // Pump Events
  EVT_PUMP_ON,
  EVT_PUMP_OFF,
  
  // Sensor Events
  EVT_SENSOR_LEVEL_FAIL,
  EVT_SENSOR_LEVEL_RECOVERED,
  EVT_SENSOR_FLOW_STUCK,
  EVT_SENSOR_FLOW_RECOVERED,
  
  // Safety Lockouts & Warnings
  EVT_DRY_RUN_WARN,
  EVT_DRY_RUN_LOCKOUT,
  EVT_DRY_RUN_CLEARED,
  EVT_MAX_RUNTIME_EXCEEDED,
  EVT_FAIL_SAFE_STOP,
  EVT_AUTO_BYPASS_ENABLED,
  
  // Network / Comm
  EVT_RS485_TIMEOUT,
  EVT_RS485_INVALID,
  EVT_WIFI_DISCONNECTED,
  
  // System Lifecycle
  EVT_CRASH_LOOP_SAFE_MODE,
  EVT_BOOT,
  EVT_CONFIG_RESTORED
};

// Helpers to stringify enums for Firebase transmission
inline const char* getCategoryString(LogCategory cat) {
  switch (cat) {
    case LogCategory::PUMP:         return "PUMP";
    case LogCategory::SENSOR:       return "SENSOR";
    case LogCategory::SAFETY:       return "SAFETY";
    case LogCategory::NETWORK:      return "NETWORK";
    case LogCategory::SYSTEM:       return "SYSTEM";
    case LogCategory::PROVISIONING: return "PROVISIONING";
    default:                        return "UNKNOWN";
  }
}

inline const char* getEventCodeString(EventCode code) {
  switch (code) {
    case EventCode::EVT_PUMP_ON:                return "EVT_PUMP_ON";
    case EventCode::EVT_PUMP_OFF:               return "EVT_PUMP_OFF";
    case EventCode::EVT_SENSOR_LEVEL_FAIL:      return "EVT_SENSOR_LEVEL_FAIL";
    case EventCode::EVT_SENSOR_LEVEL_RECOVERED: return "EVT_SENSOR_LEVEL_RECOVERED";
    case EventCode::EVT_SENSOR_FLOW_STUCK:      return "EVT_SENSOR_FLOW_STUCK";
    case EventCode::EVT_SENSOR_FLOW_RECOVERED:  return "EVT_SENSOR_FLOW_RECOVERED";
    case EventCode::EVT_DRY_RUN_WARN:           return "EVT_DRY_RUN_WARN";
    case EventCode::EVT_DRY_RUN_LOCKOUT:        return "EVT_DRY_RUN_LOCKOUT";
    case EventCode::EVT_DRY_RUN_CLEARED:        return "EVT_DRY_RUN_CLEARED";
    case EventCode::EVT_MAX_RUNTIME_EXCEEDED:   return "EVT_MAX_RUNTIME_EXCEEDED";
    case EventCode::EVT_FAIL_SAFE_STOP:         return "EVT_FAIL_SAFE_STOP";
    case EventCode::EVT_AUTO_BYPASS_ENABLED:    return "EVT_AUTO_BYPASS_ENABLED";
    case EventCode::EVT_RS485_TIMEOUT:          return "EVT_RS485_TIMEOUT";
    case EventCode::EVT_RS485_INVALID:          return "EVT_RS485_INVALID";
    case EventCode::EVT_WIFI_DISCONNECTED:      return "EVT_WIFI_DISCONNECTED";
    case EventCode::EVT_CRASH_LOOP_SAFE_MODE:   return "EVT_CRASH_LOOP_SAFE_MODE";
    case EventCode::EVT_BOOT:                   return "EVT_BOOT";
    case EventCode::EVT_CONFIG_RESTORED:        return "EVT_CONFIG_RESTORED";
    default:                                    return "UNKNOWN_EVENT";
  }
}
