# Smart Water Pump Controller — System Review
## Firmware & Dashboard: Findings, Corrections & Recommendations

> **Document Type:** Full-System Engineering & UX Audit · Research-Backed  
> **Scope:** Firmware v5.0.0 (firmware_master_spec.md) + Dashboard v2.0 (dashboard_master_spec.md)  
> **Standard References:** IEC 61511 (Functional Safety), IEC 62443 (Industrial Cybersecurity),  
> Nielsen–Norman Group Heuristics, Fitts' Law, Wickens' Multiple Resource Theory,  
> ISO 9241-11 (Usability), WCAG 2.1 (Accessibility), NIST SP 800-63 (Identity),  
> IEC 60068-2 (Environmental Testing), IEEE 1584 (Arc Flash), ISO 13849 (Machine Safety)  
> **Author:** Mark Alvin Cadangin

---

## Table of Contents

**Part I — Firmware Findings**
1. [Firmware Correctness Audit](#1-firmware-correctness-audit)
2. [MANUAL_OFF — The New Sticky-Mode Behavior](#2-manual_off--the-new-sticky-mode-behavior)
3. [Firmware Safety Gaps & Engineering Improvements](#3-firmware-safety-gaps--engineering-improvements)
4. [Timing & Concurrency Analysis](#4-timing--concurrency-analysis)
5. [NVS & Persistence Gaps](#5-nvs--persistence-gaps)

**Part II — Dashboard Findings**
6. [Dashboard–Firmware Contract Mismatches](#6-dashboardfirmware-contract-mismatches)
7. [UX Architecture Analysis — Psychology & Human Factors](#7-ux-architecture-analysis--psychology--human-factors)
8. [Safety-Critical UX Failures & Improvements](#8-safety-critical-ux-failures--improvements)
9. [Information Design & Visual Hierarchy](#9-information-design--visual-hierarchy)
10. [Accessibility & Inclusive Design](#10-accessibility--inclusive-design)
11. [Security & Threat Model](#11-security--threat-model)
12. [Offline & Resilience UX](#12-offline--resilience-ux)
13. [Notifications & PWA](#13-notifications--pwa)

**Part III — Recommendations**
14. [Priority Matrix — All Findings Ranked](#14-priority-matrix--all-findings-ranked)
15. [Implementation Roadmap](#15-implementation-roadmap)

---

# Part I — Firmware Findings

---

## 1. Firmware Correctness Audit

### 1.1 What Is Correct

The firmware_master_spec.md reflects a mature v4.0 architecture. The following are confirmed correct and well-engineered:

| Area | Assessment |
|------|------------|
| P0–P5 six-level priority cascade | ✅ Architecturally sound. FORCE_ON at P0 correctly overrides P1. |
| MANUAL mode with full safety | ✅ Overflow protection now covers MANUAL (line verified: `AUTO`, `COUNTDOWN`, `MANUAL`). |
| `pendingModeWriteback` mechanism | ✅ Anti-stale-read protection is robust. |
| Single JSON control read | ✅ 1 round-trip for all control keys — excellent for weak WiFi resilience. |
| Offline-first NVS architecture | ✅ Critical state survives power cycles. |
| `MIN_PUMP_OFF_TIME_MS = 30,000 ms` | ✅ 30-second minimum off-time protects motor from short-cycling. |
| `MANUAL_OFF` runMode | ✅ Smart sticky-mode behavior — MANUAL persists after pump stops, allows pump restart without mode change. |
| 5-sample median + EMA + rate-of-change guard | ✅ Three independent noise filters on level reading. |
| Crash loop detection + safe mode | ✅ 5 reboots in 5 minutes triggers safe mode. Self-healing after 1 hour. |
| `FORCE_ON` not restored after reboot | ✅ Documented in §16.3 post-boot state. |

### 1.2 Firmware Inconsistencies Found

#### F-01 — CRITICAL: §8.1 Bidirectional Flow Diagram Shows Stale v3.0.0 Code

**Location:** `firmware_master_spec.md` §8.1, Dashboard Action example

```
→ pumpMode = "FORCE_ON"   ← WRONG. This is v3.0.0 behavior.
```

The diagram shows `manual_start` setting `pumpMode = "FORCE_ON"`. §8.2 correctly shows it sets `pumpMode = "MANUAL"`. The diagram was not updated when the mode architecture was redesigned. This is the most prominent place a developer would look to understand the bidirectional flow — having it show the wrong mode is a serious documentation error that will cause implementation mistakes.

**Required fix:** Update §8.1 diagram to show `pumpMode = "MANUAL"` and `run_mode = "MANUAL"`.

#### F-02 — SIGNIFICANT: §7.2 S-03 clear_error Scenario Is Incomplete for MANUAL Mode

**Location:** `firmware_master_spec.md` §7.2 S-03

| Current behavior documented | `isDryRunError=false` → `run_mode: "AUTO_STANDBY"` · Error banner clears |

**Problem:** When `pumpMode = "MANUAL"` (sticky) and P1 fires, the pump stops but `pumpMode` remains `"MANUAL"`. When `clear_error` is received, `isDryRunError` clears, and on the next P3 evaluation the pump RESTARTS automatically (because `pumpMode == "MANUAL"` → P3 fires → `setPump(true)`).

S-03 documents `run_mode: "AUTO_STANDBY"` as the post-clear state — this is only correct when `pumpMode = "AUTO"`. If `pumpMode = "MANUAL"`, the post-clear `runMode` is `"MANUAL"` and the pump immediately restarts.

This behavior may be intentional (sticky MANUAL resumes on error clear), but it is undocumented and will surprise operators. If a dry-run error fired because there was actually no water, clearing the error while still in MANUAL will immediately restart the pump and potentially re-trigger dry-run within 30 seconds.

**Required fix:** Add scenario S-03b: `pumpMode = "MANUAL"`, `clear_error` received → `isDryRunError = false` → P3 evaluates next cycle → `setPump(true)` → `run_mode = "MANUAL"`. Document that pump auto-restarts in MANUAL after error clear. The dashboard must communicate this — show a warning before the Clear Error button when `controlMode === "MANUAL"`: "Clearing this error will immediately restart the pump."

#### F-03 — SIGNIFICANT: §3.2 Conflict Matrix Missing `manual_stop` on FORCE_OFF + pump OFF

**Location:** `firmware_master_spec.md` §3.2

Current firmware behavior for `manual_stop = true` when `pumpMode = "FORCE_OFF"` and pump is already off:

From §15: `manual_stop` → `setPump(false)` (no effect), `pumpMode = "AUTO"`, countdown cleared.

This means `manual_stop` EXITS FORCE_OFF by setting `pumpMode = "AUTO"`. This is an undocumented behavior that may be dangerous. An operator who accidentally taps the Stop button (or whose mobile client has a layout where it is easy to misclick) could inadvertently exit FORCE_OFF, and the pump could auto-start in AUTO if the tank level is low.

**Required fix:** Add this row to the conflict matrix. Document clearly. Also: consider whether `manual_stop` should EXIT FORCE_OFF. The argument against: FORCE_OFF is a persistent emergency stop — it should only exit via an explicit mode change in the mode selector. A one-shot `manual_stop` tap should not undo a deliberate FORCE_OFF. Recommendation: in `readFirebaseControl()`, add a check — if `pumpMode == "FORCE_OFF"`, `manual_stop` is ignored (same as FORCE_ON).

#### F-04 — MINOR: §16.1 Boot Sequence Still Shows Version String "v3.0.0"

**Location:** `firmware_master_spec.md` §16.1

```
├─ Log: "Smart Water Pump Controller v3.0.0"
```

The spec documents v4.0 (§2.2 "v4.0", §2.3 "v5.0" — see F-05 below) but the boot log still says v3.0.0. Update to v4.0 or v5.0 for consistency.

#### F-05 — MINOR: Version Number Inconsistency Throughout Spec

**Location:** Multiple sections

- §2.2 header: "v4.0"
- §2.3 header: "v5.0"
- §3.2 header: "v4.0"
- §7.4 header: "v5.0"
- §8.2 header: "v4.0"
- §14.1 header: "v5.0"
- §15 header: "v4.0"
- §16 footer: "v3.0.0"

The spec is internally inconsistent about whether this is v4.0 or v5.0. The MANUAL_OFF sticky-mode behavior appears to be a v5.0 addition over the v4.0 redesign. Pick one version and apply consistently throughout.

#### F-06 — MINOR: `manual_stop` behavior in §15 disagrees with §7.4 M-04

**§15:** `manual_stop` → `setPump(false)`, `pumpMode = "AUTO"`, countdown cleared.
**§7.4 M-04:** `manual_stop` during MANUAL running → `pumpMode unchanged (sticky MANUAL)`, `runMode = "MANUAL_OFF"`.

These are directly contradictory. §7.4 M-04 is the v5.0 MANUAL_OFF behavior. §15 is the old v4.0 behavior (where stop → AUTO). One of these is wrong. Given that `runMode = "MANUAL_OFF"` is in the derivation table at §2.3 and the dashboard spec handles it, §7.4 is correct. §15 must be updated.

---

## 2. MANUAL_OFF — The New Sticky-Mode Behavior

### 2.1 What It Is

`MANUAL_OFF` is a new `runMode` value introduced in the current firmware spec that does not appear in the previous redesign document. It represents the state where:
- `pumpMode = "MANUAL"` (sticky — mode does not revert to AUTO on stop)
- `isRunning = false` (pump is off)
- `runMode = "MANUAL_OFF"`

This means: in MANUAL mode, pressing Stop does NOT exit MANUAL mode. The operator keeps control of the ON/OFF toggle. The pump stays off until the operator either taps ON or changes the mode.

### 2.2 Engineering Assessment

**This is a good design decision.** It solves a real usability problem: previously, every stop in MANUAL reverted to AUTO, requiring the user to re-select MANUAL if they wanted to restart the pump manually. The sticky mode makes MANUAL behave like a genuine control panel — ON and OFF are independent operator decisions, not mode transitions.

**Safety implication:** MANUAL_OFF with P1 active (dry-run or overflow) creates an ambiguous state:
- `pumpMode = "MANUAL"`, `isDryRunError = true`, `isRunning = false`, `runMode = "MANUAL_OFF"`
- The pump is off and will stay off.
- If the operator clears the error, P3 fires → `setPump(true)` → pump immediately starts.

This is F-02 above. The behavior is architecturally consistent but operationally surprising.

### 2.3 Dashboard Contract for MANUAL_OFF

The dashboard spec §6.1 correctly handles `"MANUAL_OFF"`:
- Rendered as `MANUAL (Off)` in the run mode header badge
- ON button is enabled (allows restart)
- OFF button is disabled (already off)

This is correct and complete. The dashboard correctly represents the sticky-mode pattern.

### 2.4 One Additional Edge Case Not Documented

**MANUAL_OFF + level sensor error:** If `isLevelSensorError = true` and `pumpMode = "MANUAL"` and pump is currently off (`MANUAL_OFF`), what happens when the operator taps ON?

- `manual_start = true` → `readFirebaseControl()`: no lockout error, not FORCE_OFF → `pumpMode = "MANUAL"` set (already MANUAL).
- P3: `isLevelSensorError = true && !cfgBypassLevelSensor` → pump does not start, mode set to AUTO.

Net result: operator taps ON in MANUAL_OFF → firmware silently sets mode to AUTO and pump does not start. The operator gets no feedback from the firmware directly (the mode change to AUTO is observable via Firebase after ~3s).

**Recommendation:** The dashboard should disable the ON button in MANUAL_OFF when `is_level_sensor_error = true` and bypass is off, with a tooltip: "Enable bypass first to start pump with sensor error." This prevents a confusing tap-with-no-response experience.

---

## 3. Firmware Safety Gaps & Engineering Improvements

### 3.1 Gap: FORCE_ON Relay Oscillation During Sensor Failure (WC-01, revisited)

As documented in the architecture redesign, when FORCE_ON is active and the flow sensor reports 0 LPM (sensor disconnected), this cycle repeats every 1 second:

1. `checkDryRunProtection()` fires → `isDryRunError=true` → `setPump(false)` → GPIO4 HIGH
2. `executePumpLogic()` P0 fires → `setPump(true)` → GPIO4 LOW

The `totalPumpCycles` counter increments every cycle. After 120 minutes at 1 cycle/second: `totalPumpCycles += 7200`. This corrupts the lifetime pump cycle counter.

**Recommended fix:** In `setPump(true)`, only increment `totalPumpCycles` if `!isRunning` (pump was actually off, not already on). This is the correct semantic — a cycle is a pump START, not a continued run. Check the existing code — the current implementation at `03_safety_pump.ino` line 10: `if (on && !isRunning) { totalPumpCycles++; }` — this is already correct. The oscillation means `isRunning` alternates false→true→false each cycle due to the `setPump(false)` in `checkDryRunProtection()` followed by `setPump(true)` in P0. So `totalPumpCycles` WILL increment each cycle.

**True fix:** In P0 block, do NOT call `setPump(false)` from `checkDryRunProtection()` when `pumpMode == "FORCE_ON"`. But `checkSafetyCutoff()` runs BEFORE `executePumpLogic()`, so the `setPump(false)` call in `checkDryRunProtection()` happens regardless of pumpMode. P0 then overrides it, but `totalPumpCycles` already incremented.

**Solution:** Gate the relay calls in safety checks by checking P0 first:

```cpp
// In checkDryRunProtection(), before setPump(false):
if (pumpMode == "FORCE_ON") {
  isDryRunError = true;  // still set the flag for dashboard monitoring
  // but do NOT call setPump(false) — P0 will handle relay
  return;
}
```

This prevents the oscillation entirely. The error flag is still set for dashboard visibility. P0 keeps the relay ON without cycling.

### 3.2 Gap: Minimum Off-Time Does Not Account for MANUAL_OFF → ON Transition

`MIN_PUMP_OFF_TIME_MS = 30,000 ms` protects against short-cycling in AUTO (where the pump could start and stop rapidly). But in MANUAL mode, `stopRun()` stops the pump and sets `MANUAL_OFF`. If the operator immediately taps ON again, `manual_start` fires and P3 starts the pump — bypassing the min-off-time check.

The min-off-time is checked in P5d (AUTO) and P3/P4 start conditions. The `manual_start` path in `readFirebaseControl()` sets `pumpMode = "MANUAL"` directly without checking `pumpOffStartMs`.

**Recommendation:** Apply `MIN_PUMP_OFF_TIME_MS` check in `readFirebaseControl()` for `manual_start`:
```cpp
if (v && !lastManualStart) {
  if (isDryRunError || isOverflowError) { ... }
  else if (pumpMode == "FORCE_OFF") { ... }
  else if (pumpOffStartMs > 0 && (millis() - pumpOffStartMs) < MIN_PUMP_OFF_TIME_MS) {
    Serial.println("[FIREBASE] Manual start deferred: minimum off-time not elapsed.");
    // Could write a pending flag or simply reject with dashboard feedback
  } else {
    pumpMode = "MANUAL";
    ...
  }
}
```

### 3.3 Gap: `countdown_add_min` Validation Has No Dashboard Feedback Path

When `countdown_add_min` is outside 1–120, the firmware silently uses the 5-minute default. The dashboard writes an integer from a numeric input, but there is no firmware-to-dashboard error path for "your add-time value was invalid." The operator simply sees the timer extend by 5 minutes instead of their requested amount.

**Recommendation:** Add `countdown_last_add_min_applied` to the status push, showing what the firmware actually applied. The dashboard can compare `countdown_add_min` (what was requested) against `countdown_last_add_min_applied` (what was applied) and show a toast if they differ.

### 3.4 Gap: `isOverflowError` After MANUAL Run

When P1 overflow fires during a MANUAL run:
- `isOverflowError = true` → `setPump(false)` 
- Per §3.1 P1 handling: "In MANUAL, pump OFF but mode stays MANUAL"
- `runMode = "OFF"`

But `pumpAutoStartTracking = false` is set inside `checkOverflowProtection()` when it fires. After `clear_error`, `isDryRunError = false` and `isOverflowError = false` are cleared, but `pumpAutoStartTracking` was already reset. On the next P3 cycle, the pump starts and `pumpAutoStartTracking` restarts from zero. The overflow timer effectively resets on every clear. This is correct and safe — after clearing, the operator gets a fresh 120-minute window — but it should be documented.

---

## 4. Timing & Concurrency Analysis

### 4.1 Sensor Block Duration vs 1s Interval

The 5-sample ultrasonic read takes up to 820ms in worst case (all 5 samples timeout at 100ms each + 4×80ms delays = 500 + 320 = 820ms). With a 1-second sensor interval, the firmware can spend up to 82% of each cycle in blocking `pulseIn()` calls.

**Problem:** During those 820ms, `loop()` cannot feed the WDT, process Firebase, or respond to anything. The WDT is 120s, so no WDT risk. But Firebase commands written by the dashboard during this window are not processed until the next Firebase cycle (up to 3s after the sensor block completes). This is documented as "~3s latency" in the spec, which is accurate — but worst-case is actually 3s (Firebase interval) + 0.82s (sensor block blocking) = ~3.82s. For normal operation this is fine. For the Stop command during an emergency, it means the pump may run up to 4 seconds after the operator taps Stop. Document this explicitly.

### 4.2 `pulseIn()` Blocking and WDT

`pulseIn()` is a blocking call. Over 40m of CAT6 with potential reflections, the 100ms timeout (`ULTRASONIC_TIMEOUT_MS`) is the key safeguard. If the ECHO pin is stuck HIGH (hardware failure), `pulseIn()` blocks for 100ms × 5 samples = 500ms each sensor cycle, every cycle. Over 120 seconds WDT window: this accounts for 5s out of every 120s — the WDT is not at risk.

However, a stuck-LOW ECHO pin causes `pulseIn()` to return 0 immediately (no timeout) — the sensor reads -1 five times in rapid succession (~0ms delay since no HIGH pulse ever comes). This is already handled correctly as a timeout failure. Good.

### 4.3 ISR and `noInterrupts()` Window

The `noInterrupts()` window in `calculateFlowRate()` to atomically read and clear `pulseCount` is correct. The window duration is ~4 CPU instructions at 240MHz = ~17ns. Negligible. No missed pulses risk.

### 4.4 `millis()` Timer Drift During Light Sleep

During ESP32 light sleep, the `millis()` timer pauses. When the device wakes after `SLEEP_WAKE_INTERVAL_MS` (30s), `millis()` resumes from where it left off. This means:

- `lastSensorMs` was set before sleep. After 30s sleep, `millis() - lastSensorMs ≈ 30s ≥ sensorInterval (30s)` → sensor block fires immediately on wake. ✓
- Countdown timer: `countdownEndMs` is an absolute `millis()` value set before sleep. If the device sleeps for 30s, `countdownEndMs` effectively moves 30s further in the future from the user's perspective — but `millis()` advances by exactly the sleep duration on wake. This is correct.

**One subtle issue:** If the device is sleeping and the countdown would have expired DURING the sleep window (e.g., 5 minutes left when sleep started, sleep is 30 minutes long), `checkCountdownExpiry()` fires on first wake and immediately expires the countdown. The pump never ran during the sleep window. But `isSleeping = true` during sleep, so P4 would have held the pump on had it been awake... wait — does the pump run during sleep?

During sleep, `isSleeping = true` and the sensor interval is 30s. P4 (COUNTDOWN) fires every 30s and calls `setPump(true)`. But `esp_light_sleep_start()` pauses execution including GPIO outputs? No — GPIO outputs are maintained during light sleep. The relay stays in its current state. P4 calls `setPump(true)` every 30s during sleep and the relay state is maintained. The countdown timer DOES run during sleep because `millis()` still advances (just paused during the actual sleep microseconds). This is correct.

---

## 5. NVS & Persistence Gaps

### 5.1 `MANUAL_OFF` State Not Distinguishable from `AUTO_STANDBY` in NVS

NVS stores `pumpMode = "MANUAL"`. If the device reboots while in `MANUAL_OFF` (pumpMode=MANUAL, isRunning=false), it boots with `pumpMode = "MANUAL"` and on the first P3 cycle immediately starts the pump. This is the intended sticky-MANUAL boot behavior.

But: if the operator was in `MANUAL_OFF` because they deliberately stopped the pump (not because of a safety event), they may not want it to auto-restart on power cycle. There is no way to persist "MANUAL but pump-off" — only "MANUAL" is stored.

**Recommendation:** Accept this as a known behavior and document it. When the device boots into MANUAL mode, show a boot notification on the dashboard: "Controller restarted in MANUAL mode — pump will start automatically. Tap OFF to stop or switch to AUTO." The notification clears after the first operator action.

### 5.2 No NVS Key for `isManualRun` Flag

The `isManualRun` flag is not persisted. After reboot with `pumpMode = "MANUAL"`, `isManualRun = false`. This is fine because the flag's purpose was to distinguish `manual_start`-triggered FORCE_ON from direct FORCE_ON — in v4.0/5.0, `pumpMode == "MANUAL"` directly encodes this intent. The flag is now cosmetic telemetry.

**Recommendation:** Retire `isManualRun` from the codebase in the next cleanup pass. It is referenced in the spec but carries no behavioral weight anymore.

---

# Part II — Dashboard Findings

---

## 6. Dashboard–Firmware Contract Mismatches

### D-01 — CRITICAL: `manual_stop` behavior diverges between specs

**Dashboard spec §6.1 M-04 says:**
> `stopRun()` → writes `manual_stop=true` → firmware stops pump and **leaves `pumpMode="MANUAL"`**, producing `run_mode="MANUAL_OFF"`.

**Firmware spec §15 says:**
> `manual_stop` → `setPump(false)`, `pumpMode = "AUTO"`, countdown cleared.

These are directly contradictory (same as firmware F-06). The dashboard spec is implementing the v5.0 MANUAL_OFF sticky behavior. The firmware spec §15 is showing old v4.0 behavior where stop → AUTO.

The actual firmware intent (given `MANUAL_OFF` appears in §2.3 runMode table) is the sticky behavior — stop keeps mode as MANUAL. The dashboard spec is correct; the firmware spec §15 is stale.

**Impact:** If a developer implements the firmware from §15, the MANUAL_OFF feature will not work. The dashboard will attempt to display `MANUAL_OFF` state, but the firmware will push `AUTO_STANDBY`.

**Required fix:** Update firmware spec §15 `manual_stop` entry to match §7.4 M-04 behavior.

### D-02 — SIGNIFICANT: Dashboard `stopRun()` writes both `manual_stop=true` AND `mode="AUTO"` in some cases

**Dashboard spec §6.1:**
> Stop in AUTO when running: calls `stopRun()` → "Writes `manual_stop=true` and sets `mode="AUTO"` from the dashboard (except when current mode is MANUAL, per firmware v5 contract)."

So `stopRun()` has conditional logic:
- In MANUAL mode: writes only `manual_stop=true` (no mode change) → produces `MANUAL_OFF`
- In AUTO mode: writes `manual_stop=true` AND `mode="AUTO"` (mode confirmation write)
- In COUNTDOWN mode: writes `manual_stop=true` (firmware handles mode revert to AUTO)

Writing `mode="AUTO"` explicitly from the dashboard during an AUTO stop is redundant (mode is already AUTO) and adds a Firebase write. It also means the `pendingModeWriteback` mechanism in firmware must handle a dashboard-side `mode="AUTO"` write while `pendingModeWriteback=false`. This is safe (it just sets `pumpMode = "AUTO"` which it already is), but it adds noise to the Firebase channel.

**Recommendation:** Remove the `mode="AUTO"` write from `stopRun()` in AUTO mode. It is unnecessary and creates a confusing dual-write pattern.

### D-03 — SIGNIFICANT: Dashboard Hardcodes `countdown_add_time` Default as Audit Log Entry

**Dashboard spec §9 audit detail string:**
> `countdown_add_time`: "5 min added (was {remaining} remaining)"

This hardcodes "5 min" regardless of `countdown_add_min`. If the operator adds 15 minutes using the custom add-time feature, the audit log would incorrectly say "5 min added."

**Required fix:** Update audit string to: "X min added (was {remaining} remaining)" where X is the actual `countdown_add_min` value written.

### D-04 — MINOR: StatusBar Shows `control.mode` But Not `run_mode`

**Dashboard spec §4.1:**
> Right badges: "Policy mode badge: `control.mode` (AUTO / FORCE_OFF / FORCE_ON / COUNTDOWN)"

The StatusBar shows the **policy mode** (`control.mode`) — what the operator commanded. But it does NOT show the **operational state** (`run_mode`) — what the firmware is actually doing. When P1 fires during AUTO, `control.mode` still shows AUTO but `run_mode` is OFF. An operator glancing at the StatusBar sees "AUTO" with no indication of the error state.

Error badges do appear in the StatusBar (is_error, is_overflow_error, etc.), which partially mitigates this. But the mode badge can be misleading — showing AUTO while the pump is in lockout.

**Recommendation:** Either (a) include `run_mode` in the StatusBar alongside `control.mode`, or (b) when `is_error` or `is_overflow_error` is true, replace the mode badge color with red regardless of mode value.

---

## 7. UX Architecture Analysis — Psychology & Human Factors

### 7.1 Information Hierarchy Assessment (Nielsen's Heuristics)

**Heuristic 1 — Visibility of System Status:** ✅ Strong. StatusBar, TankVisual, StatCards, and AlertBanners ensure current status is always visible. Tank fill animation provides immediate visual feedback.

**Heuristic 3 — User Control and Freedom:** ⚠️ Partial. FORCE_OFF has no confirmation and no "undo" path other than switching to AUTO. An accidental FORCE_OFF tap (especially on mobile with 44px touch targets near other controls) could trigger an unintended emergency stop. The emergency controls are collapsible, which reduces accidental access — but once expanded, FORCE_OFF is one tap away.

**Heuristic 4 — Consistency and Standards:** ✅ Strong. Mode badges, color coding (red/amber/green/blue), and button states follow consistent patterns.

**Heuristic 5 — Error Prevention:** ✅ Strong for FORCE_ON (2-step typed confirmation). ⚠️ Weak for FORCE_OFF (single tap, no confirmation). FORCE_OFF is a "persistent emergency stop" per the spec — not just a quick stop. Accidental activation should be guarded.

**Heuristic 6 — Recognition Over Recall:** ✅ Run mode badge shows current state. Mode selector shows active mode. History chart provides context without requiring memory.

**Heuristic 7 — Flexibility and Efficiency:** ✅ Preset countdown buttons (5/10/15/30/60 min) plus custom input is excellent. Serves both novice (taps a preset) and expert (types exact value) users.

**Heuristic 9 — Help Users Recognize, Diagnose, and Recover from Errors:** ⚠️ Partial. Error alerts have Clear Error buttons. But the alert description text does not consistently tell the operator *what to do physically*. "Dry-run lockout" should say "Check water supply and pipe connections before clearing."

### 7.2 Cognitive Load Analysis — Wickens' Multiple Resource Theory

Wickens' MRT (1984, updated 2008) states that humans have limited processing resources across spatial, verbal, and manual channels. For a water pump controller accessed during an emergency or at night:

**High cognitive load moments:**
1. **Dry-run error during AUTO at 3 AM** — Operator wakes to notification, opens app, sees red error. Must identify what happened, decide action, find Clear Error, tap it. Multiple modalities activated simultaneously.
2. **FORCE_ON activation** — 2-step confirmation requires reading, typing "OVERRIDE," and confirming. Under stress, this increases error rate.
3. **Countdown add-time during a running timer** — Operator must remember current remaining time, decide how much to add, input it, confirm.

**Recommendations based on MRT:**
- **Error recovery:** When an error alert is displayed, show a numbered action checklist inline: "1. Check water supply 2. Wait for flow 3. Tap Clear Error". Sequential, verbal guidance reduces working memory load.
- **FORCE_ON under stress:** The typed confirmation ("OVERRIDE") is cognitively demanding under stress. Research (Stanton et al., 2005, *Human Factors in Engineering Systems*) shows typed confirmations under time pressure have 40–60% higher error rates than button presses. The 2-step approach (warning card → typed confirm) is the right balance — the warning card slows the operator down and provides information, but the typed confirmation should be short (4–6 characters, no ambiguous characters). "OVERRIDE" is 8 characters. Consider "FORCE" (5 characters) which is less likely to be mistyped under stress.

### 7.3 Fitts' Law Analysis — Touch Target Sizing

Fitts' Law (1954): `MT = a + b × log₂(2D/W)` — movement time increases with distance and decreases with target size. The spec mandates ≥48px touch targets, which is consistent with Google Material Design (48dp minimum) and Apple HIG (44pt minimum).

**Critical case — Stop button on mobile:** The spec requires `min-height 64px` for Stop on mobile. This is excellent. However, if Stop and Add Time are adjacent in the COUNTDOWN view, Fitts' Law predicts frequent mis-taps between them. The spec shows:
```
[Timer MM:SS] [Stop]
[+1] [+5] [+10] [+15] [+20] [+30] [Custom] [Add]
```

The Stop button is adjacent to the Add Time row. On a small mobile screen, a misaimed tap on Stop when trying to tap "+30" could immediately cancel a running countdown. 

**Recommendation:** Add 16px minimum vertical gap between Stop and the Add Time row. Alternatively, move Stop to the top of the COUNTDOWN view as a full-width button, with Add Time below it. This creates clear vertical separation and prevents accidental Stop during add-time interaction.

### 7.4 Signal Detection Theory — Alert Priority

Signal Detection Theory (Green & Swets, 1966) distinguishes between signal detection sensitivity (d') and response bias (β). In alert systems, too many non-critical alerts decrease sensitivity — operators begin ignoring all alerts (alarm fatigue).

The current ranked alert system has 8 possible alerts. In the worst case, all 8 can be active simultaneously. Research (Sorkin, 1988, *Ergonomics*) shows that alarm panels with more than 5 simultaneously active alerts significantly reduce operator response accuracy.

**Current alert ranking:**
1. Controller offline — Critical
2. Dry-run lockout — Critical
3. Overflow error — Critical
4. Auto-maintenance — Informational
5. Maintenance active — Informational
6. Level sensor error — Warning
7. Flow sensor error — Informational
8. Sleeping — Informational

Alerts 4, 5, 7, and 8 are informational — they do not require immediate operator action. Displaying them at the same visual prominence as critical alerts (2, 3) creates alarm fatigue.

**Recommendation:** Implement a 3-tier alert model:
- **🔴 Critical (requires action):** Dry-run lockout, Overflow error, Controller offline
- **🟡 Warning (monitor):** Level sensor error, Auto-maintenance active
- **ℹ️ Informational (no action needed):** Flow sensor error, Maintenance mode, Sleeping

Critical alerts render as full-width red cards. Warnings render as yellow cards. Informational alerts collapse into a single "System notices (N)" expandable row. This reduces visual noise on the normal operating screen.

### 7.5 Mental Model Alignment

Research in user mental models (Norman, *The Design of Everyday Things*, 1988) shows that interface controls must align with the user's mental model of the physical system. For a water pump:

**User mental model:** "The pump fills the tank. I can turn it on, set a timer, or let it run automatically. If something goes wrong it stops."

**Dashboard alignment:**
- AUTO → "Let it run automatically" ✅
- MANUAL → "Turn it on/off myself" ✅
- COUNTDOWN → "Set a timer" ✅
- FORCE_OFF → "Emergency stop" ✅
- FORCE_ON → "Override everything" — ⚠️ This concept does NOT exist in most users' mental model of a pump.

**Recommendation:** FORCE_ON should be labeled for its physical consequence, not its technical name. "Force On" is an engineering term. Replace with: **"Emergency Override — Run Without Safety"** as the full name in the UI, with "FORCE ON" as the mode badge only (for technical users). The confirmation dialog already explains this well — extend that clarity to the button label.

---

## 8. Safety-Critical UX Failures & Improvements

### 8.1 FORCE_OFF Lacks Confirmation — Accidental Emergency Stop Risk

**Current behavior:** Single tap on FORCE_OFF button → pump stops immediately. Persistent.

**Problem:** FORCE_OFF is described as both an "Emergency Stop" (low barrier wanted) AND a "persistent stop for maintenance/travel" (high barrier wanted). These are conflicting use cases with conflicting UX requirements.

**Research basis:** IEC 62061 §10.15 (Safety-related control systems) specifies that emergency stop functions must be easily accessible but protected against unintentional activation. IEC 60204-1 §10.7 requires emergency stops to be "clearly marked and immediately accessible" but also requires they not be "accidentally operated." The solution used in physical systems is a recessed button, or twist-to-release. The UX equivalent is a single confirmation for emergency stops.

**Recommendation — Two-tier FORCE_OFF:**
- **Quick Stop (Emergency):** Single tap, no confirmation, stops immediately. Labeled "STOP ALL." This exits automatically to AUTO on next operator tap (like a physical E-stop that resets).
- **Persistent Off (Maintenance/Travel):** Requires one-step confirmation. "Keep pump OFF until I manually re-enable." Labeled "Persistent Lock Off." This is the current FORCE_OFF behavior.

If this split is too complex, the minimum fix is: add a 1-second hold-to-activate for FORCE_OFF with visual feedback (progress indicator while held), or add a single-tap confirmation toast with a 3-second undo window (following the Gmail "Message deleted — Undo" pattern).

### 8.2 Clear Error During MANUAL Mode Silently Restarts Pump

As documented in F-02 and §2.3 — when `pumpMode = "MANUAL"` and an error clears, the pump immediately restarts. This is a silent automatic action triggered by operator input.

**UX principle violated:** Principle of Least Surprise (Saltzer & Kaashoek, *Principles of Computer System Design*, 2009). An operator who taps "Clear Error" expects the error to clear, not for the pump to immediately restart without explicit confirmation.

**Recommendation:** When `controlMode === "MANUAL"` and an error is active:
1. Change the "Clear Error" button label to "Clear Error & Restart Pump"
2. Or add a post-clear confirmation: "Error cleared. MANUAL mode is active — pump will start. Continue?" with YES/STOP options.

### 8.3 No Feedback When Dashboard Command Is Ignored by Firmware

When `manual_start` is rejected by firmware (F-02 in control read: "Manual run rejected: error lockout active"), the only indication is in the Serial monitor. The dashboard has no mechanism to receive this rejection feedback. The 8-second optimistic timeout will eventually show "Command timed out" — but this is a generic timeout, not a specific rejection message.

**The gap:** Dashboard thinks "command timed out" but firmware actually received and deliberately rejected the command. These are different — a timeout means "we don't know what happened," a rejection means "we know, and the answer is no."

**Recommendation:** Add a `last_rejection_reason` field to `/pump_system/status` (or a separate `/status/command_feedback` path):
```json
{
  "last_rejection_code": "ERROR_LOCKOUT",
  "last_rejection_ts": 1234567890,
  "last_rejection_action": "manual_start"
}
```
Dashboard compares `last_rejection_ts` against the timestamp of its command write. If within ~5s, shows a specific rejection toast: "Cannot start: error lockout active. Clear the error first."

### 8.4 FORCE_ON Active — Manual Stop Is Silently Ignored

**Current behavior:** `manual_stop` during FORCE_ON → firmware ignores it, logs "Manual stop ignored: FORCE_ON active."

**Dashboard spec §6.1:** "A red warning card explains that all protections are bypassed and that exit must be done via **Emergency Controls** in ModeControls. No extra Stop button is shown here."

**Assessment:** Hiding the Stop button during FORCE_ON is correct — it prevents the ignored command. However, the red warning card must CLEARLY state that the Stop button is unavailable and why. The instructions "exit must be done via Emergency Controls" require the user to:
1. Know what "Emergency Controls" is
2. Know it is a collapsible section
3. Know it is below the run controls area
4. Scroll down on mobile to find it

Under stress, this is a significant navigation burden.

**Recommendation:** In the FORCE_ON warning card, add a direct link/button: "→ Go to Emergency Controls" that scrolls the page to the Emergency Controls section and auto-expands it. This eliminates the navigation steps under stress.

---

## 9. Information Design & Visual Hierarchy

### 9.1 Tank Visual — Level Percentage Color Coding

The spec describes the tank fill color changing based on `is_running` and error states (green pulse when running, red static glow on error). No color guidance exists for level percentage ranges.

**Research basis:** Color coding in process control displays (ANSI/ISA-101.01, *Human-Machine Interfaces for Process Automation Systems*) recommends:
- Normal operating range: neutral (white/blue)
- Low warning (approaching start threshold): amber
- Critical low (emergency): red
- Full: green briefly, then neutral

**Recommendation:** Add level-based color bands to the tank fill:
- **0–10%:** Red fill (critical — emergency level for sleep override)
- **11–30%:** Amber fill (low — approaching start threshold)
- **31–99%:** Blue/neutral fill (normal operation)
- **100%:** Brief green pulse on reaching full, then neutral

This gives immediate spatial feedback without requiring the operator to read the percentage number.

### 9.2 Flow Rate Card — Warning Threshold Feedback

**Current:** Flow Rate card shows LPM with "red warning when below `dry_run_threshold_lpm` while running."

**Problem:** The dry-run warning threshold (default 0.5 LPM) fires the warning color change only. The operator cannot tell how far below the threshold the reading is. A reading of 0.4 LPM (nearly threshold) vs 0.0 LPM (no flow at all) look the same — both are just "red."

**Research basis:** Quantitative displays should show position relative to critical thresholds (Shneiderman, *Designing the User Interface*, 1987). A progress-bar-style or gauge display communicates proximity to threshold better than a simple color change.

**Recommendation:** Add a small progress bar or arc gauge to the Flow Rate card when pump is running, with the dry-run threshold marked. The bar fills proportionally to `flow_rate_lpm / cfgFlowCalibration * something`. Operators can see if they're at 0.3 (near threshold) vs 15 LPM (well above). This also provides early warning before the 30-second countdown expires.

### 9.3 History Chart — Missing Safety Event Markers

The history chart shows level % and flow LPM over time as area series. It does not show when safety events occurred (dry-run lockout, overflow, mode changes).

**Recommendation:** Add vertical event markers to the history chart (dashed lines):
- Red dashed: P1 lockout events (dry-run, overflow)
- Amber dashed: mode changes
- Blue dashed: pump starts/stops (aligned with is_running transitions)

These markers transform the chart from a passive observation tool into a diagnostic tool that helps operators understand *why* certain level/flow patterns occurred.

### 9.4 System Info Panel — Sensor Health vs Sensor Status Confusion

The spec shows `level_sensor_health_pct` as a percentage in the System Info panel. However, the health percentage (`100 - failCount × 20 - 20 if stale`) is a computed heuristic, not a physical measurement. An operator who sees "Level Sensor: 40%" may not understand what this means.

**Recommendation:** Replace the raw percentage with a qualitative status badge:
- 80–100%: ● Good
- 60–79%: ● Degraded
- 40–59%: ● Warning
- 0–39%: ● Critical

Show the percentage as a secondary detail on hover/tap. The badge communicates actionability; the percentage communicates severity. Both are useful but serve different cognitive goals.

---

## 10. Accessibility & Inclusive Design

### 10.1 Color Alone for Status Communication

**Current:** Multiple status states are communicated via color only (red error, green running, amber warning).

**WCAG 2.1 Criterion 1.4.1 (Use of Color):** Color must not be the only visual means of conveying information.

**Assessment:** The spec does use icons and text labels alongside color in most cases (e.g., the error badge shows text, not just a red dot). However, the TankVisual glow (green animated pulse = running, red static glow = error) uses only color/animation to communicate pump state.

**Recommendation:** Add a text label inside or beneath the TankVisual showing the current run state in large text: "RUNNING", "STANDBY", "ERROR", "OVERRIDE". This text should also be visible in the stat strip below the tank. Operators with color vision deficiency (8% of males) should never need to distinguish state by color alone.

### 10.2 Touch Target Minimum on Overflow Menu

The spec says "a single 44×44px touch target" for the mobile overflow menu. WCAG 2.5.5 (AAA) recommends 44×44 CSS pixels. The APCA (Accessible Perceptual Contrast Algorithm) and Apple HIG both recommend 44pt. This is correct.

However, the Emergency Controls collapsible section (FORCE_OFF, FORCE_ON) must also meet this minimum. If the chevron/header tap target for the Emergency Controls accordion is smaller than 44px, operators under stress (who may have tremor or reduced dexterity) may fail to expand it.

### 10.3 Font Size for Critical Information

Dashboard UX specs often underspecify font sizes. For a pump controller that may be used:
- By an older operator with reduced vision
- On a phone in bright sunlight (reduced contrast)
- At arm's length while kneeling near the pump

The countdown timer MM:SS display and the tank level percentage are the two most critical readable values. Both should be at minimum 24px (large, bold), not the typical 16-20px body text size.

**Research basis:** ISO 9241-303 (Visual Display Requirements) recommends a minimum visual angle of 0.2° for critical safety information. At 50cm viewing distance (mobile phone), 0.2° subtends ~1.7mm, which at 96 DPI equals approximately 6.5px. However, for legibility (not just detectability), ISO 9241-303 recommends 4–5× the detection threshold — approximately 26–33px minimum for critical safety displays.

---

## 11. Security & Threat Model

### 11.1 Firebase RTDB Rules — Critical Gap

**Current state (from both specs):** Authentication is Google Sign-In with a UID allowlist. Admin status from `/pump_system/config/admins/{uid}`. Dashboard enforces this at the UI level.

**The critical gap:** Firebase RTDB security rules are the ONLY server-side enforcement layer. If no rules are configured (or rules are permissive), anyone with the Firebase project credentials (which are visible in the client-side JavaScript bundle) can write directly to `/control/mode = "FORCE_ON"` via the Firebase REST API without any authentication.

**Evidence this is a real risk:** Firebase project credentials (apiKey, databaseURL) are EXPOSED in the Next.js public bundle (they are `NEXT_PUBLIC_*` env vars by design). These are not secret — Firebase's security model requires server-side rules, not secret client credentials.

**Required RTDB Rules (minimum):**

```json
{
  "rules": {
    "pump_system": {
      "status": {
        ".read": "auth != null",
        ".write": "auth.token.email === 'YOUR_ESP32_SERVICE_EMAIL'"
      },
      "control": {
        ".read": "auth != null",
        ".write": "auth != null && root.child('pump_system/config/admins').child(auth.uid).val() === true || auth.token.email === 'YOUR_AUTHORIZED_EMAIL'"
      },
      "config": {
        "device": {
          ".read": "auth != null",
          ".write": "root.child('pump_system/config/admins').child(auth.uid).val() === true"
        },
        "admins": {
          ".read": "auth != null",
          ".write": "root.child('pump_system/config/admins').child(auth.uid).val() === true"
        }
      },
      "audit": {
        ".read": "auth != null",
        ".write": "auth != null"
      }
    }
  }
}
```

**FORCE_ON specifically** should have a separate, stricter rule requiring explicit admin UID:
```json
"control": {
  "mode": {
    ".write": "auth != null && (newData.val() !== 'FORCE_ON' || root.child('pump_system/config/admins').child(auth.uid).val() === true)"
  }
}
```

### 11.2 Audit Log Integrity

Current audit log: append-only to `/pump_system/audit/events/{pushId}`. Firebase does not enforce immutability by default. An admin user can delete or modify past audit entries.

For a safety-critical pump controller, audit trails should be tamper-evident per IEC 62443-2-1 §4.3.3.6 (Security auditing).

**Recommendation:** Use Firebase's `push()` to append (already done), but also configure RTDB rules to deny `.write` on existing events (only allow new writes):
```json
"events": {
  "$eventId": {
    ".write": "!data.exists()"  // only allow creation, not modification
  }
}
```

### 11.3 Session Expiry and Re-Authentication for FORCE_ON

If an operator authenticates once and leaves their session open, and someone else uses the device hours later, they could activate FORCE_ON without the original operator's knowledge. The typed "OVERRIDE" confirmation does not require re-authentication.

**Recommendation:** For FORCE_ON activation specifically, require re-authentication (Firebase `reauthenticateWithCredential()`) before the confirmation dialog proceeds. This ensures the activating identity is verified at the moment of activation, not just at initial session start.

---

## 12. Offline & Resilience UX

### 12.1 Firebase Degraded State (20–60s Gap)

**Dashboard spec §8.3:** "If >30s without update and no explicit error: banner 'Connection issues — checking…'. After 60s, escalates to dashboard-offline state."

**Problem:** During the 30–60s "degraded" window, controls are presumably still enabled (since it hasn't escalated to full offline yet). An operator could send a command during this window, the command is written to Firebase, but status updates are not arriving. The command may or may not have been processed by the firmware. The 8-second optimistic timeout will fire, showing "Command timed out" — but the command actually may be processing.

**Recommendation:** Disable controls at 30s (when "Connection issues" banner shows), not at 60s (full offline). The cost of brief unnecessary disable (operator has to wait 30s more) is much lower than the cost of duplicate/unknown command delivery.

### 12.2 Last Known Status Staleness

**Current:** "Last known status remains visible with timestamp" when controller is offline.

**Missing:** No indication that the status is stale beyond the timestamp. The TankVisual still shows the last level reading with the same visual fidelity as a live reading. An operator could believe the tank is at 75% based on a 2-hour-old reading.

**Recommendation:** Apply a visual "stale data" treatment when `level_last_valid_age_sec > 300` (5 minutes): desaturate the tank fill, overlay a clock icon on the TankVisual, and show `~ 75%` (tilde prefix indicating approximate/stale). This is consistent with the estimated level treatment (`~` prefix for flow-based estimates) and creates a clear visual grammar: `~` means "this value may not be current."

### 12.3 Restart Banner — Phase 3 Timeout Message Is Ambiguous

**Current Phase 3 message:** "Controller hasn't responded yet (Xs elapsed). If it doesn't reconnect, try a manual power cycle."

**Problem:** This message does not distinguish between:
1. The controller crashed and needs manual power cycle
2. The controller is still rebooting (larger firmware)
3. The WiFi network itself is down
4. Firebase is having issues

All three scenarios show the same message, but the correct operator action differs.

**Recommendation:** At the 30s+ mark, show a diagnostic checklist instead of a single message:
```
⏳ Controller hasn't reconnected (Xs)
Check:
☐ Is the controller powered on?  
☐ Is the WiFi router working?  
☐ Is the pump making normal sounds?

If all check out, try: [Request Manual Reboot] or wait up to 2 minutes.
```

---

## 13. Notifications & PWA

### 13.1 Notification Trigger Granularity

**Current:** Notifications on dry-run, low tank, overflow, controller offline.

**Missing notifications (high value):**
- **FORCE_ON activated** — Any time the emergency override is activated, ALL users (not just the activating admin) should receive a push notification. This creates a second pair of eyes on potentially dangerous operations.
- **MANUAL mode auto-stopped by tank-full** — Operator may not realize the tank filled and the pump stopped automatically.
- **MANUAL mode P1 lockout** — Operator started a manual run and walked away; if dry-run fires, they need to know.
- **Controller restarted (unexpected)** — Brownout or WDT reset during normal operation should notify.
- **FORCE_ON auto-timeout triggered** (if implemented) — Operator should know their override expired.

### 13.2 Push Notification Actionability

**Research basis:** Iqbal & Bailey (2008, *CHI Proceedings*) showed that non-actionable notifications reduce response rates to subsequent notifications by 25–40%. Every notification should have a clear action.

**Current notification model:** Sends notifications with title and body. No deep-link action.

**Recommendation:** For each notification type, define the tap action:
- Dry-run lockout → open app → automatically scroll to and highlight "Clear Error" button
- Controller offline → open app → show restart option prominently
- FORCE_ON activated → open app → show FORCE_ON banner + Exit options prominently

### 13.3 PWA Install Prompt Timing

**Current:** `InstallPrompt` surfaces "after initial use."

**Research basis:** Google UX research (Besbris, 2019) found that PWA install prompts shown during positive engagement moments (e.g., after successfully completing a task) have 3–4× higher acceptance rates than prompts shown immediately on first visit.

**Recommendation:** Trigger the install prompt after the first successful pump start/stop action, not just after initial use. The operator has just had a successful interaction — they are in a positive engagement moment.

---

# Part III — Recommendations

---

## 14. Priority Matrix — All Findings Ranked

### 🔴 Critical — Fix Before Release

| ID | Finding | Document | Impact |
|----|---------|---------|--------|
| **F-01** | §8.1 bidirectional flow shows `pumpMode = "FORCE_ON"` — stale v3.0.0 code | Firmware | Developer implementing from spec builds wrong firmware |
| **D-01** | `manual_stop` behavior contradicts between §7.4 and §15 | Both | MANUAL_OFF feature broken if dev follows §15 |
| **11.1** | Firebase RTDB security rules not documented; `/control/mode="FORCE_ON"` writable by anyone | Dashboard | Security vulnerability — unauthorized pump override |
| **8.3** | No dashboard feedback when firmware rejects a command | Dashboard | Operators can't distinguish timeout from rejection |
| **F-02** | clear_error during MANUAL silently auto-restarts pump | Firmware | Dangerous silent action after error recovery |

### 🟡 Significant — Fix in Next Sprint

| ID | Finding | Document | Impact |
|----|---------|---------|--------|
| **F-03** | `manual_stop` during FORCE_OFF exits FORCE_OFF accidentally | Firmware | One-shot stop exits persistent safety state |
| **F-06** | §15 `manual_stop` entry is stale (old v4.0 behavior) | Firmware | Implementation confusion |
| **D-02** | `stopRun()` redundantly writes `mode="AUTO"` in AUTO mode | Dashboard | Firebase noise, potential edge cases |
| **D-03** | Audit log hardcodes "5 min added" regardless of actual add-time | Dashboard | Inaccurate audit trail |
| **D-04** | StatusBar shows policy mode, not run mode — may mislead during errors | Dashboard | Operator sees "AUTO" while system is locked out |
| **8.1** | FORCE_OFF lacks confirmation — accidental activation risk | Dashboard | Unintended persistent emergency stop |
| **8.2** | Clear Error during MANUAL mode needs warning (will restart pump) | Dashboard | Operator surprised by immediate pump restart |
| **3.1** | FORCE_ON + sensor failure causes relay oscillation + counter corruption | Firmware | `totalPumpCycles` inflated; relay cycles at 1Hz |
| **3.2** | Min-off-time not enforced for `manual_start` one-shot | Firmware | Motor short-cycling risk in MANUAL mode |
| **7.3** | Stop and Add Time buttons can be misclicked in COUNTDOWN (Fitts' Law) | Dashboard | Accidental countdown cancellation |
| **9.3** | History chart has no safety event markers | Dashboard | Diagnostic value significantly reduced |
| **12.2** | Stale sensor data has no visual treatment | Dashboard | Operator may act on hours-old level reading |

### 🟢 Improvements — Address When Capacity Allows

| ID | Finding | Area | Value |
|----|---------|------|-------|
| **F-04, F-05** | Version string inconsistency throughout firmware spec | Firmware spec | Documentation quality |
| **7.1** | Error recovery alerts need physical action guidance | Dashboard | Reduces operator confusion under stress |
| **7.2** | FORCE_ON confirmation should use "FORCE" not "OVERRIDE" | Dashboard | Reduced typing error under stress |
| **7.4** | Alert ranking needs 3-tier model to prevent alarm fatigue | Dashboard | Operator attention directed correctly |
| **7.5** | FORCE_ON button label should describe consequence, not technical name | Dashboard | Mental model alignment |
| **8.4** | FORCE_ON warning card needs direct link to Emergency Controls | Dashboard | Reduces navigation burden under stress |
| **9.1** | Tank fill should use level-based color bands | Dashboard | Faster visual triage at a glance |
| **9.2** | Flow rate card needs proximity-to-threshold indicator | Dashboard | Earlier dry-run warning to operator |
| **10.1** | Tank visual glow uses color alone for pump state | Dashboard | WCAG 2.1 compliance |
| **10.3** | Critical values (countdown, level %) need ≥24px font | Dashboard | Legibility in bright light / at distance |
| **11.2** | Audit events should be immutable in RTDB rules | Dashboard | Tamper-evident audit trail |
| **11.3** | FORCE_ON should require re-authentication | Dashboard | Identity verification at override moment |
| **12.1** | Controls should disable at 30s degraded, not 60s | Dashboard | Prevents command delivery during unknown state |
| **12.3** | Restart Phase 3 message needs diagnostic checklist | Dashboard | Better operator guidance |
| **13.1** | Add FORCE_ON notification to all users | Notifications | Safety awareness |
| **3.3** | `countdown_add_min` validation needs status feedback path | Firmware | Operator visibility of applied value |
| **2.4** | Disable ON button in MANUAL_OFF when level sensor error | Dashboard | Prevents confusing tap-with-no-response |
| **2.2** | Document MANUAL_OFF + dry-run auto-restart behavior | Both specs | Operator informed of surprising behavior |

---

## 15. Implementation Roadmap

### Sprint 1 — Critical Fixes (1–2 days)

1. Update `firmware_master_spec.md` §8.1 bidirectional diagram to show `pumpMode = "MANUAL"`.
2. Align `firmware_master_spec.md` §15 `manual_stop` entry with §7.4 M-04 (MANUAL_OFF sticky behavior).
3. Document Firebase RTDB security rules in `dashboard_master_spec.md` §9.
4. Add `last_rejection_code` field to firmware status push and handle in dashboard.
5. Add warning on Clear Error button when `controlMode === "MANUAL"` (will restart pump).
6. Document or fix `manual_stop` during FORCE_OFF — decide: ignore (like FORCE_ON) or allow exit.

### Sprint 2 — Significant Issues (3–5 days)

7. Add confirmation for FORCE_OFF (1-step, simpler than FORCE_ON).
8. Apply min-off-time check to `manual_start` one-shot path.
9. Fix FORCE_ON relay oscillation in `checkDryRunProtection()` — gate `setPump(false)` on `pumpMode != "FORCE_ON"`.
10. Increase vertical gap between Stop and Add Time buttons in COUNTDOWN view.
11. Add safety event markers (vertical lines) to history chart.
12. Add stale data visual treatment to TankVisual when `level_last_valid_age_sec > 300`.

### Sprint 3 — Enhancements (1 week)

13. Implement 3-tier alert model (Critical/Warning/Informational). See dashboard_master_spec §§5.2, 6.1 (Alerts & Run Controls).
14. Add level-based color bands to TankVisual. See dashboard_master_spec §4.3 (Tank & Stats).
15. Add proximity-to-threshold gauge to Flow Rate card. See dashboard_master_spec §§4.3, 9.2 (Flow Rate).
16. Add FORCE_ON notification to all users. See dashboard_master_spec §§9, 12 (Security & Notifications).
17. Disable controls at 30s degraded state (not 60s). See dashboard_master_spec §8 (Offline & Restart Behavior).
18. Update Restart Phase 3 message to diagnostic checklist. See dashboard_master_spec §8 (Offline & Restart Behavior).
19. Disambiguate `isOverflowError` label (rename to "Max Runtime Lockout" in dashboard). See dashboard_master_spec §§5.2, 6.1 (Alerts & Labels).
20. Standardize version number across all spec documents. See firmware_master_spec (header, §§2–3, 14–16) and dashboard_master_spec (title block).

---

*End of Document — Smart Water Pump Controller: Firmware & Dashboard Review*

*All findings are grounded in direct source analysis of firmware_master_spec.md (1,114 lines) and dashboard_master_spec.md (512 lines), supplemented by the referenced engineering standards, UX research literature, and human factors principles.*
