/**
 * @file telnet_sink.cpp
 * @brief Telnet (TCP) logging sink implementation.
 */
#include "telnet_sink.h"
#include "../app_logger.h"
#include "../../config/config.h"

TelnetSink::TelnetSink() : server(TELNET_PORT), serverStarted(false), ringHead(0), ringCount(0) {
  currentLine.reserve(128);
}

void TelnetSink::begin() {
  if (WiFi.status() != WL_CONNECTED || serverStarted) return;
  server.begin();
  server.setNoDelay(true);
  serverStarted = true;
}

void TelnetSink::addToRingBuffer(const String& line) {
  ringBuffer[ringHead] = line;
  ringHead = (ringHead + 1) % TELNET_RING_BUFFER_SIZE;
  if (ringCount < TELNET_RING_BUFFER_SIZE) {
    ringCount++;
  }
}

void TelnetSink::replayBuffer() {
  if (!client || !client.connected()) return;

  int startIdx = 0;
  if (ringCount == TELNET_RING_BUFFER_SIZE) {
    startIdx = ringHead;
  }

  client.println("--- Buffered Logs ---");
  for (int i = 0; i < ringCount; i++) {
    int idx = (startIdx + i) % TELNET_RING_BUFFER_SIZE;
    if (client.availableForWrite() > 0) {
      client.println(ringBuffer[idx]);
    }
  }
  client.println("--- Live Logs ---");
}

void TelnetSink::handle() {
  if (WiFi.status() != WL_CONNECTED) {
    if (client) client.stop();
    if (serverStarted) server.end();
    serverStarted = false;
    return;
  }

  begin();
  if (!serverStarted) return;

  // A peer can close a TCP connection without a new client arriving.  Clean
  // that stale socket on every loop, rather than only when hasClient() is
  // true, so the next developer connection is accepted and receives the
  // ring-buffer replay.
  if (client && !client.connected()) {
    client.stop();
  }

  if (server.hasClient()) {
    if (!client) {
      client = server.available();
      client.setNoDelay(true);
      replayBuffer();
      // This records a deterministic live event after the client is
      // attached.  It is useful both to a developer and to the initial
      // TCP-console verification, while exercising the same AppLogger
      // dispatch path as all other firmware events.
      app_logger.logEvent(APP_LOG_LEVEL_INFO, "LOGGER", "TCP console client connected.");
    } else {
      // Reject new client since one is already connected
      WiFiClient newClient = server.available();
      newClient.println("Console already in use.");
      newClient.stop();
    }
  }
}

size_t TelnetSink::write(uint8_t c) {
  return write(&c, 1);
}

size_t TelnetSink::write(const uint8_t* buffer, size_t size) {
  if (!buffer || size == 0) {
    return 0;
  }

  // Preserve a byte-bounded representation of completed log lines for the
  // next developer connection.  This method is deliberately implemented in
  // the concrete sink instead of relying on LogSink's character fallback.
  for (size_t i = 0; i < size; ++i) {
    const uint8_t c = buffer[i];
    currentLine += static_cast<char>(c);
    if (c == '\n') {
      currentLine.trim();
      if (currentLine.length() > 0) {
        addToRingBuffer(currentLine);
      }
      currentLine = "";
    } else if (currentLine.length() > 256) {
      // Prevent runaway memory usage for a malformed stream with no
      // line boundary.
      currentLine = "";
    }
  }

  if (client && client.connected()) {
    return client.write(buffer, size);
  }

  // The record has been retained for replay even without a live client.
  return size;
}
