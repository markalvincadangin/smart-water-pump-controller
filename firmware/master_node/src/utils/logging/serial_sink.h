/**
 * @file serial_sink.h
 * @brief Hardware serial logging sink.
 */
#pragma once
#include "log_sink.h"
#include <Arduino.h>

/**
 * @brief Log sink that outputs to a HardwareSerial port.
 */
class SerialSink : public LogSink {
private:
  HardwareSerial* serialPort;
  unsigned long baudRate;
public:
  SerialSink(HardwareSerial* port, unsigned long baud = 115200);
  virtual void begin() override;
  virtual size_t write(uint8_t c) override;
};
