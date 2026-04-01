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

// Compile-time mode select (must be defined BEFORE including this header, or here)
#ifndef DEBUG_USB_MODE
  #define DEBUG_USB_MODE 0   // 0 = production (RS-485 on UART0, debug on Serial1 GPIO2)
#endif                        // 1 = bench mode (USB Serial UART0, RS-485 disabled)

// Ultrasonic
#define US_TIMEOUT_US     100000UL  // 100ms
#define US_SAMPLES        5
#define US_SAMPLE_DELAY_MS  60

// Tank level: distance (cm) — two-point calibration.
#define TANK_US_DIST_EMPTY_CM  122.0f
#define TANK_US_DIST_FULL_CM    8.0f

// Flow calibration (YF-G1 typical).
// flow_lpm = pulse_hz / FLOW_HZ_PER_LPM  (must be bucket-calibrated)
#define FLOW_HZ_PER_LPM   7.5f

// Hardening parameters
#define US_MEAS_INTERVAL_MS        1000UL
#define US_SAMPLE_SPACING_MS         40UL
#define FLOW_MIN_PULSE_INTERVAL_US 2000UL
#define FLOW_MAX_SANE_LPM          100.0f
#define FLOW_TIMEOUT_MS            5000UL
#define LEVEL_MAX_DELTA_PCT        20
#define SENSOR_DEBUG_INTERVAL_MS   3000UL
#define SENSOR_DEBUG_BAUD         115200


// =============================================================================
// DEBUG_USB_MODE selects the physical transport:
//   0 (default/production): RS-485 on UART0, debug output on Serial1 (GPIO2 TX-only)
//   1 (bench/flash mode):   USB Serial (UART0) for all output; RS-485 slave DISABLED.
//
// When DEBUG_USB_MODE=1, a #warning reminds the developer to disconnect MAX485.
// =============================================================================

// Compile-time transport routing
#if DEBUG_USB_MODE == 1
  #warning "DEBUG_USB_MODE=1: RS-485 DISABLED. Disconnect DI/RO from NodeMCU TX/RX before flashing."
  #define SN_SERIAL_DEBUG Serial     // USB Serial — readable output
  // SN_SERIAL_RS485 intentionally undefined — RS-485 code must guard with !DEBUG_USB_MODE
#else
  #define SN_SERIAL_DEBUG Serial1    // GPIO2 TX-only — USB-TTL adapter RX → GPIO2, shared GND
  #define SN_SERIAL_RS485 Serial     // UART0 GPIO1/3 — to MAX485 module
#endif

// Sensor node log compile floor (same levels as ESP32 side)
#ifndef LOG_ERROR
  #define LOG_ERROR   0
  #define LOG_WARN    1
  #define LOG_INFO    2
  #define LOG_DEBUG   3
  #define LOG_VERBOSE 4
#endif

#ifndef SN_LOG_COMPILE_FLOOR
  #define SN_LOG_COMPILE_FLOOR LOG_DEBUG
#endif

extern uint8_t snLogLevel;   // initialized to LOG_INFO in 01_config.ino

#ifndef LOG_LEVEL_CHAR
static const char LOG_LEVEL_CHAR[] = { 'E', 'W', 'I', 'D', 'V' };
#endif

// LOG_SN() macro — compile-time + runtime gated. Routes to SN_SERIAL_DEBUG.
// REFACTOR [H-01]: replaces SENSOR_DBGF/SENSOR_DBGLN.
#define LOG_SN(level, module, fmt, ...) \
  do { \
    if ((level) <= SN_LOG_COMPILE_FLOOR && (level) <= snLogLevel) { \
      SN_SERIAL_DEBUG.printf("[%c][%s][%010lu] " fmt "\n", \
        LOG_LEVEL_CHAR[(level) <= 4 ? (level) : 4], (module), millis(), ##__VA_ARGS__); \
    } \
  } while(0)

// Legacy compat aliases (will be removed in Phase 2 cleanup)
#define SENSOR_DBGLN(msg)   LOG_SN(LOG_DEBUG, "SENSOR", "%s", (msg))
#define SENSOR_DBGF(...)    LOG_SN(LOG_DEBUG, "SENSOR", __VA_ARGS__)

// ERR codes for response (LVL/ FLOW still sent even if ERR != 0)
// 0 = OK
// 1 = ultrasonic error (timeout / invalid)
// 2 = flow error (noise / impossible)
// 3 = both
extern volatile uint32_t flowPulseCount;
extern volatile uint32_t flowPulseDiscardCount;
extern volatile uint32_t flowLastPulseUs;

// Phase 2 state (H-02, H-04)
extern uint16_t snLevelDiscardCount;  // H-02: plausibility-rejected readings this window
extern uint8_t  flowErrAssertCount;   // H-04: consecutive seconds above assert threshold
extern uint8_t  flowErrClearCount;    // H-04: consecutive seconds below clear threshold

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

