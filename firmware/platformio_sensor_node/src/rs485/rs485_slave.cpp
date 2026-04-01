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
  uint8_t ldsc = (snLevelDiscardCount > 255) ? 255 : (uint8_t)snLevelDiscardCount;
  char payload[104];
  int n = snprintf(payload, sizeof(payload), "LVL:%d;DIST:%.1f;FLOW:%.2f;ERR:%d;LDSC:%u;SEQ:%u;",
                   lvl, dist, flow, err, (unsigned)ldsc, (unsigned)seq);
  if (n <= 0 || (size_t)n >= sizeof(payload)) return;
  uint16_t crc = crc16_modbus((const uint8_t*)payload, (size_t)n);

  char frame[128];
  int m = snprintf(frame, sizeof(frame), "\x02%sCRC:%04X\x03", payload, (unsigned)crc);
  if (m <= 0 || (size_t)m >= sizeof(frame)) return;

  rs485SetTx(true);
  Serial.write((const uint8_t*)frame, (size_t)m);
  Serial.flush();
  delay(2); // Wait for hardware shift register to fully empty
  rs485SetTx(false);

#if SENSOR_DEBUG_ENABLED
  SENSOR_DBGF("[SN][DBG] TX frame seq=%u err=%d lvl=%d dist=%.1f flow=%.2f ldsc=%u\n", (unsigned)seq, err, lvl, dist, flow, (unsigned)ldsc);
#endif
}

void rs485_slave_init() {
  pinMode(PIN_RS485_DE_RE, OUTPUT);
  rs485SetTx(false);
}

void rs485_slave_poll() {
  // REFACTOR [M-03]: reset partial frame if inter-byte stall exceeds 20ms.
  static uint32_t lastByteMs = 0;
  if (rxPos > 0 && (millis() - lastByteMs) > 20) {
    rxPos = 0;
  }

  while (Serial.available() > 0) {
    int c = Serial.read();
    if (c < 0) return;
    lastByteMs = millis();
    if (c == '\r') continue;

    if (c == '\n') {
      rxLine[rxPos] = '\0';
      rxPos = 0;

      if (strcmp(rxLine, "REQ") == 0) {
        sendFrame();
      } else {
#if SENSOR_DEBUG_ENABLED
        SENSOR_DBGF("[SN][INFO] RX unknown cmd: '%s'\n", rxLine);
#endif
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

