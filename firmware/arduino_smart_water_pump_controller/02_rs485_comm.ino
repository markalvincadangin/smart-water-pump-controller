// -----------------------------------------------------------------------------
// RS-485 Tank Link (ESP32 Master)
// - Sends "REQ\n" to the remote sensor node (NodeMCU V2 / ESP8266)
// - Expects: "LVL:<value>;FLOW:<value>;ERR:<code>\n"
// - Updates: waterLevelPct, flowRateLpm, remoteSensorOnline, remoteSensorLastErrCode
// -----------------------------------------------------------------------------

#include <smart_water_pump_controller_shared.h>

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
  digitalWrite(RS485_DE_RE_PIN, tx ? HIGH : LOW);
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

  while ((millis() - start) < timeoutMs) {
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

static bool parseSensorFrameStrict(const char* payload, int& lvlOut, float& flowOut, int& errOut, uint32_t& seqOut) {
  const char* crcPos = strstr(payload, "CRC:");
  if (!crcPos) return false;
  if (strlen(crcPos) < 8) return false;
  char crcHex[5] = {0};
  memcpy(crcHex, crcPos + 4, 4);
  for (int i = 0; i < 4; i++) {
    char c = crcHex[i];
    bool ok = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
    if (!ok) return false;
  }
  uint32_t rxCrc = (uint32_t)strtoul(crcHex, nullptr, 16);
  size_t crcInputLen = (size_t)(crcPos - payload);
  if (crcInputLen == 0) return false;
  uint16_t calc = crc16_modbus((const uint8_t*)payload, crcInputLen);
  if (((uint32_t)calc & 0xFFFFu) != (rxCrc & 0xFFFFu)) return false;

  int lvl = 0;
  float dist = -1.0f;
  float flow = 0.0f;
  int err = 0;
  uint32_t seq = 0;

  // LVL may be legacy-derived. DIST (cm) is preferred when present to prevent calibration drift.
  if (!parseIntField(payload, "LVL:", lvl)) return false;
  (void)parseFloatField(payload, "DIST:", dist); // optional
  if (!parseFloatField(payload, "FLOW:", flow)) return false;
  if (!parseIntField(payload, "ERR:", err)) return false;
  if (!parseUIntField(payload, "SEQ:", seq)) return false;

  if (dist >= 0.0f) {
    if (!(dist >= 1.0f) || dist > 300.0f) return false;
    float rangeCm = (float)(cfgTankEmptyCm - cfgTankFullCm);
    if (rangeCm <= 0.1f) return false;
    float pct = 100.0f * ((float)cfgTankEmptyCm - dist) / rangeCm;
    if (!(pct >= -5.0f) || !(pct <= 105.0f)) return false;
    if (pct < 0.0f) pct = 0.0f;
    if (pct > 100.0f) pct = 100.0f;
    lvl = (int)(pct + 0.5f);
  }

  if (lvl < 0 || lvl > 100) return false;
  if (!(flow >= 0.0f) || flow > FLOW_MAX_SANE_LPM) return false;
  if (err < 0 || err > 7) return false;

  lvlOut = lvl;
  flowOut = flow;
  errOut = err;
  seqOut = seq;
  return true;
}

bool pollRemoteSensorNode() {
  unsigned long now = millis();

  // Derive online/offline from last RX age
  remoteSensorOnline = (remoteSensorLastRxMs > 0) && ((now - remoteSensorLastRxMs) <= REMOTE_SENSOR_OFFLINE_MS);

  // Request pacing (reuse SENSOR_INTERVAL_MS schedule in loop)
  static unsigned long lastReqMs = 0;
  if (lastReqMs > 0 && (now - lastReqMs) < RS485_REQ_INTERVAL_MS) {
    return remoteSensorOnline;
  }
  lastReqMs = now;

  bool gotFrame = false;
  int lvl = waterLevelPct;
  float flow = flowRateLpm;
  int err = remoteSensorLastErrCode < 0 ? 0 : remoteSensorLastErrCode;
  uint32_t seq = 0;
  static uint32_t lastSeqSeen = 0xFFFFFFFFu;
  static uint16_t dupSeqCount = 0;

  for (int attempt = 0; attempt < RS485_MAX_RETRIES && !gotFrame; attempt++) {
    rs485DrainInput();

    rs485SetTx(true);
    Serial2.print("REQ\n");
    Serial2.flush();
    delay(2); // Wait for hardware shift register to empty
    rs485SetTx(false);

    char payload[RS485_RX_LINE_MAX];
    if (!rs485ReadFrame(payload, sizeof(payload), RS485_FRAME_TIMEOUT_MS)) {
      Serial.println("[RS485-ERR] ReadFrame Timeout/Fail");
      continue;
    }
    if (!parseSensorFrameStrict(payload, lvl, flow, err, seq)) {
      Serial.print("[RS485-ERR] Parse strict failed. Payload: ");
      Serial.println(payload);
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
    levelLastUpdateMs = now;

    remoteSensorOkStreak++;
    remoteSensorFailStreak = 0;
    if (remoteSensorOkStreak >= REMOTE_STABLE_ONLINE_N) remoteSensorStable = true;

    if (seq == lastSeqSeen) {
      if (dupSeqCount < 0xFFFFu) dupSeqCount++;
    } else {
      lastSeqSeen = seq;
      dupSeqCount = 0;
    }

    // Preserve legacy telemetry field by reconstructing an equivalent distance (cm)
    // from level% using the configured tank calibration.
    float rangeCm = (float)(cfgTankEmptyCm - cfgTankFullCm);
    float distanceCm = (float)cfgTankEmptyCm - (rangeCm * ((float)waterLevelPct / 100.0f));
    if (rangeCm > 0.1f) {
      ultrasonicLastGoodCmX10 = (uint32_t)(distanceCm * 10.0f + 0.5f);
    }
    ultrasonicCycleOkCount++;
    ultrasonicCycleOkCountWin++;

    // Map remote ERR codes into local flags (ERR:0 ok; 1 ultrasonic; 2 flow; 3 both)
    bool lvlErr = (err & 0x01) != 0;
    bool flwErr = (err & 0x02) != 0;
    if (lvlErr) {
      // leave level to last received (it may be stale), but mark health as failing
      ultrasonicCycleTimeoutCount++;
      ultrasonicCycleTimeoutCountWin++;
    }
    isLevelSensorError = lvlErr;
    isFlowSensorError = flwErr;
  } else {
    remoteSensorConsecutiveFailCount++;
    remoteSensorOnline = (remoteSensorLastRxMs > 0) && ((now - remoteSensorLastRxMs) <= REMOTE_SENSOR_OFFLINE_MS);

    remoteSensorLastErrCode = 4;  // local: frame timeout/parse failure
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

// v3.0: Flow-based level estimate while pump runs. Resets on sensor recovery (see checkLevelSensorFailure).
void updateFlowBasedEstimate() {
  if (!isRunning || flowRateLpm < cfgDryRunThresholdLpm) {
    lastFlowEstimateMs = millis();
    return;
  }
  unsigned long now = millis();
  float dtSec = (now - lastFlowEstimateMs) / 1000.0f;
  lastFlowEstimateMs = now;
  if (dtSec > 5.0f) return;

  flowVolumeAddedL += flowRateLpm * (dtSec / 60.0f);
  if (levelAnchorPct >= 0) {
    float added = (flowVolumeAddedL / (float)TANK_CAPACITY_L) * 100.0f;
    estimatedLevelPct = constrain((float)levelAnchorPct + added, 0.0f, 100.0f);
  }
}

