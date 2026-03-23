// -----------------------------------------------------------------------------
// Sensors (NodeMCU / ESP8266): ultrasonic level + YF-G1 flow
// -----------------------------------------------------------------------------

#include "sensor_node_shared.h"

static uint32_t lastFlowCalcMs = 0;

static void IRAM_ATTR flowIsr() {
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
  attachInterrupt(digitalPinToInterrupt(PIN_FLOW_INPUT), flowIsr, RISING);

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
      if ((uint32_t)(nowUs - usStateEnteredUs) >= 10UL) {
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
        float cm = (float)widthUs / 58.0f;
        if (cm < 2.0f || cm > 250.0f) cm = -1.0f;
        usPushSample(cm);

        if (usSampleCount >= US_SAMPLES) {
          float med = usMedianValid();
          if (med < 0.0f) {
            snLevelError = true;
          } else {
            snLastDistanceCm = med;
            int lvl = cmToPercent(med);
            if (snLastLevelUpdateMs > 0 && abs(lvl - snLastGoodLevelPct) > LEVEL_MAX_DELTA_PCT) {
              // ignore
            } else {
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
    uint32_t disc = flowPulseDiscardCount;
    flowPulseDiscardCount = 0;
    interrupts();

    // dt-aware conversion: Hz = pulses / dt(s), then L/min = Hz / FLOW_HZ_PER_LPM
    const float dtSec = (dtMs > 0) ? ((float)dtMs / 1000.0f) : 1.0f;
    const float hz = (dtSec > 0.0f) ? ((float)pulses / dtSec) : 0.0f;
    snFlowRateLpm = (FLOW_HZ_PER_LPM > 0.01f) ? (hz / FLOW_HZ_PER_LPM) : 0.0f;
    if (snFlowRateLpm < 0.0f) snFlowRateLpm = 0.0f;
    if (snFlowRateLpm > FLOW_MAX_SANE_LPM) snFlowRateLpm = FLOW_MAX_SANE_LPM;
    snLastFlowUpdateMs = now;

    snFlowError = (disc > 50);
  }

  usServiceOnce(now, nowUs);
  snErrCode = (snLevelError ? 1 : 0) | (snFlowError ? 2 : 0);

#if SENSOR_DEBUG_ENABLED
  static uint32_t lastDbgMs = 0;
  if ((uint32_t)(now - lastDbgMs) >= SENSOR_DEBUG_INTERVAL_MS) {
    lastDbgMs = now;
    SENSOR_DBGF("[SN] lvl=%d%% dist=%.1fcm flow=%.2fLPM err=%d seq=%u pulses_discarded=%lu\n",
                snWaterLevelPct,
                snLastDistanceCm,
                snFlowRateLpm,
                snErrCode,
                (unsigned)snSeq,
                (unsigned long)flowPulseDiscardCount);
  }
#endif
}

