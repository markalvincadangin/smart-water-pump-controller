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
  Serial.println("\n====================================");
  Serial.println(" Sensor Node (ESP8266) - RS485 Slave");
  Serial.println("====================================");

  pinMode(PIN_RS485_DE_RE, OUTPUT);
  digitalWrite(PIN_RS485_DE_RE, LOW);  // RX by default

  initSensors();
  initRs485Slave();
}

void loop() {
  serviceRs485Slave();
  serviceSensorsNonBlocking();
  delay(1);
}

