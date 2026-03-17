#pragma once

#include <Arduino.h>

// -----------------------------------------------------------------------------
// Production pinout (see hardware/wiring_notes.md)
// -----------------------------------------------------------------------------
// NodeMCU V2 (ESP8266)
// - RS-485 UART: TX=GPIO1, RX=GPIO3 (Serial)
// - RS-485 DE/RE (tied): D5 = GPIO14
// - Flow input: D6 = GPIO12 (interrupt capable)
// - Ultrasonic: TRIG D1=GPIO5, ECHO D2=GPIO4 (ECHO MUST be level shifted to 3.3V)

#define PIN_RS485_DE_RE   14   // D5
#define PIN_FLOW_INPUT    12   // D6
#define PIN_US_TRIG        5   // D1
#define PIN_US_ECHO        4   // D2

#define RS485_BAUD        115200
#define RS485_TX_TURNAROUND_US  60

// Ultrasonic
#define US_TIMEOUT_US     100000UL  // 100ms
#define US_SAMPLES        5
#define US_SAMPLE_DELAY_MS  60

// Flow calibration (YF-G1 typical).
// Convert pulse frequency (Hz) to liters per minute:
//   flow_lpm = hz / FLOW_HZ_PER_LPM
// This MUST be bucket-calibrated for the installed sensor + plumbing.
#define FLOW_HZ_PER_LPM   7.5f

// ----------------------------
// Hardening parameters
// ----------------------------
#define US_MEAS_INTERVAL_MS        1000UL
#define US_SAMPLE_SPACING_MS         40UL

#define FLOW_MIN_PULSE_INTERVAL_US 2000UL
#define FLOW_MAX_SANE_LPM          100.0f
#define FLOW_TIMEOUT_MS            5000UL

#define LEVEL_MAX_DELTA_PCT        20

// ERR codes for response (LVL/ FLOW still sent even if ERR != 0)
// 0 = OK
// 1 = ultrasonic error (timeout / invalid)
// 2 = flow error (noise / impossible)
// 3 = both
extern volatile uint32_t flowPulseCount;
extern volatile uint32_t flowPulseDiscardCount;
extern volatile uint32_t flowLastPulseUs;

extern int   snWaterLevelPct;     // 0..100
extern float snLastDistanceCm;    // last good median distance (cm), or <0 if unknown
extern float snFlowRateLpm;       // >=0
extern int   snErrCode;           // 0..3
extern uint32_t snLastLevelUpdateMs;
extern uint32_t snLastFlowUpdateMs;
extern int      snLastGoodLevelPct;
extern bool     snLevelError;
extern bool     snFlowError;
extern uint8_t  snSeq;

void initSensors();
void serviceSensorsNonBlocking();

void initRs485Slave();
void serviceRs485Slave();

