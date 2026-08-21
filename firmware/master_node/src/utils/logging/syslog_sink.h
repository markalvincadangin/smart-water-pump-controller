/**
 * @file syslog_sink.h
 * @brief UDP Syslog logging sink.
 */
#pragma once
#include "log_sink.h"
#include <WiFi.h>
#include <WiFiUdp.h>

#if defined(__has_include)
#if __has_include(<Syslog.h>)
#include <Syslog.h>
#define SMARTFLOW_HAS_SYSLOG 1
#else
#define SMARTFLOW_HAS_SYSLOG 0
#endif
#else
#define SMARTFLOW_HAS_SYSLOG 0
#endif

/**
 * @brief Log sink that outputs via UDP Syslog.
 */
class SyslogSink : public LogSink {
private:
  WiFiUDP udpClient;
#if SMARTFLOW_HAS_SYSLOG
  Syslog* syslog;
#endif
  String logBuffer;

public:
  SyslogSink(const char* server, uint16_t port, const char* hostname, const char* appName);
  ~SyslogSink();

  virtual void begin() override;
  virtual size_t write(uint8_t c) override;
};
