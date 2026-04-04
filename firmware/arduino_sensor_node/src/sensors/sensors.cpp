#include "sensors.h"

#include "../state/state.h"

static uint32_t lastFlowCalcMs = 0;
static volatile uint32_t flowRawEdgeCount = 0;

// ----------------------------
// Flow ISR (hardened)
// ----------------------------
// flowRawEdgeCount was previously incremented unconditionally at ISR entry,
// racing with the main-loop read outside the noInterrupts() guard. It is now incremented
// inside the deglitch window check only when a pulse is accepted (same path as
// flowPulseCount), so both counters share identical ISR semantics. The main-loop read
// and zero of both counters remain inside noInterrupts()/interrupts().
static void IRAM_ATTR flowIsr() {
  uint32_t nowUs = micros();
  uint32_t lastUs = flowLastPulseUs;
  if ((uint32_t)(nowUs - lastUs) >= FLOW_MIN_PULSE_INTERVAL_US) {
    flowLastPulseUs = nowUs;
    flowPulseCount++;
    flowRawEdgeCount++;  // accepted edge — same gate as flowPulseCount
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

// ----------------------------
// Ultrasonic non-blocking state machine
// ----------------------------
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
  // Reset per-window discard counter.
  snLevelDiscardCount = 0;
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
  // insertion sort (small n)
  for (uint8_t i = 1; i < n; i++) {
    float key = tmp[i];
    int j = (int)i - 1;
    while (j >= 0 && tmp[j] > key) {
      tmp[j + 1] = tmp[j];
      j--;
    }
    tmp[j + 1] = key;
  }
  // Even-sample windows use upper median by design (n/2), not mean-of-two-middle.
  return tmp[n / 2];
}

static void usStartTrigger() {
  // Ensure low, then raise for 10us (non-blocking: do the 10us via state)
  digitalWrite(PIN_US_TRIG, LOW);
  usState = UsState::TriggerHigh;
  usStateEnteredUs = micros();
  digitalWrite(PIN_US_TRIG, HIGH);
}

static void usServiceOnce(uint32_t nowMs, uint32_t nowUs) {
  switch (usState) {
    case UsState::Idle: {
      // Start a new measurement window periodically
      if (!usWindowActive) {
        if ((int32_t)(nowMs - usNextMeasMs) < 0) return;
        usNextMeasMs = nowMs + US_MEAS_INTERVAL_MS;
        usNextSampleMs = nowMs;
        usResetWindow();
      }

      // Within an active window, trigger the next sample when due
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
        // Timeout waiting for rise
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

        // Schedule next sample in the same window
        if (usSampleCount >= US_SAMPLES) {
          float med = usMedianValid();
          if (med < 0.0f) {
            snLevelError = true;
          } else {
            int lvl = cmToPercent(med);
            if (med <= fullClampCm) {
              // Safety saturation: treat near-sensor region as effectively full.
              lvl = 100;
            }
            // Plausibility filter with discard observability.
            if (snLastLevelUpdateMs > 0 && abs(lvl - snLastGoodLevelPct) > LEVEL_MAX_DELTA_PCT) {
              snLevelDiscardCount++;
              static uint32_t lastLvlDiscardWarnMs = 0;
              if (millis() - lastLvlDiscardWarnMs >= 60000UL) {
                lastLvlDiscardWarnMs = millis();
                SENSOR_DBGF("[SN][WARN] Level discard: new=%d last=%d count=%u\n", lvl, snLastGoodLevelPct, (unsigned)snLevelDiscardCount);
              }
              if (snLevelDiscardCount >= US_SAMPLES) {
                snLevelError = true;
                SENSOR_DBGF("[SN][WARN] All level samples discarded in window.\n");
              }
            } else {
              snLastDistanceCm = med;
              snWaterLevelPct = lvl;
              snLastGoodLevelPct = lvl;
              snLastLevelUpdateMs = nowMs;
              snLevelError = false;
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
        // Timeout waiting for fall
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

void sensors_init() {
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
  snLevelDiscardCount = 0;
  flowErrAssertCount = 0;
  flowErrClearCount = 0;
}

void sensors_update_nonblocking() {
  uint32_t now = millis();
  uint32_t nowUs = micros();

  // Flow calc: 1s window, from ISR count
  if ((now - lastFlowCalcMs) >= 1000UL) {
    const uint32_t dtMs = now - lastFlowCalcMs;
    lastFlowCalcMs = now;

    noInterrupts();
    uint32_t pulses = flowPulseCount;
    flowPulseCount = 0;
    uint32_t disc = flowPulseDiscardCount;
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

    // Flow error heuristics:
    // - If we see excessive discarded pulses, treat as noisy/floating input.
    // - If no pulses for a long time AND input reads floating-like (rapid discards) -> error.
    // Since node doesn't know pump state, we do NOT treat "no pulses" alone as an error.
    // Two-stage hysteresis for flow error state.
    if (disc > 50) {
      flowErrAssertCount++;
      flowErrClearCount = 0;
      if (flowErrAssertCount >= 3 && !snFlowError) {
        snFlowError = true;
        SENSOR_DBGF("[SN][ERR] Flow error asserted (disc=%lu).\n", (unsigned long)disc);
      }
    } else if (disc <= 20) {
      flowErrClearCount++;
      flowErrAssertCount = 0;
      if (flowErrClearCount >= 5 && snFlowError) {
        snFlowError = false;
        SENSOR_DBGF("[SN][INFO] Flow error cleared (disc=%lu).\n", (unsigned long)disc);
      }
    } else {
      flowErrAssertCount = 0;
      flowErrClearCount = 0;
    }

#if SENSOR_DEBUG_ENABLED
  SENSOR_DBGF("[SN][INFO] flow=%.2fLPM pulse=%lu raw=%lu disc=%lu pin=%d\n",
        snFlowRateLpm,
        (unsigned long)pulses,
        (unsigned long)rawEdges,
        (unsigned long)disc,
        (int)digitalRead(PIN_FLOW_INPUT));
#endif
  }

  // Ultrasonic service (non-blocking). Uses cached level.
  // The state machine internally schedules its own windows.
  usServiceOnce(now, nowUs);

  // Aggregate ERR code
  snErrCode = (snLevelError ? 1 : 0) | (snFlowError ? 2 : 0);

#if SENSOR_DEBUG_ENABLED
  static uint32_t lastDbgMs = 0;
  if ((uint32_t)(now - lastDbgMs) >= SENSOR_DEBUG_INTERVAL_MS) {
    lastDbgMs = now;
    SENSOR_DBGF("[SN][DBG] lvl=%d%% dist=%.1fcm flow=%.2fLPM err=%d seq=%u ldsc=%u\n", snWaterLevelPct,
                snLastDistanceCm,
                snFlowRateLpm,
                snErrCode,
                (unsigned)snSeq,
                (unsigned)snLevelDiscardCount);
  }
#endif
}

void sensors_get_last_values(int& levelPctOut, float& distanceCmOut, float& flowLpmOut, int& errCodeOut, uint8_t& seqOut) {
  levelPctOut = snWaterLevelPct;
  distanceCmOut = snLastDistanceCm;
  flowLpmOut = snFlowRateLpm;
  errCodeOut = snErrCode;
  seqOut = snSeq;
}

