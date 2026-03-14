// -----------------------------------------------------------------------------
// Sensors: flow ISR, ultrasonic level, and flow rate calculation
// -----------------------------------------------------------------------------

// ---- Flow sensor ISR ----

// YF-G1 at max ~100 LPM with K=7.5 → 750 Hz → ~1333µs between real pulses.
// 2ms filters noise bursts while still accepting all real pulses at max flow.
#define FLOW_DEBOUNCE_US  2000ULL

void IRAM_ATTR flowPulseISR() {
  uint64_t now = esp_timer_get_time();
  if (now - lastPulseUs > FLOW_DEBOUNCE_US) {
    lastPulseUs = now;
    pulseCount = pulseCount + 1;
  }
}

// ---- Ultrasonic level ----

/**
 * @brief Takes a single JSN-SR04T distance reading.
 * @return float Distance in cm, or -1.0f on timeout.
 */
float readSingleUltrasonic() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, ULTRASONIC_TIMEOUT_MS * 1000UL);
  if (duration == 0) return -1.0f;

  float distanceCm = duration / 58.0f;

  // Reject physically impossible readings (usually noise / long-cable glitches)
  if (distanceCm < 2.0f || distanceCm > 200.0f) return -1.0f;

  return distanceCm;
}

/**
 * @brief Reads JSN-SR04T with 5-sample median filter, float percentage,
 *        and EMA smoothing. Includes rate-of-change guard.
 * @return int Water level percentage (0-100), or -1 on total sensor failure.
 */
int readUltrasonicSensor() {
  // Take ULTRASONIC_SAMPLES readings, then median-filter the valid ones.
  float readings[ULTRASONIC_SAMPLES];
  int validCount = 0;

  for (int i = 0; i < ULTRASONIC_SAMPLES; i++) {
    float d = readSingleUltrasonic();
    if (d >= 0.0f) {
      readings[validCount++] = d;
    }
    if (i < ULTRASONIC_SAMPLES - 1) delay(ULTRASONIC_SAMPLE_DELAY);
  }

  // No valid readings this cycle
  if (validCount == 0) {
    ultrasonicCycleTimeoutCount++;
    ultrasonicCycleTimeoutCountWin++;
    return -1;
  }

  ultrasonicCycleOkCount++;
  ultrasonicCycleOkCountWin++;

  // Median (insertion sort)
  for (int i = 1; i < validCount; i++) {
    float key = readings[i];
    int j = i - 1;
    while (j >= 0 && readings[j] > key) {
      readings[j + 1] = readings[j];
      j--;
    }
    readings[j + 1] = key;
  }
  float medianDist = readings[validCount / 2];

  ultrasonicLastGoodCmX10 = (uint32_t)(medianDist * 10.0f + 0.5f);
  return updateLevelFromReading(medianDist);
}

int updateLevelFromReading(float distanceCm) {
  distanceCm = constrain(distanceCm, (float)cfgTankFullCm, (float)cfgTankEmptyCm);
  float range = (float)(cfgTankEmptyCm - cfgTankFullCm);
  float levelFloat = 100.0f * ((float)cfgTankEmptyCm - distanceCm) / range;
  levelFloat = constrain(levelFloat, 0.0f, 100.0f);
  if (waterLevelEma < 0.1f && waterLevelPct == 0) waterLevelEma = levelFloat;
  else waterLevelEma = ULTRASONIC_EMA_ALPHA * levelFloat + (1.0f - ULTRASONIC_EMA_ALPHA) * waterLevelEma;
  int newLevel = (int)(waterLevelEma + 0.5f);
  newLevel = constrain(newLevel, 0, 100);
  int delta = abs(newLevel - prevWaterLevelPct);
  if (prevWaterLevelPct > 0 && delta > LEVEL_RATE_OF_CHANGE_MAX) {
    Serial.printf("[SENSOR][WARN] Level jumped %d%% (prev=%d%%, new=%d%%). Holding previous.\n", delta, prevWaterLevelPct, newLevel);
    return prevWaterLevelPct;
  }
  return newLevel;
}

void updateFlowBasedEstimate() {
  if (!isRunning || flowRateLpm < cfgDryRunThresholdLpm) { lastFlowEstimateMs = millis(); return; }
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

// ---- Flow rate ----

/**
 * @brief Calculates flow rate from ISR pulse count over a 1-second window.
 *        Atomically reads and resets pulseCount using noInterrupts().
 *        YF-G1: Q (L/min) = F (Hz) / 7.5  (datasheet: F = 7.5 * Q).
 *        Variants may differ — verify with bucket test.
 * @return float Flow rate in Litres Per Minute.
 */
float calculateFlowRate() {
  // Atomically read and reset the ISR pulse counter (1-second window)
  noInterrupts();
  uint32_t count = pulseCount;
  pulseCount = 0;
  interrupts();

  // If pump is OFF and has been OFF for a while (tracked from actual stop time),
  // any pulses are treated as noise and flow is forced to 0.0 LPM.
  if (!isRunning && pumpOffStartMs > 0 && (millis() - pumpOffStartMs) > FLOW_PUMP_OFF_ZERO_MS) {
    return 0.0f;
  }

  // Convert pulses/sec to L/min using calibration factor
  float lpm = (float)count / cfgFlowCalibration;

  // Discard physically impossible readings (usually noise)
  if (lpm > FLOW_MAX_SANE_LPM) {
    flowDiscardMaxSaneCount++;
    flowDiscardMaxSaneCountWin++;
    Serial.printf("[SENSOR][WARN] Flow %.1f LPM exceeds max sane (%.0f). Discarded.\n",
                  lpm, FLOW_MAX_SANE_LPM);
    return flowRateLpm;  // Keep previous value
  }

  return lpm;
}
