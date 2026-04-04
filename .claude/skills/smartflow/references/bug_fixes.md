# SmartFlow — Bug Fix Patterns Reference

All fixes use the comment pattern: `// REFACTOR [BUG_ID]: description`
Only fix bugs confirmed present in Phase 0 audit. Do not touch working code.

---

## C-01 — Missing `void setup()` Declaration

**File:** `arduino_smart_water_pump_controller.ino`
**Fix:** Add explicit declaration if absent.

```cpp
// REFACTOR [C-01]: explicit setup() declaration required for Arduino IDE
void setup() {
  // ... existing setup body
}
```

---

## C-02 — `waterLevelPct` Initialized to 0

**Problem:** ESP32 pushes `water_level_percent: 0` to Firebase before receiving the first
valid RS-485 frame. If `pump_start_level` is above 0%, this triggers a false AUTO pump start.

**Fix — 01_config.ino or shared header:**
```cpp
// REFACTOR [C-02]: sentinel -1 means "not yet known"
int waterLevelPct = -1;
```

**Fix — pushFirebaseStatus() in 05_connectivity_cloud.ino:**
```cpp
// REFACTOR [C-02]: omit level from push until first valid frame
if (waterLevelPct >= 0) {
  statusJson.set("water_level_percent", waterLevelPct);
}
```

**Fix — executePumpLogic() in 03_safety_pump.ino:**
```cpp
// REFACTOR [C-02]: guard all level comparisons against sentinel
if (waterLevelPct < 0) {
  LOG(LOG_INFO, "PUMP", "Level not yet valid — pump logic suspended");
  return;
}
```

---

## H-02 — Level Discard Filter Silent (NodeMCU)

**Problem:** The level plausibility filter `abs(lvl - snLastGoodLevelPct) > LEVEL_MAX_DELTA_PCT`
discards readings with no counter, no log, no error promotion.

**Fix — sensor node:**
```cpp
// REFACTOR [H-02]: level discard observability
static uint16_t snLevelDiscardCount = 0;
static uint32_t lastLevelWarnMs = 0;

// Inside measurement window loop, on plausibility rejection:
snLevelDiscardCount++;
if (millis() - lastLevelWarnMs > 60000 || snLevelDiscardCount == 1) {
  LOG_SN(LOG_WARN, "SENSOR", "Level discard #%d: lvl=%d delta exceeds max",
         snLevelDiscardCount, lvl);
  lastLevelWarnMs = millis();
}

// After window: if all samples rejected, set error
if (snLevelDiscardCount >= validSamplesExpected) {
  snLevelError = true;
  LOG_SN(LOG_ERROR, "SENSOR", "All %d level samples rejected — setting snLevelError",
         validSamplesExpected);
}

// Include in RS-485 response frame (see rs485_protocol.md for LDSC field)
```

---

## H-03 — Flow Discard Debug Print Reads Zeroed Global (NodeMCU)

**Problem:** `pulses_discarded` debug print reads `flowPulseDiscardCount` after it has
been zeroed into local variable `disc`. Always prints 0.

**Fix — one line change:**
```cpp
// BEFORE (wrong — reads zeroed global):
LOG_SN(LOG_DEBUG, "FLOW", "Discarded: %d", flowPulseDiscardCount);

// AFTER (correct — reads local variable):
// REFACTOR [H-03]: use local disc, not zeroed global flowPulseDiscardCount
LOG_SN(LOG_DEBUG, "FLOW", "Discarded: %d", disc);
```

---

## H-04 — Flow Error Flag Non-Hysteretic (NodeMCU)

**Problem:** `snFlowError = (disc > 50)` oscillates true/false every second on borderline
sensor behavior.

**Fix — replace single-sample check with two-stage hysteresis:**
```cpp
// REFACTOR [H-04]: two-stage hysteresis for flow error flag
static uint8_t flowErrorAssertCount = 0;
static uint8_t flowErrorClearCount = 0;

if (disc > 50) {
  flowErrorAssertCount++;
  flowErrorClearCount = 0;
  if (flowErrorAssertCount >= 3) {
    if (!snFlowError) LOG_SN(LOG_WARN, "FLOW", "Flow error asserted after 3s. disc=%d", disc);
    snFlowError = true;
  }
} else if (disc <= 20) {
  flowErrorClearCount++;
  flowErrorAssertCount = 0;
  if (flowErrorClearCount >= 5) {
    if (snFlowError) LOG_SN(LOG_INFO, "FLOW", "Flow error cleared after 5s. disc=%d", disc);
    snFlowError = false;
  }
} else {
  // Hysteresis band: 20 < disc <= 50 — hold current state
  flowErrorAssertCount = 0;
  flowErrorClearCount = 0;
}
```

Assert threshold: `disc > 50`. Clear threshold: `disc <= 20`.
Assert dwell: 3 consecutive seconds. Clear dwell: 5 consecutive seconds.

---

## H-05 — Overflow Protection Stops Pump in MANUAL Mode (ESP32)

**Problem:** Overflow protection fires in MANUAL mode, stopping the pump silently.
MANUAL mode means the operator has explicitly chosen to run the pump.

**Engineering basis:** NEMA MG-1 — operator override expectations for manual control.

**Fix — 03_safety_pump.ino:**
```cpp
// BEFORE (wrong):
if (pumpRunTimeMs > cfgMaxPumpRuntimeMin * 60000UL) {
  stopPump(); setOverflowError();
}

// AFTER: REFACTOR [H-05]: overflow stops pump only in AUTO/COUNTDOWN, warns in MANUAL
if (pumpRunTimeMs > (uint32_t)cfgMaxPumpRuntimeMin * 60000UL) {
  if (controlMode == MODE_MANUAL) {
    // Non-latching warning only — pump continues
    manualRuntimeWarning = true;
    LOG(LOG_WARN, "PUMP", "Manual runtime exceeded %dmin. Operator supervision recommended.",
        cfgMaxPumpRuntimeMin);
  } else {
    stopPump();
    setOverflowError();
  }
}
```

Add `manualRuntimeWarning` to `pushFirebaseStatus()`:
```cpp
statusJson.set("manual_runtime_warning", manualRuntimeWarning);
```

---

## H-06 — Crash Loop Counter Clears at 60s (ESP32)

**Problem:** A 60s time-based clear is too short — full cold boot (WiFi + Firebase) can
take 70–90s, so a crash at 70s is counted as a clean boot.

**Fix — replace time-based with success-based clear:**
```cpp
// REFACTOR [H-06]: clear on first successful Firebase push, 180s fallback
bool crashCounterCleared = false;

// In pushFirebaseStatus(), on first successful push:
if (!crashCounterCleared) {
  crashLoopCount = 0;
  saveToNVS("crash_count", 0);
  crashCounterCleared = true;
  LOG(LOG_INFO, "BOOT", "Crash loop counter cleared on successful Firebase push");
}

// Fallback: still clear after 180s if Firebase never connects
if (!crashCounterCleared && millis() > 180000UL) {
  crashLoopCount = 0;
  saveToNVS("crash_count", 0);
  crashCounterCleared = true;
  LOG(LOG_WARN, "BOOT", "Crash loop counter cleared by 180s fallback (no Firebase)");
}
```

---

## H-07 — No AUTO_COOLDOWN runMode (ESP32)

**Problem:** When the pump stops and the motor off-timer is active, `runMode` stays at its
previous value. Dashboard cannot show cooldown state or countdown.

**Fix — 03_safety_pump.ino / executePumpLogic():**
```cpp
// REFACTOR [H-07]: set cooldown runMode when off-timer is active
if (offTimerActive) {
  uint32_t remainMs = (offTimerEndMs > millis()) ? (offTimerEndMs - millis()) : 0;
  pumpCooldownRemainingSec = (int)(remainMs / 1000);
  runMode = (controlMode == MODE_MANUAL) ? "MANUAL_COOLDOWN" : "AUTO_COOLDOWN";
} else {
  pumpCooldownRemainingSec = 0;
  if (controlMode == MODE_MANUAL) {
    runMode = manualDesired ? "MANUAL_ON" : "MANUAL_OFF";
  } else {
    runMode = isRunning ? "AUTO" : "AUTO_STANDBY";
  }
}
```

Add to `pushFirebaseStatus()`:
```cpp
statusJson.set("pump_cooldown_remaining_sec", pumpCooldownRemainingSec);
```

---

## ISR Safety — Flow Pulse Counter (ESP32)

**Fix — smart_water_pump_controller_shared.h:**
```cpp
// REFACTOR [ISR]: volatile qualifier required for ISR-modified variable
volatile uint32_t g_flowPulseCount = 0;
```

**Fix — ISR (IRAM_ATTR required on ESP32):**
```cpp
void IRAM_ATTR onFlowPulse() {
  g_flowPulseCount++;  // Keep ISR minimal
}
```

**Fix — safe accessor (replaces all direct reads in main loop):**
```cpp
uint32_t readAndResetFlowPulses() {
  portDISABLE_INTERRUPTS();
  uint32_t count = g_flowPulseCount;
  g_flowPulseCount = 0;
  portENABLE_INTERRUPTS();
  return count;
}
```

---

## M-01 — Two Overlapping Level Timestamps (ESP32)

**Fix:** Remove `levelLastValidMs`. Use `levelLastUpdateMs` as the single authoritative
timestamp everywhere. Update it only when a valid RS-485 frame arrives AND the ultrasonic
error bit in the frame's ERR field is clear (`(remoteSensorLastErrCode & 0x01) == 0`).

```cpp
// REFACTOR [M-01]: single authoritative level timestamp
// In pollRemoteSensorNode(), on valid frame with no ultrasonic error:
if (frameValid && (remoteSensorLastErrCode & 0x01) == 0) {
  levelLastUpdateMs = millis();
  // ... update waterLevelPct, etc.
}

// Apply consistently in:
// - checkDryRunProtection() — use levelLastUpdateMs
// - executePumpLogic() — use levelLastUpdateMs
// - pushFirebaseStatus() — use levelLastUpdateMs for level_fresh calculation
```

---

## M-02 — `bypass_flow_sensor` No Firebase Control Path (ESP32)

**Fix — readFirebaseControl() in 05_connectivity_cloud.ino:**
```cpp
// REFACTOR [M-02]: bypass_flow_sensor runtime control via Firebase
bool newBypassFlow = controlJson.getBool("bypass_flow_sensor", cfgBypassFlowSensor);
if (newBypassFlow != cfgBypassFlowSensor) {
  cfgBypassFlowSensor = newBypassFlow;
  saveToNVS("bypass_flow", cfgBypassFlowSensor ? 1 : 0);
  LOG(LOG_INFO, "NVS", "bypass_flow_sensor=%s saved to NVS",
      cfgBypassFlowSensor ? "true" : "false");
}
```

Add to `pushFirebaseStatus()`:
```cpp
statusJson.set("bypass_flow_sensor", cfgBypassFlowSensor);
```

---

## M-03 — RS-485 Partial Frame Never Resets on Stall (NodeMCU)

**Fix — RS-485 slave receive loop:**
```cpp
// REFACTOR [M-03]: inter-byte stall reset for partial frames
static uint32_t lastByteMs = 0;

if (SN_SERIAL_RS485.available()) {
  lastByteMs = millis();
  byte b = SN_SERIAL_RS485.read();
  // ... existing byte processing
}

// Stall detection: partial frame with no new bytes for 20ms
if (rxPos > 0 && (millis() - lastByteMs) > 20) {
  LOG_SN(LOG_DEBUG, "RS485", "Partial frame stall — resetting. rxPos=%d", rxPos);
  rxPos = 0;
}
```

---

## M-05 — `runMode` Initialized to "OFF" (ESP32)

**Fix — 01_config.ino or shared header:**
```cpp
// REFACTOR [M-05]: initial runMode should be AUTO_STANDBY, not OFF
String runMode = "AUTO_STANDBY";
```

---

## M-06 — `is_idle_mode` Not in Firebase Status (ESP32)

**Fix — pushFirebaseStatus() in 05_connectivity_cloud.ino:**
```cpp
// REFACTOR [M-06]: report idle mode to dashboard
statusJson.set("is_idle_mode", isIdleMode);
```

---

## DRY_RUN_THRESHOLD_LPM — Default Update

```cpp
// REFACTOR: update default from 0.5f to 1.0f
// Engineering basis: YF-G1 datasheet — working range 1–60 L/min.
// Sub-1 L/min pulse detection is unreliable against ISR timing noise.
// Confirm via bucket calibration after installation.
const float DRY_RUN_THRESHOLD_LPM = 1.0f;
```

---

## Firebase Write Error Backoff

```cpp
// REFACTOR: exponential backoff for Firebase write retries
uint32_t backoffMs = min(1000UL * (1UL << consecutiveFirebaseFailures), 30000UL);
// Use non-blocking millis() gate, not delay(), to avoid watchdog
```

---

## Arduino String Heap Fragmentation — Hot Path Fix

Replace `String` concatenation in the main loop body and per-cycle functions:

```cpp
// BEFORE (heap-fragmenting):
String payload = "LVL:" + String(waterLevelPct) + ";FLOW:" + String(flowRate);

// AFTER (fixed-size buffer, no heap allocation):
// REFACTOR [HEAP]: char[] + snprintf in hot paths to prevent fragmentation
char payload[64];
snprintf(payload, sizeof(payload), "LVL:%d;FLOW:%.2f", waterLevelPct, flowRate);
```

Reserve Arduino `String` class only for one-time setup operations (boot banner, NVS reads
during `setup()`).
