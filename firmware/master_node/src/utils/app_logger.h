#pragma once
#include <Arduino.h>
#include <vector>
#include "logging/log_sink.h"

class AppLogger : public Stream {
private:
    std::vector<LogSink*> sinks;
public:
    AppLogger() {}

    void initSinks();
    void beginSinks();

    // Register a new sink
    void addSink(LogSink* sink);

    virtual size_t write(uint8_t c) override;
    virtual size_t write(const uint8_t *buffer, size_t size) override;

    // We delegate these to ::Serial for backward compatibility
    // since the global Serial is redefined to app_logger.
    virtual int available() override { return ::Serial.available(); }
    virtual int read() override { return ::Serial.read(); }
    virtual int peek() override { return ::Serial.peek(); }
    virtual void flush() override { ::Serial.flush(); }

    void begin(unsigned long baud) {
        ::Serial.begin(baud);
        beginSinks();
    }

    void handle() {
        for (auto sink : sinks) {
            sink->handle();
        }
    }

    void logEvent(int level, const char* comp, const char* fmt, ...);
};

extern AppLogger app_logger;
