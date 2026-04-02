// -----------------------------------------------------------------------------
// Sensors (NodeMCU / ESP8266): ultrasonic level + YF-G1 flow
// Phase 2: H-02 (level discard observability), H-03 (flow discard log var), H-04 (flow error hysteresis)
// -----------------------------------------------------------------------------

#include "sensor_node_shared.h"

static uint32_t lastFlowCalcMs = 0;
static volatile uint32_t flowRawEdgeCount = 0;

static void IRAM_ATTR flowIsr() {
  flowRawEdgeCount++;
  uint32_t nowUs = micros();
  uint32_t lastUs = flowLastPulseUs;
  if ((uint32_t)(nowUs - lastUs) >= FLOW_MIN_PULSE_INTERVAL_US) {
    flowLastPulseUs = nowUs;
    flowPulseCount++;
  } else {
    flowPulseDiscardCount++;
  }
}

static int cmToPercent(float distanceCm) {
  const float emptyCm = TANK_US_DIST_EMPTY_CM;
  const float fullCm  = TANK_US_DIST_FULL_CM;

  distanceCm = constrain(distanceCm, fullCm, emptyCm);
  float range = (emptyCm - fullCm);
  if (range <= 1.0f) {
    return 0;
  }
  float pct = 100.0f * (emptyCm - distanceCm) / range;
  pct = constrain(pct, 0.0f, 100.0f);
  return (int)(pct + 0.5f);
}

void initSensors() {
  pinMode(PIN_US_TRIG, OUTPUT);
  pinMode(PIN_US_ECHO, INPUT);
  digitalWrite(PIN_US_TRIG, LOW);

  pinMode(PIN_FLOW_INPUT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_FLOW_INPUT), flowIsr, FALLING);

  lastFlowCalcMs = millis();
  snLastFlowUpdateMs = millis();
  snLastLevelUpdateMs = 0;
  snLastGoodLevelPct = 0;
  snLevelError = false;
  snFlowError = false;
}

enum class UsState : uint8_t { Idle, TriggerHigh, WaitRise, WaitFall };
static UsState usState = UsState::Idle;
static uint32_t usStateEnteredUs = 0;
static uint32_t usEchoRiseUs = 0;
static uint32_t usNextSampleMs = 0;
static uint32_t usNextMeasMs = 0;
static bool usWindowActive = false;
static float usSamples[US_SAMPLES];
static uint8_t usSampleCount = 0;

static void usResetWindow() {
  usSampleCount = 0;
  snLevelDiscardCount = 0;  // REFACTOR [H-02]: reset per-window discard counter
  usWindowActive = true;
}

static void usPushSample(float cm) {
  if (usSampleCount >= US_SAMPLES) return;
  usSamples[usSampleCount++] = cm;
}

static float usMedianValid() {
  float tmp[US_SAMPLES];
  uint8_t n = 0;
  for (uint8_t i = 0; i < usSampleCount; i++) {
    if (usSamples[i] >= 0.0f) tmp[n++] = usSamples[i];
  }
  if (n == 0) return -1.0f;
  for (uint8_t i = 1; i < n; i++) {
    float key = tmp[i];
    int j = (int)i - 1;
    while (j >= 0 && tmp[j] > key) {
      tmp[j + 1] = tmp[j];
      j--;
    }
    tmp[j + 1] = key;
  }
  return tmp[n / 2];
}

static void usStartTrigger() {
  digitalWrite(PIN_US_TRIG, LOW);
  usState = UsState::TriggerHigh;
  usStateEnteredUs = micros();
  digitalWrite(PIN_US_TRIG, HIGH);
}

static void usServiceOnce(uint32_t nowMs, uint32_t nowUs) {
  switch (usState) {
    case UsState::Idle: {
      if (!usWindowActive) {
        if ((int32_t)(nowMs - usNextMeasMs) < 0) return;
        usNextMeasMs = nowMs + US_MEAS_INTERVAL_MS;
        usNextSampleMs = nowMs;
        usResetWindow();
      }
      if (usSampleCount < US_SAMPLES && (int32_t)(nowMs - usNextSampleMs) >= 0) {
        usStartTrigger();
      }
      return;
    }
    case UsState::TriggerHigh: {
      if ((uint32_t)(nowUs - usStateEnteredUs) >= 20UL) {
        digitalWrite(PIN_US_TRIG, LOW);
        usState = UsState::WaitRise;
        usStateEnteredUs = nowUs;
      }
      return;
    }
    case UsState::WaitRise: {
      if (digitalRead(PIN_US_ECHO) == HIGH) {
        usEchoRiseUs = nowUs;
        usState = UsState::WaitFall;
        usStateEnteredUs = nowUs;
        return;
      }
      if ((uint32_t)(nowUs - usStateEnteredUs) >= US_TIMEOUT_US) {
        usPushSample(-1.0f);
        usNextSampleMs = nowMs + US_SAMPLE_SPACING_MS;
        usState = UsState::Idle;
      }
      return;
    }
    case UsState::WaitFall: {
      if (digitalRead(PIN_US_ECHO) == LOW) {
        uint32_t widthUs = (uint32_t)(nowUs - usEchoRiseUs);
        float rawCm = (float)widthUs / 58.0f;
        const float fullClampCm = US_RELIABLE_MIN_CM + TANK_US_DISTANCE_OFFSET_CM;
        float cm = rawCm + TANK_US_DISTANCE_OFFSET_CM;
        if (rawCm < US_RELIABLE_MIN_CM) {
          // Near-field readings are unreliable on JSN-SR04T; clamp to a safe full threshold.
          cm = fullClampCm;
        } else if (rawCm > US_RELIABLE_MAX_CM) {
          cm = -1.0f;
        }
        usPushSample(cm);

        if (usSampleCount >= US_SAMPLES) {
          float med = usMedianValid();
          if (med < 0.0f) {
            // All samples timed out — ultrasonic sensor failure
            snLevelError = true;
          } else {
            int lvl = cmToPercent(med);
            if (med <= fullClampCm) {
              // Safety saturation: treat near-sensor region as effectively full.
              lvl = 100;
            }

            // REFACTOR [H-02]: plausibility filter with counter, rate-limited log, error promotion
            if (snLastLevelUpdateMs > 0 && abs(lvl - snLastGoodLevelPct) > LEVEL_MAX_DELTA_PCT) {
              // Reading rejected by plausibility filter — preserve last known good
              snLevelDiscardCount++;
              static uint32_t lastLvlDiscardWarnMs = 0;
              if (millis() - lastLvlDiscardWarnMs >= 60000UL) {
                lastLvlDiscardWarnMs = millis();
                LOG_SN(LOG_WARN, "SENSOR", "Level discard: new=%d%% last=%d%% delta>%d. count=%u",
                       lvl, snLastGoodLevelPct, LEVEL_MAX_DELTA_PCT, (unsigned)snLevelDiscardCount);
              }
              // Promote to sensor error if every sample this window was rejected
              if (snLevelDiscardCount >= US_SAMPLES) {
                snLevelError = true;
                LOG_SN(LOG_WARN, "SENSOR",
                       "All %d level samples discarded. snLevelError=true.", US_SAMPLES);
              }
            } else {
              // Reading accepted
              snLastDistanceCm = med;
              snWaterLevelPct     = lvl;
              snLastGoodLevelPct  = lvl;
              snLastLevelUpdateMs = nowMs;
              snLevelError        = false;
            }
          }
          usWindowActive = false;
          usState = UsState::Idle;
        } else {
          usNextSampleMs = nowMs + US_SAMPLE_SPACING_MS;
          usState = UsState::Idle;
        }
        return;
      }
      if ((uint32_t)(nowUs - usStateEnteredUs) >= US_TIMEOUT_US) {
        usPushSample(-1.0f);
        if (usSampleCount >= US_SAMPLES) {
          snLevelError = true;
          usWindowActive = false;
        } else {
          usNextSampleMs = nowMs + US_SAMPLE_SPACING_MS;
        }
        usState = UsState::Idle;
      }
      return;
    }
  }
}

void serviceSensorsNonBlocking() {
  uint32_t now = millis();
  uint32_t nowUs = micros();

  if ((now - lastFlowCalcMs) >= 1000UL) {
    const uint32_t dtMs = now - lastFlowCalcMs;
    lastFlowCalcMs = now;

    noInterrupts();
    uint32_t pulses = flowPulseCount;
    flowPulseCount = 0;
    uint32_t disc = flowPulseDiscardCount;  // REFACTOR [H-03]: local copy used for all logging
    flowPulseDiscardCount = 0;
    uint32_t rawEdges = flowRawEdgeCount;
    flowRawEdgeCount = 0;
    interrupts();

    // dt-aware conversion: Hz = pulses / dt(s), then L/min = Hz / FLOW_HZ_PER_LPM
    const float dtSec = (dtMs > 0) ? ((float)dtMs / 1000.0f) : 1.0f;
    const float hz = (dtSec > 0.0f) ? ((float)pulses / dtSec) : 0.0f;
    snFlowRateLpm = (FLOW_HZ_PER_LPM > 0.01f) ? (hz / FLOW_HZ_PER_LPM) : 0.0f;
    if (snFlowRateLpm < 0.0f) snFlowRateLpm = 0.0f;
    if (snFlowRateLpm > FLOW_MAX_SANE_LPM) snFlowRateLpm = FLOW_MAX_SANE_LPM;
    snLastFlowUpdateMs = now;

    // REFACTOR [H-04]: two-stage hysteresis — assert after 3s dwell, clear after 5s clear dwell.
    // Replaces the old non-hysteretic: snFlowError = (disc > 50)
    if (disc > 50) {
      flowErrAssertCount++;
      flowErrClearCount = 0;
      if (flowErrAssertCount >= 3 && !snFlowError) {
        snFlowError = true;
        // REFACTOR [H-03]: 'disc' is the local variable, not the zeroed global
        LOG_SN(LOG_WARN, "FLOW", "Flow error asserted: disc=%lu for %d consecutive windows.",
               (unsigned long)disc, (int)flowErrAssertCount);
      }
    } else if (disc <= 20) {
      flowErrClearCount++;
      flowErrAssertCount = 0;
      if (flowErrClearCount >= 5 && snFlowError) {
        snFlowError = false;
        LOG_SN(LOG_INFO, "FLOW", "Flow error cleared: disc=%lu for %d consecutive clean windows.",
               (unsigned long)disc, (int)flowErrClearCount);
      }
    } else {
      // Hysteresis band [21..50]: hold current state, reset both dwelling counters
      flowErrAssertCount = 0;
      flowErrClearCount  = 0;
    }
      LOG_SN(LOG_INFO, "FLOW", "flow=%.2fLPM pulse=%lu raw=%lu disc=%lu pin=%d",
        snFlowRateLpm,
        (unsigned long)pulses,
        (unsigned long)rawEdges,
        (unsigned long)disc,
        (int)digitalRead(PIN_FLOW_INPUT));
    }  // end flow 1s window

  usServiceOnce(now, nowUs);
  snErrCode = (snLevelError ? 1 : 0) | (snFlowError ? 2 : 0);

  // Phase 1 LOG_SN replaces legacy SENSOR_DBGF block — no #if guard needed
  // Shows snLevelDiscardCount to confirm H-02 counter is live (replaces zeroed global H-03)
  static uint32_t lastDbgMs = 0;
  if ((uint32_t)(now - lastDbgMs) >= SENSOR_DEBUG_INTERVAL_MS) {
    lastDbgMs = now;
    LOG_SN(LOG_DEBUG, "SENSOR", "lvl=%d%% dist=%.1fcm flow=%.2fLPM err=%d seq=%u ldsc=%u",
           snWaterLevelPct, snLastDistanceCm, snFlowRateLpm,
           snErrCode, (unsigned)snSeq, (unsigned)snLevelDiscardCount);
  }
}
