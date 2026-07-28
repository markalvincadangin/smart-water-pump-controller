#include "telnet_sink.h"

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

    if (server.hasClient()) {
        if (!client || !client.connected()) {
            if (client) client.stop();
            client = server.available();
            client.setNoDelay(true);
            replayBuffer();
        } else {
            // Reject new client since one is already connected
            WiFiClient newClient = server.available();
            newClient.println("Console already in use.");
            newClient.stop();
        }
    }
}

size_t TelnetSink::write(uint8_t c) {
    // Add to current line buffer
    currentLine += (char)c;
    if (c == '\n') {
        currentLine.trim();
        if (currentLine.length() > 0) {
            addToRingBuffer(currentLine);
        }
        currentLine = "";
    } else if (currentLine.length() > 256) {
        // Prevent runaway memory usage for long lines without newlines
        currentLine = "";
    }

    if (client && client.connected() && client.availableForWrite() > 0) {
        return client.write(c);
    }
    return 0;
}
