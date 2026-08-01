/**
 * @file telnet_sink.h
 * @brief Telnet (TCP) logging sink.
 */
#pragma once
#include "log_sink.h"
#include <WiFi.h>

#define TELNET_PORT 2323
#define TELNET_RING_BUFFER_SIZE 50

/**
 * @brief Log sink that outputs via a TCP Telnet server with ring buffer.
 */
class TelnetSink : public LogSink {
private:
  WiFiServer server;
  WiFiClient client;
  bool serverStarted;

  String ringBuffer[TELNET_RING_BUFFER_SIZE];
  int ringHead;
  int ringCount;
  String currentLine;

  void addToRingBuffer(const String& line);
  void replayBuffer();

public:
  TelnetSink();
  virtual void begin() override;
  virtual void handle() override;
  virtual size_t write(uint8_t c) override;
  virtual size_t write(const uint8_t* buffer, size_t size) override;
};
