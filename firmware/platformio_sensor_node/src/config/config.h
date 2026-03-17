#pragma once

#include <Arduino.h>

// NodeMCU V2 (ESP8266) production pinout (see hardware/wiring_notes.md)
#define PIN_RS485_DE_RE   14   // D5 / GPIO14
#define PIN_FLOW_INPUT    12   // D6 / GPIO12
#define PIN_US_TRIG        5   // D1 / GPIO5
#define PIN_US_ECHO        4   // D2 / GPIO4

#define RS485_BAUD        115200
#define RS485_TX_TURNAROUND_US  60

// Ultrasonic
#define US_TIMEOUT_US       100000UL
#define US_SAMPLES          5
#define US_SAMPLE_DELAY_MS  60

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
// Reject pulses faster than this (deglitch / EMI). 2ms => 500Hz (~66 L/min at 7.5)
#define FLOW_MIN_PULSE_INTERVAL_US 2000UL
#define FLOW_MAX_SANE_LPM          100.0f
#define FLOW_TIMEOUT_MS            5000UL

// Ultrasonic plausibility (percent points per update)
#define LEVEL_MAX_DELTA_PCT        20

