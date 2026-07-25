#include <Arduino.h>

#include "config/config.h"
#include "state/state.h"
#include "sensors/sensors.h"
#include "rs485/rs485_slave.h"
#include "ota/ota_wifi.h"
#include "utils/syslog_helper.h"

void setup() {
  Serial.begin(RS485_BAUD);
  delay(50);
#if SENSOR_DEBUG_ENABLED
#if !DEBUG_USB_MODE
  SENSOR_DBG_PORT.begin(SENSOR_DEBUG_BAUD);
  delay(10);
  SENSOR_DBGF("[SN][INFO] Boot: Sensor node debug enabled (Serial1 GPIO2 TX).\n");
#else
  SENSOR_DBGF("[SN][INFO] Boot: DEBUG_USB_MODE enabled (UART0 over USB). RS-485 slave disabled.\n");
#endif
  SENSOR_DBGF("[SN][INFO] RS485 UART0 baud: %lu\n", (unsigned long)RS485_BAUD);
#endif

#if !DEBUG_USB_MODE
  rs485_slave_init();
#endif
  ota_wifi_setup();
  syslog_init();
  sensors_init();
}

void loop() {
#if !DEBUG_USB_MODE
  rs485_slave_poll();
#endif
  sensors_update_nonblocking();
  ota_wifi_loop();
  delay(1);
}
