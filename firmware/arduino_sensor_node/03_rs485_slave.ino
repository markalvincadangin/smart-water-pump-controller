// -----------------------------------------------------------------------------
// RS-485 slave (NodeMCU / ESP8266)
// - Listens for "REQ"
// - Replies with a framed payload:
//   STX + "LVL:<pct>;FLOW:<lpm>;ERR:<code>;SEQ:<n>;CRC:<hex>" + ETX
//
// The NodeMCU uses Serial (GPIO1/3) shared with USB flashing.
// Disconnect the MAX485 from TX/RX during flashing, then reconnect.
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

  char payload[96];
  int n = snprintf(payload, sizeof(payload), "LVL:%d;DIST:%.1f;FLOW:%.2f;ERR:%d;SEQ:%u;",
                   snWaterLevelPct, snLastDistanceCm, snFlowRateLpm, snErrCode, (unsigned)seq);
  if (n <= 0 || (size_t)n >= sizeof(payload)) return;
  uint16_t crc = crc16_modbus((const uint8_t*)payload, (size_t)n);

  char frame[128];
  int m = snprintf(frame, sizeof(frame), "\x02%sCRC:%04X\x03", payload, (unsigned)crc);
  if (m <= 0 || (size_t)m >= sizeof(frame)) return;

  rs485SetTx(true);
  Serial.write((const uint8_t*)frame, (size_t)m);
  Serial.flush();
  rs485SetTx(false);
}

void initRs485Slave() {
  Serial.setTimeout(10);
  // Serial already begun in setup()
  rs485SetTx(false);
}

void serviceRs485Slave() {
  while (Serial.available() > 0) {
    int c = Serial.read();
    if (c < 0) return;
    if (c == '\r') continue;

    if (c == '\n') {
      rxLine[rxPos] = '\0';
      rxPos = 0;

      if (strcmp(rxLine, "REQ") == 0) {
        sendFrame();
      }
      return;
    }

    if ((size_t)(rxPos + 1) < sizeof(rxLine)) {
      rxLine[rxPos++] = (char)c;
    } else {
      // overflow, reset
      rxPos = 0;
    }
  }
}

