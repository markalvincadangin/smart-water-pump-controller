#include "syslog_helper.h"
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "../config/config.h"
#if defined(__has_include)
  #if __has_include("../config/secrets_ota.h")
  #include "../config/secrets_ota.h"
  #else
  #include "../config/secrets_ota.h.example"
  #endif
#else
  #include "../config/secrets_ota.h.example"
#endif

#if defined(__has_include)
#if __has_include(<Syslog.h>)
#include <Syslog.h>
#define SMARTFLOW_HAS_SYSLOG 1
#else
#define SMARTFLOW_HAS_SYSLOG 0
#endif
#else
#define SMARTFLOW_HAS_SYSLOG 0
#endif

#if SENSOR_DEBUG_ENABLED

static WiFiUDP udpClient;
#if SMARTFLOW_HAS_SYSLOG
static Syslog syslog(udpClient, SYSLOG_SERVER, SYSLOG_PORT, OTA_HOSTNAME, "sensor_node", LOG_KERN);
#endif

void syslog_init() {
  if (WiFi.status() == WL_CONNECTED) {
  char msg[96];
  snprintf(msg, sizeof(msg), "Syslog target %s:%u", SYSLOG_SERVER, (unsigned)SYSLOG_PORT);
#if SMARTFLOW_HAS_SYSLOG
  syslog.log(LOG_INFO, msg);
#endif
  }
}

void send_syslog(const char* msg) {
  if (WiFi.status() == WL_CONNECTED) {
#if SMARTFLOW_HAS_SYSLOG
  syslog.log(LOG_INFO, msg);
#endif
  }
}

void send_syslog_f(const char* format, ...) {
  if (WiFi.status() == WL_CONNECTED) {
  char buf[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);
#if SMARTFLOW_HAS_SYSLOG
  syslog.log(LOG_INFO, buf);
#endif
  }
}

#endif // SENSOR_DEBUG_ENABLED
