#include "app_logger.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include "../config/config.h"

// config.h rebinds Serial to app_logger for application code; restore the core symbol here.
#undef Serial

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

static WiFiUDP udpClient;
#if SMARTFLOW_HAS_SYSLOG
static Syslog syslog(udpClient, SYSLOG_SERVER, SYSLOG_PORT, APP_HOSTNAME, "main_controller", LOG_KERN);
#endif

AppLogger app_logger;

size_t AppLogger::write(uint8_t c) {
#if defined(PLATFORMIO)
    Serial.write(c);
#else
    Serial0.write(c);
#endif
    
    // Only buffer if WiFi is connected to avoid memory leaks or useless string ops
    if (WiFi.status() == WL_CONNECTED) {
        logBuffer += (char)c;
        if (c == '\n') {
            if (logBuffer.length() > 0) {
                // Remove trailing \r or \n
                logBuffer.trim();
                if (logBuffer.length() > 0) {
#if SMARTFLOW_HAS_SYSLOG
                    syslog.log(LOG_INFO, logBuffer.c_str());
#endif
                }
                logBuffer = "";
            }
        } else if (logBuffer.length() > 1024) {
            // Prevent runaway buffers
            logBuffer = "";
        }
    }
    return 1;
}

size_t AppLogger::write(const uint8_t *buffer, size_t size) {
    size_t n = 0;
    while (size--) {
        n += write(*buffer++);
    }
    return n;
}
