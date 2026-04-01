// -----------------------------------------------------------------------------
// RS-485 slave (NodeMCU / ESP8266)
// - Listens for "REQ\n" on SN_SERIAL_RS485 (UART0 GPIO1/3)
// - Replies with a framed payload:
//   STX "LVL:<pct>;DIST:<cm>;FLOW:<lpm>;ERR:<code>;LDSC:<n>;SEQ:<n>;CRC:<hex4>" ETX
//
// Phase 2 [M-03]: partial frame stall recovery via 20ms inter-byte timeout.
// Phase 2 [2.5]:  LDSC field added (level discard count since last frame).
// -----------------------------------------------------------------------------

#include "sensor_node_shared.h"

static char rxLine[32];
static uint8_t rxPos = 0;

static uint16_t crc16_modbus(const uint8_t* data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i];
    for (int b = 0; b < 8; b++) {
      if (crc & 0x0001) crc = (uint16_t)((crc >> 1) ^ 0xA001);
      else crc = (uint16_t)(crc >> 1);
    }
  }
  return crc;
}

static void rs485SetTx(bool tx) {
  digitalWrite(PIN_RS485_DE_RE, tx ? HIGH : LOW);
  delayMicroseconds(RS485_TX_TURNAROUND_US);
}

static void sendFrame() {
  // Respond immediately from cached values (never block on sensors)
  uint8_t seq = snSeq;
  snSeq = (uint8_t)(snSeq + 1);

  char payload[104];  // enlarged for LDSC field
  uint8_t ldsc = (snLevelDiscardCount > 255) ? 255 : (uint8_t)snLevelDiscardCount;  // Phase 2 [2.5]
  int n = snprintf(payload, sizeof(payload),
                   "LVL:%d;DIST:%.1f;FLOW:%.2f;ERR:%d;LDSC:%u;SEQ:%u;",
                   snWaterLevelPct, snLastDistanceCm, snFlowRateLpm,
                   snErrCode, (unsigned)ldsc, (unsigned)seq);
  if (n <= 0 || (size_t)n >= sizeof(payload)) return;
  uint16_t crc = crc16_modbus((const uint8_t*)payload, (size_t)n);

  char frame[128];
  int m = snprintf(frame, sizeof(frame), "\x02%sCRC:%04X\x03", payload, (unsigned)crc);
  if (m <= 0 || (size_t)m >= sizeof(frame)) return;

  rs485SetTx(true);
  SN_SERIAL_RS485.write((const uint8_t*)frame, (size_t)m);
  SN_SERIAL_RS485.flush();
  delay(2);  // Wait for hardware shift register to fully empty
  rs485SetTx(false);

  LOG_SN(LOG_DEBUG, "RS485", "TX seq=%u err=%d lvl=%d dist=%.1f flow=%.2f ldsc=%u",
         (unsigned)seq, snErrCode, snWaterLevelPct, snLastDistanceCm, snFlowRateLpm, (unsigned)ldsc);
}

void initRs485Slave() {
  SN_SERIAL_RS485.setTimeout(10);
  // SN_SERIAL_RS485 already begun in setup()
  rs485SetTx(false);
}

void serviceRs485Slave() {
  // REFACTOR [M-03]: inter-byte timeout — reset partial frame after 20ms stall
  static uint32_t lastByteMs = 0;
  if (rxPos > 0 && (millis() - lastByteMs) > 20) {
    LOG_SN(LOG_DEBUG, "RS485", "Partial frame stall (rxPos=%d). Receiver reset.", (int)rxPos);
    rxPos = 0;
  }

  while (SN_SERIAL_RS485.available() > 0) {
    int c = SN_SERIAL_RS485.read();
    if (c < 0) return;
    lastByteMs = millis();  // M-03: update on every received byte
    if (c == '\r') continue;

    if (c == '\n') {
      rxLine[rxPos] = '\0';
      rxPos = 0;

      if (strstr(rxLine, "REQ") != nullptr) {
        sendFrame();
      } else {
        LOG_SN(LOG_DEBUG, "RS485", "Unknown cmd: '%s'", rxLine);
      }
      return;
    }

    if ((size_t)(rxPos + 1) < sizeof(rxLine)) {
      rxLine[rxPos++] = (char)c;
    } else {
      // Buffer overflow — reset
      rxPos = 0;
    }
  }
}

