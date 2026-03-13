# Sensor Refinement Plan — Verified Against Research

This plan implements the sensor fixes recommended after cross-checking Gemini’s proposals with ESP32 documentation and real-world IoT reports. It refines the **existing** v2.4.0 changes (ISR debounce, ultrasonic constants, flow logic) and adds the **pump-off flow gate**.

---

## Goals

1. **Flow ISR**: Use **ISR-safe** timing (`esp_timer_get_time()`) and a **2 ms** debounce to eliminate ghost pulses from crosstalk/EMI without risking ESP32 panic.
2. **Ultrasonic display**: Retune **EMA** and **rate-of-change guard** so they work together — faster response without passing EMI spikes (alpha 0.5, ROC 15%, timeout 100 ms, sample delay 80 ms).
3. **Flow when pump off**: Add a **pump-off flow gate** in `calculateFlowRate()` so any pulses after 3 s with pump off are treated as noise (hard zero to dashboard and dry-run logic).

No new hardware is required for this plan. Optional hardware (pull-up at tank, capacitors) remains as documented elsewhere.

---

## Current State (After First Round of Changes)

| Item | Current value | Location |
|------|----------------|----------|
| Flow ISR timing | `micros()` | `flowPulseISR()` |
| Flow debounce | 800 µs, `lastFlowPulseMicros` (uint32_t) | SECTION 7 |
| EMA alpha | 0.6f | SECTION 4 |
| Rate-of-change max | 50 %/s | SECTION 4 |
| Ultrasonic timeout | 100 ms | SECTION 4 |
| Sample delay | 90 ms | SECTION 4 |
| Pump-off flow gate | None | `calculateFlowRate()` |

---

## Planned Changes (Detailed)

### 1. Flow ISR — Switch to `esp_timer_get_time()` and 2 ms Debounce

**Rationale**

- `micros()` in an ISR on ESP32 can involve 64-bit division that is not IRAM-safe and may cause panic (documented in community reports).
- `esp_timer_get_time()` returns a 64-bit microsecond count and is ISR-safe.
- 2 ms (2000 µs) debounce: at max flow (e.g. 100 LPM, K=7.5 → 750 Hz) real pulses are ~1333 µs apart; 2 ms still accepts all real pulses and strongly filters crosstalk/contactor bursts.

**File:** `firmware/smart_pump_controller/smart_pump_controller.ino`

**1.1 Include (if not already available)**

- Add near other ESP-IDF includes (e.g. after `esp_sleep.h`):
  ```c
  #include <esp_timer.h>   // ISR-safe time for flow debounce
  ```
- If the project already builds without it (transitive include), the build will still succeed; add only if needed for a clean compile.

**1.2 Global variable (SECTION 6 — Sensor Data)**

- **Remove:** `volatile uint32_t lastFlowPulseMicros = 0;`
- **Add:** `volatile uint64_t lastPulseUs = 0;`  
  (64-bit to match `esp_timer_get_time()` and avoid rollover issues.)

**1.3 Constant and ISR (SECTION 7)**

- **Replace** the current debounce constant and ISR body:
  - Set debounce to **2000 µs** (2 ms), e.g.:
    ```c
    // YF-G1 at max ~100 LPM with K=7.5 → 750 Hz → min ~1333 µs between real pulses.
    // 2 ms filters noise bursts; still accepts all real pulses at max flow.
    #define FLOW_DEBOUNCE_US  2000ULL
    ```
  - In `flowPulseISR()`:
    - Use `uint64_t now = esp_timer_get_time();`
    - Use `if (now - lastPulseUs > FLOW_DEBOUNCE_US)`
    - Then `pulseCount = pulseCount + 1;` and `lastPulseUs = now;`
- Keep the rest of the ISR minimal (no Serial, no heavy math).

**1.4 Edge cases**

- **First pulse after boot:** `lastPulseUs == 0` → `now - 0` is large → first pulse always accepted; reset of `lastPulseUs` on first run is correct.
- **Rollover:** `esp_timer_get_time()` is a 64-bit microsecond counter; no 32-bit rollover issue.

---

### 2. Ultrasonic Constants — EMA 0.5, ROC 15%, Timeout 100 ms, Sample Delay 80 ms

**Rationale**

- **EMA 0.5:** Balances response speed and noise; 0.6 was too aggressive for an EMI-heavy environment (research recommendation).
- **ROC 15%:** Real 660 L tank cannot change 15% in 1 s; this rejects EMI-induced jumps while allowing normal EMA movement.
- **Timeout 100 ms:** Already set; keep (cable attenuation over 40 m).
- **Sample delay 80 ms:** Slightly less than current 90 ms; matches research; still gives plenty of settling time between pings.

**File:** `firmware/smart_pump_controller/smart_pump_controller.ino`  
**Section:** SECTION 4 (SAFETY & TIMING CONSTANTS)

**2.1 Exact edits**

| Constant | Current | New | Comment |
|----------|---------|-----|--------|
| `LEVEL_RATE_OF_CHANGE_MAX` | 50 | **15** | Tighter guard; physical max << 15%/s |
| `ULTRASONIC_EMA_ALPHA` | 0.6f | **0.5f** | Better noise/response balance |
| `ULTRASONIC_SAMPLE_DELAY` | 90 | **80** | Settling between pings (ms) |
| `ULTRASONIC_TIMEOUT_MS` | 100 | 100 | No change |

**2.2 Comment updates**

- For `LEVEL_RATE_OF_CHANGE_MAX`: e.g. “Max % change per second before holding previous (tight guard for EMI; real tank << 15%/s)”.
- For `ULTRASONIC_EMA_ALPHA`: e.g. “Exponential moving average (0.5 = balance response vs noise for long cable).”
- For `ULTRASONIC_SAMPLE_DELAY`: e.g. “ms between samples (settling for JSN-SR04T over 40 m CAT6).”

---

### 3. Pump-Off Flow Gate in `calculateFlowRate()`

**Rationale**

- When the pump has been off for more than 3 seconds, any flow reading is physically impossible in this plumbing; pulses are noise (crosstalk/EMI).
- Forcing 0.0 LPM after 3 s pump-off prevents ghost flow from affecting the dashboard and dry-run logic.

**File:** `firmware/smart_pump_controller/smart_pump_controller.ino`  
**Function:** `calculateFlowRate()` (SECTION 10)

**3.1 Logic**

- **Static state:** `static unsigned long pumpOffSince = 0;` (first time pump goes off, record time).
- **When pump is OFF:**
  - If `pumpOffSince == 0`, set `pumpOffSince = millis();`
  - If `millis() - pumpOffSince > 3000`, return `0.0f` **after** atomically reading and clearing `pulseCount` (so ghost pulses are not carried into the next window).
- **When pump is ON:** Set `pumpOffSince = 0` so the gate resets for the next pump-off period.

**3.2 Placement**

- Immediately after the `noInterrupts()/interrupts()` block that reads and resets `pulseCount`.
- Before computing `lpm` from `count`.
- Use a constant for the 3000 ms, e.g. `#define FLOW_PUMP_OFF_ZERO_MS 3000` in SECTION 4 (with other flow/sensor constants) and reference it in the comment.

**3.3 Behaviour**

- First 3 seconds after pump turns off: reported flow can still show residual pulses (or last window’s value); after 3 s, report 0.0 LPM until pump runs again.
- Dry-run logic sees 0.0 LPM when pump is off for >3 s, which is correct (no false dry-run from ghost flow).

---

### 4. Documentation Updates

**File:** `firmware/README.md`

**4.1 Sensor / filtering section (if present)**

- State that flow ISR uses **esp_timer_get_time()** for ISR-safe debounce (2 ms).
- State that **pump-off flow gate** forces 0.0 LPM when pump has been off >3 s.
- List current ultrasonic tuning: EMA 0.5, ROC 15%, timeout 100 ms, sample delay 80 ms.

**4.2 Optional: “Sensor hardening (40 m CAT6)” subsection**

- Short note: long CAT6 run + 220 V/contactor → crosstalk and EMI; firmware uses ISR debounce, pump-off gate, median + EMA + ROC guard; optional hardware (pull-up at tank, 100 µF + 100 nF caps) recommended if issues persist.

---

## Implementation Order

1. **Include and globals** — Add `esp_timer.h` (if needed), replace `lastFlowPulseMicros` with `lastPulseUs` (uint64_t).
2. **Flow ISR** — Implement 2 ms debounce using `esp_timer_get_time()` and `lastPulseUs`.
3. **Ultrasonic constants** — Apply the four constant changes and comment updates in SECTION 4.
4. **Pump-off gate** — Add `FLOW_PUMP_OFF_ZERO_MS` and the gate logic in `calculateFlowRate()`.
5. **README** — Update sensor/filtering and optional hardening subsection.

---

## Verification (Minimal Checklist)

- **Build:** Sketch compiles without errors or warnings for target ESP32 board.
- **Flow ISR:** No use of `millis()` or unsafe operations in the ISR; only `esp_timer_get_time()`, comparison, and assignment.
- **Constants:** Grep confirms `LEVEL_RATE_OF_CHANGE_MAX` 15, `ULTRASONIC_EMA_ALPHA` 0.5f, `ULTRASONIC_SAMPLE_DELAY` 80, `ULTRASONIC_TIMEOUT_MS` 100.
- **Pump-off gate:** When pump is off >3 s, `calculateFlowRate()` returns 0.0f; when pump is on, `pumpOffSince` is reset and normal conversion is used.
- **Runtime (optional):** With pump off and long CAT6, flow stays 0.0 LPM after 3 s; level display moves without getting stuck at a single value when tank level actually changes.

---

## Summary Table

| Change | Location | Purpose |
|--------|----------|--------|
| `#include <esp_timer.h>` | Top of .ino | ISR-safe time (if needed) |
| `lastPulseUs` (uint64_t) | SECTION 6 | Replace lastFlowPulseMicros |
| FLOW_DEBOUNCE_US 2000 | SECTION 7 | 2 ms debounce |
| flowPulseISR() use esp_timer_get_time() | SECTION 7 | Safe ISR timing |
| LEVEL_RATE_OF_CHANGE_MAX 15 | SECTION 4 | Tighter ROC guard |
| ULTRASONIC_EMA_ALPHA 0.5f | SECTION 4 | Balance response vs noise |
| ULTRASONIC_SAMPLE_DELAY 80 | SECTION 4 | Settling between pings |
| FLOW_PUMP_OFF_ZERO_MS 3000 | SECTION 4 | Constant for gate |
| Pump-off gate in calculateFlowRate() | SECTION 10 | Force 0 LPM when pump off >3 s |
| README sensor/hardening notes | firmware/README.md | Document behaviour and options |

This plan is self-contained and does not modify the plan file or other features (sleep, reboot, dashboard).
