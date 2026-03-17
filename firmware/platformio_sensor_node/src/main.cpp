#include <Arduino.h>

#include "config/config.h"
#include "state/state.h"
#include "sensors/sensors.h"
#include "rs485/rs485_slave.h"

void setup() {
  Serial.begin(RS485_BAUD);
  delay(50);

  rs485_slave_init();
  sensors_init();
}

void loop() {
  rs485_slave_poll();
  sensors_update_nonblocking();
  delay(1);
}
