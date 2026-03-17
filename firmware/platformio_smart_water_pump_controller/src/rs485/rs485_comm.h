#pragma once

#include "../config/config.h"

struct Rs485SensorData {
  int   waterLevelPct;   // 0..100
  float flowRateLpm;     // >=0
  int   errCode;         // protocol ERR:<code>
  bool  online;          // derived from last receive age
};

// Initialize UART2 + DE/RE pin for RS-485.
void rs485_init();

// Send request + parse response (with retry). Updates global state and returns true on fresh frame.
bool rs485_requestData();

// Returns the most recently parsed data (may be stale if offline).
Rs485SensorData rs485_getParsedData();

