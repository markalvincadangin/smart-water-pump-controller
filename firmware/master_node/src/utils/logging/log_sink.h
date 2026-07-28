#pragma once
#include <Arduino.h>

class LogSink {
public:
    virtual ~LogSink() {}

    // Called once during startup
    virtual void begin() {}

    // Called continuously in the main loop
    virtual void handle() {}

    // Write a single character to the sink
    virtual size_t write(uint8_t c) = 0;

    // Write a buffer to the sink
    virtual size_t write(const uint8_t *buffer, size_t size) {
        size_t n = 0;
        while (size--) {
            n += write(*buffer++);
        }
        return n;
    }
};
