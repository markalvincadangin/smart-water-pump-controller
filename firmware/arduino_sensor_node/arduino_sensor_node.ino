// =============================================================================
// Sensor Node (NodeMCU V2 / ESP8266) — Arduino multi-tab
//
// Modules:
//   `01_config.ino`, `02_sensors.ino`, `03_rs485_slave.ino`
// =============================================================================

#include "sensor_node_shared.h"

void setup() {
  Serial.begin(115200);
  delay(50);
#if SENSOR_DEBUG_ENABLED
#if !DEBUG_USB_MODE
  SENSOR_DBG_PORT.begin(SENSOR_DEBUG_BAUD);
  delay(10);
  SENSOR_DBGLN("\n[SN] Boot: Sensor node debug enabled (Serial1 GPIO2 TX).");
#else
  SENSOR_DBGLN("\n[SN] Boot: DEBUG_USB_MODE enabled (UART0 over USB). RS-485 slave disabled.");
#endif
  SENSOR_DBGF("[SN] RS485 UART0 baud: %lu\n", (unsigned long)RS485_BAUD);
#endif


  pinMode(PIN_RS485_DE_RE, OUTPUT);
  digitalWrite(PIN_RS485_DE_RE, LOW);  // RX by default

  initSensors();
#if !DEBUG_USB_MODE
  initRs485Slave();
#endif
}

void loop() {
#if !DEBUG_USB_MODE
  serviceRs485Slave();
#endif
  serviceSensorsNonBlocking();
  delay(1);
}

