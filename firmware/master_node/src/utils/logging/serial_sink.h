#pragma once
#include "log_sink.h"
#include <Arduino.h>

class SerialSink : public LogSink {
private:
    HardwareSerial* serialPort;
    unsigned long baudRate;
public:
    SerialSink(HardwareSerial* port, unsigned long baud = 115200);
    virtual void begin() override;
    virtual size_t write(uint8_t c) override;
};
