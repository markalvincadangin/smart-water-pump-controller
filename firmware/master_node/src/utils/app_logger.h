#pragma once
#include <Arduino.h>

class AppLogger : public Stream {
private:
    String logBuffer;
public:
    AppLogger() { logBuffer.reserve(128); }
    virtual size_t write(uint8_t c) override;
    virtual size_t write(const uint8_t *buffer, size_t size) override;
    
    virtual int available() override { return ::Serial.available(); }
    virtual int read() override { return ::Serial.read(); }
    virtual int peek() override { return ::Serial.peek(); }
    virtual void flush() override { ::Serial.flush(); }
    
    void begin(unsigned long baud) {
        ::Serial.begin(baud);
    }
    
    void logEvent(int level, const char* comp, const char* fmt, ...);
};

extern AppLogger app_logger;
