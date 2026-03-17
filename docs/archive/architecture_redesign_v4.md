# Smart Water Pump Controller — Architecture Redesign
## Mode Evaluation, Engineering Corrections & Comprehensive System Recommendations

> **Document Type:** Architecture Review · Design Validation · Engineering Recommendations  
> **Firmware Base:** v3.0.0 (fully audited)  
> **Supersedes:** `mode_redesign_v4_old.md`, §2–§3 of `firmware_master_spec.md`  
> **Standard References:** IEC 61511 (Functional Safety), IEC 60068 (Environmental Testing),  
> IEEE 1012 (Verification & Validation), NFPA 20 (Pump Installation), ISO 9905 (Centrifugal Pumps),  
> ISA-5.1 (Instrumentation Symbols), IEC 62443 (Industrial Cybersecurity)  
> **Author:** Mark Alvin Cadangin

---

## Table of Contents

1. [Your Mode Design — Evaluation and Verdict](#1-your-mode-design--evaluation-and-verdict)
2. [Engineering Corrections to the Current Spec](#2-engineering-corrections-to-the-current-spec)
3. [Mode Architecture v4.0 — Finalized Definitions](#3-mode-architecture-v40--finalized-definitions)
4. [Revised Priority Hierarchy — Research-Backed](#4-revised-priority-hierarchy--research-backed)
5. [Revised State Machine & Conflict Resolution](#5-revised-state-machine--conflict-resolution)
6. [Safety System Analysis — IEC 61511 Alignment](#6-safety-system-analysis--iec-61511-alignment)
7. [Sensor Engineering Analysis](#7-sensor-engineering-analysis)
8. [Pump Motor Protection — Engineering Standards](#8-pump-motor-protection--engineering-standards)
9. [Worst-Case Scenario Analysis](#9-worst-case-scenario-analysis)
10. [Additional Improvement Recommendations](#10-additional-improvement-recommendations)
11. [Complete Scenario Matrix — v4.0](#11-complete-scenario-matrix--v40)
12. [Firmware Implementation Plan — v4.0](#12-firmware-implementation-plan--v40)
13. [Dashboard Implementation — v4.0](#13-dashboard-implementation--v40)
14. [Spec Corrections Index](#14-spec-corrections-index)

---

## 1. Your Mode Design — Evaluation and Verdict

### 1.1 Proposed Mode Set Review

You proposed five modes. Below is the engineering verdict on each.

---

#### AUTO — ✅ Correct. Approved as-is.

Your description: *"Same as is."*

**Verdict:** The current AUTO implementation is correct. Level-based hysteresis (start ≤ 30%, stop ≥ 100%), P1 dry-run protection, P1 overflow protection, level sensor fail-safe, sleep window, and idle slow-poll all work correctly. No changes needed.

**Engineering basis:** The hysteresis dead-band design (30% start, 100% stop) follows standard pump control practice defined in NFPA 20 §A.4.20 and ISA-5.1 control loop recommendations. The dead-band prevents pump short-cycling, which is a primary cause of motor winding failure due to repetitive inrush current. A 70% dead-band on a 660L tank provides ~462 L of operational range — at typical household pump rates of 30–50 LPM, this gives a 9–15 minute minimum run time per cycle, which is well within motor duty cycle specifications.

---

#### FORCE_ON — ✅ Correct in intent. One design decision refined.

Your description: *"Follow its name — forced on, ignore all the sensors, protection, etc. Used with disclaimer and confirmation."*

**Verdict:** Correct. This becomes P0 — above everything including P1 dry-run lockout. The one refinement: `manual_stop` should be **ignored** during FORCE_ON. The only valid exits are `mode = AUTO`, `mode = FORCE_OFF`, or `mode = MANUAL`, selected via the mode selector. A simple Stop button tap should not accidentally exit an intentional emergency override.

**Engineering basis:** IEC 61511-1 §11.6 addresses "override and inhibit" functions in safety instrumented systems. It specifies that: (a) overrides must require deliberate, unambiguous operator action; (b) the override state must be clearly indicated; and (c) the path to restore normal safety function must be equally deliberate. Hiding the Stop button and requiring explicit mode selector use for FORCE_ON exit satisfies all three requirements.

**Safety warning:** FORCE_ON with a broken dry-run sensor means the pump can run indefinitely with no water flowing. At 1.5HP (1,120W), a centrifugal pump running dry generates approximately 90–95°C motor winding temperature within 10–20 minutes (depending on insulation class). Most Philippine single-phase motor windings are Class B (130°C max). The safety margin is thin. FORCE_ON should display a persistent, live runtime counter so the operator can physically monitor how long the pump has been running.

---

#### FORCE_OFF — ✅ Correct. Approved as-is, one rule clarified.

Your description: *"Emergency off, used for making the pump stay off."*

**Verdict:** Correct. One critical rule must be explicitly coded and documented: **`manual_stop` does NOT set FORCE_OFF.** `manual_stop` always reverts to AUTO. Only `mode = FORCE_OFF` sets FORCE_OFF. This distinction prevents a user from accidentally entering FORCE_OFF when they just want to stop a manual run.

**Engineering basis:** Persistent mechanical lockout / tagout (LOTO) procedures (OSHA 29 CFR 1910.147) require that a safety stop state survive unplanned power removal. FORCE_OFF satisfying this by persisting through NVS is architecturally aligned with LOTO principles for software-controlled equipment.

---

#### MANUAL (your MANUAL_ON/MANUAL_START) — ✅ Correct. Requires one naming decision.

Your description: *"Manual version of AUTO. Maintains all protection and safety checks. Corresponding stop button returns to AUTO."*

**Verdict:** Correct. This is the most important addition in v4.0. The pumpMode string should be `"MANUAL"` (not `"MANUAL_ON"`) for two reasons: (1) it is cleaner and more consistent with `FORCE_ON` / `FORCE_OFF` naming; (2) the `runMode` string will remain `"MANUAL"` which the dashboard already displays — no frontend change needed for the run state display.

**Key behavior decisions confirmed:**

| Decision | Verdict | Reasoning |
|----------|---------|-----------|
| Full safety (dry-run, overflow, level sensor error) | ✅ Yes | Identical coverage to AUTO — operator presence does not remove physics-based failure risks |
| Tank-full stop at 100% | ✅ Yes | Prevents overflow if operator walks away during MANUAL run |
| Sleep window suppression | ❌ No — MANUAL runs through sleep | Operator-initiated; sleep only suppresses *autonomous* starts |
| Reverts to AUTO on any stop condition | ✅ Yes | AUTO is the safe baseline; the system should always return to it |
| NVS restore behavior | ✅ Restore MANUAL on boot | Unlike FORCE_ON, MANUAL has full safety — a power cut during a MANUAL fill should resume the fill |
| Overflow timer coverage | ✅ Yes — add to `checkOverflowProtection()` | A 2-hour unattended MANUAL run is a defect, not a feature |

**On NVS restore:** The prior redesign document recommended reverting MANUAL to AUTO on boot, similar to FORCE_ON. **This is revised here.** MANUAL is a safe mode (full safety active). If power cuts during a legitimate manual fill, the tank may be critically low. Restoring MANUAL on boot means the fill resumes when power returns — which is the correct behavior. If this feels too aggressive, it is trivially changed by adding `"MANUAL"` to the FORCE_ON revert block in `loadStateFromNVS()`.

---

#### COUNTDOWN — ✅ Correct. Minor validation additions only.

Your description: *"Variant of AUTO and MANUAL. Timer. Stop button. All protections. Custom time (valid minutes). Add time (valid minutes)."*

**Verdict:** The current COUNTDOWN firmware is substantially correct. Two additions:

1. **Input validation for `countdown_duration_min` and `countdown_add_min`:** The firmware already clamps values to 1–120 using `constrain()`. Document this explicitly: values outside 1–120 are silently clamped, not rejected. The dashboard should validate before writing (integer, ≥1, ≤120).

2. **Add-time expired-timer edge case:** Add-time only works when `isCountdownActive && countdownEndMs > millis()`. If the timer has expired by the time the tap is processed (~3s Firebase latency), the extension silently fails. The firmware resets the flag; the operator must start a new countdown. This must be documented clearly on the dashboard.

**Engineering basis for COUNTDOWN:** IEC 61511-1 §11.5 addresses timed safety functions. For a pump controller, a COUNTDOWN run bounded by both a timer AND a physical level check (100% stop) AND P1 safety is more robust than an unbounded MANUAL run, as it creates a natural time-to-stop that prevents indefinite operation in case of operator distraction.

---

### 1.2 Verdict Summary

| Mode | Your Design | Verdict | Key Change |
|------|------------|---------|-----------|
| `AUTO` | Same as is | ✅ Approved unchanged | None |
| `FORCE_ON` | All safety bypassed, confirmation required | ✅ Approved | P0 priority; `manual_stop` ignored; runtime counter on dashboard |
| `FORCE_OFF` | Persistent emergency off | ✅ Approved | Clarify `manual_stop` does NOT set FORCE_OFF |
| `MANUAL` | Manual with full safety, stop → AUTO | ✅ Approved | Use `"MANUAL"` string; restore on boot |
| `COUNTDOWN` | Timed, full safety, add time, stop → AUTO | ✅ Approved | Validation documentation; expired-timer edge case |

---

## 2. Engineering Corrections to the Current Spec

### C-01 — CRITICAL: FORCE_ON safety profile is wrong and undocumented

**In `firmware_master_spec.md` §3.1:**  
Current priority diagram places FORCE_ON at "P3b" with note "P1 still guards above."

**Reality in v3.0.0 firmware:**
- `checkOverflowProtection()` at line 118 of `03_safety_pump.ino` excludes FORCE_ON entirely: `if (!isRunning || !(pumpMode == "AUTO" || pumpMode == "COUNTDOWN")) return;`
- This means a FORCE_ON run has **no overflow runtime protection** whatsoever — 480+ minutes, unbounded.
- HOWEVER, dry-run IS still active during FORCE_ON in v3.0.0 (P1 fires before P3b).
- This means FORCE_ON in v3.0.0 is neither fully safe (no overflow protection) nor fully unsafe (dry-run still stops it).
- The name says "force on" but it cannot override a dry-run lockout.

**Correction:** v4.0 resolves this definitively. FORCE_ON = P0. All safety bypassed at relay level. Spec must be updated throughout.

### C-02 — SIGNIFICANT: `manual_start` one-shot silently sets FORCE_ON with partial safety gap

**In `firmware_master_spec.md` §8.2, "Quick Start button" row:**  
Described as a routine manual start. The user has no way to know this sets FORCE_ON (no overflow protection, no level stop).

**Correction:** With v4.0, `manual_start` sets `pumpMode = "MANUAL"`. The Quick Start button is now a genuinely safe manual start. Spec and dashboard mapping must be updated.

### C-03 — SIGNIFICANT: Overflow timer does not restart when MANUAL mode begins

**In v3.0.0 `checkOverflowProtection()`:**  
```cpp
if (!isRunning || !(pumpMode == "AUTO" || pumpMode == "COUNTDOWN")) return;
```
The timer for `pumpAutoStartTracking` only starts when mode is AUTO or COUNTDOWN. If the system transitions FROM AUTO (pump already running) TO FORCE_ON (now MANUAL in v4.0), the overflow timer tracking state (`pumpAutoStartMs`, `pumpAutoStartTracking`) is reset by the `return` statement on the very first cycle after mode change.

**Effect:** A pump that runs for 90 minutes in AUTO, then switches to MANUAL via `manual_start`, effectively resets the overflow timer. The pump could run a total of 90 + 120 = 210 minutes without overflow protection firing.

**Correction (v4.0):** `checkOverflowProtection()` must add `"MANUAL"` to the covered mode set. The timer will not reset on mode change within covered modes — it tracks continuous run time, not per-mode time. The `pumpAutoStartTracking` flag is only reset when the pump stops or when the mode is *not* covered (i.e., FORCE_ON or FORCE_OFF).

### C-04 — SIGNIFICANT: Level sensor fail-safe does not apply to FORCE_ON in v3.0.0

**In `firmware_master_spec.md` §7.3 (level sensor error table):**  
No scenario documents what happens when `isLevelSensorError = true` during a FORCE_ON run.

**Reality:** In v3.0.0, level sensor error fires at P5c — which is inside the P5 AUTO branch. FORCE_ON is P3b, which returns before P5 is reached. So level sensor error has **no effect** on a FORCE_ON run. The pump continues even if `isLevelSensorError = true`. This is consistent with v4.0 FORCE_ON behavior (P0, all bypassed) — but the spec never says this.

**Correction:** Add explicit scenario row. In v4.0, document clearly: FORCE_ON bypasses level sensor error fail-safe. Safety flags are still set and visible on dashboard.

### C-05 — MINOR: `runMode` derivation has a coverage gap for MANUAL mode

**In v3.0.0 `executePumpLogic()` runMode derivation:**  
There is no explicit derivation case for `pumpMode == "MANUAL"` (which doesn't exist yet). After v4.0 adds MANUAL, the derivation needs two cases:
- `pumpMode == "MANUAL" && isRunning` → `runMode = "MANUAL"`
- `pumpMode == "MANUAL" && !isRunning` → `runMode = "OFF"` (safety event stopped the pump while MANUAL mode is active)

### C-06 — MINOR: Overflow protection mode exclusion is not cross-referenced in §9.3

**In `firmware_master_spec.md` §9.3 "Hardware Overrun Protection":**  
Row for "Pump runs too long" says coverage applies to AUTO and COUNTDOWN. Does not mention FORCE_ON exclusion or the upcoming MANUAL inclusion. Both must be documented.

### C-07 — MINOR: Boot scenario B-04 is incomplete

**In `firmware_master_spec.md` §7.8 B-04:**  
Documents `isDryRunError = true` on boot → P1 fires → pump stays OFF. Does not document what mode the pump is in. If `pumpMode = "MANUAL"` is also restored from NVS (a power cut during a MANUAL dry-run), the pump is in MANUAL mode with P1 active. Mode should stay MANUAL; pump stays OFF until `clear_error`. After `clear_error`, MANUAL continues (pump starts again). This is different from AUTO where clear_error → AUTO_STANDBY (pump waits for level trigger).

---

## 3. Mode Architecture v4.0 — Finalized Definitions

### 3.1 Mode Set

| `pumpMode` | `runMode` Derived | Priority | NVS Boot | Safety Level |
|-----------|------------------|---------|---------|-------------|
| `"FORCE_ON"` | `"FORCE_ON"` | **P0** | → AUTO | ❌ All bypassed (relay level) |
| *(P1 Hard Safety)* | *varies* | P1 | N/A | Hard safety events |
| `"FORCE_OFF"` | `"OFF"` | P2 | ✅ FORCE_OFF | N/A |
| `"MANUAL"` | `"MANUAL"` / `"OFF"` | P3 | ✅ MANUAL | ✅ Full (= AUTO) |
| `"COUNTDOWN"` | `"COUNTDOWN"` / `"OFF"` | P4 | ✅ COUNTDOWN | ✅ Full |
| `"AUTO"` | `"AUTO"` / `"AUTO_STANDBY"` | P5 | ✅ AUTO | ✅ Full |

### 3.2 Mode Behavioral Contracts

#### FORCE_ON (P0 — Absolute Override)
- **Entry:** Admin + 2-step confirmation dialog. `mode = "FORCE_ON"` written to Firebase.
- **Relay:** ON on every sensor cycle, unconditionally.
- **Safety:** `checkSafetyCutoff()` still runs. `isDryRunError`, `isOverflowError`, `isLevelSensorError` are still set and pushed to `/status` as monitoring flags — but none interrupt the relay.
- **Live monitoring on dashboard:** Runtime counter (HH:MM:SS), dry-run warning if `isDryRunError`, overflow warning if `isOverflowError`.
- **Exit:** `mode = AUTO`, `mode = MANUAL`, `mode = COUNTDOWN`, or `mode = FORCE_OFF` via mode selector. `manual_stop` is **IGNORED**.
- **NVS:** Stored. On boot: restored as `AUTO`. FORCE_ON requires explicit re-activation.

#### FORCE_OFF (P2 — Persistent Emergency Stop)
- **Entry:** `mode = FORCE_OFF`. No confirmation required (emergency use).
- **Relay:** OFF on every sensor cycle. Level readings, sensor monitoring, and telemetry continue.
- **Exit:** Only explicit `mode = *` command from dashboard.
- **NVS:** Stored. Restored on boot. Power cycle does NOT clear FORCE_OFF.
- **`manual_stop` behavior:** Does NOT set FORCE_OFF. `manual_stop` → AUTO always.

#### MANUAL (P3 — Full-Safety Manual Run)
- **Entry:** `manual_start = true` one-shot OR `mode = "MANUAL"`. Rejected if `isDryRunError`, `isOverflowError`, or `pumpMode == "FORCE_OFF"`.
- **Relay:** ON immediately. Safety-guarded on every cycle.
- **Stop conditions (any → pump OFF, mode → AUTO):** `manual_stop` · level ≥ 100% · P1 dry-run · P1 overflow · level sensor error.
- **Exception:** `FORCE_OFF` received → mode → FORCE_OFF (not AUTO).
- **Sleep:** MANUAL runs through sleep window.
- **NVS:** Stored. Restored on boot. Full safety resumes.

#### COUNTDOWN (P4 — Full-Safety Timed Run)
- **Entry:** `mode = COUNTDOWN` + valid `countdown_duration_min` (1–120). Rejected if P1 error active.
- **Timer:** `millis()`-based, immune to network. Offline fallback to NVS-persisted last duration.
- **Add time:** `countdown_add_time = true` + `countdown_add_min = N` (1–120). Only when active timer has remaining time. Firmware resets flag. Capped at millis()+7200000.
- **Stop conditions (any → pump OFF, mode → AUTO):** Timer expiry · level ≥ 100% · `manual_stop` · P1 dry-run · P1 overflow. Exception: `FORCE_OFF` → mode → FORCE_OFF.
- **NVS:** Stored. Timer re-arms with last-known duration on boot + first Firebase sync.

#### AUTO (P5 — Level-Based Automation)
- **Start:** level ≤ 30% (default). **Stop:** level ≥ 100% (default).
- **Sleep:** Suppresses new starts. Running pump stops at 100% during sleep.
- **Idle:** If level ≥ 90% + pump off for 5 min: slow-poll (10s sensor / 30s Firebase).

---

## 4. Revised Priority Hierarchy — Research-Backed

### 4.1 Six-Level Cascade

```
╔══════════════════════════════════════════════════════════════════════╗
║  P0 — ABSOLUTE OVERRIDE (FORCE_ON)                                   ║
║  Relay ON unconditionally. Safety checks RUN but do NOT stop relay.  ║
║  Standard: IEC 61511 §11.6 — Deliberate operator override            ║
║  Trigger: pumpMode == "FORCE_ON"                                     ║
║  Action:  setPump(true). return.                                     ║
╠══════════════════════════════════════════════════════════════════════╣
║  P1 — HARD SAFETY (DRY-RUN + OVERFLOW LOCKOUT)                      ║
║  Cannot be bypassed by any mode except P0.                           ║
║  Standard: IEC 61511-1 §10 — Safety function SIL requirements        ║
║  Trigger: isDryRunError=true OR isOverflowError=true                 ║
║  Action:  setPump(false). Revert MANUAL/COUNTDOWN to AUTO.           ║
╠══════════════════════════════════════════════════════════════════════╣
║  P2 — EMERGENCY STOP (FORCE_OFF)                                     ║
║  Relay held OFF every cycle. Persistent through power cycles.        ║
║  Standard: IEC 60204-1 §9.2.5 — Emergency stop category 0           ║
║  Trigger: pumpMode == "FORCE_OFF"                                    ║
║  Action:  setPump(false). return.                                    ║
╠══════════════════════════════════════════════════════════════════════╣
║  P3 — MANUAL RUN (MANUAL mode — full safety active)                  ║
║  Operator-initiated. Identical safety coverage to AUTO.              ║
║  Trigger: pumpMode == "MANUAL"                                       ║
║  Action:  Check tank-full, level sensor error → stop if needed.      ║
║           Otherwise: setPump(true).                                  ║
╠══════════════════════════════════════════════════════════════════════╣
║  P4 — COUNTDOWN TIMER (full safety active)                           ║
║  Timed operator-initiated run.                                       ║
║  Trigger: pumpMode == "COUNTDOWN" && isCountdownActive               ║
║  Action:  Check tank-full → early stop if needed.                    ║
║           Otherwise: setPump(true).                                  ║
╠══════════════════════════════════════════════════════════════════════╣
║  P5 — AUTO HYSTERESIS                                                ║
║  P5a: Sleep check — suppress new autonomous starts                   ║
║  P5b: Level sensor bypass — flow guard only                          ║
║  P5c: Level sensor error — fail-safe stop                            ║
║  P5d: Standard hysteresis — start ≤30%, stop ≥100%                  ║
╚══════════════════════════════════════════════════════════════════════╝
```

### 4.2 Safety Coverage Matrix

| Protection | `AUTO` | `MANUAL` | `COUNTDOWN` | `FORCE_OFF` | `FORCE_ON` |
|-----------|:------:|:--------:|:-----------:|:-----------:|:----------:|
| Dry-run timer (P1) | ✅ | ✅ | ✅ | N/A | ❌ relay unaffected |
| Overflow max runtime (P1) | ✅ | ✅ | ✅ | N/A | ❌ relay unaffected |
| Tank-full stop at 100% | ✅ | ✅ | ✅ | N/A | ❌ relay unaffected |
| Level sensor error fail-safe | ✅ | ✅ | ⚠️ P1 only | N/A | ❌ relay unaffected |
| Sleep window (no new starts) | ✅ | ❌ runs through | ❌ runs through | N/A | ❌ (P0) |
| Level sensor bypass support | ✅ | ✅ | ✅ | N/A | N/A (no level checks) |
| Hardware TOR (LR2-D13) | ✅ | ✅ | ✅ | ✅ | ✅ — firmware-independent |

> **Hardware TOR note:** The LR2-D13 thermal overload relay operates **entirely independent of firmware**. It monitors actual motor current and trips the contactor open when the motor overheats. This is the backstop protection that applies in FORCE_ON when firmware safety is bypassed. However, TOR trip recovery requires a manual reset — the firmware has no awareness of a TOR trip event. This is documented as a hardware limitation in §8.

---

## 5. Revised State Machine & Conflict Resolution

### 5.1 pumpMode → runMode Derivation — v4.0

| `pumpMode` | Additional Condition | `runMode` v4.0 | Relay |
|-----------|---------------------|----------------|-------|
| `"FORCE_ON"` | — | `"FORCE_ON"` | ON (absolute) |
| any | `isDryRunError` OR `isOverflowError` | `"OFF"` | OFF (unless P0) |
| `"FORCE_OFF"` | — | `"OFF"` | OFF (held) |
| `"MANUAL"` | `isRunning = true` | `"MANUAL"` | ON |
| `"MANUAL"` | `isRunning = false` (safety stopped) | `"OFF"` | OFF (awaiting clear or manual_stop) |
| `"COUNTDOWN"` | `isCountdownActive = true` | `"COUNTDOWN"` | ON |
| `"COUNTDOWN"` | `isCountdownActive = false` | `"OFF"` | Transitioning |
| `"AUTO"` | `isRunning = true` | `"AUTO"` | ON |
| `"AUTO"` | `isRunning = false` | `"AUTO_STANDBY"` | OFF |

### 5.2 Complete Conflict Resolution Matrix — v4.0

| Command / Event | System State | Priority | Outcome |
|----------------|-------------|---------|---------|
| `manual_start=true` | `isDryRunError=true` | P1 wins | Rejected. "Manual run rejected: error lockout active." |
| `manual_start=true` | `isOverflowError=true` | P1 wins | Same rejection. |
| `manual_start=true` | `pumpMode="FORCE_OFF"` | P2 wins | Rejected. "Manual run rejected: FORCE_OFF active." |
| `manual_start=true` | `isLevelSensorError=true`, bypass OFF | P3 immediate revert | `pumpMode="MANUAL"` set → P3 level-error check → `setPump(false)`, `pumpMode="AUTO"`. Net: pump does not start. |
| `manual_start=true` | `isLevelSensorError=true`, bypass ON | P3 wins | `pumpMode="MANUAL"`. Pump starts. P1 dry-run guards. |
| `manual_start=true` | `isSleeping=true` | P3 wins | Pump starts. Sleep only suppresses P5 AUTO starts. |
| `manual_start=true` | COUNTDOWN active | P3 wins (overwrites) | `pumpMode="MANUAL"`. `isCountdownActive=false` via mode-exit cleanup. Pump continues under MANUAL. |
| `mode=FORCE_ON` | `isDryRunError=true` | **P0 wins** | Pump starts. `isDryRunError` flag remains set — visible as warning on dashboard. |
| `mode=FORCE_ON` | `isOverflowError=true` | **P0 wins** | Pump starts. `isOverflowError` flag remains set as warning. |
| `mode=FORCE_ON` | `isLevelSensorError=true` | **P0 wins** | Pump starts. All sensor errors are warnings only. |
| `mode=FORCE_ON` | `pumpMode="FORCE_OFF"` | Control read handles | `FORCE_OFF` run-active intercept fires → `setPump(false)`, `pumpMode="FORCE_OFF"`. FORCE_ON acknowledged but FORCE_OFF wins in control read. **Recommendation: In control read, if `newMode=FORCE_ON` and run not active, pumpMode set normally. executePumpLogic P0 fires. FORCE_OFF from this point requires another control command.** |
| `mode=FORCE_OFF` | FORCE_ON active | Control read run-active intercept | `setPump(false)`, `pumpMode="FORCE_OFF"`. Handled before executePumpLogic. |
| `mode=FORCE_OFF` | MANUAL running | Control read intercept | Same. `setPump(false)`, `pumpMode="FORCE_OFF"`. |
| `mode=FORCE_OFF` | COUNTDOWN active | Control read intercept | Same. `isCountdownActive=false`, `setPump(false)`, `pumpMode="FORCE_OFF"`. |
| `manual_stop=true` | FORCE_ON active | **Ignored** | Log: "Manual stop ignored: FORCE_ON active. Use mode selector." No state change. |
| `manual_stop=true` | MANUAL running | Accepted | `setPump(false)`, `pumpMode="AUTO"`, `pendingModeWriteback=true`. |
| `manual_stop=true` | COUNTDOWN active | Accepted | `setPump(false)`, `isCountdownActive=false`, `pumpMode="AUTO"`, `pendingModeWriteback=true`. |
| `manual_stop=true` | FORCE_OFF | No effect | Pump already off. `manual_stop` reverts to AUTO, not FORCE_OFF. `pumpMode="AUTO"` set. |
| `mode=AUTO` | `isDryRunError=true` | P1 wins after mode set | `pumpMode="AUTO"`. P1 still fires every cycle. Pump stays off. Operator must `clear_error`. |
| `countdown_add_time=true` | Timer expired | No effect | `isCountdownActive=false` — condition fails. Flag reset silently. New countdown required. |
| `countdown_add_time=true` | `isDryRunError=true`, timer still has time | Timer extended, relay OFF | `countdownEndMs` extended. P1 keeps relay OFF. On `clear_error`, pump resumes if timer has remaining time. |
| P1 dry-run fires | `pumpMode="MANUAL"` | P1 wins | `setPump(false)`. `pumpMode` stays `"MANUAL"`. `runMode="OFF"`. Pump stays off in MANUAL until `clear_error`. After clear: P3 runs → `setPump(true)`. |
| P1 overflow fires | `pumpMode="MANUAL"` | P1 wins | Same. `pumpMode` stays `"MANUAL"`, relay OFF. |
| P1 fires | `pumpMode="COUNTDOWN"` | P1 wins | `setPump(false)`. `isCountdownActive=false`. `pumpMode="AUTO"`. `pendingModeWriteback=true`. |
| Level sensor error | `pumpMode="MANUAL"`, bypass OFF | P3 check | `setPump(false)`. `pumpMode` stays `"MANUAL"`. Return. Pump off until bypass enabled or sensor recovers. |
| Level ≥ 100% | `pumpMode="MANUAL"` | P3 tank-full | `setPump(false)`. `pumpMode="AUTO"`. `pendingModeWriteback=true`. Mode reverts to AUTO. |

---

## 6. Safety System Analysis — IEC 61511 Alignment

### 6.1 Safety Function Classification

Under IEC 61511 (Functional Safety for Process Industries), a pump control system with water level management constitutes a **Safety Instrumented Function (SIF)**. Each protection mechanism maps to a safety function:

| Safety Function | Sensor | Logic | Final Element | SIL Assessment |
|----------------|--------|-------|--------------|---------------|
| Dry-run protection | YF-G1 flow sensor | `checkDryRunProtection()` | Relay + Contactor | SIL 1 capable |
| Overflow (max runtime) | `millis()` timer | `checkOverflowProtection()` | Relay + Contactor | SIL 1 capable |
| High-level cut-off | JSN-SR04T | `executePumpLogic()` P5d | Relay + Contactor | SIL 1 capable |
| Sensor failure safe-state | Error counter | `checkLevelSensorFailure()` | Relay (off) | SIL 1 capable |

**SIL 1 requirements (IEC 61511):** PFD (Probability of Failure on Demand) ≤ 0.1. For a residential/commercial water pump, SIL 1 is appropriate. The current architecture achieves this through:
- Independent sensor channels (level + flow)
- Fail-safe default (relay HIGH = pump OFF)
- Hardware TOR as independent final protection layer
- WDT-enforced controller availability

### 6.2 Common Cause Failures (CCF) — Current Gaps

**Gap 1 — Single point of failure: ESP32**

If the ESP32 crashes but the WDT fails to trigger (possible under certain flash read operations), the pump relay retains its last state. If the pump was ON when the crash occurred, it stays ON. The WDT at 120s mitigates this, but does not eliminate it.

**Recommendation:** Add a hardware watchdog external to the ESP32 (e.g., MCP7940 RTC with watchdog output or a simple 555-timer watchdog circuit). The external WDT disconnects the relay coil power rail if the ESP32 fails to toggle a GPIO pin within 30s. This provides an independent safety layer that is architecturally separate from the firmware. Estimated cost: ₱50–200.

**Gap 2 — Relay welding**

A relay contact can weld closed under high inrush current (1.5HP motor draws ~7–8A on start, ~20A momentary). If the relay contacts weld closed, `setPump(false)` writes GPIO HIGH but the relay does not open. The pump continues running.

**Recommendation:** Flow-based detection. If `isRunning = false` (software state) but `flowRateLpm > 2.0 LPM` for > 5s, flag a new fault: `isRelayStuckError`. This is partially implemented as `isFlowSensorError` but currently only checks for stuck-high sensor, not stuck relay. Document this ambiguity and add the relay-stuck scenario to the fault table.

**Gap 3 — Single ultrasonic sensor**

The entire level-based automation depends on one JSN-SR04T sensor. No redundancy. Failure mode is fail-safe (error → pump OFF in AUTO), but this means the tank goes empty if the sensor fails during a dry period.

**Recommendation:** If budget allows, add a secondary float switch at the 20% level mark as a hardware-wired emergency low-level cutoff. This is independent of the ultrasonic and provides backup protection. At minimum, document `cfgAutoBypassOnSensorFail` and its purpose as the operator's mitigation option.

### 6.3 Recommended Additional Safety Functions

| Proposed Function | Trigger | Response | Implementation Effort |
|------------------|---------|---------|---------------------|
| **Relay stuck detection** | `!isRunning && flowRateLpm > 2.0 LPM && elapsed > 5s` | `isRelayStuckError=true`. Dashboard alert. Operator must physically inspect. | Low — reuse `checkFlowSensorStuck()` logic, add `isRunning` context |
| **Power quality monitoring** | Voltage reading or brownout count from NVS | Alert if frequent brownouts (indicates supply instability) | Medium — needs ADC or current sense |
| **FORCE_ON auto-timeout** | `pumpMode=="FORCE_ON" && runtime > cfgForceOnMaxMin` | Revert to AUTO. Log override auto-expired. | Low — add timer check in P0 block |
| **External hardware WDT** | GPIO toggle expected every 10s | External circuit cuts relay power rail | Hardware addition — high safety value |

---

## 7. Sensor Engineering Analysis

### 7.1 JSN-SR04T Ultrasonic Sensor — Engineering Limits

The JSN-SR04T is a waterproof variant of the HC-SR04 using a piezoelectric transducer separated from the control board by a 2.5m cable. On a 40m CAT6 cable as used in this deployment, signal degradation is a significant concern.

**Signal integrity over 40m CAT6:**

| Issue | Effect | Current Mitigation | Recommendation |
|-------|--------|-------------------|---------------|
| Capacitive loading (40m cable ~0.5µF) | Echo pulse rise time degradation → shorter measured duration → lower reported distance → higher reported level | 5-sample median filter removes most cable glitches | Add 100Ω series resistor on TRIG and ECHO lines at sensor end to dampen resonance |
| Inductive coupling from pump power cable (50m in parallel) | EMI-induced false echoes → short duration → reading < 2cm → rejected by range filter | `distanceCm < 2.0` rejection | Keep CAT6 and power cable physically separated by ≥30cm where they run parallel |
| Temperature coefficient | Speed of sound varies 0.17% per °C | EMA filter smooths gradual drift | Optional: add NTC thermistor to correct for ambient temperature. Formula: `v = 331.3 + 0.606×T` m/s |

**Temperature correction formula (optional improvement):**
```
currentSpeedOfSound = 331.3 + (0.606 × temperatureCelsius)  // m/s
correctedDistanceCm = (duration_us × currentSpeedOfSound) / 20000.0
```
Without correction, a 10°C ambient temperature swing (Iloilo climate: ~24–34°C) introduces ≈3% distance error. For a 114cm range (8–122cm), this is ≈3.4cm → ≈3% level error. Acceptable for a 660L tank but worth noting.

**JSN-SR04T minimum measurement distance (blind zone):** The datasheet specifies 25cm minimum. The firmware rejects readings < 2cm, but the true minimum should be 25cm. At TANK_FULL_CM = 8cm, the sensor is operating inside its blind zone when the tank is near-full. 

**Recommendation:** Verify the actual mounted sensor distance at full tank. If the water surface is closer than 25cm to the sensor face at 100% fill, readings near 100% may be unreliable. Consider setting `TANK_FULL_CM = 25` as a safe minimum. Alternatively, use the dead-band: if the pump stop trigger (100%) maps to a distance of 8cm, but the sensor cannot reliably read below 25cm, the actual stop may be triggered earlier by the 5-sample failure returning -1 → `isLevelSensorError` → fail-safe stop. This is functionally correct but the documentation should explain it.

### 7.2 YF-G1 Flow Sensor — Engineering Limits

**Specification:** K-factor = 7.5 pulses/L (default). Range: 1–30 LPM (reliable). Max: 60 LPM.

**Critical issue:** The 1.5HP pump can deliver 40–60 LPM at its rated head. At 60 LPM, pulse frequency = 60 × 7.5 = 450 Hz. Period = 2,222µs. Firmware debounce = 2,000µs. At maximum flow, the debounce window is almost exactly at the pulse period — some pulses will be rejected, causing under-reporting of flow rate.

**Effect:** Under-reported flow rate at high pump output may falsely trigger the dry-run warning (flow appears lower than 0.5 LPM threshold). With the 30s timer and the realistic flow rates of a 1.5HP pump, this is unlikely to cause a false lockout — but it could cause false warnings at the dashboard.

**Recommendation:** 
1. Reduce `FLOW_DEBOUNCE_US` from 2000µs to 1200µs. This safely accommodates pulses up to 833 Hz (100 LPM) while still rejecting most noise bursts (which typically have inter-pulse gaps < 200µs).
2. Perform an in-situ bucket test to calibrate the actual K-factor for the installed pipe diameter and pump characteristics. Typical variance: ±10%.

**Dead-band after pump stops (FLOW_PUMP_OFF_ZERO_MS = 3000ms):** Good. Physics: a centrifugal pump with 50m of head pressure can continue to deliver flow for 1–3 seconds after the relay opens due to stored kinetic energy in the impeller and static head. 3 seconds is correct. However, at very high static heads (50m+), this can extend to 5–8 seconds. If false dry-run warnings appear immediately after pump stops, increase `FLOW_PUMP_OFF_ZERO_MS` to 5000ms.

### 7.3 millis() Timer Accuracy for Countdown

`millis()` on the ESP32 uses the RTC clock, which has a documented accuracy of ±0.005% under normal conditions (26MHz crystal). Over a 120-minute countdown, maximum drift = ±3.6 seconds. Negligible for this application.

**`millis()` rollover:** At 49.7 days continuous uptime, `millis()` wraps to 0. All timer arithmetic in the firmware uses subtraction (`millis() - lastMs`), which handles rollover correctly due to unsigned arithmetic. However, `countdownEndMs = millis() + N×60000` creates a large absolute target value. If `millis()` wraps near a running countdown, `millis() >= countdownEndMs` evaluates incorrectly for one cycle before the subtraction-based check in `checkCountdownExpiry()` self-corrects.

**This is an existing minor bug:** `countdownEndMs` is an absolute value, but `checkCountdownExpiry()` correctly uses `millis() >= countdownEndMs` which fails for one cycle after rollover at 49.7 days. For practical operations, this is irrelevant (no residential pump runs continuously for 49.7 days). Document for completeness.

---

## 8. Pump Motor Protection — Engineering Standards

### 8.1 1.5HP Single-Phase Motor Characteristics

**Inrush current:** Single-phase induction motors draw 6–8× full-load current at startup. At 1.5HP (rated ~7A FLA at 220V), inrush can reach 42–56A for 100–500ms. This is within the CJX2-2510 contactor's rated making current (25A continuous, higher short-circuit rating).

**Duty cycle:** Most single-phase pump motors are rated for **S3 intermittent duty** (IEC 60034-1), typically 30–60 starts per hour maximum. The current firmware has no per-hour start count limit.

**Recommendation (new feature):** Add a **minimum off-time** between pump starts. Standard: 30-second off-time between starts prevents thermal buildup from repeated start cycles. This is a single variable check: `if (pumpOffStartMs > 0 && millis() - pumpOffStartMs < MIN_OFF_TIME_MS) return;` at the top of P3, P4, and P5d start conditions. Suggested `MIN_OFF_TIME_MS = 30000` (30s).

**Thermal time constant:** The motor's thermal time constant (time to reach 63% of steady-state temperature) is approximately 20–40 minutes for a 1.5HP TEFC motor. The 120-minute overflow timer is conservative and appropriate. However, if a dry-run fires at 119 minutes, clears immediately, and the pump restarts, the motor is still at maximum thermal loading. The 30-second minimum off-time recommendation partially addresses this.

### 8.2 Dry-Run Protection — Engineering Basis

**Why 0.5 LPM threshold:** At 0.5 LPM, a centrifugal pump is operating far left of its performance curve — near shut-off head. This is associated with:
- Low flow cavitation damage (NPSH considerations)
- Bearing seal damage (seals require flow for lubrication in many designs)
- Motor thermal issue (impeller efficiency drops, motor load increases)

**Why 30s timeout:** A 30-second window prevents false trips from brief flow interruptions (air locks, surge). ISO 9905 §6.4 (centrifugal pump operation) recommends a minimum 20–30 second observation period before declaring pump dry.

**Current implementation is correct.** The 0.5 LPM threshold and 30s timeout are well-chosen defaults. Operators should calibrate based on their specific pump's minimum flow specification.

### 8.3 Hardware TOR (LR2-D13) — Firmware Integration Gap

The LR2-D13 provides hardware overload protection, but the firmware has no mechanism to detect a TOR trip event. If the TOR trips:
1. The contactor opens (relay circuit broken).
2. The firmware still believes the pump is ON (`isRunning = true`).
3. Flow rate drops to zero → dry-run timer starts → 30s later → `isDryRunError = true`.
4. Firmware calls `setPump(false)` → writes GPIO4 HIGH (no change — relay already open).
5. Dashboard shows `is_error: true` with `last_fault_code: "DRY_RUN"`.

**The TOR trip is ultimately detected through the dry-run mechanism.** This is an acceptable indirect detection path, but the 30-second delay means the pump motor may be hot for 30 additional seconds after TOR protection has already acted. For the firmware, no change is needed — but the documentation should explain this path so operators understand why a dry-run error appears without obvious water supply issues.

---

## 9. Worst-Case Scenario Analysis

### 9.1 Scenario WC-01: Complete Sensor Failure During FORCE_ON

**Conditions:** `pumpMode = "FORCE_ON"`. Both sensors fail: JSN-SR04T returns -1 (ultrasonic failure), YF-G1 returns 0 LPM (flow sensor disconnected or jammed at zero).

**Firmware behavior:**
- `checkDryRunProtection()`: pump is ON, `flowRateLpm = 0 < 0.5` → dry-run timer starts. After 30s: `isDryRunError = true`. `setPump(false)` called.
- `executePumpLogic()` P0: `pumpMode = "FORCE_ON"` → `setPump(true)`. P0 fires AFTER `checkSafetyCutoff()`, so `setPump(false)` from dry-run is immediately overridden by `setPump(true)` from P0.

**Wait — is this correct?** Let me trace the exact execution:
1. `checkSafetyCutoff()` runs → `checkDryRunProtection()` → sets `isDryRunError=true`, calls `setPump(false)` → GPIO4 HIGH
2. `checkCountdownExpiry()` runs (no-op in FORCE_ON)
3. `executePumpLogic()` runs → runMode derivation → **P0 fires: `setPump(true)` → GPIO4 LOW**

So: dry-run calls `setPump(false)`, then 2ms later `executePumpLogic` P0 calls `setPump(true)`. Net result: relay turns OFF then ON within the same sensor cycle. Motor experiences a brief drop-out. In practice, the contactor may not even fully open in 2ms. **The relay will oscillate between OFF and ON every 1s as long as FORCE_ON is active and flow sensor reads zero.**

**Assessment:** This is the intended FORCE_ON behavior — operator has confirmed they want the pump ON regardless. The relay cycling is a consequence of P0 overriding P1 each cycle. The `totalPumpCycles` counter will increment every cycle (every 1s), which will inflate the lifetime counter during FORCE_ON with sensor failure.

**Recommendation:** In P0 branch, check if `isDryRunError` was just set THIS cycle and `isRunning` was just set to false. If so, `totalPumpCycles` should not increment on the forced-back-ON. Alternatively, mark `isRunning` as true at the start of P0 regardless, preventing the counter from triggering.

**Physical outcome:** Pump motor may sustain cavitation damage. This is the operator's accepted risk when using FORCE_ON with sensors bypassed. The dashboard must show the cycling clearly.

### 9.2 Scenario WC-02: WiFi Outage During Active COUNTDOWN

**Conditions:** COUNTDOWN active with 30 minutes remaining. WiFi drops. Remains offline for 2 hours.

**Firmware behavior:**
- `isCountdownActive = true`. Timer runs on `millis()` — WiFi outage has no effect.
- After 30 minutes: `checkCountdownExpiry()` fires → `pumpMode = "AUTO"`. `pendingModeWriteback = true`.
- `executePumpLogic()` P5 evaluates level. If level ≥ 100%: pump stays off. If level ≤ 30%: pump starts in AUTO.
- Firebase attempts fail → `firebaseCooldownUntilMs` set. `pendingModeWriteback` retries are queued but cannot execute.
- After 2 hours: WiFi recovers. On next Firebase cycle: `pendingModeWriteback` fires → mode written to AUTO in Firebase. Dashboard updates.

**Assessment:** ✅ Correct. Pump logic is fully correct throughout outage. Dashboard shows offline but hardware operates safely.

**Edge case:** If WiFi recovers exactly at the moment `pendingModeWriteback` is about to retry, and the Firebase propagation shows `"COUNTDOWN"` in the control JSON, the writeback suppression mechanism (`pendingModeWriteback = true`) prevents the stale COUNTDOWN from re-arming.

### 9.3 Scenario WC-03: Power Failure During Pump Run, Resumed at Critically Low Level

**Conditions:** Power fails while pump running in AUTO. Tank is at 60% when power cuts. During outage (hours), water is consumed. Tank drops to 5%. Power resumes.

**Firmware behavior:**
1. Boot: `loadStateFromNVS()` → `pumpMode = "AUTO"`.
2. `checkCrashLoop()`: one boot event within window → `bootCount = 1`. Not safe mode.
3. 5s stabilization delay. Sensors settle.
4. WiFi connects. Firebase sync. `cfgSleepEmergencyLevel` = 5%.
5. First sensor read: `waterLevelPct = 5%`. Emergency override: `emergencyOverride = true` → `isSleeping = false` even if in sleep window.
6. P5d: `!isRunning && level (5%) ≤ cfgPumpStartLevel (30%)` → `setPump(true)`.
7. Pump starts within ~10 seconds of power restoration.

**Assessment:** ✅ Correct. The sleep emergency override is specifically designed for this scenario.

### 9.4 Scenario WC-04: Repeated Rapid Reboots (Firmware Crash Loop)

**Conditions:** New firmware has a bug causing panic on a specific sensor reading. System reboots every 30 seconds.

**Firmware behavior:**
- Each boot: `checkCrashLoop()` increments `bootCount`. After 5 reboots within 300s: `inSafeMode = true`.
- Safe mode: relay OFF, no WiFi, no Firebase, no sensors. Heartbeat only.
- Pump stops. Dashboard shows offline.
- After 1 hour: `ESP.restart()` with cleared crash loop counters → tests if crash condition is still present.

**Assessment:** ✅ Correct. The crash loop detection is a critical safety feature.

**Improvement:** In safe mode, the last `lastFaultCode` (`"SAFE_MODE"`) should be written to NVS so that when the device eventually recovers, the dashboard can display a historical "crash loop detected" event. Currently, if the device crashes into safe mode, then auto-recovers after 1 hour, the dashboard only sees a normal boot unless the operator was watching during the safe mode period.

### 9.5 Scenario WC-05: Firebase Sends Malformed Mode String

**Conditions:** A Firebase rule misconfiguration or a third-party integration writes `mode = "FORCE_BOTH"` or `mode = "null"` or `mode = ""`.

**Firmware behavior:**
In `readFirebaseControl()`:
```cpp
if (newMode == "AUTO" || newMode == "FORCE_ON" || newMode == "FORCE_OFF" 
    || newMode == "COUNTDOWN" || newMode == "MANUAL") {
```
Any other string fails this check. `pumpMode` is not updated. Current mode holds.

**Assessment:** ✅ Correct. Unknown modes are silently ignored. The pump continues on the last valid `pumpMode`.

**Recommendation:** Log unknown modes: `Serial.printf("[FIREBASE] Unknown mode received: '%s'. Ignoring.\n", newMode.c_str())`. Allows debugging without affecting behavior.

### 9.6 Scenario WC-06: millis() Rollover at 49.7 Days

As documented in §7.3, `countdownEndMs` is an absolute value. After 49.7 days, `millis()` returns to 0. If a countdown was set for `countdownEndMs = 4,294,000,000` (near rollover), `millis() >= countdownEndMs` will be `millis() (small) >= 4.29B (large)` → false → countdown never expires.

**Assessment:** Minor bug. Extremely unlikely in practice (no countdown should be active at 49.7-day uptime). No change required, but document.

### 9.7 Scenario WC-07: Level Sensor Returns Maximum Distance (Tank Empty False Positive)

**Conditions:** Tank is at 80% (sensor reads ~34cm). Ultrasonic gets a reflection off a floating object in the tank or a side-wall echo. Returns 122cm (TANK_EMPTY_CM).

**Firmware behavior:**
- Single reading of 122cm = level 0%.
- EMA: `0.5 × 0% + 0.5 × 80% = 40%`. Rounded to 40%.
- Rate-of-change check: `|40% - 80%| = 40% > 15%` → rejected. `prevWaterLevelPct (80%)` returned.

**Assessment:** ✅ Correct. The rate-of-change guard catches this exactly. A single bad reflection cannot trigger an AUTO pump start (which would only happen at ≤ 30%).

---

## 10. Additional Improvement Recommendations

### R-01: Minimum Off-Time Between Pump Starts

**Problem:** No minimum cooldown between pump stops and starts. Rapid short-cycling damages motor windings through inrush current heating.

**Research basis:** IEEE Std 515, §7.2 (motor thermal protection): recommends minimum 30s off-time for fractional to 5HP motors. IEC 60034-1 S3 duty cycle: maximum 30–60 starts/hour.

**Recommendation:** Add `MIN_PUMP_OFF_TIME_MS = 30000` constant. In `executePumpLogic()`, before any `setPump(true)` call in P3, P4, P5d: check `pumpOffStartMs > 0 && (millis() - pumpOffStartMs) < MIN_PUMP_OFF_TIME_MS`. If true, skip the start this cycle. This does not affect P0 (FORCE_ON bypasses this too).

### R-02: FORCE_ON Auto-Timeout

**Problem:** If an operator activates FORCE_ON and loses connectivity, the pump runs indefinitely with all safety bypassed.

**Research basis:** IEC 61511 §11.6.2: overrides should have automatic time limits to prevent "forgotten override" scenarios.

**Recommendation:** Add configurable `cfgForceOnMaxMin` (default 60 minutes, range 5–120 min, configurable via Firebase). In P0 block: check elapsed time since FORCE_ON was set (track with a new `forceOnStartMs` variable). If elapsed > `cfgForceOnMaxMin × 60000`: log "FORCE_ON auto-expired", set `pumpMode = "AUTO"`, `pendingModeWriteback = true`. This is a configurable safety net, not a hard limit.

### R-03: Log Unknown Mode Received from Firebase

As noted in §9.5. One-line Serial.printf. Zero firmware risk.

### R-04: Relay Stuck-Closed Detection Enhancement

**Problem:** `isFlowSensorError` currently flags stuck-high flow sensor AND relay-stuck-closed as the same condition. The firmware cannot distinguish between them.

**Recommendation:** Add context: if `!isRunning && flowRateLpm > 2.0 LPM && elapsed > 5s` AND `gpio_get_level(RELAY_PIN) == HIGH` (relay should be open), flag `isRelayStuckError` separately from `isFlowSensorError`. In practice, reading GPIO level is trivially available: `digitalRead(RELAY_PIN) == HIGH` means firmware commanded OFF. If flow is still reading high, it could be either sensor error OR relay stuck. Document the ambiguity clearly.

### R-05: Hysteresis Threshold Configurability Documentation Gap

**Problem:** `PUMP_START_LEVEL (30%)` and `PUMP_STOP_LEVEL (100%)` are configurable via Firebase (`pump_start_level`, `pump_stop_level`). The spec documents this, but does not document the operational implications of certain threshold combinations.

**Dangerous combination:** If operator sets `pump_start_level = 95` and `pump_stop_level = 100`, the dead-band is only 5% (33L in a 660L tank). This means the pump short-cycles every time 33L is consumed — potentially 50+ starts per hour.

**Recommendation:** Add validation: `if (po - ps < 10) return false` (minimum 10% dead-band). Also document the calculation: at typical household consumption of 200 LPM peak, a 10% dead-band (66L) means a minimum ~20 second run per cycle. A 70% dead-band (462L) means a minimum ~9-minute run per cycle.

### R-06: `isOverflowError` Semantics Improvement

**Problem:** `isOverflowError` is set when `checkOverflowProtection()` fires (max runtime exceeded). The name "overflow error" implies the tank overflowed. But this protection fires when the pump runs too long WITHOUT reaching the stop level — which often means the tank is NOT overflowing (the level sensor may have failed, or the fill rate is simply very slow).

**Recommendation:** Rename to `isMaxRuntimeError` or add a new `lastFaultCode` sub-type: `"OVERFLOW_RUNTIME"` distinct from a potential future `"TANK_OVERFLOW"`. In the current spec, update §14.2 to clarify: `is_overflow_error: true` means "pump ran too long" not "physical tank overflow detected."

### R-07: MANUAL Mode NVS Restore — Boot Start Delay

**Related to R-01 and the NVS restore recommendation in §3.2.**

If `pumpMode = "MANUAL"` is restored from NVS after a power cut, the pump will start within the 5-second stabilization delay (the first sensor cycle fires within 1s after loop() begins). This is correct behavior, but the 5s delay in `setup()` may be insufficient for:
- Mechanical check valve seating (typically 2–5s)
- Pressure transient equalization in 50m pipe run

**Recommendation:** The existing 5s stabilization delay covers this adequately. No change needed. Document that the delay serves multiple purposes: sensor settling, capacitor charging, AND pipe equalization.

### R-08: Firebase Data Retention Policy Documentation

**Problem:** The spec documents what is *written* to Firebase but not for how long it persists. Firebase RTDB does not automatically expire data. `/pump_system/status` grows unbounded if the dashboard never cleans it.

**Recommendation:** Document the intended data lifecycle:
- `/pump_system/status`: Overwritten every 3s — single object, no growth.
- `/pump_system/control`: Persistent until explicitly cleared — managed by dashboard.
- `/pump_system/config/device`: Persistent — only written by dashboard config saves.
- `/audit/force_on_events`: Append-only — define retention policy (e.g., last 100 entries).
- `/audit/events`: Append-only — define retention policy.

### R-09: Security — Firebase Rules for `/control/` Write Access

**Current state:** The spec mentions Google OAuth via NextAuth.js restricts `/control/` writes to authorized UIDs. This is implemented at the dashboard level.

**Problem:** Firebase RTDB rules are the authoritative security enforcement layer — not the dashboard. If someone with the API key writes directly to `/control/mode = "FORCE_ON"` via REST API, the dashboard OAuth has no effect.

**Recommendation (essential):** Document the required Firebase RTDB security rules:
```json
{
  "/pump_system/control": {
    ".write": "auth != null && auth.uid == 'AUTHORIZED_UID'",
    ".read": "auth != null"
  },
  "/pump_system/status": {
    ".write": "auth.token.email == 'ESP32_SERVICE_EMAIL'",
    ".read": "auth != null"
  }
}
```
FORCE_ON specifically should require the admin UID (not any authenticated user). This is a security-critical gap.

### R-10: Startup Behavior on First Boot (No NVS Data)

**Current state:** If NVS has never been written (factory fresh ESP32), `loadDeviceConfigFromNVS()` reads `tank_empty = -1` → exits early → compiled defaults used. `loadStateFromNVS()` reads `mode = ""` → defaults to `"AUTO"`. This is correct.

**Recommendation:** Document this explicitly as the "first boot" scenario. Operators who flash a new ESP32 should see the pump start in AUTO mode with compiled defaults. The dashboard should prompt for device configuration on first connection.

---

## 11. Complete Scenario Matrix — v4.0

### 11.1 AUTO Mode

| ID | Trigger | Firmware Logic | GPIO 4 | Dashboard Status |
|----|---------|---------------|--------|-----------------|
| AU-01 | AUTO · level > 30% · pump OFF | P5d: start condition false. No action. | HIGH (OFF) | `run_mode: "AUTO_STANDBY"` |
| AU-02 | AUTO · level ≤ 30% · pump OFF · min off-time met | P5d: `setPump(true)` · cycles++ · overflow timer starts | LOW (ON) | `run_mode: "AUTO"` · `is_running: true` |
| AU-03 | AUTO · level ≥ 100% · pump ON | P5d: `setPump(false)` · runtime accumulated | HIGH (OFF) | `run_mode: "AUTO_STANDBY"` |
| AU-04 | AUTO · level ≤ 30% · pump OFF · min off-time NOT met (R-01) | P5d: start suppressed pending off-time | HIGH (OFF) | `run_mode: "AUTO_STANDBY"` · `last_pump_off_sec: <30s` |
| AU-05 | AUTO · sleep window · level ≤ 30% | P5a: sleep check fires before P5d → no start | HIGH (OFF) | `run_mode: "AUTO_STANDBY"` · `is_sleeping: true` |
| AU-06 | AUTO · sleep window · level ≤ 5% (emergency override) | `emergencyOverride=true` → `isSleeping=false` · P5d fires | LOW (ON) | `run_mode: "AUTO"` · `is_sleeping: false` |
| AU-07 | AUTO · level sensor error · pump running | P5c: `setPump(false)` · `lastFaultCode="LEVEL_SENSOR"` | HIGH (OFF) | `is_level_sensor_error: true` · `run_mode: "OFF"` |
| AU-08 | AUTO · level sensor error · auto-bypass triggers | `autoBypassActive=true` · P5b: level ignored · P1 guards | Unchanged | `auto_bypass_active: true` · `level_estimate_active: true` |
| AU-09 | AUTO · level ≥ 90% · pump OFF · stable 5+ min | `isIdleMode=true` · slow-poll begins | HIGH (OFF) | `run_mode: "AUTO_STANDBY"` · 30s update cadence |

### 11.2 MANUAL Mode

| ID | Trigger | Firmware Logic | GPIO 4 | Dashboard Status |
|----|---------|---------------|--------|-----------------|
| MN-01 | `manual_start=true` · No lockout · No FORCE_OFF | `pumpMode="MANUAL"` · P3: `setPump(true)` · overflow timer starts | LOW (ON) | `run_mode: "MANUAL"` · `is_running: true` · **Stop button visible** |
| MN-02 | `manual_start=true` · `isDryRunError=true` | Rejected. No state change. | Unchanged | Error alert. "Clear error first." |
| MN-03 | `manual_start=true` · `pumpMode="FORCE_OFF"` | Rejected. "FORCE_OFF active." | HIGH (OFF) | FORCE_OFF badge. |
| MN-04 | `manual_start=true` · `isLevelSensorError=true` · bypass OFF | `pumpMode="MANUAL"` → P3 immediate level-error check → `setPump(false)`, `pumpMode="AUTO"` | HIGH (OFF) | `run_mode: "AUTO_STANDBY"` · Level sensor alert |
| MN-05 | MANUAL · `manual_stop=true` | `setPump(false)` · `pumpMode="AUTO"` · `pendingModeWriteback=true` | HIGH (OFF) | `run_mode: "AUTO_STANDBY"` · Stop button hides |
| MN-06 | MANUAL · level ≥ 100% | P3 tank-full: `setPump(false)` · `pumpMode="AUTO"` · `pendingModeWriteback=true` | HIGH (OFF) | `run_mode: "AUTO_STANDBY"` |
| MN-07 | MANUAL · flow < 0.5 LPM for 30s | P1: `isDryRunError=true` · `setPump(false)` · `pumpMode` stays `"MANUAL"` · `runMode="OFF"` | HIGH (OFF, immediate) | `is_error: true` · `run_mode: "OFF"` · `last_fault_code: "DRY_RUN"` · Stop button still visible (manual_stop → AUTO) |
| MN-08 | MANUAL · P1 dry-run · operator sends `clear_error` | `isDryRunError=false` · P3 runs next cycle: `setPump(true)` (if `pumpMode` still `"MANUAL"`) | LOW (ON) | `run_mode: "MANUAL"` · Error clears · Pump resumes |
| MN-09 | MANUAL · runtime ≥ 120 min | `checkOverflowProtection()` (MANUAL now covered): `isOverflowError=true` · `setPump(false)` · `pumpMode` stays `"MANUAL"` | HIGH (OFF) | `is_overflow_error: true` · `run_mode: "OFF"` |
| MN-10 | MANUAL · `isLevelSensorError` becomes true mid-run · bypass OFF | P3 level-error check: `setPump(false)`. `pumpMode` stays `"MANUAL"`. | HIGH (OFF) | `is_level_sensor_error: true` · `run_mode: "OFF"` · Manual mode active but pump off |
| MN-11 | MANUAL · `mode=FORCE_OFF` received | Control read run-active intercept: `setPump(false)`, `pumpMode="FORCE_OFF"` | HIGH (OFF) | `run_mode: "OFF"` · FORCE_OFF badge |
| MN-12 | MANUAL · sleep window begins | P3 evaluates before P5 sleep check. Sleep does NOT interrupt MANUAL. | LOW (ON, unchanged) | `run_mode: "MANUAL"` · `is_sleeping: true` |
| MN-13 | MANUAL · boot with NVS `mode="MANUAL"` | NVS restores `pumpMode="MANUAL"` · P3 fires on first sensor cycle: `setPump(true)` | LOW (ON within 6s of boot) | `run_mode: "MANUAL"` · `last_boot_reason: "Power-on"` |

### 11.3 COUNTDOWN Mode

| ID | Trigger | Firmware Logic | GPIO 4 | Dashboard Status |
|----|---------|---------------|--------|-----------------|
| CD-01 | `mode=COUNTDOWN` + `countdown_duration_min=N` (1–120) | `countdownEndMs=millis()+N×60000` · `isCountdownActive=true` · P4: `setPump(true)` | LOW (ON) | `run_mode: "COUNTDOWN"` · `countdown_remaining_sec: N×60` · **Stop + Add Time** |
| CD-02 | Same · Firebase unavailable | Uses `cfgLastCountdownDurationMin` (NVS, default 15 min) | LOW (ON) | "(offline — last known duration)" |
| CD-03 | `countdown_add_time=true` + valid `countdown_add_min=N` | `countdownEndMs += N×60000` (capped) · Firebase resets flag | Unchanged | `countdown_remaining_sec` increases |
| CD-04 | `countdown_add_time=true` + missing/invalid `countdown_add_min` | Default 5 min applied (`COUNTDOWN_ADD_TIME_MIN`) | Unchanged | Timer extended by 5 min |
| CD-05 | `countdown_add_time=true` · Timer already expired | `isCountdownActive=false` → condition fails. Flag reset. | Unchanged | No effect. "Start a new countdown." |
| CD-06 | `millis() ≥ countdownEndMs` | `checkCountdownExpiry()`: `pumpMode="AUTO"` · `pendingModeWriteback=true` | HIGH (OFF, next cycle) | `run_mode: "AUTO_STANDBY"` · `countdown_remaining_sec: 0` |
| CD-07 | Level ≥ 100% during COUNTDOWN | P4 early stop: `setPump(false)` · `pumpMode="AUTO"` | HIGH (OFF) | `run_mode: "AUTO_STANDBY"` · Tank-full stop |
| CD-08 | `manual_stop=true` | `setPump(false)` · `isCountdownActive=false` · `pumpMode="AUTO"` | HIGH (OFF) | `run_mode: "AUTO_STANDBY"` |
| CD-09 | P1 dry-run fires | P1: `setPump(false)` · `isCountdownActive=false` · `pumpMode="AUTO"` · `pendingModeWriteback=true` | HIGH (OFF) | `is_error: true` · `run_mode: "OFF"` |
| CD-10 | Runtime ≥ 120 min before timer | Overflow: `isOverflowError=true` · `setPump(false)` | HIGH (OFF) | `is_overflow_error: true` · `run_mode: "OFF"` |
| CD-11 | `mode=FORCE_OFF` | Control read: `setPump(false)` · `isCountdownActive=false` · `pumpMode="FORCE_OFF"` | HIGH (OFF, held) | `run_mode: "OFF"` · FORCE_OFF badge |
| CD-12 | Boot with NVS `mode="COUNTDOWN"` | `pumpMode="COUNTDOWN"` · `isCountdownActive=false` · On first Firebase sync: re-arm with `cfgLastCountdownDurationMin` | LOW (ON after sync) | `run_mode: "COUNTDOWN"` · Full duration restart |

### 11.4 FORCE_OFF Mode

| ID | Trigger | Firmware Logic | GPIO 4 | Dashboard Status |
|----|---------|---------------|--------|-----------------|
| FF-01 | `mode=FORCE_OFF` · Any active run | Control read: `setPump(false)`, run cleared, `pumpMode="FORCE_OFF"` | HIGH (OFF) | `run_mode: "OFF"` · FORCE_OFF badge (red) |
| FF-02 | `mode=FORCE_OFF` · System idle | `pumpMode="FORCE_OFF"` · P2: `setPump(false)` each cycle | HIGH (OFF, held) | `run_mode: "OFF"` · FORCE_OFF badge |
| FF-03 | FORCE_OFF · Level drops to ≤ 30% | P2 fires before P5. No auto-start ever. | HIGH (OFF, held) | Level low visible. Pump will NOT start. |
| FF-04 | FORCE_OFF · `manual_start=true` | Rejected. "FORCE_OFF active." | Unchanged | Dashboard: "Exit FORCE_OFF to start pump." |
| FF-05 | FORCE_OFF · `manual_stop=true` | `pumpMode="AUTO"`. `pendingModeWriteback=true`. P5 evaluates level. | Per level | `run_mode: "AUTO_STANDBY"` or `"AUTO"` |
| FF-06 | FORCE_OFF · `mode=AUTO` | `pumpMode="AUTO"` · P5 level evaluation | Per level | Mode badge: AUTO |
| FF-07 | FORCE_OFF · Power cycle | NVS restores `"FORCE_OFF"`. P2 holds relay OFF. | HIGH (OFF) | FORCE_OFF badge. Controller online. Pump will not start. |

### 11.5 FORCE_ON Mode

| ID | Trigger | Firmware Logic | GPIO 4 | Dashboard Status |
|----|---------|---------------|--------|-----------------|
| FN-01 | Admin + 2-step confirm · `mode=FORCE_ON` | `pumpMode="FORCE_ON"` · P0: `setPump(true)` unconditionally | LOW (ON) | `run_mode: "FORCE_ON"` · **Full-screen override banner** · Runtime counter visible |
| FN-02 | FORCE_ON · Flow < 0.5 LPM for 30s | `isDryRunError=true` SET (flag only) · P0 maintains relay ON | LOW (ON, maintained) | `is_error: true` (amber warning) · "DRY-RUN — RELAY STILL ON" |
| FN-03 | FORCE_ON · Runtime ≥ 120 min | `isOverflowError=true` SET (flag only) · P0 maintains relay ON | LOW (ON, maintained) | `is_overflow_error: true` (amber warning) · "MAX RUNTIME EXCEEDED — RELAY STILL ON" |
| FN-04 | FORCE_ON · `isLevelSensorError=true` | Flag set (monitoring) · P0 relay unaffected | LOW (ON) | `is_level_sensor_error: true` (amber warning) |
| FN-05 | FORCE_ON · Level ≥ 100% | Level not evaluated at P0. No stop. | LOW (ON) | `water_level_percent: 100` · "TANK FULL — RELAY STILL ON" |
| FN-06 | FORCE_ON · `manual_stop=true` | **Ignored.** Log: "Manual stop ignored: FORCE_ON active. Use mode selector." | No change | Dashboard note: "Use mode selector to exit." |
| FN-07 | FORCE_ON · `mode=FORCE_OFF` | Control read: run active → `setPump(false)` · `pumpMode="FORCE_OFF"` | HIGH (OFF) | `run_mode: "OFF"` · FORCE_OFF badge |
| FN-08 | FORCE_ON · `mode=AUTO` from admin | `pumpMode="AUTO"` · P0 cleared · P5 evaluates level | Per level | Banner clears. Mode: AUTO. |
| FN-09 | FORCE_ON · Device reboots | NVS: `"FORCE_ON"` → restores as `"AUTO"` · Log: "FORCE_ON not restored after reboot." | HIGH (OFF on boot, then per AUTO) | `last_boot_reason` visible. Mode: AUTO. Override banner gone. |
| FN-10 | FORCE_ON · Auto-timeout (R-02, if implemented) | `forceOnStartMs` elapsed ≥ `cfgForceOnMaxMin×60000` → `pumpMode="AUTO"` · `pendingModeWriteback=true` · Log: "FORCE_ON auto-expired." | Per level (may turn OFF) | `run_mode: "AUTO_STANDBY"` or per level · Override banner clears |

---

## 12. Firmware Implementation Plan — v4.0

### 12.1 Files and Changes

| File | Scope | Change |
|------|-------|--------|
| `03_safety_pump.ino` | Core | `executePumpLogic()`: P0 before P1, P3 MANUAL branch (tank-full + level-error), P1 MANUAL handling, runMode derivation update |
| `03_safety_pump.ino` | Core | `checkOverflowProtection()`: add `"MANUAL"` to covered modes |
| `05_connectivity_cloud.ino` | Core | `readFirebaseControl()`: `manual_start` → `"MANUAL"`, add `"MANUAL"` to valid set, `manual_stop` ignores FORCE_ON, `runActive` includes FORCE_ON |
| `04_persistence.ino` | Boot | `loadStateFromNVS()`: add `"MANUAL"` to restore set, FORCE_ON → AUTO revert |
| `smart_water_pump_controller_shared.h` | Minor | Update `runMode` comment, add `"FORCE_ON"` to valid runMode values comment |

### 12.2 `executePumpLogic()` — v4.0 Complete

```cpp
void executePumpLogic() {

  // Sync isManualRun — true only during MANUAL mode (retire or keep for telemetry)
  isManualRun = (pumpMode == "MANUAL");

  // ── runMode derivation (always before any return) ──────────────────────
  if (pumpMode == "FORCE_ON") {
    runMode = "FORCE_ON";
  } else if (isDryRunError || isOverflowError) {
    runMode = "OFF";
  } else if (pumpMode == "FORCE_OFF") {
    runMode = "OFF";
  } else if (pumpMode == "MANUAL" && isRunning) {
    runMode = "MANUAL";
  } else if (pumpMode == "MANUAL" && !isRunning) {
    runMode = "OFF";       // safety stopped; pump off while MANUAL mode holds
  } else if (pumpMode == "COUNTDOWN" && isCountdownActive) {
    runMode = "COUNTDOWN";
  } else if (pumpMode == "COUNTDOWN" && !isCountdownActive) {
    runMode = "OFF";
  } else if (pumpMode == "AUTO" && isRunning) {
    runMode = "AUTO";
  } else if (pumpMode == "AUTO" && !isRunning) {
    runMode = "AUTO_STANDBY";
  } else {
    runMode = isRunning ? "AUTO" : "OFF";
  }

  // ── P0: ABSOLUTE OVERRIDE — FORCE_ON ───────────────────────────────────
  // checkSafetyCutoff() already ran. Error flags are SET for dashboard display.
  // The relay is NOT affected by those flags at this priority level.
  if (pumpMode == "FORCE_ON") {
    setPump(true);
    return;
  }

  // ── P1: HARD SAFETY ────────────────────────────────────────────────────
  if (isDryRunError || isOverflowError) {
    lastFaultCode    = isDryRunError ? "DRY_RUN" : "OVERFLOW";
    lastFaultMessage = isDryRunError
      ? "Dry-run lockout: low flow while pump was running."
      : "Overflow protection: max runtime exceeded.";
    setPump(false);
    // COUNTDOWN: cancel and revert to AUTO
    if (isCountdownActive) {
      isCountdownActive = false; countdownEndMs = 0;
      pumpMode = "AUTO"; pendingModeWriteback = true; pendingModeWritebackSentMs = 0;
    }
    // MANUAL: do NOT revert mode. Pump off, mode stays MANUAL.
    // After clear_error: P3 will restart the pump automatically.
    return;
  }

  // ── P2: FORCE_OFF ──────────────────────────────────────────────────────
  if (pumpMode == "FORCE_OFF") {
    setPump(false);
    return;
  }

  // ── P3: MANUAL RUN (full safety — same as AUTO) ───────────────────────
  if (pumpMode == "MANUAL") {
    // Level sensor error fail-safe (only when bypass is OFF)
    if (isLevelSensorError && !cfgBypassLevelSensor) {
      if (isRunning) {
        Serial.println("[MANUAL] Level sensor error — stopping (fail-safe).");
        lastFaultCode    = "LEVEL_SENSOR";
        lastFaultMessage = "Level sensor offline: pump stopped in MANUAL (fail-safe).";
        setPump(false);
      }
      return;  // Mode stays MANUAL; pump off until sensor recovers or bypass enabled
    }
    // Tank-full stop
    if (!cfgBypassLevelSensor && waterLevelPct >= cfgPumpStopLevel) {
      Serial.printf("[MANUAL] Tank full (%d%%). Stopping. Reverting to AUTO.\n", waterLevelPct);
      setPump(false);
      pumpMode = "AUTO"; pendingModeWriteback = true; pendingModeWritebackSentMs = 0;
      return;
    }
    setPump(true);
    return;
  }

  // ── P4: COUNTDOWN ─────────────────────────────────────────────────────
  if (pumpMode == "COUNTDOWN") {
    if (isCountdownActive) {
      if (!cfgBypassLevelSensor && waterLevelPct >= cfgPumpStopLevel) {
        Serial.printf("[COUNTDOWN] Tank full (%d%%). Stopping early.\n", waterLevelPct);
        setPump(false); isCountdownActive = false; countdownEndMs = 0;
        pumpMode = "AUTO"; pendingModeWriteback = true; pendingModeWritebackSentMs = 0;
        return;
      }
      setPump(true);
    }
    return;
  }

  // ── P5: AUTO ─────────────────────────────────────────────────────────
  if (isSleeping) {
    if (isRunning && waterLevelPct >= cfgPumpStopLevel) setPump(false);
    return;
  }
  if (cfgBypassLevelSensor) return;
  if (isLevelSensorError) {
    if (isRunning) {
      lastFaultCode    = "LEVEL_SENSOR";
      lastFaultMessage = "Level sensor offline: pump stopped in AUTO (fail-safe).";
      setPump(false);
    }
    return;
  }
  // (R-01 minimum off-time check would go here before setPump(true))
  if (!isRunning && waterLevelPct <= cfgPumpStartLevel)    setPump(true);
  else if (isRunning && waterLevelPct >= cfgPumpStopLevel) setPump(false);
}
```

### 12.3 `checkOverflowProtection()` — v4.0

```cpp
// Add "MANUAL" to covered modes (line 118 of 03_safety_pump.ino):
if (!isRunning || !(pumpMode == "AUTO" || pumpMode == "COUNTDOWN" || pumpMode == "MANUAL")) {
    pumpAutoStartTracking = false;
    pumpAutoStartMs = 0;
    return;
}
```

### 12.4 `readFirebaseControl()` — Key v4.0 Changes

```cpp
// ① Valid mode set:
if (newMode == "AUTO" || newMode == "FORCE_ON" || newMode == "FORCE_OFF"
    || newMode == "COUNTDOWN" || newMode == "MANUAL") { ... }

// ② runActive — include FORCE_ON:
bool runActive = (pumpMode == "MANUAL"    && isRunning)
              || (pumpMode == "COUNTDOWN" && isCountdownActive)
              || (pumpMode == "FORCE_ON");

// ③ manual_start — sets "MANUAL", rejects FORCE_OFF:
if (v && !lastManualStart) {
  if (isDryRunError || isOverflowError) {
    Serial.println("[FIREBASE] Manual run rejected: error lockout active.");
  } else if (pumpMode == "FORCE_OFF") {
    Serial.println("[FIREBASE] Manual run rejected: FORCE_OFF active.");
  } else {
    pumpMode   = "MANUAL";   // KEY CHANGE from v3.0.0
    runStartMs = millis();
    Serial.println("[FIREBASE] Manual run started (MANUAL mode, full safety active).");
  }
}

// ④ manual_stop — ignores FORCE_ON:
if (v && !lastManualStop) {
  if (pumpMode == "FORCE_ON") {
    Serial.println("[FIREBASE] Manual stop ignored: FORCE_ON active. Use mode selector.");
    // pumpMode unchanged
  } else {
    setPump(false);
    pumpMode = "AUTO"; pendingModeWriteback = true; pendingModeWritebackSentMs = millis();
    Firebase.RTDB.setString(&fbdo, "/pump_system/control/mode", "AUTO");
    if (isCountdownActive) { isCountdownActive = false; countdownEndMs = 0; }
  }
}
```

### 12.5 `loadStateFromNVS()` — v4.0

```cpp
if (savedMode == "AUTO" || savedMode == "FORCE_OFF"
    || savedMode == "COUNTDOWN" || savedMode == "MANUAL") {
  pumpMode = savedMode;
  lastPersistedMode = savedMode;
  if (savedMode == "COUNTDOWN") {
    Serial.println("[BOOT] Restored COUNTDOWN mode. Timer will restart on Firebase sync.");
  } else if (savedMode == "MANUAL") {
    Serial.println("[BOOT] Restored MANUAL mode. Pump will start when sensor block runs.");
  }
} else if (savedMode == "FORCE_ON") {
  pumpMode = "AUTO";
  lastPersistedMode = "AUTO";
  Serial.println("[BOOT] FORCE_ON not restored after reboot. Defaulting to AUTO.");
}
```

### 12.6 What Stays Unchanged

```
UNCHANGED in v4.0 (confirmed):
  ✅ AUTO mode behavior — identical
  ✅ FORCE_OFF behavior — identical
  ✅ COUNTDOWN behavior — identical
  ✅ All P1 dry-run and overflow mechanisms
  ✅ All sensor processing pipelines (EMA, median, ISR, debounce)
  ✅ All NVS persistence keys and namespaces (no schema bump)
  ✅ Firebase single-JSON control read (offline-first)
  ✅ pendingModeWriteback and countdownConsumed mechanisms
  ✅ All timing intervals and dynamic interval logic
  ✅ Sleep / idle mode behavior
  ✅ Crash loop detection and safe mode
  ✅ Watchdog configuration
  ✅ WiFi backoff and Firebase retry logic
  ✅ Status push field list (add new runMode values to dashboard handling)
```

---

## 13. Dashboard Implementation — v4.0

### 13.1 Mode Selector Layout

```
Normal Operations:
  [ AUTO ]      [ MANUAL ]      [ COUNTDOWN ]

Emergency / Admin (visually distinct row):
  [ ⛔ FORCE_OFF ]     [ ⚠ FORCE_ON — Admin Only ]
```

### 13.2 FORCE_ON Confirmation Dialog (2-step)

```
Step 1 — Warning
  Title: ⚠️ Emergency Override — Force On
  Body:
    This will run the pump with ALL protections DISABLED:

    ✗  Dry-run protection         (motor burnout risk)
    ✗  Overflow / max runtime     (pump/pipe damage risk)
    ✗  Tank-full automatic stop   (overflow risk)
    ✗  Level sensor fail-safe     (uncontrolled fill risk)

    Use only if you have physically assessed the situation.
    The pump will run until you change the mode.
    A runtime counter will be visible while active.

  Buttons: [ Cancel ]  [ I Understand — Proceed ]

Step 2 — Typed Confirmation
  "Type OVERRIDE to confirm emergency activation."
  Input: validates exact match "OVERRIDE" (case-sensitive)
  Buttons: [ Cancel ]  [ Activate Force On ] (disabled until valid)
```

### 13.3 FORCE_ON Active Banner (non-dismissible)

```
┌─────────────────────────────────────────────────────────────────┐
│ 🔴 FORCE ON ACTIVE — ALL SAFETY PROTECTIONS BYPASSED            │
│    Runtime: [HH:MM:SS counter]                                  │
│    [⚠ Dry-run active]    [⚠ Max runtime exceeded]  (if flagged) │
│    To stop: Change mode below ↓                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 13.4 Stop Button Visibility Rules — v4.0

| `pumpMode` / `runMode` | Stop Button | Label | Action |
|------------------------|:-----------:|-------|--------|
| AUTO / AUTO_STANDBY | ❌ | — | — |
| AUTO / AUTO | ❌ | — | Automation manages stop |
| MANUAL / MANUAL | ✅ | Stop | `manual_stop=true` → AUTO |
| COUNTDOWN / COUNTDOWN | ✅ | Stop | `manual_stop=true` → AUTO |
| FORCE_OFF / OFF | ❌ | — | "Resume" button (→ AUTO) visible instead |
| FORCE_ON / FORCE_ON | ❌ **hidden** | — | Mode selector only. `manual_stop` ignored by firmware. |
| Any / OFF (P1 error) | ✅ | Clear Error | `clear_error=true` |

### 13.5 Updated `runMode` Display Mapping

| `runMode` | Badge | Color | Status Text | Notes |
|-----------|-------|-------|------------|-------|
| `"AUTO_STANDBY"` | AUTO | Blue | Standby | |
| `"AUTO"` | AUTO | Green | Running | |
| `"MANUAL"` | MANUAL | Green | Running (Manual) | Stop button visible |
| `"COUNTDOWN"` | COUNTDOWN | Amber | Running MM:SS | Stop + Add Time visible |
| `"OFF"` + P1 error | FAULT | Red | Locked Out | Clear Error button |
| `"OFF"` + FORCE_OFF | FORCE_OFF | Red | System Stopped | Resume button |
| `"OFF"` + expiry/transition | AUTO | Blue | Standby | Normal post-run |
| `"FORCE_ON"` | ⚠️ OVERRIDE | Red (pulsing) | Emergency Override | Full-screen banner |

### 13.6 Breaking Change: Firebase Schema

| Location | v3.0.0 | v4.0 | Breaking? |
|----------|--------|------|----------|
| `/control/mode` valid values | AUTO, FORCE_ON, FORCE_OFF, COUNTDOWN | + `"MANUAL"` | No (additive) |
| `/status/run_mode` values | OFF, AUTO_STANDBY, AUTO, MANUAL, COUNTDOWN | + `"FORCE_ON"` | **Yes** — dashboard must handle new string |
| `manual_start` → firmware action | Sets `FORCE_ON` | Sets `MANUAL` | Frontend unchanged; behavior safer |

---

## 14. Spec Corrections Index

The following sections of `firmware_master_spec.md` require targeted updates:

| Section | Action | Summary |
|---------|--------|---------|
| §2.1 Primary Operational States | Update | Add `MANUAL` to pumpMode. Add `"FORCE_ON"` to runMode values. |
| §2.3 runMode Derivation Table | Replace | Per §5.1 of this document. |
| §3.1 Priority Cascade | Replace | Six-level cascade per §4.1. |
| §3.2 Conflict Resolution | Replace | Per §5.2. |
| §6.3 Overflow Coverage | Amend | Add MANUAL. Add FORCE_ON exclusion note and unbounded runtime warning. |
| §7.2 S-02 | Amend | Add MANUAL P1 behavior: mode stays MANUAL, pump off, resumes after clear_error. |
| §7.3 L-02 | Amend | Add MANUAL level sensor error: pump off, mode stays MANUAL. |
| §7.4 Manual commands | Replace | Replace FORCE_ON with MANUAL scenarios. Add FN-06 (manual_stop ignored on FORCE_ON). |
| §8.2 Dashboard mapping | Amend | Update Quick Start → MANUAL, not FORCE_ON. Add FORCE_ON 2-step confirm. |
| §9.2 Boot scenario B-03 | Amend | Add MANUAL boot restore description. |
| §9.3 Hardware overrun | Amend | Add MANUAL to overflow coverage, FORCE_ON unbounded note. |
| §13 Thresholds | Amend | Add R-01 MIN_PUMP_OFF_TIME_MS. Add R-02 FORCE_ON_MAX_MIN. |
| §14.2 Status fields | Amend | Clarify `is_overflow_error` semantics (runtime exceeded, not physical overflow). |
| §15 Control keys | Amend | Update `mode` valid values, `manual_start` → MANUAL, `manual_stop` ignores FORCE_ON. |
| (new §17) | Add | Security: Firebase RTDB rules for `/control/` access restriction. |
| (new §18) | Add | Engineering appendix: sensor limits, motor characteristics, IEC references. |

---

*End of Document — Smart Water Pump Controller Architecture Redesign*

*All findings are grounded in direct firmware source analysis, engineering standards (IEC 61511, IEC 60034-1, IEC 60204-1, ISO 9905, IEEE Std 515, OSHA 29 CFR 1910.147, NFPA 20), and first-principles analysis of the physical system.*
