#include "syslog_helper.h"
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Syslog.h>
#include "../config/config.h"
#include "../config/secrets_ota.h"

#if SENSOR_DEBUG_ENABLED

static WiFiUDP udpClient;
static Syslog syslog(udpClient, SYSLOG_SERVER, SYSLOG_PORT, OTA_HOSTNAME, "sensor_node", LOG_KERN);

void syslog_init() {
  if (WiFi.status() == WL_CONNECTED) {
    char msg[96];
    snprintf(msg, sizeof(msg), "Syslog target %s:%u", SYSLOG_SERVER, (unsigned)SYSLOG_PORT);
    syslog.log(LOG_INFO, msg);
  }
}

void send_syslog(const char* msg) {
  if (WiFi.status() == WL_CONNECTED) {
    syslog.log(LOG_INFO, msg);
  }
}

void send_syslog_f(const char* format, ...) {
  if (WiFi.status() == WL_CONNECTED) {
    char buf[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    syslog.log(LOG_INFO, buf);
  }
}

#endif // SENSOR_DEBUG_ENABLED
