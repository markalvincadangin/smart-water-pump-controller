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
    if (!buffer || size == 0) {
        return 0;
    }

    // Dispatch the complete record to every sink.  Besides avoiding one virtual
    // call per character, this makes the logger's buffer-oriented contract
    // explicit: sinks receive the same bytes in the same order.
    for (auto sink : sinks) {
        sink->write(buffer, size);
    }
    return size;
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

    // Do not use Print::printf() here.  Some Arduino core builds route that
    // helper through a non-overridden Print path, which bypasses registered
    // sinks.  Formatting first and explicitly calling this class's write()
    // guarantees Serial, TCP, and future sinks observe the identical record.
    char entry[384];
    const int entryLength = snprintf(
        entry,
        sizeof(entry),
        "[%02lu:%02lu:%02lu.%03lu] [%s] %s\n",
        h,
        m,
        s,
        ms,
        comp ? comp : "LOG",
        buf);
    if (entryLength > 0) {
        const size_t bytesToWrite = static_cast<size_t>(
            entryLength < static_cast<int>(sizeof(entry))
                ? entryLength
                : sizeof(entry) - 1);
        write(reinterpret_cast<const uint8_t*>(entry), bytesToWrite);
    }
}
void AppLogger::logCloudEvent(int level, LogCategory cat, EventCode code, const char* fmt, ...) {
    char details[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(details, sizeof(details), fmt, args);
    va_end(args);

    // 1. Format the standard text for local serial debugging
    const char* catStr = getCategoryString(cat);
    const char* codeStr = getEventCodeString(code);
    
    char buf[768];
    snprintf(buf, sizeof(buf), "[%s] %s", codeStr, details);
    
    // Log locally
    logEvent(level, catStr, buf);
    
    // 2. Push explicitly to the cloud with the structured code
    String levelStr = (level == APP_LOG_LEVEL_ERROR) ? "ERROR" : (level == APP_LOG_LEVEL_WARN ? "WARN" : "INFO");
    CloudManager::pushCloudEvent(levelStr, catStr, codeStr, details);
}
