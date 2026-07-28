#include "app_logger.h"
#include "../config/config.h"

// config.h rebinds Serial to app_logger for application code; restore the core symbol here.
#undef Serial

#include <stdarg.h>
#include "../cloud/cloud_manager.h"

#if ENABLE_SERIAL_LOG
#include "logging/serial_sink.h"
SerialSink serialSink(&Serial);
#endif

#if ENABLE_TELNET_LOG
#include "logging/telnet_sink.h"
TelnetSink telnetSink;
#endif

#if ENABLE_SYSLOG_LOG
#include "logging/syslog_sink.h"
SyslogSink syslogSink(SYSLOG_SERVER, SYSLOG_PORT, APP_HOSTNAME, "main_controller");
#endif

AppLogger app_logger;
static bool sinksStarted = false;

void AppLogger::initSinks() {
#if ENABLE_SERIAL_LOG
    addSink(&serialSink);
#endif
#if ENABLE_TELNET_LOG
    addSink(&telnetSink);
#endif
#if ENABLE_SYSLOG_LOG
    addSink(&syslogSink);
#endif
}

void AppLogger::beginSinks() {
    if (sinksStarted) return;
    if (sinks.empty()) return;
    for (auto sink : sinks) {
        sink->begin();
    }
    sinksStarted = true;
    logEvent(APP_LOG_LEVEL_INFO, "LOGGER", "Log sinks initialized.");
}

void AppLogger::addSink(LogSink* sink) {
    if (sink) {
        sinks.push_back(sink);
    }
}

size_t AppLogger::write(uint8_t c) {
    size_t written = 1;
    for (auto sink : sinks) {
        sink->write(c);
    }
    return written;
}

size_t AppLogger::write(const uint8_t *buffer, size_t size) {
    size_t n = 0;
    while (size--) {
        n += write(*buffer++);
    }
    return n;
}

void AppLogger::logEvent(int level, const char* comp, const char* fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    // Get timestamp
    unsigned long ms = millis();
    unsigned long s = ms / 1000;
    unsigned long h = s / 3600;
    unsigned long m = (s % 3600) / 60;
    s = s % 60;
    ms = ms % 1000;

    this->printf("[%02lu:%02lu:%02lu.%03lu] [%s] %s\n", h, m, s, ms, comp, buf);
    
    // Only push to cloud if it's an error or warning, to avoid spamming the events node.
    if (level <= APP_LOG_LEVEL_WARN) {
        String levelStr = (level == APP_LOG_LEVEL_ERROR) ? "ERROR" : "WARN";
        CloudManager::pushEventLog(levelStr, comp, buf);
    }
}
