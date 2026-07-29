#include "syslog_sink.h"

SyslogSink::SyslogSink(const char* server, uint16_t port, const char* hostname, const char* appName) {
    logBuffer.reserve(128);
#if SMARTFLOW_HAS_SYSLOG
    syslog = new Syslog(udpClient, server, port, hostname, appName, LOG_KERN);
#endif
}

SyslogSink::~SyslogSink() {
#if SMARTFLOW_HAS_SYSLOG
    delete syslog;
#endif
}

void SyslogSink::begin() {
    // Syslog is stateless UDP, nothing to begin.
}

size_t SyslogSink::write(uint8_t c) {
    // Only buffer if WiFi is connected to avoid memory leaks or useless string ops
    if (WiFi.status() == WL_CONNECTED) {
        logBuffer += (char)c;
        if (c == '\n') {
            if (logBuffer.length() > 0) {
                // Remove trailing \r or \n
                logBuffer.trim();
                if (logBuffer.length() > 0) {
#if SMARTFLOW_HAS_SYSLOG
                    syslog->log(LOG_INFO, logBuffer.c_str());
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
