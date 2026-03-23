#include "app_logger.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Syslog.h>
#include "../config/config.h"

static WiFiUDP udpClient;
static Syslog syslog(udpClient, SYSLOG_SERVER, SYSLOG_PORT, APP_HOSTNAME, "main_controller", LOG_KERN);

AppLogger app_logger;

// undefine Serial in case it was redefined in config.h so we can use the original HardwareSerial
#undef Serial

size_t AppLogger::write(uint8_t c) {
    ::Serial.write(c);
    
    // Only buffer if WiFi is connected to avoid memory leaks or useless string ops
    if (WiFi.status() == WL_CONNECTED) {
        logBuffer += (char)c;
        if (c == '\n') {
            if (logBuffer.length() > 0) {
                // Remove trailing \r or \n
                logBuffer.trim();
                if (logBuffer.length() > 0) {
                    syslog.log(LOG_INFO, logBuffer.c_str());
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
