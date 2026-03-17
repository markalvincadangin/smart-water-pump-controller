#include "rs485_slave.h"

#include "../state/state.h"
#include "../sensors/sensors.h"
#include "../utils/crc16_modbus.h"

static char rxLine[32];
static uint8_t rxPos = 0;

static void rs485SetTx(bool tx) {
  digitalWrite(PIN_RS485_DE_RE, tx ? HIGH : LOW);
  delayMicroseconds(RS485_TX_TURNAROUND_US);
}

static void sendFrame() {
  // Respond immediately from cached values (never block on sensors)
  int lvl = 0;
  float dist = -1.0f;
  float flow = 0.0f;
  int err = 0;
  uint8_t seq = snSeq;
  sensors_get_last_values(lvl, dist, flow, err, seq);

  // increment sequence for each response
  snSeq = (uint8_t)(snSeq + 1);

  // Build payload in canonical order for strict parsing + CRC
  // CRC is computed over payload up to and including the trailing ';' after SEQ.
  char payload[96];
  int n = snprintf(payload, sizeof(payload), "LVL:%d;DIST:%.1f;FLOW:%.2f;ERR:%d;SEQ:%u;",
                   lvl, dist, flow, err, (unsigned)seq);
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

void rs485_slave_init() {
  pinMode(PIN_RS485_DE_RE, OUTPUT);
  rs485SetTx(false);
}

void rs485_slave_poll() {
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
      rxPos = 0;
    }
  }
}

