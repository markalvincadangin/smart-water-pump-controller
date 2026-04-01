#include <Arduino.h>

// -----------------------------------------------------------------------------
// SmartFlow RS-485 Link Test: Master Continuous Diagnostics (ESP32)
// Purpose: Poll node every 3s and print direct communication health/errors.
// -----------------------------------------------------------------------------

#define MASTER_RS485_BAUD 115200
#define MASTER_RS485_TX_PIN 17
#define MASTER_RS485_RX_PIN 25
#define MASTER_RS485_DE_RE_PIN 5
#define MASTER_RS485_TURNAROUND_US 80

#define MASTER_REQ_INTERVAL_MS 3000
#define MASTER_RX_TIMEOUT_MS 500
#define MASTER_SUMMARY_INTERVAL_MS 15000

static uint32_t g_txReq = 0;
static uint32_t g_rxValid = 0;
static uint32_t g_timeoutErr = 0;
static uint32_t g_crcErr = 0;
static uint32_t g_frameErr = 0;
static uint32_t g_seqErr = 0;

uint16_t crc16_modbus(const uint8_t* data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i];
    for (int b = 0; b < 8; b++) {
      if (crc & 0x0001) {
        crc = (uint16_t)((crc >> 1) ^ 0xA001);
      } else {
        crc = (uint16_t)(crc >> 1);
      }
    }
  }
  return crc;
}

bool readFrame(char* out, size_t outLen, uint32_t timeoutMs) {
  uint32_t start = millis();
  size_t pos = 0;
  bool started = false;

  while ((millis() - start) < timeoutMs) {
    if (Serial2.available()) {
      int c = Serial2.read();
      if (c < 0) continue;

      if (!started) {
        if (c == 0x02) {
          started = true;
          pos = 0;
          out[pos++] = (char)c;
        }
        continue;
      }

      if (c == 0x03) {
        if (pos < outLen - 1) {
          out[pos++] = (char)c;
          out[pos] = '\0';
          return true;
        }
        return false;
      }

      if (pos < outLen - 1) {
        out[pos++] = (char)c;
      } else {
        return false;
      }
    }
  }

  return false;
}

bool extractPayloadAndValidateCrc(const char* frame, char* payloadOut, size_t payloadOutLen) {
  const char* crcPos = strstr(frame, "CRC:");
  if (!crcPos) return false;

  if (frame[0] != 0x02) return false;

  size_t payloadLen = (size_t)(crcPos - frame - 1); // exclude STX
  if (payloadLen == 0 || payloadLen >= payloadOutLen) return false;

  memcpy(payloadOut, frame + 1, payloadLen);
  payloadOut[payloadLen] = '\0';

  uint32_t rxCrc = (uint32_t)strtoul(crcPos + 4, nullptr, 16);
  uint16_t calc = crc16_modbus((const uint8_t*)payloadOut, payloadLen);
  return ((uint32_t)calc == (rxCrc & 0xFFFFu));
}

bool parseSeqFromPayload(const char* payload, uint32_t& seqOut) {
  const char* pos = strstr(payload, "SEQ:");
  if (!pos) return false;

  pos += 4;
  char* endPtr = nullptr;
  unsigned long seq = strtoul(pos, &endPtr, 10);
  if (endPtr == pos) return false;

  seqOut = (uint32_t)seq;
  return true;
}

void sendPing(uint32_t seq) {
  char req[24];
  snprintf(req, sizeof(req), "PING:%lu\n", (unsigned long)seq);

  digitalWrite(MASTER_RS485_DE_RE_PIN, HIGH);
  delayMicroseconds(MASTER_RS485_TURNAROUND_US);
  Serial2.print(req);
  Serial2.flush();
  delayMicroseconds(MASTER_RS485_TURNAROUND_US);
  digitalWrite(MASTER_RS485_DE_RE_PIN, LOW);
}

void printSummary() {
  Serial.printf(
    "[MASTER-SUMMARY] tx=%lu ok=%lu timeout=%lu crc=%lu frame=%lu seq=%lu\n",
    (unsigned long)g_txReq,
    (unsigned long)g_rxValid,
    (unsigned long)g_timeoutErr,
    (unsigned long)g_crcErr,
    (unsigned long)g_frameErr,
    (unsigned long)g_seqErr
  );

  if (g_rxValid == 0 && g_txReq >= 5) {
    Serial.println("[MASTER-HINT] No valid replies. Check: node power, A/B lines, common GND, DE/RE pin, node UART wiring.");
  } else if (g_crcErr > 0) {
    Serial.println("[MASTER-HINT] CRC errors seen. Check baud rate mismatch/noise/ground quality.");
  } else if (g_seqErr > 0) {
    Serial.println("[MASTER-HINT] Seq mismatch. Responses are arriving but command/response pairing is unstable.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);

  Serial2.begin(MASTER_RS485_BAUD, SERIAL_8N1, MASTER_RS485_RX_PIN, MASTER_RS485_TX_PIN);
  pinMode(MASTER_RS485_DE_RE_PIN, OUTPUT);
  digitalWrite(MASTER_RS485_DE_RE_PIN, LOW);

  Serial.println("=== RS485 Master Link Diagnostic Test ===");
  Serial.println("Role: master continuous poller");
  Serial.printf("Pins: TX=GPIO%d RX=GPIO%d DE/RE=GPIO%d baud=%d\n",
                MASTER_RS485_TX_PIN,
                MASTER_RS485_RX_PIN,
                MASTER_RS485_DE_RE_PIN,
                MASTER_RS485_BAUD);
  Serial.println("Request: PING:<seq> every 3s");
  Serial.println("Expect payload: HELLO;SEQ:<seq>;NODE_OK:1;");
}

void loop() {
  static uint32_t lastReqMs = 0;
  static uint32_t lastSummaryMs = 0;
  static uint32_t seq = 0;

  uint32_t now = millis();
  if ((now - lastReqMs) >= MASTER_REQ_INTERVAL_MS) {
    lastReqMs = now;
    seq++;

    sendPing(seq);
    g_txReq++;

    char frame[160];
    if (!readFrame(frame, sizeof(frame), MASTER_RX_TIMEOUT_MS)) {
      g_timeoutErr++;
      Serial.printf("[MASTER-DIAG][%lu] FAIL timeout\n", (unsigned long)seq);
    } else {
      char payload[120];
      if (!extractPayloadAndValidateCrc(frame, payload, sizeof(payload))) {
        g_crcErr++;
        Serial.printf("[MASTER-DIAG][%lu] FAIL crc/frame raw='%s'\n", (unsigned long)seq, frame);
      } else {
        uint32_t rxSeq = 0;
        if (!parseSeqFromPayload(payload, rxSeq)) {
          g_frameErr++;
          Serial.printf("[MASTER-DIAG][%lu] FAIL missing SEQ payload='%s'\n", (unsigned long)seq, payload);
        } else if (rxSeq != seq && rxSeq != 0) {
          g_seqErr++;
          Serial.printf("[MASTER-DIAG][%lu] FAIL seq-mismatch rx=%lu payload='%s'\n",
                        (unsigned long)seq,
                        (unsigned long)rxSeq,
                        payload);
        } else {
          g_rxValid++;
          Serial.printf("[MASTER-DIAG][%lu] PASS payload='%s'\n", (unsigned long)seq, payload);
        }
      }
    }
  }

  if ((now - lastSummaryMs) >= MASTER_SUMMARY_INTERVAL_MS) {
    lastSummaryMs = now;
    printSummary();
  }

  delay(5);
}
