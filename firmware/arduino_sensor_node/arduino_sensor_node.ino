// =============================================================================
// SmartFlow — Sensor Node (NodeMCU V2 / ESP8266) — Arduino multi-tab
//
// Modules:
//   01_config.ino, 02_sensors.ino, 03_rs485_slave.ino
// =============================================================================

#include "sensor_node_shared.h"

void setup() {
#if DEBUG_USB_MODE == 1
  // Bench mode: USB Serial for all output; RS-485 slave is disabled.
  Serial.begin(115200);
  while (!Serial && millis() < 3000) delay(10);
  snLogLevel = LOG_DEBUG;  // Auto-verbose in bench mode
  LOG_SN(LOG_INFO, "BOOT", "DEBUG_USB_MODE=1 — RS-485 DISABLED. USB Serial active.");
#else
  // Production mode: UART0 for RS-485, Serial1 (GPIO2) for debug output.
  Serial.begin(RS485_BAUD);     // UART0 → RS-485
  delay(50);
  SN_SERIAL_DEBUG.begin(SENSOR_DEBUG_BAUD);  // Serial1 GPIO2 → USB-TTL for debug
  delay(10);
  LOG_SN(LOG_INFO, "BOOT", "RS-485 UART0 baud=%lu. Debug on GPIO2 (Serial1).", (unsigned long)RS485_BAUD);
#endif

  pinMode(PIN_RS485_DE_RE, OUTPUT);
  digitalWrite(PIN_RS485_DE_RE, LOW);  // RX by default

  initSensors();
#if DEBUG_USB_MODE != 1
  initRs485Slave();
#endif

  LOG_SN(LOG_INFO, "BOOT", "Sensor node init complete. snLogLevel=%d.", (int)snLogLevel);
}

void loop() {
#if DEBUG_USB_MODE != 1
  serviceRs485Slave();
#endif
  serviceSensorsNonBlocking();
  delay(1);
}
