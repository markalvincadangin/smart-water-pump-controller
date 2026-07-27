#pragma once

#include <Arduino.h>

// NodeMCU V2 (ESP8266) production pinout (see hardware/wiring_notes.md)
#define PIN_RS485_DE_RE   14   // D5 / GPIO14
#define PIN_FLOW_INPUT    12   // D6 / GPIO12 (temporary diagnostic reroute)
#define PIN_US_TRIG        5   // D1 / GPIO5
#define PIN_US_ECHO       16   // D0 / GPIO16

#define RS485_BAUD        115200
#define RS485_TX_TURNAROUND_US  60
// Guard time after receiving a complete command before the node drives TX.
// This avoids clipping the first bytes when the master is still switching DE/RE to RX.
#define RS485_RX_TO_TX_GUARD_US  1200

// ----------------------------
// Debug transport selection
// ----------------------------
// 0 = production: RS-485 on UART0, debug on Serial1 (GPIO2 TX-only)
// 1 = debug bench mode: use USB Serial (UART0) for readable logs, disable RS-485 slave traffic
#ifndef DEBUG_USB_MODE
  #define DEBUG_USB_MODE            0
#endif

#if DEBUG_USB_MODE
  #warning "DEBUG_USB_MODE=1: bench debug mode enabled; RS-485 slave traffic is disabled for production safety."
#endif

// Ultrasonic
#define US_TIMEOUT_US       100000UL
#define US_SAMPLES          5
#define US_SAMPLE_DELAY_MS  60

// Tank level mapping (two-point calibration, distance sensor → water surface in cm).
// The HC-SR04 reports distance from the module to the water surface (round-trip time).
// - TANK_US_DIST_FULL_CM: reading when the tank is physically FULL (water close to sensor).
// - TANK_US_DIST_EMPTY_CM: reading when the tank is physically EMPTY (water far from sensor).
// Level% = 100 * (EMPTY - measured) / (EMPTY - FULL), clamped 0–100.
// Calibrated to current field geometry (2026-04-01, corrected 2026-04-03):
// - sensor to tank bottom (empty reference): 120 cm
// - sensor to water surface at full level: 30 cm (matches Firebase/master config)
// User measurement 61cm detected as 55.2cm → applying -5.8cm offset for accuracy
#ifndef TANK_US_DIST_EMPTY_CM
  #define TANK_US_DIST_EMPTY_CM  120.0f
#endif
#ifndef TANK_US_DIST_FULL_CM
  #define TANK_US_DIST_FULL_CM   30.0f
#endif

// Additive trim for installation-specific ultrasonic reference offset.
// Positive values increase reported distance.
// Calibrated 2026-04-03: measured 61cm, sensor read 55.2cm → apply -5.8cm offset
#ifndef TANK_US_DISTANCE_OFFSET_CM
  #define TANK_US_DISTANCE_OFFSET_CM -5.8f
#endif

// JSN-SR04T capability guardrail with safety margin.
// Readings outside this reliability band are treated as invalid samples.
#ifndef US_SENSOR_MIN_CM
  #define US_SENSOR_MIN_CM 20.0f
#endif
#ifndef US_SENSOR_MAX_CM
  #define US_SENSOR_MAX_CM 450.0f
#endif
#ifndef US_SENSOR_MARGIN_CM
  #define US_SENSOR_MARGIN_CM 5.0f
#endif
#define US_RELIABLE_MIN_CM (US_SENSOR_MIN_CM + US_SENSOR_MARGIN_CM)
#define US_RELIABLE_MAX_CM (US_SENSOR_MAX_CM - US_SENSOR_MARGIN_CM)

// Flow calibration (YF-G1 typical).
// Convert pulse frequency (Hz) to liters per minute:
//   flow_lpm = hz / FLOW_HZ_PER_LPM
// This MUST be bucket-calibrated for the installed sensor + plumbing.
#define FLOW_HZ_PER_LPM     7.5f

// ----------------------------
// Hardening parameters
// ----------------------------

// Non-blocking ultrasonic state machine scheduling
#define US_MEAS_INTERVAL_MS     1000UL
#define US_SAMPLE_SPACING_MS      40UL

// Flow signal hardening
// Reject pulses faster than this (deglitch / EMI).
// 5ms => 200Hz (~26.7 L/min at 7.5), which is still above expected field flow
// while strongly suppressing floating-line interrupt storms.
#define FLOW_MIN_PULSE_INTERVAL_US 5000UL
#define FLOW_MAX_SANE_LPM          100.0f
#define FLOW_TIMEOUT_MS            5000UL

// Ultrasonic plausibility (percent points per update)
// Set to 80 to allow full tank swings without rejection (e.g., 11cm drop in ~90cm range = ~12%)
#define LEVEL_MAX_DELTA_PCT        80

// ----------------------------
// Debug output (does not use UART0/RS-485)
// ----------------------------
// ESP8266 Serial1 is TX-only on GPIO2. Use a USB-TTL adapter (RX to GPIO2, GND common)
// to read debug logs while UART0 remains dedicated to RS-485.
#ifndef SENSOR_DEBUG_ENABLED
  #define SENSOR_DEBUG_ENABLED       1
#endif
#ifndef SENSOR_DEBUG_BAUD
  #define SENSOR_DEBUG_BAUD     115200
#endif
#ifndef SENSOR_DEBUG_INTERVAL_MS
  #define SENSOR_DEBUG_INTERVAL_MS 3000UL
#endif

// Syslog configuration
#ifndef SYSLOG_SERVER
  #define SYSLOG_SERVER "255.255.255.255" // Broadcast by default
#endif
#ifndef SYSLOG_PORT
  #define SYSLOG_PORT 514
#endif

#if SENSOR_DEBUG_ENABLED
  #if DEBUG_USB_MODE
    #define SENSOR_DBG_PORT Serial
  #else
    #define SENSOR_DBG_PORT Serial1
  #endif
  extern void send_syslog(const char* msg);
  extern void send_syslog_f(const char* format, ...);
  #define SENSOR_DBGLN(msg) do { SENSOR_DBG_PORT.println(msg); send_syslog(msg); } while (0)
  #define SENSOR_DBGF(...)  do { SENSOR_DBG_PORT.printf(__VA_ARGS__); send_syslog_f(__VA_ARGS__); } while (0)
#else
  #define SENSOR_DBGLN(msg) do {} while (0)
  #define SENSOR_DBGF(...)  do {} while (0)
#endif


