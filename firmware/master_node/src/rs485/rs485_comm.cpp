#include "rs485_comm.h"

#include "../state/state.h"
#include "../utils/crc16_modbus.h"
#include "../utils/time_utils.h"

static void rs485SetTx(bool tx) {
  digitalWrite(PIN_RS485_DE_RE, tx ? HIGH : LOW);
  delayMicroseconds(RS485_TX_TURNAROUND_US);
}

static void rs485DrainInput() {
  while (Serial2.available() > 0) {
    (void)Serial2.read();
  }
}

static bool rs485ReadFrame(char* out, size_t outLen, uint32_t timeoutMs) {
  if (outLen < 4) return false;
  uint32_t start = millis();
  size_t n = 0;
  bool inFrame = false;

  while (elapsedMillis32(millis(), start) < timeoutMs) {
    while (Serial2.available() > 0) {
      int c = Serial2.read();
      if (c < 0) break;

      if (!inFrame) {
        if (c == 0x02) { // STX
          inFrame = true;
          n = 0;
        }
        continue;
      }

      if (c == 0x03) { // ETX
        out[n] = '\0';
        return n > 0;
      }

      if (n + 1 < outLen) out[n++] = (char)c;
      else return false;
    }
    yield();
  }
  return false;
}

static bool parseUIntField(const char* s, const char* key, uint32_t& out) {
  const char* p = strstr(s, key);
  if (!p) return false;
  p += strlen(key);
  if (*p == '\0') return false;
  char* end = nullptr;
  unsigned long v = strtoul(p, &end, 10);
  if (end == p) return false;
  out = (uint32_t)v;
  return true;
}

static bool parseIntField(const char* s, const char* key, int& out) {
  const char* p = strstr(s, key);
  if (!p) return false;
  p += strlen(key);
  if (*p == '\0') return false;
  char* end = nullptr;
  long v = strtol(p, &end, 10);
  if (end == p) return false;
  out = (int)v;
  return true;
}

static bool parseFloatField(const char* s, const char* key, float& out) {
  const char* p = strstr(s, key);
  if (!p) return false;
  p += strlen(key);
  if (*p == '\0') return false;
  char* end = nullptr;
  double v = strtod(p, &end);
  if (end == p) return false;
  out = (float)v;
  return true;
}

static bool validateSensorResponseFrame(const char* payload) {
  if (!payload) return false;

  size_t len = strlen(payload);
  if (len < 24 || len >= RS485_RX_LINE_MAX) return false;

  // Payload returned by rs485ReadFrame must not still contain frame delimiters.
  if (strchr(payload, 0x02) != nullptr || strchr(payload, 0x03) != nullptr) return false;

  if (strstr(payload, "LVL:") == nullptr) return false;
  if (strstr(payload, "FLOW:") == nullptr) return false;
  if (strstr(payload, "ERR:") == nullptr) return false;
  if (strstr(payload, "SEQ:") == nullptr) return false;

  const char* crcPos = strstr(payload, "CRC:");
  if (!crcPos) return false;
  if (strstr(crcPos + 1, "CRC:") != nullptr) return false;
  if (strlen(crcPos) < 8) return false;  // CRC: + at least 4 hex chars

  // Require first 4 CRC characters to be hex, allow optional trailing separators.
  for (int i = 0; i < 4; i++) {
    char c = crcPos[4 + i];
    bool ok = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
    if (!ok) return false;
  }

  // Require at least one key-value separator to ensure basic frame shape.
  if (strchr(payload, ';') == nullptr) return false;

  return true;
}

static bool parseSensorFrameStrict(const char* payload, int& lvlOut, float& flowOut, int& errOut, uint32_t& seqOut, uint32_t& ldscOut) {
  // Expected payload (no STX/ETX):
  // - Minimal: LVL:..;FLOW:..;ERR:..;SEQ:..;CRC:XXXX
  // - Full:    LVL:..;DIST:..;FLOW:..;ERR:..;SEQ:..;CRC:XXXX
  const char* crcPos = strstr(payload, "CRC:");
  if (!crcPos) return false;

  // CRC hex must be exactly 4 chars
  if (strlen(crcPos) < 8) return false;
  char crcHex[5] = {0};
  memcpy(crcHex, crcPos + 4, 4);
  for (int i = 0; i < 4; i++) {
    char c = crcHex[i];
    bool ok = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
    if (!ok) return false;
  }
  uint32_t rxCrc = (uint32_t)strtoul(crcHex, nullptr, 16);

  // Compute CRC over substring up to and including the trailing ';' after SEQ.
  size_t crcInputLen = (size_t)(crcPos - payload);
  if (crcInputLen == 0) return false;
  uint16_t actualCrc = Crc16::calculateModbus((const uint8_t*)payload, crcPos - payload);
  if (((uint32_t)actualCrc & 0xFFFFu) != (rxCrc & 0xFFFFu)) return false;

  int lvl = 0;
  float dist = -1.0f;
  float flow = 0.0f;
  int err = 0;
  uint32_t seq = 0;
  uint32_t ldsc = 0;

  // DIST (cm) is preferred when present to prevent calibration drift.
  // Keep a valid LVL fallback so we do not drop an otherwise good frame
  // when DIST is temporarily inconsistent with calibration bounds.
  if (!parseIntField(payload, "LVL:", lvl)) return false;
  int lvlFromFrame = lvl;
  (void)parseFloatField(payload, "DIST:", dist); // optional
  if (!parseFloatField(payload, "FLOW:", flow)) return false;
  if (!parseIntField(payload, "ERR:", err)) return false;
  if (!parseUIntField(payload, "SEQ:", seq)) return false;
  // LDSC is optional for backward compatibility.
  (void)parseUIntField(payload, "LDSC:", ldsc);

  if (dist >= 0.0f) {
    // Keep protocol tolerant: if DIST is out of sane range, ignore DIST and keep LVL.
    // This avoids false offline states from otherwise valid CRC frames.
    if (!(dist >= 1.0f) || dist > 500.0f) {
      dist = -1.0f;
    }
  }

  if (dist >= 0.0f) {
    float rangeCm = (float)(cfgTankEmptyCm - cfgTankFullCm);
    if (rangeCm <= 0.1f) {
      // Invalid calibration window: preserve LVL from frame rather than dropping link data.
      lvl = lvlFromFrame;
      dist = -1.0f;
    }
  }

  if (dist >= 0.0f) {
    float rangeCm = (float)(cfgTankEmptyCm - cfgTankFullCm);
    float pct = 100.0f * ((float)cfgTankEmptyCm - dist) / rangeCm;
    if ((pct >= -5.0f) && (pct <= 105.0f)) {
      if (pct < 0.0f) pct = 0.0f;
      if (pct > 100.0f) pct = 100.0f;
      lvl = (int)(pct + 0.5f);
    } else {
      // DIST is inconsistent (often transient at near-full levels); keep LVL fallback.
      lvl = lvlFromFrame;
      dist = -1.0f;
    }
  }

  if (lvl < 0 || lvl > 100) return false;
  if (!(flow >= 0.0f) || flow > FLOW_MAX_SANE_LPM) return false;
  if (err < 0 || err > 7) return false;

  lvlOut = lvl;
  flowOut = flow;
  errOut = err;
  seqOut = seq;
  ldscOut = ldsc;
  return true;
}

void Rs485Comm::init() {
  pinMode(PIN_RS485_DE_RE, OUTPUT);
  digitalWrite(PIN_RS485_DE_RE, LOW);  // RX mode by default
  Serial2.begin(RS485_UART_BAUD, SERIAL_8N1, PIN_RS485_RX, PIN_RS485_TX);
}

static bool pollRemoteSensorNodeInternal(uint32_t timeBudgetMs) {
  uint32_t now = millis();
  uint32_t callStartMs = now;

  remoteSensorOnline = (remoteSensorLastRxMs > 0) &&
                       (elapsedMillis32(now, remoteSensorLastRxMs) <= REMOTE_SENSOR_OFFLINE_MS);

  static uint32_t lastReqMs = 0;
  if (lastReqMs > 0 && elapsedMillis32(now, lastReqMs) < RS485_REQ_INTERVAL_MS) {
    return remoteSensorOnline;
  }
  lastReqMs = now;

  bool gotFrame = false;
  int lvl = waterLevelPct;
  float flow = flowRateLpm;
  int err = (remoteSensorLastErrCode < 0) ? 0 : remoteSensorLastErrCode;
  uint32_t seq = 0;
  uint32_t ldsc = 0;
  static uint32_t lastSeqSeen = 0xFFFFFFFFu;
  static uint16_t dupSeqCount = 0;

  // Drain once per poll call to clear stale bytes without discarding late replies between retries.
  rs485DrainInput();

  for (int attempt = 0; attempt < RS485_MAX_RETRIES && !gotFrame; attempt++) {
    if (timeBudgetMs != 0) {
      uint32_t elapsedCallMs = elapsedMillis32(millis(), callStartMs);
      if (elapsedCallMs >= timeBudgetMs) break;
    }

    rs485SetTx(true);
    Serial2.print("REQ\n");
    Serial2.flush();
    delay(2); // Wait for hardware shift register to empty
    rs485SetTx(false);

    char payload[RS485_RX_LINE_MAX];
    uint32_t frameTimeoutMs = RS485_FRAME_TIMEOUT_MS;
    if (timeBudgetMs != 0) {
      uint32_t elapsedCallMs = elapsedMillis32(millis(), callStartMs);
      uint32_t remainingBudgetMs = (elapsedCallMs < timeBudgetMs) ? (timeBudgetMs - elapsedCallMs) : 0;
      frameTimeoutMs = (remainingBudgetMs < (uint32_t)RS485_FRAME_TIMEOUT_MS) ? remainingBudgetMs : (uint32_t)RS485_FRAME_TIMEOUT_MS;
    }

    if (frameTimeoutMs < 5) break;

    if (!rs485ReadFrame(payload, sizeof(payload), frameTimeoutMs)) {
      if (attempt == (RS485_MAX_RETRIES - 1)) {
        LOG(LOG_LEVEL_WARN, "RS485-ERR", "ReadFrame Timeout/Fail (all retries)");
      }
      delay(4);
      continue;
    }
    if (!validateSensorResponseFrame(payload)) {
      if (attempt == (RS485_MAX_RETRIES - 1)) {
        LOG(LOG_LEVEL_WARN, "RS485-ERR", "Frame structure invalid (all retries).");
      }
      delay(4);
      continue;
    }
    if (!parseSensorFrameStrict(payload, lvl, flow, err, seq, ldsc)) {
      if (attempt == (RS485_MAX_RETRIES - 1)) {
        LOG(LOG_LEVEL_ERROR, "RS485-ERR", "Parse strict failed after retries. Payload: %s", payload);
      }
      delay(4);
      continue;
    }
    gotFrame = true;
  }

  if (gotFrame) {
    prevWaterLevelPct = waterLevelPct;
    waterLevelPct = lvl;
    flowRateLpm = flow;
    remoteSensorLastErrCode = err;
    remoteSensorLastRxMs = now;
    remoteSensorOnline = true;
    remoteSensorConsecutiveFailCount = 0;
    remoteSensorLevelDiscardCount = ldsc;
    levelLastUpdateMs = now; // freshness timestamp (valid RS485 frame)

    remoteSensorOkStreak++;
    remoteSensorFailStreak = 0;
    if (remoteSensorOkStreak >= REMOTE_STABLE_ONLINE_N) remoteSensorStable = true;

    if (seq == lastSeqSeen) {
      if (dupSeqCount < 0xFFFFu) dupSeqCount++;
    } else {
      lastSeqSeen = seq;
      dupSeqCount = 0;
    }

    float rangeCm = (float)(cfgTankEmptyCm - cfgTankFullCm);
    float distanceCm = (float)cfgTankEmptyCm - (rangeCm * ((float)waterLevelPct / 100.0f));
    if (rangeCm > 0.1f) {
      ultrasonicLastGoodCmX10 = (uint32_t)(distanceCm * 10.0f + 0.5f);
    }
    ultrasonicCycleOkCount++;
    ultrasonicCycleOkCountWin++;

    bool lvlErr = (err & 0x01) != 0;
    bool flwErr = (err & 0x02) != 0;
    if (lvlErr) {
      ultrasonicCycleTimeoutCount++;
      ultrasonicCycleTimeoutCountWin++;
    }
    isLevelSensorError = lvlErr;
    isFlowSensorError = flwErr;
  } else {
    remoteSensorConsecutiveFailCount++;
    // Use wrap-safe elapsed helper for uptime rollover safety.
    remoteSensorOnline = (remoteSensorLastRxMs > 0) &&
                         (elapsedMillis32(now, remoteSensorLastRxMs) <= REMOTE_SENSOR_OFFLINE_MS);

    // Keep transient single-miss behavior non-disruptive while link is still fresh.
    // Escalate to local comm-loss code only after the online window is stale.
    if (!remoteSensorOnline) {
      remoteSensorLastErrCode = 4;  // local: frame timeout/parse failure
    }
    ultrasonicCycleTimeoutCount++;
    ultrasonicCycleTimeoutCountWin++;
    // Comm loss: keep last known values; downstream freshness/stability gates fail-safe.
    // Do not synthesize FLOW=0 (can create false DRY_RUN or false stuck-flow errors).
    isFlowSensorError = false;
    // Keep last known level error state; freshness/stability gates will block starts.

    remoteSensorFailStreak++;
    remoteSensorOkStreak = 0;
    if (remoteSensorFailStreak >= REMOTE_STABLE_OFFLINE_N) remoteSensorStable = false;
  }

  return gotFrame;
}

bool Rs485Comm::requestData(uint32_t timeBudgetMs) {
  return pollRemoteSensorNodeInternal(timeBudgetMs);
}

Rs485SensorData Rs485Comm::getParsedData() {
  Rs485SensorData d{};
  d.waterLevelPct = waterLevelPct;
  d.flowRateLpm = flowRateLpm;
  d.errCode = remoteSensorLastErrCode;
  d.online = remoteSensorOnline;
  return d;
}

