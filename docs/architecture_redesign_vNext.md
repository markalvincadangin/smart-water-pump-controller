# Smart Water Pump Controller — Architecture Redesign (vNext)
## Mode Reduction, Safety Re-Architecture, and Human-Factors-Driven Control Model (Distributed RS-485 System)

> **Document Type:** Architecture Redesign · Safety & Reliability Requirements · HMI/UX Behavior Contract  
> **System Architecture:** ESP32 Master + ESP8266 Sensor Node (RS-485)  
> **Firmware Baseline:** Current distributed RS-485 firmware (CRC/SEQ framing, strict parsing, stale/stable gating)  
> **Supersedes (conceptual):** FORCE_ON / FORCE_OFF based mode architecture in `docs/archive/architecture_redesign_v4.md`  
> **Author:** Mark Alvin Cadangin (updated with engineering consolidation)

---

## Table of Contents

1. [Design Goal and Executive Verdict](#1-design-goal-and-executive-verdict)  
2. [System Reality Check (Current Implementation Baseline)](#2-system-reality-check-current-implementation-baseline)  
3. [Mode Set vNext (FORCE modes removed)](#3-mode-set-vnext-force-modes-removed)  
4. [Revised Priority Hierarchy (Safety First, Always)](#4-revised-priority-hierarchy-safety-first-always)  
5. [State Model (Policy vs Intent vs Operational State)](#5-state-model-policy-vs-intent-vs-operational-state)  
6. [RS-485 Link Contract (Determinism, Integrity, Freshness)](#6-rs-485-link-contract-determinism-integrity-freshness)  
7. [Sensor Engineering Rules (Flow + Ultrasonic)](#7-sensor-engineering-rules-flow--ultrasonic)  
8. [Emergency Stop (Action, not Mode)](#8-emergency-stop-action-not-mode)  
9. [Dashboard/HMI Architecture (Human Factors and Safety UX)](#9-dashboardhmi-architecture-human-factors-and-safety-ux)  
10. [Worst-Case Scenario Analysis (Safety Cases)](#10-worst-case-scenario-analysis-safety-cases)  
11. [Complete Scenario Matrix (vNext)](#11-complete-scenario-matrix-vnext)  
12. [Firmware Implementation Requirements (vNext)](#12-firmware-implementation-requirements-vnext)  
13. [Dashboard Implementation Requirements (vNext)](#13-dashboard-implementation-requirements-vnext)  
14. [RTDB Contract (Fields, Semantics, Idempotency)](#14-rtdb-contract-fields-semantics-idempotency)  
15. [Verification & Validation Plan (Bench + Field)](#15-verification--validation-plan-bench--field)  
16. [Spec Corrections Index (from v4 assumptions)](#16-spec-corrections-index-from-v4-assumptions)

---

## 1. Design Goal and Executive Verdict

### 1.1 Primary Goal

Reduce operator confusion and unsafe overrides by removing:

- **FORCE_ON**
- **FORCE_OFF**

…and replacing them with a smaller, clearer, more deterministic control set:

- **AUTO** (default autonomous)
- **MANUAL** (operator policy) with explicit **MANUAL ON / MANUAL OFF**
- **COUNTDOWN** (semi-automatic timed run)
- **Emergency Stop** as a **contextual action** (only when running), not a persistent mode

### 1.2 Executive Verdict

This mode reduction is recommended because it:

- Minimizes **mode confusion** and “hidden state” failures
- Removes high-risk override semantics that are easy to misuse
- Keeps safety invariant: **hard safety always enforced** (dry-run, overflow, stale sensor data, comm instability)
- Aligns with distributed sensing reality (RS-485 sensor node)

---

## 2. System Reality Check (Current Implementation Baseline)

This redesign assumes the following are already true in firmware:

### 2.1 Distributed Control

- **ESP32 master** controls relay/pump, runs safety, persistence, cloud sync.
- **ESP8266 sensor node** reads:
  - Ultrasonic level (JSN-SR04T)
  - Flow pulses (YF-G1)
- Master obtains sensor data via **RS-485**.

### 2.2 RS-485 Frame Integrity

The RS-485 link must be treated as a noisy industrial channel. Baseline assumptions:

- Frames are integrity-protected (CRC16) and delimited (STX/ETX).
- Master uses strict parsing (reject partial/corrupt frames).
- Master maintains:
  - **freshness** (last update timestamp)
  - **stability latch** (N consecutive good frames to become “stable online”)

### 2.3 Safety Gates

The controller must never run pump autonomously on stale or unstable level data.

---

## 3. Mode Set vNext (FORCE modes removed)

### 3.1 Allowed policy modes (`pumpMode`)

| `pumpMode` | Purpose | Operator Mental Model |
|---|---|---|
| `"AUTO"` | Default autonomous fill | “The system decides when to run” |
| `"MANUAL"` | Operator-controlled run intent ON/OFF | “I decide whether it should run now” |
| `"COUNTDOWN"` | Operator sets a timed run | “Run for N minutes safely” |

### 3.2 Removed modes (deprecated)

| Removed | Why removed |
|---|---|
| `"FORCE_ON"` | Too easy to misuse; bypass semantics conflict with safety-critical expectations; creates operator overconfidence and hardware damage risk |
| `"FORCE_OFF"` | Persistent lockout is frequently mistaken as a normal stop and later misdiagnosed as “system broken,” leading to unsafe bypassing/re-wiring |

---

## 4. Revised Priority Hierarchy (Safety First, Always)

The system shall evaluate conditions in the following order (highest priority first):

### P1 — Hard Safety Lockouts (always enforced)

- Dry-run lockout (sustained low flow while pump running)
- Overflow runtime lockout (maximum continuous runtime)
- Emergency stop latch (see §8)

**Hard requirement:** No policy mode may override P1.

### P2 — Sensor Validity + Communication Integrity

Pump operation that depends on level must be gated by:

- Level freshness (data age ≤ `LEVEL_STALE_TIMEOUT_MS`)
- RS-485 stability latch (stable online)
- Validity flags (level sensor error when bypass is OFF)

If not satisfied:

- Pump must be stopped (failsafe)
- Starts must be blocked (failsafe)

### P3 — Operator Stop Intent

In MANUAL:

- `manual_desired = false` means pump OFF (subject to P1 already having stopped it anyway)

In COUNTDOWN:

- countdown expired means pump OFF and policy returns to AUTO (or MANUAL_OFF if chosen)

### P4 — Operator Run Intent

- MANUAL ON (`manual_desired = true`) requests pump run (still gated by P1/P2)
- COUNTDOWN active requests pump run (still gated by P1/P2)

### P5 — AUTO Policy

- Standard hysteresis control
- Sleep window suppression of new starts (optional)

---

## 5. State Model (Policy vs Intent vs Operational State)

### 5.1 Why split policy vs intent?

Human operators interpret “MANUAL mode” as “pump is running” even when it’s not.
This is a classic HMI failure: **policy state** is not the same as **actuator state**.

To reduce human error, the system uses:

- **Policy**: what rules should apply (`pumpMode`)
- **Intent**: what the operator wants right now (`manual_desired`, countdown parameters)
- **Operational state**: what the pump is doing (`runMode`, `isRunning`)

### 5.2 Required state variables

| Variable | Type | Meaning |
|---|---|---|
| `pumpMode` | enum/string | `"AUTO"`, `"MANUAL"`, `"COUNTDOWN"` |
| `manual_desired` | bool | operator intent in MANUAL |
| `countdown_active` | bool | timer running |
| `countdown_end_ms` | uint32 | timer end time |
| `isRunning` | bool | actual relay state |
| `runMode` | enum/string | `"OFF"`, `"AUTO"`, `"AUTO_STANDBY"`, `"MANUAL_ON"`, `"MANUAL_OFF"`, `"COUNTDOWN"`, `"STOPPED"` |

### 5.3 runMode derivation (contract)

`runMode` must be derived deterministically from the full state each cycle.

Suggested mapping:

| Condition | runMode |
|---|---|
| emergency stop latched | `"STOPPED"` |
| hard lockout (dry-run/overflow) | `"OFF"` |
| pumpMode=AUTO & pump off | `"AUTO_STANDBY"` |
| pumpMode=AUTO & pump on | `"AUTO"` |
| pumpMode=MANUAL & manual_desired=false | `"MANUAL_OFF"` |
| pumpMode=MANUAL & manual_desired=true & running | `"MANUAL_ON"` |
| pumpMode=COUNTDOWN & countdown_active | `"COUNTDOWN"` |
| default | `"OFF"` |

---

## 6. RS-485 Link Contract (Determinism, Integrity, Freshness)

### 6.1 Frame format (required)

RS-485 response frames must be self-delimiting and integrity-protected:

```text
<STX>LVL:xx;FLOW:yy.yy;ERR:z;SEQ:n;CRC:hhhh<ETX>
```

- `STX` = 0x02, `ETX` = 0x03
- `CRC` = CRC16(Modbus) computed over the payload up to and including the `;` after `SEQ`
- `SEQ` = monotonically incrementing 8-bit or 16-bit sequence number on the node

### 6.2 Master acceptance rules (strict)

The master must reject frames if:

- CRC mismatch
- Missing required keys
- Out-of-range values (LVL not 0–100, FLOW negative/out-of-sane-range, ERR invalid)

### 6.3 Freshness + stability latch (required)

Master must maintain:

- `levelLastUpdateMs` updated only when a frame passes strict checks
- `isLevelFresh = (now - levelLastUpdateMs) <= LEVEL_STALE_TIMEOUT_MS`
- `remoteSensorStable` asserted only after N consecutive valid frames and deasserted after N consecutive failures

### 6.4 Safety linkage

AUTO, MANUAL_ON, and COUNTDOWN must not run unless:

- `remoteSensorStable == true`
- `isLevelFresh == true` (unless a specifically defined “no-level mode” exists—which this redesign explicitly forbids)

---

## 7. Sensor Engineering Rules (Flow + Ultrasonic)

### 7.1 Flow sensor (YF-G1)

Requirements:

- ISR shall implement **minimum pulse interval** filter (deglitch/EMI control)
- Flow rate is computed from pulses/sec using calibration factor (site bucket-calibration recommended)

Error heuristics:

- Detect gross noise / floating input via discarded pulses (high rate of rejected pulses)
- (Optional) If pump-state is available to node in future, implement “no pulses while pump running” as flow error.

ERR bit mapping:

- bit0: ultrasonic error
- bit1: flow error

### 7.2 Ultrasonic sensor (JSN-SR04T)

Requirements:

- Measurement must be non-blocking
- Multi-sample median (noise reduction)
- Plausibility filter: reject sudden large jumps beyond physically plausible delta

---

## 8. Emergency Stop (Action, not Mode)

### 8.1 Definition

Emergency Stop is a **contextual action**:

- Visible/enabled only when `isRunning == true`
- Immediately stops relay and latches a state requiring deliberate restart

### 8.2 Latch semantics

Recommended semantics:

- `emergency_stop_latched = true` after E-Stop
- Clearing requires an explicit operator action (“Reset Stop”) plus safety conditions satisfied

This avoids the “I pressed stop and it restarted by itself” trust failure.

---

## 9. Dashboard/HMI Architecture (Human Factors and Safety UX)

### 9.1 Control surface (reduced modes)

Primary controls (always visible):

- Mode selector: AUTO / MANUAL / COUNTDOWN
- MANUAL: explicit ON / OFF intent toggle
- COUNTDOWN: duration + start/stop

Contextual control:

- **Emergency Stop** button shown only when running

### 9.2 Psychological design objectives

The dashboard must:

- Reduce **cognitive load** (fewer modes, fewer hidden latches)
- Minimize **mode confusion** by clearly separating:
  - Policy mode
  - Operator intent
  - Actual pump state
- Always explain blocked actions with a clear, actionable reason (e.g., “Cannot start: stale level data”)

### 9.3 Alerts alignment

Critical alerts should include:

- Dry-run lockout
- Overflow lockout
- Controller offline / stale level (no fresh data)
- Emergency stop latched

---

## 10. Worst-Case Scenario Analysis (Safety Cases)

This section defines required system outcomes for realistic failures.

### 10.1 RS-485 unplugged while pump running

Required behavior:

- Within `LEVEL_STALE_TIMEOUT_MS`, controller must stop pump and latch a fault reason.
- Must not restart until stable+fresh link returns (and operator intent allows).

### 10.2 Sensor node brownout/reboot loop

Required behavior:

- Master must not oscillate pump starts.
- Must require stable latch before allowing AUTO start or MANUAL_ON run.

### 10.3 Ultrasonic false low spike

Required behavior:

- Node plausibility filter should reject sudden jumps.
- Master should also treat implausible transitions as degraded validity if present (optional belt-and-suspenders).

---

## 11. Complete Scenario Matrix (vNext)

Legend:

- ✅ allowed/expected
- ❌ blocked/stopped (failsafe)
- ⚠️ allowed but bounded/guarded

| Scenario | AUTO | MANUAL_ON | MANUAL_OFF | COUNTDOWN | Emergency Stop |
|---|---:|---:|---:|---:|---:|
| Normal operation, stable+fresh | ✅ | ✅ | ✅ (pump off) | ✅ | ✅ (stops) |
| Stale level (no fresh RS-485) | ❌ stop/block | ❌ stop/block | ✅ (off) | ❌ stop/block | ✅ |
| RS-485 unstable (not stable latch) | ❌ stop/block | ❌ stop/block | ✅ (off) | ❌ stop/block | ✅ |
| Dry-run lockout active | ❌ stop | ❌ stop | ✅ (off) | ❌ stop | ✅ |
| Overflow lockout active | ❌ stop | ❌ stop | ✅ (off) | ❌ stop | ✅ |
| Tank full detected (valid) | ✅ stop | ✅ stop | ✅ | ✅ stop (early) | ✅ |
| Emergency stop latched | ❌ stop | ❌ stop | ✅ (off) | ❌ stop | (already latched) |

---

## 12. Firmware Implementation Requirements (vNext)

### 12.1 Control inputs (RTDB)

- `/pump_system/control/mode` ∈ `"AUTO"`, `"MANUAL"`, `"COUNTDOWN"`
- `/pump_system/control/manual_desired` boolean
- `/pump_system/control/countdown_duration_min` integer (clamped)
- `/pump_system/control/countdown_start` one-shot boolean (or reuse mode transition, but must be explicit)
- `/pump_system/control/emergency_stop` one-shot boolean (only meaningful if running)
- `/pump_system/control/reset_stop` one-shot boolean (clears emergency stop latch if safe)

### 12.2 Backward compatibility (recommended transition)

If old dashboards write `"FORCE_ON"` / `"FORCE_OFF"`:

- Firmware should map them to `"AUTO"` and log an event (do not honor them).

### 12.3 Safety invariants (must)

- No mode overrides dry-run or overflow lockouts
- No mode allows starting with stale/unstable level data (unless a dedicated “no-level mode” exists, which is explicitly prohibited here)

---

## 13. Dashboard Implementation Requirements (vNext)

### 13.1 UI changes

- Remove FORCE_ON/FORCE_OFF controls entirely
- Add MANUAL intent toggle (ON/OFF)
- Add Emergency Stop button visible only while running
- Add explicit “Reset Stop” when latched

### 13.2 UX copy requirements

Every blocked action must show:

- What is blocked
- Why (stale level, unstable RS-485, dry-run lockout, overflow, emergency stop)
- What to do next (wait for sensor, reset stop, clear lockout after inspection)

---

## 14. RTDB Contract (Fields, Semantics, Idempotency)

### 14.1 Status fields (minimum)

- `water_level_percent`
- `flow_rate_lpm`
- `is_running`
- `run_mode`
- `is_error` (or split: dry-run/overflow)
- `is_overflow_error`
- `is_level_sensor_error`
- `is_flow_sensor_error`
- `level_last_valid_age_sec`
- `wifi_rssi`, `uptime_minutes`
- `emergency_stop_latched` (new)

### 14.2 Idempotency and one-shots

One-shot booleans must be written back to false by firmware after consumption.

---

## 15. Verification & Validation Plan (Bench + Field)

### 15.1 Bench tests (must pass)

- RS-485 unplug: pump stops within stale timeout
- Inject CRC errors: master rejects frames (no unsafe starts)
- Emergency stop while running: pump stops immediately; restart requires reset
- Sensor node reboot loop: no oscillatory starts; stability latch works

### 15.2 Field tests

- Brownout at tank enclosure: verify recovery and safe behavior
- Noise tests on RS-485 cable length used in install

---

## 16. Spec Corrections Index (from v4 assumptions)

| v4 assumption | vNext correction |
|---|---|
| FORCE_ON/P0 bypasses safety | Removed entirely; hard safety always enforced |
| FORCE_OFF persistent emergency mode | Replaced by contextual Emergency Stop latch |
| Single-board direct sensors | Distributed RS-485 sensor node is baseline reality |

