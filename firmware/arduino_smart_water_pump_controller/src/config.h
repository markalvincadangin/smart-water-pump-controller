#pragma once

// Central compile-time configuration for ESP32 master.
// This file intentionally keeps macro-based settings aligned with the Arduino tab build.

#include <Arduino.h>

// ---- Core libraries ----
#include <WiFi.h>
#include <Preferences.h>
#include <math.h>
#include <esp_task_wdt.h>
#include <esp_system.h>
#include <esp_sleep.h>
#include <time.h>
#include <esp_timer.h>

// ---- Firebase ----
#include <Firebase_ESP_Client.h>
// NOTE: Do not include Firebase add-on helper headers here; they contain
// non-inline function definitions and will cause multiple-definition link errors
// when pulled into multiple translation units. Include them in ONE .cpp instead.

// ---- Credentials ----
// Copy `secrets.h.example` → `secrets.h` and fill in WiFi + Firebase credentials.
// Never commit `secrets.h`.
#if defined(__has_include)
	#if __has_include("secrets.h")
		#include "secrets.h"
	#else
		#include "secrets.h.example"
	#endif
#else
	#include "secrets.h.example"
#endif

// ---- GPIO mapping ----
#define RELAY_PIN        4

// RS-485 (Tank Link) — production pinout (see hardware/wiring_notes.md)
// ESP32 UART2: TX2=GPIO17, RX2=GPIO25. Half-duplex DE/RE tied to one GPIO.
#define RS485_UART_BAUD     115200
#define RS485_TX_PIN        17
#define RS485_RX_PIN        25
#define RS485_DE_RE_PIN      5   // LOW=RX, HIGH=TX

// ---- Tank calibration ----
#define TANK_EMPTY_CM   122
#define TANK_FULL_CM     8

// AUTO mode hysteresis thresholds (percent)
#define PUMP_START_LEVEL  30
#define PUMP_STOP_LEVEL  100

// ---- Safety + timing ----
#define DRY_RUN_THRESHOLD_LPM  1.0f
#define DRY_RUN_TIMEOUT_MS     30000

#define FLOW_CALIBRATION_FACTOR  7.5f
#define MAX_PUMP_RUNTIME_MIN     120

#define SENSOR_FAILURE_THRESHOLD  5
#define FLOW_STUCK_THRESHOLD_LPM  2.0f
#define FLOW_STUCK_TIMEOUT_MS     5000
#define FLOW_MAX_SANE_LPM         100.0f
#define FLOW_PUMP_OFF_ZERO_MS     3000
#define LEVEL_RATE_OF_CHANGE_MAX  15

#define ULTRASONIC_SAMPLES        5
#define ULTRASONIC_SAMPLE_DELAY   80
#define ULTRASONIC_EMA_ALPHA      0.5f

#define SENSOR_INTERVAL_MS         1000
#define FIREBASE_INTERVAL_MS       3000
#define FIREBASE_AUTH_COOLDOWN_MS  30000UL
#define DEVICE_CONFIG_INTERVAL_MS  30000
#define ULTRASONIC_TIMEOUT_MS      100

// RS-485 poll + framing
#define RS485_REQ_INTERVAL_MS      1000
#define RS485_FRAME_TIMEOUT_MS     350
#define RS485_MAX_RETRIES          3
#define RS485_RX_LINE_MAX          160
#define REMOTE_SENSOR_OFFLINE_MS   5000UL
#define RS485_TX_TURNAROUND_US     80   // DE/RE settle time; tune for cable/transceiver

// Startup
#define STARTUP_STABILIZE_MS       1000UL

// Level freshness hard safety
#define LEVEL_STALE_TIMEOUT_MS     2500UL

// Remote sensor stability latch
#define REMOTE_STABLE_ONLINE_N     3
#define REMOTE_STABLE_OFFLINE_N    3

// NVS namespaces + schema
#define NVS_NAMESPACE        "pump_cfg"
#define NVS_STATE_NAMESPACE  "pump_state"
#define NVS_SCHEMA_VERSION   1

// Crash loop detection
#define CRASH_LOOP_THRESHOLD    5
#define CRASH_LOOP_WINDOW_SEC   300
#define SAFE_MODE_TIMEOUT_MS    3600000UL

// WiFi exponential backoff
#define WIFI_BACKOFF_INITIAL_MS 5000
#define WIFI_BACKOFF_MAX_MS     60000
#define WIFI_JITTER_MS          2000

// Watchdog
// Note: With weak WiFi, Firebase RTDB reads can still block long enough to starve loopTask.
// Keep this comfortably above worst-case network stalls to avoid crash-loop safe mode.
#define WDT_TIMEOUT_SEC         120

// Status push retry
#define STATUS_PUSH_RETRY_MAX   3
#define STATUS_PUSH_RETRY_MS    1000

// NVS wear reduction
#define NVS_LEVEL_DELTA_THRESHOLD 5
#define NVS_LEVEL_INTERVAL_MS     300000UL
#define NVS_UPTIME_INTERVAL_MS    60000UL

// Scheduled sleep (light sleep)
#define SLEEP_DEFAULT_ENABLED       false
#define SLEEP_DEFAULT_START_HOUR    23
#define SLEEP_DEFAULT_END_HOUR      5
#define SLEEP_DEFAULT_EMERGENCY_LVL 5
#define SLEEP_WAKE_INTERVAL_MS      30000UL

// Idle slow-poll mode (outside sleep hours)
#define IDLE_LEVEL_THRESHOLD          90
#define IDLE_STABLE_TIME_MS           300000UL
#define IDLE_SENSOR_INTERVAL_MS_DEF   10000
#define IDLE_FIREBASE_INTERVAL_MS_DEF 30000

// COUNTDOWN mode (semi-automatic timed run)
#define COUNTDOWN_ADD_TIME_MIN       5
#define COUNTDOWN_MAX_DURATION_MIN   120

// Sensor resilience
#define TANK_CAPACITY_L              660   // Bestank WT660
#define AUTO_BYPASS_FAILURE_SEC_DEF  60

// Minimum off-time between pump starts (motor protection)
#define MIN_PUMP_OFF_TIME_MS         30000

// Syslog configuration
#ifndef APP_HOSTNAME
	#define APP_HOSTNAME "smartflow-controller"
#endif

#ifndef SYSLOG_SERVER
	#define SYSLOG_SERVER "255.255.255.255"
#endif

#ifndef SYSLOG_PORT
	#define SYSLOG_PORT 514
#endif



// Redirect all Serial prints in the application to our AppLogger (which routes to HardwareSerial + UDP Syslog)
#include "../utils/app_logger.h"
#define Serial app_logger

// Logging levels and shared logger macro.
#define LOG_LEVEL_ERROR   0
#define LOG_LEVEL_WARN    1
#define LOG_LEVEL_INFO    2
#define LOG_LEVEL_DEBUG   3
#define LOG_LEVEL_VERBOSE 4

// Keep verbose level compiled in so debug_log_level can raise verbosity at runtime.
#define LOG_COMPILE_FLOOR LOG_LEVEL_VERBOSE

extern uint8_t gLogLevel;

#define LOG(level, comp, fmt, ...) do { \
  if ((level) <= LOG_COMPILE_FLOOR && (level) <= gLogLevel) { \
    app_logger.printf("[%s] " fmt "\n", comp, ##__VA_ARGS__); \
  } \
} while(0)

