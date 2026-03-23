#pragma once

#include <Arduino.h>

// -----------------------------------------------------------------------------
// Production pinout (see hardware/wiring_notes.md)
// -----------------------------------------------------------------------------
// NodeMCU V2 (ESP8266)
// - RS-485 UART: TX=GPIO1, RX=GPIO3 (Serial)
// - RS-485 DE/RE (tied): D5 = GPIO14
// - Flow input: D7 = GPIO13 (interrupt capable)
// - Ultrasonic: TRIG D1=GPIO5, ECHO D0=GPIO16 (ECHO MUST be level shifted to 3.3V)

#define PIN_RS485_DE_RE   14   // D5
#define PIN_FLOW_INPUT    13   // D7
#define PIN_US_TRIG        5   // D1
#define PIN_US_ECHO       16   // D0

#define RS485_BAUD        115200
#define RS485_TX_TURNAROUND_US  60
#ifndef APP_HOSTNAME
  #define APP_HOSTNAME "smartflow-sensor"
#endif

// ----------------------------
// Debug transport selection
// ----------------------------
// 0 = production: RS-485 on UART0, debug on Serial1 (GPIO2 TX-only)
// 1 = debug bench mode: use USB Serial (UART0) for readable logs, disable RS-485 slave traffic
#define DEBUG_USB_MODE              0

// Ultrasonic
#define US_TIMEOUT_US     100000UL  // 100ms
#define US_SAMPLES        5
#define US_SAMPLE_DELAY_MS  60

// Tank level: distance (cm) sensor → water at EMPTY vs FULL (two-point calibration).
#define TANK_US_DIST_EMPTY_CM  122.0f
#define TANK_US_DIST_FULL_CM    8.0f

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

// ----------------------------
// Debug output (does not use UART0/RS-485)
// ----------------------------
// ESP8266 Serial1 is TX-only on GPIO2. Use USB-TTL RX on GPIO2 + common GND.
#define SENSOR_DEBUG_ENABLED         1
#define SENSOR_DEBUG_BAUD       115200
#define SENSOR_DEBUG_INTERVAL_MS 3000UL

#if SENSOR_DEBUG_ENABLED
  #if DEBUG_USB_MODE
    #define SENSOR_DBG_PORT Serial
  #else
    #define SENSOR_DBG_PORT Serial1
  #endif
  #define SENSOR_DBGLN(msg) SENSOR_DBG_PORT.println(msg)
  #define SENSOR_DBGF(...)  SENSOR_DBG_PORT.printf(__VA_ARGS__)
#else
  #define SENSOR_DBGLN(msg) do {} while (0)
  #define SENSOR_DBGF(...)  do {} while (0)
#endif

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

