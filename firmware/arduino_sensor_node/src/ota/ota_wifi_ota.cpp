#include "ota_wifi.h"

#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>

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

#ifndef OTA_WIFI_CONNECT_TIMEOUT_MS
  #define OTA_WIFI_CONNECT_TIMEOUT_MS 30000UL
#endif

static bool s_need_ota_begin = true;

static void ota_install_handlers() {
  static bool installed = false;
  if (installed) {
    return;
  }
  installed = true;

  ArduinoOTA.setHostname(OTA_HOSTNAME);

  // Macro is always a string literal; use runtime check (empty "" is valid).
  {
    const char* pwd = OTA_UPLOAD_PASSWORD;
    if (pwd != nullptr && pwd[0] != '\0') {
      ArduinoOTA.setPassword(pwd);
    }
  }

  ArduinoOTA.onStart([]() {
#if SENSOR_DEBUG_ENABLED
    SENSOR_DBGF("[SN][INFO] OTA: start\n");
#endif
  });
  ArduinoOTA.onEnd([]() {
#if SENSOR_DEBUG_ENABLED
    SENSOR_DBGF("[SN][INFO] OTA: end - rebooting\n");
#endif
  });
  ArduinoOTA.onError([](ota_error_t e) {
#if SENSOR_DEBUG_ENABLED
    SENSOR_DBGF("[SN][ERR] OTA error: %u\n", (unsigned)e);
#endif
  });
}

void ota_wifi_setup() {
  ota_install_handlers();
  WiFi.mode(WIFI_STA);
  WiFi.hostname(OTA_HOSTNAME);
  WiFi.begin(OTA_WIFI_SSID, OTA_WIFI_PASSWORD);

  const unsigned long deadline = millis() + OTA_WIFI_CONNECT_TIMEOUT_MS;
  while (WiFi.status() != WL_CONNECTED && (long)(deadline - millis()) > 0) {
    delay(250);
    yield();
  }

#if SENSOR_DEBUG_ENABLED
  if (WiFi.status() == WL_CONNECTED) {
    SENSOR_DBGF("[SN][INFO] OTA: Wi-Fi connected.\n");
    SENSOR_DBGF("[SN][INFO] OTA: IP %s  (hostname %s.local)\n", WiFi.localIP().toString().c_str(),
                OTA_HOSTNAME);
  } else {
    SENSOR_DBGF("[SN][INFO] OTA: Wi-Fi not connected yet (will retry; RS-485 still runs).\n");
  }
#endif

  if (WiFi.status() == WL_CONNECTED) {
    ArduinoOTA.begin();
    s_need_ota_begin = false;
#if SENSOR_DEBUG_ENABLED
    SENSOR_DBGF("[SN][INFO] OTA: listener ready (ArduinoOTA / espota).\n");
#endif
  }
}

void ota_wifi_loop() {
  if (WiFi.status() != WL_CONNECTED) {
    s_need_ota_begin = true;
    static unsigned long next_reconnect_ms = 0;
    const unsigned long now = millis();
    if (now >= next_reconnect_ms) {
      next_reconnect_ms = now + 5000UL;
      WiFi.mode(WIFI_STA);
      WiFi.hostname(OTA_HOSTNAME);
      WiFi.begin(OTA_WIFI_SSID, OTA_WIFI_PASSWORD);
#if SENSOR_DEBUG_ENABLED
      SENSOR_DBGF("[SN][INFO] OTA: Wi-Fi reconnect attempt...\n");
#endif
    }
    return;
  }

  if (s_need_ota_begin) {
    ArduinoOTA.begin();
    s_need_ota_begin = false;
#if SENSOR_DEBUG_ENABLED
    SENSOR_DBGF("[SN][INFO] OTA: listener active, IP %s\n", WiFi.localIP().toString().c_str());
#endif
  }

  ArduinoOTA.handle();
}
