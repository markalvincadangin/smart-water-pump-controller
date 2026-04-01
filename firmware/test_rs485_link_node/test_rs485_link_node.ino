#include <Arduino.h>

// -----------------------------------------------------------------------------
// SmartFlow RS-485 Link Test: Sensor Node Responder (NodeMCU ESP8266)
// Purpose: Keep a simple, always-on responder for master link diagnostics.
// -----------------------------------------------------------------------------

#define NODE_RS485_BAUD 115200
#define NODE_PIN_DE_RE 14  // D5
#define NODE_TURNAROUND_US 60

static uint32_t g_rxCommands = 0;
static uint32_t g_txFrames = 0;
static uint32_t g_badCommands = 0;
static uint32_t g_parseErrors = 0;

static char g_lineBuf[48];
static size_t g_linePos = 0;

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

void sendFramedPayload(const char* payload) {
  uint16_t crc = crc16_modbus((const uint8_t*)payload, strlen(payload));

  digitalWrite(NODE_PIN_DE_RE, HIGH);
  delayMicroseconds(NODE_TURNAROUND_US);

  Serial.write(0x02);      // STX
  Serial.print(payload);   // payload without STX/ETX
  Serial.printf("CRC:%04X", (unsigned)crc);
  Serial.write(0x03);      // ETX
  Serial.flush();

  delay(2);
  digitalWrite(NODE_PIN_DE_RE, LOW);

  g_txFrames++;
}

bool parsePingSeq(const char* line, uint32_t& seqOut) {
  if (strncmp(line, "PING:", 5) != 0) return false;

  const char* p = line + 5;
  if (*p == '\0') return false;

  char* endPtr = nullptr;
  unsigned long seq = strtoul(p, &endPtr, 10);
  if (endPtr == p || *endPtr != '\0') return false;

  seqOut = (uint32_t)seq;
  return true;
}

void handleCommand(const char* line) {
  g_rxCommands++;

  uint32_t seq = 0;
  if (parsePingSeq(line, seq)) {
    char payload[80];
    snprintf(payload, sizeof(payload), "HELLO;SEQ:%lu;NODE_OK:1;", (unsigned long)seq);
    sendFramedPayload(payload);
    return;
  }

  if (strcmp(line, "PING") == 0) {
    sendFramedPayload("HELLO;SEQ:0;NODE_OK:1;");
    return;
  }

  g_badCommands++;
  sendFramedPayload("ERR:BAD_CMD;NODE_OK:1;");
}

void printStatus() {
  Serial1.printf(
    "[NODE-DIAG] rx=%lu tx=%lu bad=%lu parse=%lu DE/RE=D5(%d) baud=%d\n",
    (unsigned long)g_rxCommands,
    (unsigned long)g_txFrames,
    (unsigned long)g_badCommands,
    (unsigned long)g_parseErrors,
    NODE_PIN_DE_RE,
    NODE_RS485_BAUD
  );
}

void setup() {
  pinMode(NODE_PIN_DE_RE, OUTPUT);
  digitalWrite(NODE_PIN_DE_RE, LOW);

  Serial.begin(NODE_RS485_BAUD);   // UART0 shared with USB and RS-485 wiring
  Serial.setTimeout(20);

  Serial1.begin(115200);           // TX-only debug on GPIO2
  delay(300);

  Serial1.println("=== RS485 Node Responder Test ===");
  Serial1.println("Role: sensor node responder");
  Serial1.printf("Pins: DE/RE=D5(%d), UART0 TX/RX, baud=%d\n", NODE_PIN_DE_RE, NODE_RS485_BAUD);
  Serial1.println("Expected command: PING:<seq>");
  Serial1.println("Response payload: HELLO;SEQ:<seq>;NODE_OK:1;");
}

void loop() {
  while (Serial.available() > 0) {
    int c = Serial.read();
    if (c < 0) break;

    if (c == '\r') {
      continue;
    }

    if (c == '\n') {
      g_lineBuf[g_linePos] = '\0';
      if (g_linePos > 0) {
        handleCommand(g_lineBuf);
      }
      g_linePos = 0;
      continue;
    }

    if (g_linePos < (sizeof(g_lineBuf) - 1)) {
      g_lineBuf[g_linePos++] = (char)c;
    } else {
      g_parseErrors++;
      g_linePos = 0;
    }
  }

  static uint32_t lastStatusMs = 0;
  uint32_t now = millis();
  if ((now - lastStatusMs) >= 5000) {
    lastStatusMs = now;
    printStatus();
  }

  delay(2);
}
