/**
 * @file log_sink.h
 * @brief Base interface for application logging sinks.
 */
#pragma once
#include <Arduino.h>

/**
 * @brief Abstract base class for all log sinks.
 */
class LogSink {
public:
  virtual ~LogSink() {}

  /**
   * @brief Called once during startup.
   */
  virtual void begin() {}

  /**
   * @brief Called continuously in the main loop.
   */
  virtual void handle() {}

  /**
   * @brief Write a single character to the sink.
   * @param c The character to write.
   * @return Number of bytes written.
   */
  virtual size_t write(uint8_t c) = 0;

  /**
   * @brief Write a buffer to the sink.
   * @param buffer Pointer to the data.
   * @param size Number of bytes.
   * @return Number of bytes written.
   */
  virtual size_t write(const uint8_t *buffer, size_t size) {
    size_t n = 0;
    while (size--) {
      n += write(*buffer++);
    }
    return n;
  }
};
