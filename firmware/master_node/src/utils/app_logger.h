/**
 * @file app_logger.h
 * @brief Application logging subsystem with multi-sink support.
 */
#pragma once
#include <Arduino.h>
#include <vector>
#include "logging/log_sink.h"
#include "log_events.h"

/**
 * @brief Main application logger class.
 */
class AppLogger : public Stream {
private:
  std::vector<LogSink*> sinks;
public:
  AppLogger() {}

  /**
   * @brief Initialize all configured logging sinks.
   */
  void initSinks();

  /**
   * @brief Start all initialized sinks.
   */
  void beginSinks();

  /**
   * @brief Register a new sink.
   * @param sink Pointer to the LogSink to add.
   */
  void addSink(LogSink* sink);

  virtual size_t write(uint8_t c) override;
  virtual size_t write(const uint8_t *buffer, size_t size) override;

  // We delegate these to ::Serial for backward compatibility
  // since the global Serial is redefined to app_logger.
  virtual int available() override { return ::Serial.available(); }
  virtual int read() override { return ::Serial.read(); }
  virtual int peek() override { return ::Serial.peek(); }
  virtual void flush() override { ::Serial.flush(); }

  /**
   * @brief Begin the logger with the specified baud rate (starts Serial and sinks).
   * @param baud The baud rate for the hardware serial port.
   */
  void begin(unsigned long baud) {
    ::Serial.begin(baud);
    beginSinks();
  }

  /**
   * @brief Service loop for all sinks.
   */
  void handle() {
    for (auto sink : sinks) {
      sink->handle();
    }
  }

  /**
   * @brief Log a formatted event locally.
   * @param level The log level (e.g., APP_LOG_LEVEL_INFO).
   * @param comp The component name.
   * @param fmt The format string.
   * @param ... Variable arguments.
   */
  void logEvent(int level, const char* comp, const char* fmt, ...);

  /**
   * @brief Log a formatted event locally and to the cloud.
   * @param level The log level.
   * @param cat The log category.
   * @param code The event code.
   * @param fmt The format string.
   * @param ... Variable arguments.
   */
  void logCloudEvent(int level, LogCategory cat, EventCode code, const char* fmt, ...);
};

extern AppLogger app_logger;
