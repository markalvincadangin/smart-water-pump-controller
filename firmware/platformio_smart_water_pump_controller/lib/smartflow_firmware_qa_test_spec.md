# SmartFlow — Firmware Software Quality Assurance Test Specification
### Software Test Specification v1.0

**System under test:** SmartFlow Firmware
**Nodes covered:** ESP32 Master Controller · NodeMCU V2 Sensor Node
**Classification:** Safety-related embedded software — 220V AC motor control
**Test methodology:** Black-box behavioral testing · White-box structural testing ·
Fault injection · State machine transition coverage · Boundary value analysis ·
Equivalence partitioning · Decision table testing

**Standards basis:**
- IEEE 829-2008: Software Test Documentation
- IEEE 1028-2008: Software Reviews and Audits
- IEC 61508-3:2010: Software requirements for safety-related systems
- IEC 61511-1:2016: Functional safety — Safety Instrumented Systems
- DO-178C (adapted): Software Considerations in Airborne Systems (state machine coverage)
- ISTQB Foundation Level v4.0: Test design techniques
- ISO/IEC 25010:2011: Systems and software quality models

---

> **Scope of this document**
> This specification covers firmware software behavior exclusively. Hardware is assumed
> correct and calibrated. All tests are designed to be executable by connecting the ESP32
> and NodeMCU to a Serial Monitor and Firebase console — no oscilloscope or hardware
> instruments required unless explicitly stated. Every test procedure is step-by-step
> reproducible. Every expected result is precisely stated with measurable pass criteria.
>
> This is NOT a repeat of the hardware integration tests. Every test here isolates
> firmware logic by treating the hardware as a transparent medium.

---

## Table of Contents

1. [QA Framework & Standards](#1-qa-framework--standards)
2. [Test Environment Setup](#2-test-environment-setup)
3. [Module 1: Boot & Initialization](#3-module-1-boot--initialization)
4. [Module 2: NVS Persistence & Configuration](#4-module-2-nvs-persistence--configuration)
5. [Module 3: Mode State Machine](#5-module-3-mode-state-machine)
6. [Module 4: AUTO Mode Logic](#6-module-4-auto-mode-logic)
7. [Module 5: MANUAL Mode Logic](#7-module-5-manual-mode-logic)
8. [Module 6: COUNTDOWN Mode Logic](#8-module-6-countdown-mode-logic)
9. [Module 7: Safety Functions](#9-module-7-safety-functions)
10. [Module 8: Sensor Node Firmware (NodeMCU)](#10-module-8-sensor-node-firmware-nodemcu)
11. [Module 9: RS-485 Protocol Engine](#11-module-9-rs-485-protocol-engine)
12. [Module 10: Firebase Communication](#12-module-10-firebase-communication)
13. [Module 11: Debug & Log System](#13-module-11-debug--log-system)
14. [Module 12: Fault Injection & Recovery](#14-module-12-fault-injection--recovery)
15. [Module 13: Boundary Value & Equivalence Tests](#15-module-13-boundary-value--equivalence-tests)
16. [Module 14: State Transition Coverage Matrix](#16-module-14-state-transition-coverage-matrix)
17. [Module 15: Long-Duration Behavioral Tests](#17-module-15-long-duration-behavioral-tests)
18. [Regression Checklist](#18-regression-checklist)
19. [Test Traceability Matrix](#19-test-traceability-matrix)

---

## 1. QA Framework & Standards

### 1.1 Testing Philosophy

This test specification applies four complementary techniques from ISTQB:

**Equivalence Partitioning (EP):** Input domains are divided into classes where all values in a class are expected to produce the same behavior. One representative from each class is tested. Example: flow rates are partitioned into {< 1.0 LPM (dry-run zone)}, {1.0–60 LPM (normal zone)}, {> 60 LPM (sensor error zone)}.

**Boundary Value Analysis (BVA):** Values at and immediately around boundaries are tested because firmware logic using comparison operators (`<`, `<=`, `>`, `>=`) is most likely to contain off-by-one errors at boundaries. Example: for `pump_start_level = 20`, test at 19%, 20%, and 21%.

**State Transition Testing:** The firmware is a finite state machine. Every valid transition, every invalid transition attempt, and every state-specific behavior is explicitly tested. Coverage target: 100% of transitions, 100% of states.

**Decision Table Testing:** For complex conditions involving multiple inputs (mode + pump state + sensor state + error flags), decision tables enumerate all relevant combinations and verify the output (relay state, run_mode, Firebase fields) for each.

**Fault Injection:** Known failure modes (CRC errors, sensor dropout, WiFi loss, partial RS-485 frames) are deliberately induced and the firmware's response verified against specification.

### 1.2 Pass/Fail Criteria Standard

Every test has:
- **Precondition:** System state required before the test begins
- **Stimulus:** What is injected or changed
- **Observable:** Where to observe the result (Serial Monitor, Firebase field)
- **Expected result:** Precisely stated, measurable criterion
- **Pass criteria:** The measurable threshold for PASS
- **Fail criteria:** What constitutes a FAIL and the likely defect category

### 1.3 Test Priority Levels

| Priority | Meaning |
|----------|---------|
| **P1 — CRITICAL** | Safety-critical. System must not be deployed if this fails. |
| **P2 — HIGH** | Functional correctness. Significant operational impact if failing. |
| **P3 — MEDIUM** | Quality/UX impact. Should be fixed; can deploy with documented waiver. |
| **P4 — LOW** | Minor; cosmetic or edge case. |

### 1.4 Defect Categories

| Category | Description |
|----------|-------------|
| SAFETY | Could cause pump to run when it should not, or fail to stop |
| FUNCTIONAL | Incorrect behavior in normal operation |
| PROTOCOL | RS-485 framing, CRC, or timing violation |
| DATA | Firebase field missing, wrong type, wrong value |
| RELIABILITY | System degrades or crashes over time |
| SECURITY | Unauthorized control or data exposure |

---

## 2. Test Environment Setup

### 2.1 Required Configuration for All Firmware Tests

Flash the ESP32 with **production firmware** but with **debug log level set to DEBUG** (either `LOG_COMPILE_FLOOR = LOG_DEBUG` in build, or set `debug_log_level = 3` via Firebase config after first boot).

NodeMCU: Flash with `DEBUG_USB_MODE = 1` (bench mode) for tests that require reading NodeMCU Serial output directly. For integration tests, use `DEBUG_USB_MODE = 0` with USB-TTL adapter on GPIO2.

**Serial Monitor setup:**
- ESP32: Arduino IDE Serial Monitor → COM port for ESP32 → 115200 baud → No line ending
- NodeMCU: Separate Serial Monitor instance OR USB-TTL adapter → 115200 baud

**Firebase observation method:**
Use one of:
- Firebase Realtime Database console (web) — manual polling
- A simple HTML page with `onValue()` listener printing to console
- Chrome DevTools WebSocket inspector showing Firebase frames

### 2.2 Firmware Test States

| State abbreviation | Meaning |
|---|---|
| `[AUTO-IDLE]` | Mode=AUTO, pump stopped, level within normal range |
| `[AUTO-RUN]` | Mode=AUTO, pump running |
| `[AUTO-COOL]` | Mode=AUTO, pump stopped, off-timer active |
| `[MANUAL-OFF]` | Mode=MANUAL, manual_desired=false |
| `[MANUAL-ON]` | Mode=MANUAL, manual_desired=true |
| `[MANUAL-COOL]` | Mode=MANUAL, pump stopped, off-timer active |
| `[COUNT-RUN]` | Mode=COUNTDOWN, pump running |
| `[STOPPED]` | Emergency stop latched |
| `[ERROR]` | is_error=true (DRY_RUN, OVERFLOW, or LEVEL_SENSOR) |
| `[BLANK-NVS]` | NVS erased, first boot |
| `[SENSOR-OFFLINE]` | CAT6 disconnected, RS-485 no response |

### 2.3 Test Data Configuration

Unless a test specifies otherwise, use these configuration values (write via Firebase config):

```
tank_empty_cm:          200    (sensor-to-surface when tank is empty)
tank_full_cm:           10     (sensor-to-surface when tank is full)
pump_start_level:       20     (%)
pump_stop_level:        90     (%)
dry_run_threshold_lpm:  1.0    (LPM)
dry_run_timeout_sec:    30     (s)
max_pump_runtime_min:   120    (min)
flow_calibration_factor: 7.5   (pulses/L)
sensor_failure_threshold: 5    (consecutive failures before error flag)
```

### 2.4 Simulating Sensor Values

Since hardware is assumed good, sensor values are simulated by adjusting the physical setup:

| Desired condition | Method |
|---|---|
| Level = X% | Fill/drain tank to physical level matching calibration |
| Flow = 0 LPM | Close valve or disconnect flow sensor signal wire |
| Flow = N LPM | Regulate valve; verify with Firebase `flow_rate_lpm` |
| Sensor offline | Unplug CAT6 at NodeMCU end |
| CRC error | Use test sketch to inject corrupted frame |
| Sensor error (ultrasonic) | Disconnect JSN-SR04T TRIG wire |
| Sensor error (flow) | Ground flow signal wire to simulate stuck-high |

---

## 3. Module 1: Boot & Initialization

> **Covers:** `setup()` function, NVS load on boot, initial Firebase push, initial run_mode,
> crash loop detection, watchdog configuration.
> **QA technique:** State-based testing, first-time execution testing.

---

### FW-BOOT-001 [P1] — Initial run_mode is AUTO_STANDBY, Not OFF

**Requirement:** Bug M-05 fix. `runMode` must initialize to `"AUTO_STANDBY"`.
**QA technique:** First-time execution test.

**Precondition:** `[BLANK-NVS]` or any cold boot.
**Stimulus:** Power on ESP32.
**Observable:** Firebase `/pump_system/status/run_mode` — first value after boot.

**Procedure:**
1. Erase flash completely (Arduino IDE: Tools → Erase Flash → All Flash Contents).
2. Flash production firmware.
3. Open Firebase console. Watch `/pump_system/status` in real time.
4. Power on ESP32. Record first value of `run_mode` that appears in Firebase.

**Expected result:** `run_mode = "AUTO_STANDBY"`.

**Pass criteria:** First `run_mode` value is `"AUTO_STANDBY"` (exact string).
**Fail criteria:** Value is `"OFF"`, `""`, `null`, or absent. Defect category: FUNCTIONAL.

---

### FW-BOOT-002 [P1] — water_level_percent Absent Before First Valid RS-485 Frame

**Requirement:** Bug C-02 fix. Do not push `water_level_percent: 0` before sensor data is valid.
**QA technique:** First-time execution test, negative test.

**Precondition:** `[BLANK-NVS]`, CAT6 DISCONNECTED (force sensor unavailability).
**Stimulus:** Power on ESP32 with no sensor node reachable.
**Observable:** Firebase `/pump_system/status` — first 60 seconds of data.

**Procedure:**
1. Disconnect CAT6 at NodeMCU end.
2. Erase and flash ESP32.
3. Monitor Firebase status in real time for 60 seconds.
4. Check every status push received: does `water_level_percent` appear?

**Expected result:** `water_level_percent` field is **absent** from all status pushes for the duration of the test. The field key must not be present at all — not zero, not null.

**Pass criteria:** Zero occurrences of `water_level_percent` in any Firebase push during the 60-second observation window.
**Fail criteria:** `water_level_percent` appears with any value (0, null, or otherwise). Defect category: SAFETY (could cause false AUTO start).

---

### FW-BOOT-003 [P2] — Default Config Loaded from Compile-Time Defaults on Blank NVS

**Requirement:** Firmware must load documented defaults when NVS has no config stored.
**QA technique:** Equivalence partitioning — first-boot partition.

**Precondition:** `[BLANK-NVS]`.
**Stimulus:** Power on. Let firmware boot fully (Firebase connected, uptime > 30s).
**Observable:** Firebase `/pump_system/config/device`.

**Procedure:**
1. Full flash erase + reflash.
2. Wait 35 seconds for full boot and Firebase sync.
3. Read all fields from `/pump_system/config/device`.
4. Compare against documented defaults.

**Expected result — each field must match:**

| Field | Expected default value |
|---|---|
| `pump_start_level` | 20 |
| `pump_stop_level` | 90 |
| `dry_run_threshold_lpm` | 1.0 |
| `dry_run_timeout_sec` | 30 |
| `max_pump_runtime_min` | 120 |
| `flow_calibration_factor` | 7.5 |
| `debug_log_level` | 2 (LOG_INFO) |

**Pass criteria:** All fields present and matching expected defaults exactly.
**Fail criteria:** Any field absent, wrong value, or wrong type. Defect category: FUNCTIONAL.

---

### FW-BOOT-004 [P1] — Boot Does Not Trigger Pump Start Before Sensor Data Valid

**Requirement:** On boot with TANK_LOW condition, pump must not start until first valid RS-485 frame arrives and `water_level_percent` is valid.
**QA technique:** Safety test, timing test.

**Precondition:** `[BLANK-NVS]`. Physical tank level is below `pump_start_level` (20%). CAT6 connected but NodeMCU takes 5s to boot.

**Stimulus:** Power on both nodes simultaneously.
**Observable:** Relay activation time (audible click) vs. Serial log timestamp of first valid RS-485 frame.

**Procedure:**
1. Set up conditions (tank low, both nodes off).
2. Power on simultaneously. Start stopwatch.
3. Serial Monitor: watch for "first valid RS-485 frame" log message.
4. Record timestamp T1 of first valid frame.
5. Record timestamp T2 of first relay activation (audible click or `is_running: true` in Firebase).

**Expected result:** T2 ≥ T1. Relay NEVER activates before first valid sensor frame.

**Pass criteria:** Relay does not activate until T1 has occurred. `is_running` does not appear as `true` before first valid RS-485 log line.
**Fail criteria:** Relay activates before first valid sensor frame. Defect category: SAFETY.

---

### FW-BOOT-005 [P2] — Uptime Counter Starts from Zero on Power-On

**Precondition:** Any cold boot.
**Observable:** Firebase `uptime_minutes`.
**Expected result:** `uptime_minutes` = 0 in first Firebase push after boot.

---

### FW-BOOT-006 [P2] — last_boot_reason Reflects Power-On Correctly

**Procedure:** Power off for 30s. Power on. Read `last_boot_reason` from first Firebase push.
**Expected result:** `last_boot_reason` = `"Power-on"` or equivalent power-on reset string.
**Note:** Subsequent tests for watchdog reset, software reset are in Module 12.

---

### FW-BOOT-007 [P2] — crash_loop_count Clears on First Successful Firebase Push

**Requirement:** Bug H-06 fix. Crash counter clears on first successful Firebase push, not at 60s.
**QA technique:** Success-based state transition test.

**Procedure:**
1. Verify `crash_loop_count` in NVS by inducing one deliberate restart (use `ESP.restart()` via a test trigger, or power cycle rapidly 2x).
2. Boot normally. Monitor Serial log.
3. Note timestamp of first successful Firebase push log message (LOG_INFO: "Firebase push OK" or equivalent).
4. Note timestamp of crash counter clear log message.

**Expected result:** Crash counter clears at the first successful Firebase push, not at a fixed 60s timeout. The two timestamps should be within 1 second of each other.

---

### FW-BOOT-008 [P2] — debug_log_level Pushed in First Firebase Status

**Precondition:** Any cold boot.
**Observable:** Firebase `/pump_system/status/debug_log_level`.
**Expected result:** `debug_log_level` present in first Firebase status push with value matching `gLogLevel` initial value (2 = LOG_INFO by default).

---

## 4. Module 2: NVS Persistence & Configuration

> **Covers:** `readDeviceConfigFromFirebase()`, `saveToNVS()`, `loadFromNVS()`.
> **QA technique:** State transition testing, equivalence partitioning.

---

### FW-NVS-001 [P2] — Config Changes Survive Power Cycle

**Requirement:** Settings written from dashboard persist across ESP32 power cycle.

**Procedure:**
1. Write `pump_start_level = 35` to Firebase config from dashboard.
2. Wait 35s for firmware to read config. Verify in Serial log: config re-read occurred.
3. Power cycle ESP32.
4. Wait 30s for boot.
5. Read `/pump_system/config/device/pump_start_level` from Firebase.

**Expected result:** `pump_start_level = 35` after power cycle.
**Pass criteria:** Value matches what was written before power cycle.
**Fail criteria:** Value reverted to 20 (compile-time default). Defect category: FUNCTIONAL.

---

### FW-NVS-002 [P2] — All Configurable Fields Persist (Bulk Test)

**Procedure:** Write non-default values for all fields listed in test setup §2.3. Power cycle. Verify all persist.

| Field | Test value |
|---|---|
| `pump_start_level` | 25 |
| `pump_stop_level` | 85 |
| `dry_run_threshold_lpm` | 1.5 |
| `dry_run_timeout_sec` | 45 |
| `max_pump_runtime_min` | 60 |
| `flow_calibration_factor` | 8.2 |
| `bypass_flow_sensor` | true |
| `bypass_level_sensor` | false |
| `debug_log_level` | 3 |

**Expected result:** All 9 fields retain non-default values after power cycle.

---

### FW-NVS-003 [P2] — Config Re-Read Every 30 Seconds

**Requirement:** Firmware reads `/pump_system/config/device` every 30 seconds (not every 3s).

**Procedure:**
1. With firmware running and debug logging enabled, monitor Serial for config read events.
2. Observe timestamps of consecutive config read log messages.
3. Calculate interval between them.

**Expected result:** Config read interval = 30s ±3s.
**Pass criteria:** Interval consistently 27–33s.
**Fail criteria:** Interval < 10s (too frequent — wastes Firebase bandwidth) or > 60s (too slow — settings take too long to apply). Defect category: FUNCTIONAL.

---

### FW-NVS-004 [P2] — Config Change Applied Within 30s of Dashboard Save

**Procedure:**
1. Monitor pump_start_level behavior in firmware (which level triggers pump start).
2. Start stopwatch. Change `pump_start_level = 30` in Firebase config.
3. Wait for next config re-read (max 30s).
4. Verify firmware now uses 30% as start level by observing behavior.

**Expected result:** New config value applied within 30s + 3s (poll tolerance) = 33s max.

---

### FW-NVS-005 [P3] — bypass_flow_sensor Persists to NVS and Survives Restart

**Procedure:**
1. Write `bypass_flow_sensor = true` via Firebase control.
2. Wait 6s for firmware to read control.
3. Power cycle ESP32.
4. Wait 30s.
5. Read `bypass_flow_sensor` from Firebase status.

**Expected result:** `bypass_flow_sensor = true` after power cycle (loaded from NVS).

---

## 5. Module 3: Mode State Machine

> **Covers:** State transitions between AUTO, MANUAL, COUNTDOWN, STOPPED.
> **QA technique:** State transition testing — all-transitions coverage.
> **Requirement:** IEC 61508-3 Annex A: State machine formal testing.

### 5.1 State Transition Diagram

```
                    ┌──────────────────────────────────────────────┐
                    │              STOPPED (E-stop latched)         │
                    │     Entry: relay OFF, emergency_stop_latched  │
                    │     Exit: reset_stop:true ONLY               │
                    └───────────────────┬──────────────────────────┘
                                        │ reset_stop: true
                     ┌──────────────────▼──────────────────────────┐
    mode:AUTO ──────►│                AUTO_STANDBY                  │
                     │     pump OFF, level ≥ start                  │◄── mode:AUTO from any state
    mode:MANUAL ────►│                MANUAL_OFF                    │
                     │     pump OFF, manual_desired: false          │◄── mode:MANUAL
    mode:COUNTDOWN ─►│                COUNTDOWN_WAIT                │◄── mode:COUNTDOWN
                     └──┬──────────────────────────────────────────┘
                        │
              ┌─────────┴──────────────────────────┐
              │ [Condition met]                     │ [manual_desired:true / countdown_start:true]
              ▼                                     ▼
    ┌─────────────────┐                   ┌─────────────────────┐
    │   AUTO RUNNING  │                   │  MANUAL/COUNTDOWN   │
    │   pump ON       │                   │  RUNNING — pump ON  │
    └────────┬────────┘                   └──────────┬──────────┘
             │ [level ≥ stop OR overflow]            │ [manual_desired:false / timer done]
             ▼                                       ▼
    ┌─────────────────┐                   ┌─────────────────────┐
    │  AUTO_COOLDOWN  │                   │ MANUAL/COUNTDOWN    │
    │  pump OFF,      │                   │  COOLDOWN           │
    │  off-timer      │                   │  pump OFF, timer    │
    └────────┬────────┘                   └──────────┬──────────┘
             │ [timer expires]                        │ [timer expires]
             └──────────────────┬────────────────────┘
                                │
              ┌─────────────────▼─────────────────┐
              │         Back to STANDBY            │
              └───────────────────────────────────┘

    emergency_stop:true ──► STOPPED (from ANY state)
    is_error:true (DRY_RUN etc) ──► pump OFF (mode does not change)
```

---

### FW-SM-001 [P1] — AUTO → MANUAL: Mode Change While Pump Running Stops Pump

**Precondition:** `[AUTO-RUN]` (pump running in AUTO).
**Stimulus:** Write `mode: "MANUAL"` to Firebase control.
**Observable:** Firebase `run_mode`, `is_running`, relay state.

**Procedure:**
1. With pump running in AUTO, write `mode: "MANUAL"` to `/pump_system/control/mode`.
2. Observe Firebase within 6s.

**Expected result:**
- `run_mode = "MANUAL_OFF"` (pump stops on mode change, manual_desired is initially false)
- `is_running = false`
- Relay de-activates (audible click)

**Pass criteria:** Within 6s of mode write, pump stops and run_mode = MANUAL_OFF.
**Fail criteria:** Pump continues running. Defect category: FUNCTIONAL.

---

### FW-SM-002 [P1] — MANUAL → AUTO: Pump Resumes AUTO Logic

**Precondition:** `[MANUAL-OFF]` (MANUAL mode, pump stopped). Tank level is below pump_start_level.
**Stimulus:** Write `mode: "AUTO"`.
**Observable:** Firebase `run_mode`, `is_running`.

**Expected result:** Within 6s, pump starts (level is below start level), `run_mode = "AUTO"`, `is_running = true`.
**Pass criteria:** Pump starts within 6s.

---

### FW-SM-003 [P1] — Any State → STOPPED on emergency_stop: true

**Test from each starting state:**

| Starting state | Precondition |
|---|---|
| AUTO running | `[AUTO-RUN]` |
| AUTO standby | `[AUTO-IDLE]` |
| MANUAL ON | `[MANUAL-ON]` |
| MANUAL OFF | `[MANUAL-OFF]` |
| COUNTDOWN running | `[COUNT-RUN]` |

**For each starting state:**
1. Write `emergency_stop: true` to `/pump_system/control/emergency_stop`.
2. Observe Firebase within 6s.

**Expected result for each:**
- `run_mode = "STOPPED"`
- `emergency_stop_latched = true`
- `is_running = false`
- `emergency_stop` field reset to `false` (one-shot behavior)

**Pass criteria:** All 5 states produce STOPPED within 6s.
**Fail criteria:** Any state fails to reach STOPPED or relay stays active. Defect category: SAFETY.

---

### FW-SM-004 [P1] — STOPPED Cannot Be Exited by Mode Change

**Precondition:** `[STOPPED]` (emergency stop latched).
**Stimulus (attempt each, verify no state change):**
1. Write `mode: "AUTO"` → pump must not start.
2. Write `mode: "MANUAL"` + `manual_desired: true` → pump must not start.
3. Write `clear_error: true` → latch must not clear (clear_error is for DRY_RUN/OVERFLOW, not E-stop).
4. Write `mode: "COUNTDOWN"` + `countdown_start: true` → pump must not start.

**Expected result:** All four attempts leave system in STOPPED state, pump remains off.
**Pass criteria:** `run_mode = "STOPPED"` and `is_running = false` after all four attempts.
**Fail criteria:** Any attempt causes pump to start. Defect category: SAFETY.

---

### FW-SM-005 [P1] — STOPPED Exits Only via reset_stop: true

**Precondition:** `[STOPPED]`.
**Stimulus:** Write `reset_stop: true` to Firebase control.
**Observable:** Firebase `run_mode`, `emergency_stop_latched`, `is_running`.

**Expected result:**
- `emergency_stop_latched = false`
- `reset_stop` field reset to `false` (one-shot)
- System returns to appropriate mode (AUTO_STANDBY, MANUAL_OFF, etc. based on current mode)
- `run_mode` matches the active mode (not STOPPED)

**Pass criteria:** Within 6s of reset_stop write, `emergency_stop_latched = false`.
**Fail criteria:** System remains in STOPPED after reset_stop. Defect category: FUNCTIONAL.

---

### FW-SM-006 [P2] — reset_stop Blocked When Error Is Active (DRY_RUN Lockout)

**Requirement:** If DRY_RUN error is active alongside E-stop, reset_stop alone must not unlock the pump. clear_error must also be sent.

**Precondition:** Both `emergency_stop_latched = true` AND `is_error = true` (DRY_RUN).

**Procedure:**
1. Trigger DRY_RUN fault. Then trigger E-stop. Both are now active.
2. Write `reset_stop: true`.
3. Verify: E-stop latch clears BUT pump does not start (DRY_RUN still active).
4. Write `clear_error: true`.
5. Verify: pump now able to restart.

**Expected result:** Both conditions must be cleared independently before pump can restart.

---

### FW-SM-007 [P2] — Mode Change Persists Across Firebase Disconnect/Reconnect

**Precondition:** Firmware running in `[MANUAL-OFF]`.
**Procedure:**
1. Disable WiFi on router (Firebase disconnect).
2. Wait 30s.
3. Re-enable WiFi.
4. Wait for Firebase reconnect (observe in Serial log).
5. Read `run_mode` from Firebase.

**Expected result:** `run_mode = "MANUAL_OFF"` — mode persisted in firmware, correctly pushed after reconnect.
**Pass criteria:** Mode matches pre-disconnect state.

---

### FW-SM-008 [P2] — Simultaneous Mode Write + Emergency Stop

**Precondition:** `[AUTO-RUN]`.
**Stimulus:** Write `mode: "MANUAL"` and `emergency_stop: true` in rapid succession (within same second).
**Observable:** Final state.

**Expected result:** STOPPED takes priority. `run_mode = "STOPPED"`, pump off.
**Rationale:** Safety function (E-stop) must take priority over mode change. If order of reads matters, document the actual behavior and verify it is safe.

---

## 6. Module 4: AUTO Mode Logic

> **Covers:** Level-based pump start/stop logic, cooldown timer, level freshness gate.
> **QA technique:** Decision table testing, boundary value analysis.

### 6.1 AUTO Mode Decision Table

| Level | Pump state | off-timer | Expected action |
|---|---|---|---|
| < start (20%) | OFF | Not active | Start pump → run_mode: AUTO |
| < start (20%) | OFF | Active | Stay off → run_mode: AUTO_COOLDOWN |
| ≥ start, ≤ stop | ON | N/A | Keep running → run_mode: AUTO |
| ≥ stop (90%) | ON | N/A | Stop pump → run_mode: AUTO_COOLDOWN |
| ≥ stop (90%) | OFF | Not active | Stay off → run_mode: AUTO_STANDBY |
| Level unknown (-1) | OFF | N/A | Stay off (no valid data) |
| Level stale (> staleness threshold) | ON | N/A | Stop pump, log STALE_LEVEL |

---

### FW-AUTO-001 [P1] — Pump Starts When Level Drops Below Start Level

**Precondition:** `[AUTO-IDLE]`, level initially at 50%.
**Stimulus:** Tank drains below 20% (pump_start_level).
**Observable:** Firebase `is_running`, `run_mode`.

**Expected result:** Within 3s of level reading below 20%, `is_running = true`, `run_mode = "AUTO"`.

---

### FW-AUTO-002 [P1] — Pump Stops When Level Reaches Stop Level

**Precondition:** `[AUTO-RUN]`, level rising.
**Stimulus:** Level reaches 90% (pump_stop_level).
**Observable:** Firebase `is_running`, `run_mode`.

**Expected result:** Within 3s of level reading ≥ 90%, `is_running = false`, `run_mode = "AUTO_COOLDOWN"`.

---

### FW-AUTO-003 [P1] — Pump Does NOT Start When Level Is Already Above Stop Level

**Precondition:** Level at 95% (above stop), MODE = AUTO.
**Stimulus:** Cold boot.
**Observable:** `is_running` for 60s after boot.

**Expected result:** `is_running = false` throughout 60s. No spurious start.
**Pass criteria:** Zero occurrences of `is_running = true` during observation window.

---

### FW-AUTO-004 [P1] — Cooldown Timer Prevents Immediate Restart

**Precondition:** `[AUTO-RUN]`, pump just stopped (level hit stop threshold).
**Stimulus:** Immediately drain tank below start level (20%) while cooldown is active.
**Observable:** `run_mode`, `is_running`, `pump_cooldown_remaining_sec`.

**Procedure:**
1. Let pump stop at stop level. Observe `run_mode = "AUTO_COOLDOWN"`.
2. Immediately drain tank below 20% (physically drain or simulate via sensor).
3. Monitor for 60s.

**Expected result:**
- During cooldown: `is_running = false`, `run_mode = "AUTO_COOLDOWN"`, `pump_cooldown_remaining_sec > 0`.
- After cooldown expires: pump starts, `run_mode = "AUTO"`.

**Pass criteria:** Pump stays off for full cooldown duration even with level below start.
**Fail criteria:** Pump starts before cooldown expires. Defect category: FUNCTIONAL.

---

### FW-AUTO-005 [P2] — pump_cooldown_remaining_sec Decrements Correctly

**Precondition:** `[AUTO-COOL]` — cooldown active.
**Observable:** Firebase `pump_cooldown_remaining_sec` over 30 seconds.

**Expected result:** Field decrements by approximately 1 per second. Should not jump or stall.
**Measurement:** Record values at t=0, t=10, t=20, t=30. Difference per 10s should be ~10.

---

### FW-AUTO-006 [P1] — Level Data Freshness Gate: Stale Level Stops Pump

**Requirement:** When level data age exceeds the staleness threshold (verify in firmware, expected ~2500ms), pump must stop and log STALE_LEVEL.

**Precondition:** `[AUTO-RUN]`.
**Stimulus:** Disconnect CAT6 mid-run. Level data stops updating.
**Observable:** Firebase `is_running`, `level_fresh`, `last_fault_code`.

**Procedure:**
1. Pump running in AUTO. Disconnect CAT6.
2. Observe Firebase after staleness threshold elapses.

**Expected result:**
- `level_fresh = false` within 3s of last valid frame.
- `is_running = false` within REMOTE_SENSOR_OFFLINE_MS (verify this constant in firmware).
- `last_fault_code = "COMM_LOSS"` or `"STALE_LEVEL"` (confirm which code is used).

**Pass criteria:** Pump stops when level goes stale. Level_fresh transitions correctly.
**Fail criteria:** Pump continues running on stale data. Defect category: SAFETY.

---

### FW-AUTO-007 [P2] — Level = Exactly pump_start_level: Boundary Behavior

**Requirement:** BVA at start boundary. Clarify: does pump start at `level ≤ start` or `level < start`?

**Procedure:**
1. Set `pump_start_level = 40`. Set physical level to exactly 40%.
2. In AUTO mode with pump stopped: observe if pump starts.
3. Set level to 39%. Observe if pump starts (should start at < or ≤ 40).
4. Set level to 41%. Observe (should NOT start).

**Expected result:** Document exact boundary behavior. Verify it matches the firmware condition operator (`<` vs `<=`) and is consistent.
**Fail criteria:** Inconsistent behavior at boundary, or behavior does not match documented operator. Defect category: FUNCTIONAL.

---

### FW-AUTO-008 [P2] — Level = Exactly pump_stop_level: Boundary Behavior

**Same as FW-AUTO-007 but for pump_stop_level.**

**Procedure:**
1. Set `pump_stop_level = 70`. Pump running.
2. Set level to exactly 70%. Observe if pump stops.
3. Set level to 69%. Observe (should NOT stop if stop condition is `level >= stop`).
4. Set level to 71%. Observe (should stop).

**Document exact boundary behavior.**

---

### FW-AUTO-009 [P2] — Idle Mode: Activates When Tank Full and Pump Off

**Precondition:** Level at 100%, pump off, no error.
**Observable:** Firebase `is_idle_mode`, poll frequency.

**Expected result:** After idle activation timeout, `is_idle_mode = true`. Firebase updates slow to `idle_firebase_interval_ms`.

---

### FW-AUTO-010 [P2] — Idle Mode: Deactivates When Level Drops or Pump Starts

**Precondition:** `[AUTO-IDLE]` in idle mode, `is_idle_mode = true`.
**Stimulus:** Level drops below idle threshold (or pump starts).
**Observable:** `is_idle_mode`.

**Expected result:** `is_idle_mode = false` within one sensor poll cycle after condition changes.

---

### FW-AUTO-011 [P2] — total_pump_cycles Increments on Each Complete Run

**Procedure:**
1. Note current `total_pump_cycles` value.
2. Run 3 complete AUTO pump cycles (pump starts, runs, stops).
3. Read `total_pump_cycles`.

**Expected result:** `total_pump_cycles` increased by exactly 3.

---

## 7. Module 5: MANUAL Mode Logic

> **Covers:** manual_desired flag, tank level independence, safety functions still active.
> **QA technique:** Decision table, negative testing.

### 7.1 MANUAL Mode Decision Table

| manual_desired | is_error | emergency_stop_latched | Expected relay state |
|---|---|---|---|
| true | false | false | ON (pump running) |
| false | false | false | OFF (pump stopped) |
| true | true (DRY_RUN) | false | OFF (error latch wins) |
| true | false | true | OFF (E-stop wins) |
| true | true | true | OFF (both wins) |
| false | true | false | OFF |
| false | false | true | OFF |

---

### FW-MAN-001 [P1] — MANUAL ON: Pump Starts on manual_desired: true

**Precondition:** `[MANUAL-OFF]`.
**Stimulus:** Write `manual_desired: true`.
**Observable:** `is_running`, `run_mode`, relay.
**Expected result:** Within 6s: `is_running = true`, `run_mode = "MANUAL_ON"`.

---

### FW-MAN-002 [P1] — MANUAL OFF: Pump Stops on manual_desired: false

**Precondition:** `[MANUAL-ON]`.
**Stimulus:** Write `manual_desired: false`.
**Expected result:** Within 6s: `is_running = false`, `run_mode = "MANUAL_OFF"`.

---

### FW-MAN-003 [P1] — MANUAL Mode: Tank Level Has No Effect on Pump

**Test A — Tank full does not stop pump:**
1. `[MANUAL-ON]`. Fill tank to > 90% (stop level).
2. Confirm: pump continues running, `is_running = true`.
3. Duration: 30s. Monitor continuously.
**Expected result:** No pump stop from level.

**Test B — Tank empty does not start pump:**
1. `[MANUAL-OFF]`, `manual_desired = false`. Drain tank below 20% (start level).
2. Confirm: pump stays off, `is_running = false`.
3. Duration: 30s. Monitor continuously.
**Expected result:** No auto-start from level.

**Pass criteria for both:** No level-triggered pump action in MANUAL mode.
**Fail criteria:** Pump starts or stops due to level in MANUAL mode. Defect category: FUNCTIONAL.

---

### FW-MAN-004 [P1] — MANUAL Mode: DRY_RUN Protection Still Active

**Precondition:** `[MANUAL-ON]`, FLOW_BLOCKED (valve closed, 0 LPM).
**Observable:** `is_error`, `last_fault_code`, `is_running` at t=0, t=15, t=30, t=35.

**Expected result:**
- t=0 to t<30s: `is_error = false`, `is_running = true`.
- t=30s ±6s: `is_error = true`, `last_fault_code = "DRY_RUN"`, `is_running = false`.

**Pass criteria:** DRY_RUN fires within 30 ±6s. Pump stops.
**Fail criteria:** DRY_RUN does not fire, or fires before 24s (too early). Defect category: SAFETY.

---

### FW-MAN-005 [P1] — MANUAL Mode: Overflow Protection Issues Warning, Does NOT Stop Pump

**Requirement:** Bug H-05 fix. In MANUAL, overflow protection is informational only.

**Setup:** Set `max_pump_runtime_min = 2` (for test speed). `[MANUAL-ON]`.
**Observable:** `manual_runtime_warning`, `is_running`, `is_overflow_error` at t=0, t=1m, t=2m10s.

**Expected result at t=2m10s:**
- `manual_runtime_warning = true` ← warning flag set
- `is_overflow_error = false` ← overflow error NOT set
- `is_running = true` ← pump STILL RUNNING

**Pass criteria:** Warning flag set, pump still running, no overflow error.
**Fail criteria:** Pump stopped by overflow in MANUAL mode. Defect category: SAFETY (H-05 regression).

---

### FW-MAN-006 [P2] — MANUAL Cooldown: Pump Does Not Immediately Restart After Stop

**Precondition:** `[MANUAL-ON]`. Write `manual_desired: false`.
**Stimulus:** Immediately write `manual_desired: true` again (within 1s of stop).
**Observable:** `run_mode`, `is_running`.

**Expected result:** If off-timer is active: `run_mode = "MANUAL_COOLDOWN"`, pump stays off until timer expires.

**Note:** If firmware does not implement MANUAL cooldown identically to AUTO cooldown, document the actual behavior.

---

### FW-MAN-007 [P2] — manual_runtime_warning Clears When Pump Stops

**Precondition:** `manual_runtime_warning = true` (from FW-MAN-005).
**Stimulus:** Write `manual_desired: false`. Pump stops.
**Observable:** `manual_runtime_warning`.

**Expected result:** `manual_runtime_warning` becomes `false` when pump stops (non-latching).
**Pass criteria:** Flag clears on pump stop.

---

## 8. Module 6: COUNTDOWN Mode Logic

> **Covers:** Countdown timer start, remaining time display, +time extension, auto-revert to AUTO.
> **QA technique:** State machine testing, timing tests.

---

### FW-COUNT-001 [P2] — Countdown Starts Pump and Begins Timer

**Precondition:** `[AUTO-IDLE]`. Switch to COUNTDOWN mode.
**Stimulus:** Write `countdown_start: true`, `countdown_duration_min: 3`.
**Observable:** `run_mode`, `is_running`, `countdown_remaining_sec`.

**Expected result within 6s:**
- `is_running = true`
- `run_mode = "COUNTDOWN"`
- `countdown_remaining_sec` ≈ 180 (3 minutes × 60s)

---

### FW-COUNT-002 [P2] — countdown_remaining_sec Decrements Over Time

**Precondition:** `[COUNT-RUN]`, `countdown_remaining_sec` = 180.
**Observable:** `countdown_remaining_sec` at t=0, t=30s, t=60s.

**Expected result:**
- t=30s: `countdown_remaining_sec` ≈ 150 (180 - 30)
- t=60s: `countdown_remaining_sec` ≈ 120 (180 - 60)

**Pass criteria:** Decrement rate within ±3s per 30s window.

---

### FW-COUNT-003 [P2] — Countdown Stops Pump and Reverts to AUTO at Timer End

**Precondition:** `[COUNT-RUN]`, timer set to 2 minutes (for test speed). Set `max_pump_runtime_min = 5` to avoid overflow interference.
**Observable:** `run_mode`, `is_running` at t=2m10s.

**Expected result:** `is_running = false`, `run_mode = "AUTO_STANDBY"` (or AUTO_COOLDOWN then STANDBY).
**Pass criteria:** Pump stops and mode reverts to AUTO within 10s of timer expiry.

---

### FW-COUNT-004 [P2] — Add Time: Extends Active Countdown

**Precondition:** `[COUNT-RUN]`, `countdown_remaining_sec` = 60 (1 minute left).
**Stimulus:** Write `countdown_add_time: true`, `countdown_add_min: 5`.
**Observable:** `countdown_remaining_sec`.

**Expected result:** `countdown_remaining_sec` ≈ 360 (60 + 300) within 6s of write.

---

### FW-COUNT-005 [P2] — Mode Change Cancels Countdown

**Precondition:** `[COUNT-RUN]`, `countdown_remaining_sec` = 120.
**Stimulus:** Write `mode: "AUTO"`.
**Observable:** `run_mode`, `countdown_remaining_sec`.

**Expected result:** Countdown cancelled. `run_mode` transitions to AUTO (standby or running based on level).
**Pass criteria:** `countdown_remaining_sec = 0` after mode change.

---

### FW-COUNT-006 [P1] — DRY_RUN Fires in COUNTDOWN Mode

**Precondition:** `[COUNT-RUN]`, FLOW_BLOCKED.
**Expected result:** DRY_RUN fires within `dry_run_timeout_sec` + 6s tolerance. Pump stops. Countdown cancelled.
**Pass criteria:** Same as FW-MAN-004.

---

### FW-COUNT-007 [P1] — COUNTDOWN Overflow: Pump Stops and Error Set

**Setup:** Set `max_pump_runtime_min = 2`. Start countdown with 10 minutes (longer than max_runtime).
**Expected result at t=2m10s:** `is_overflow_error = true`, pump stops, mode reverts to AUTO_STANDBY after error clear.

---

## 9. Module 7: Safety Functions

> **Covers:** DRY_RUN lockout, OVERFLOW protection, crash loop detection, sensor failure gate.
> **QA technique:** Fault injection, decision table testing, timing verification.
> **Standard reference:** IEC 61508-3 §7.4.5: Safety function requirements.

---

### FW-SAF-001 [P1] — DRY_RUN: Activates at Exact Timeout Boundary

**Requirement:** Dry-run fault must trigger at exactly `dry_run_timeout_sec` after flow drops below `dry_run_threshold_lpm` while pump is running.

**Setup:** `dry_run_timeout_sec = 30`, `dry_run_threshold_lpm = 1.0`. Pump running with flow.

**Procedure:**
1. Note timestamp T0. Close valve (flow drops to 0 LPM).
2. Note timestamp T1 when `is_error = true` appears in Firebase.
3. Compute elapsed time: T1 - T0.

**Expected result:** T1 - T0 = 30s ±6s (one poll cycle tolerance each way).

**Pass criteria:** Elapsed time within 24–36s.
**Fail criteria:** Fires before 24s (too early, nuisance trip) or after 36s (too late, safety gap). Defect category: SAFETY.

---

### FW-SAF-002 [P1] — DRY_RUN: Timer Resets If Flow Recovers

**Requirement:** The dry-run timer is NOT cumulative. If flow recovers above threshold, timer resets to zero.

**Procedure:**
1. Block flow for 15s (half timeout).
2. Open valve. Flow recovers above 1.0 LPM for 5s.
3. Block flow again.
4. Observe: `is_error` should NOT fire at 15 + 5 + 10 = 30s total (timer reset on recovery).
5. Verify: `is_error` fires 30s after the second block, not 30s after the first block.

**Expected result:** Timer resets on recovery. Second block triggers fault at second block T+30s.
**Fail criteria:** `is_error` fires earlier than expected (timer was accumulative). Defect category: SAFETY.

---

### FW-SAF-003 [P1] — DRY_RUN: Relay Turns OFF Immediately on Fault

**Precondition:** Dry-run fault triggered (is_error becomes true).
**Observable:** Time between `is_error = true` in Firebase and relay de-activation.
**Expected result:** Relay de-activates within 1s of `is_error` flag set (same execution cycle).
**Pass criteria:** Relay off within 1 pump cycle (≤ 1s after error flag set).

---

### FW-SAF-004 [P1] — DRY_RUN: Cannot Be Cleared by Mode Change

**Precondition:** `[ERROR]` — DRY_RUN active, pump stopped.
**Stimulus (attempt each, verify no change):**
1. Write `mode: "AUTO"`.
2. Write `mode: "MANUAL"` + `manual_desired: true`.
3. Write `mode: "COUNTDOWN"` + `countdown_start: true`.

**Expected result:** `is_error = true` persists. Pump stays off. Mode may change but pump does not start.
**Fail criteria:** Any mode change clears the error or starts the pump. Defect category: SAFETY.

---

### FW-SAF-005 [P1] — DRY_RUN: Cleared Only by clear_error: true

**Precondition:** `[ERROR]`, DRY_RUN active. Flow restored above threshold.
**Stimulus:** Write `clear_error: true`.
**Observable:** `is_error`, `last_fault_code`.

**Expected result within 6s:**
- `is_error = false`
- `last_fault_code = ""` or cleared
- `clear_error` field reset to `false` (one-shot)
- Pump can now restart based on mode and conditions

**Pass criteria:** Error clears within 6s of clear_error write. Field resets.
**Fail criteria:** Error does not clear, or field not reset to false. Defect category: FUNCTIONAL.

---

### FW-SAF-006 [P1] — OVERFLOW: Fires at Exact Runtime Limit in AUTO Mode

**Setup:** `max_pump_runtime_min = 2`. Pump running in AUTO with level between start and stop (won't auto-stop on level). Block level updates if needed to prevent auto-stop during test.

**Procedure:**
1. Note timestamp T0 when pump starts.
2. Note timestamp T1 when `is_overflow_error = true`.
3. Compute T1 - T0.

**Expected result:** T1 - T0 = 120s ±10s.
**Pass criteria:** Overflow fires at 120s ±10s.

---

### FW-SAF-007 [P1] — OVERFLOW: Does NOT Fire in MANUAL Mode

**Requirement:** Bug H-05 fix verification under fault injection.

**Setup:** `max_pump_runtime_min = 2`. `[MANUAL-ON]`.
**Observable:** `is_overflow_error`, `is_running` at t=2m10s.

**Expected result:** `is_overflow_error = false` and `is_running = true` at t=2m10s.
**Pass criteria:** No overflow error in MANUAL after max_runtime exceeded.
**Fail criteria:** `is_overflow_error = true` or pump stops. Defect category: SAFETY (H-05 regression).

---

### FW-SAF-008 [P1] — OVERFLOW: Cleared by clear_error

**Precondition:** `is_overflow_error = true`, pump stopped.
**Stimulus:** Write `clear_error: true`.
**Expected result:** `is_overflow_error = false` within 6s.

---

### FW-SAF-009 [P1] — Sensor Failure Gate: Pump Stops on Consecutive RS-485 Failures

**Setup:** `sensor_failure_threshold = 5` (verify in firmware). Pump running in AUTO.
**Stimulus:** Disconnect CAT6. RS-485 frames fail with timeouts.
**Observable:** `remote_sensor_stable`, `is_sensor_error`, `is_running`.

**Expected result:**
- After 3 consecutive failures: `remote_sensor_stable = false`.
- After `sensor_failure_threshold` consecutive failures: `is_sensor_error = true`, pump stops.

**Pass criteria:** Pump stops after threshold is reached. `is_sensor_error = true`.
**Fail criteria:** Pump continues running despite sensor failure. Defect category: SAFETY.

---

### FW-SAF-010 [P1] — Sensor Failure: Auto-Clears on Recovery

**Precondition:** `is_sensor_error = true` (from FW-SAF-009).
**Stimulus:** Reconnect CAT6. RS-485 frames resume.
**Observable:** `is_sensor_error`, `remote_sensor_stable`.

**Expected result:** After 3 consecutive successful frames, `remote_sensor_stable = true`. `is_sensor_error = false`. System resumes normal operation.
**Note:** Check whether `is_sensor_error` auto-clears or requires `clear_error`. Document and verify behavior is consistent with spec.

---

### FW-SAF-011 [P2] — Crash Loop: Counter Increments and Triggers Safe Mode

**Procedure:**
1. Note current NVS crash count (read from Serial log on boot).
2. Force 4 rapid restarts (power cycle within 30s each time, before Firebase push succeeds).
3. On 5th boot: observe Serial log for SAFE_MODE detection.
4. Verify pump does not start automatically in safe mode.

**Expected result:** SAFE_MODE entered after threshold restarts. Pump inhibited.

---

### FW-SAF-012 [P2] — bypass_level_sensor: Level Protections Disabled, Flow Still Active

**Precondition:** `bypass_level_sensor = true`. Pump running in AUTO.
**Test:** Fill tank above stop level (90%). Pump should continue running.
**Expected result:** Pump continues. Level-based stop does not fire.
**Then:** Block flow (0 LPM). Dry-run should still fire.
**Expected result:** DRY_RUN fires within `dry_run_timeout_sec`. Flow protection remains.

---

### FW-SAF-013 [P2] — bypass_flow_sensor: Flow Protections Disabled, Level Still Active

**Precondition:** `bypass_flow_sensor = true`. Pump running in AUTO.
**Test:** Block flow completely (0 LPM) for `dry_run_timeout_sec + 10s`.
**Expected result:** DRY_RUN does NOT fire. `is_error = false`.
**Then:** Fill tank above stop level.
**Expected result:** Pump stops based on level (level protection still active).

---

### FW-SAF-014 [P2] — Sleep Mode: Pump Does Not Start During Sleep Window

**Setup:** Configure sleep window to cover current time (test is time-dependent; adjust accordingly). Set `sleep_enabled = true`, `sleep_start_hour = X`, `sleep_end_hour = X+1`.

**Procedure:**
1. Set sleep window to current hour.
2. Drain tank below start level.
3. Observe: `is_sleeping = true` in Firebase.
4. Confirm pump does NOT start during sleep window.

**Expected result:** Pump stays off during sleep window despite level below start.

---

### FW-SAF-015 [P2] — Sleep Mode: Emergency Level Override

**Setup:** Sleep active. Tank drops below `sleep_emergency_level` threshold.
**Expected result:** Pump starts despite sleep window (emergency override). `is_sleeping = true` but `is_running = true`.

---

## 10. Module 8: Sensor Node Firmware (NodeMCU)

> **Covers:** Ultrasonic measurement, flow counting, level plausibility filter, RS-485 response frame generation.
> **QA technique:** Equivalence partitioning, fault injection.

---

### FW-SN-001 [P2] — Level Calculation: Correct Percentage from Distance

**Requirement:** `LVL = clamp((tank_empty_cm - dist_cm) / (tank_empty_cm - tank_full_cm) × 100, 0, 100)`

**Procedure (verify in Serial output with `DEBUG_USB_MODE=1`):**

| Measured distance | tank_empty=200, tank_full=10 | Expected LVL% |
|---|---|---|
| 200 cm | (200-200)/(200-10)×100 | 0% |
| 105 cm | (200-105)/190×100 | 50% |
| 10 cm | (200-10)/190×100 | 100% |
| 5 cm (beyond full) | clamped | 100% (not >100%) |
| 210 cm (beyond empty) | clamped | 0% (not negative) |

**Pass criteria:** All 5 cases match expected. Clamping verified.

---

### FW-SN-002 [P2] — Level Plausibility Filter: Extreme Jump Rejected

**Requirement:** Bug H-02 fix. Readings that differ from last good reading by more than `LEVEL_MAX_DELTA_PCT` are discarded and `snLevelDiscardCount` incremented.

**Procedure (bench mode, Serial direct):**
1. Establish stable readings at 50% level.
2. Momentarily cover sensor completely (simulate 0 cm reading — extreme jump).
3. Observe Serial output: discard log message should appear.
4. Observe `snLevelDiscardCount` in next RS-485 frame (`LDSC` field).
5. Confirm `LVL` in frame remains at approximately 50% (last good value used).

**Pass criteria:** Discard logged, LDSC increments, LVL not updated to bad reading.
**Fail criteria:** Bad reading accepted, LDSC = 0, LVL jumps to extreme value. Defect category: FUNCTIONAL (H-02 regression).

---

### FW-SN-003 [P2] — Level Plausibility Filter: All-Rejected Window Sets snLevelError

**Procedure:**
1. Cover sensor for long enough that all readings in one window are rejected.
2. Observe `ERR` field bit 0 in RS-485 frame — should be set (snLevelError = true).
3. Remove obstruction. Observe bit 0 clears on recovery.

**Pass criteria:** ERR bit 0 set when all samples rejected; clears on recovery.

---

### FW-SN-004 [P2] — Flow Error Hysteresis: Assert After 3 Seconds

**Requirement:** Bug H-04 fix. snFlowError asserts after 3 consecutive seconds with `disc > 50`.

**Procedure (bench mode):**
1. Inject noise on flow sensor pin for exactly 2s.
2. Observe: `ERR` bit 1 should remain 0 (not yet triggered).
3. Continue noise for 1 more second (total 3s).
4. Observe: `ERR` bit 1 = 1 (snFlowError = true).

**Pass criteria:** Bit 1 asserts at exactly 3s of sustained noise, not before.

---

### FW-SN-005 [P2] — Flow Error Hysteresis: Clear After 5 Seconds

**Procedure:**
1. Assert flow error (3s of noise from FW-SN-004).
2. Remove noise (disc ≤ 20).
3. Observe at t=1, t=3, t=5 after noise removal:
   - t=1: ERR bit 1 = 1 (not yet cleared)
   - t=3: ERR bit 1 = 1 (still clearing)
   - t=5: ERR bit 1 = 0 (cleared)

**Pass criteria:** Flag remains set for 5s after noise removal, then clears.

---

### FW-SN-006 [P2] — Flow Discard Debug Print Uses Local Variable

**Requirement:** Bug H-03 fix. Debug print for `disc` reads local variable, not zeroed global.

**Procedure (bench mode, `DEBUG_USB_MODE=1`):**
1. Inject brief noise to generate non-zero `disc` value.
2. Observe Serial output: "Discarded: N" message.
3. Verify N > 0 when noise is active.

**Pass criteria:** Non-zero discard count shown in Serial.
**Fail criteria:** Always shows "Discarded: 0" despite active noise. Defect category: FUNCTIONAL (H-03 regression).

---

### FW-SN-007 [P2] — LDSC Field Present in RS-485 Response Frame

**Procedure:**
1. Capture RS-485 response frame (bench mode: read NodeMCU Serial directly, or use ESP32 Serial Monitor in debug mode).
2. Parse frame: verify `LDSC:N` field is present between `ERR:X;` and `SEQ:Y;`.

**Expected result:** Frame format: `LVL:82;DIST:45.2;FLOW:8.30;ERR:0;LDSC:0;SEQ:142;CRC:XXXX`.
**Pass criteria:** LDSC field present in every frame.

---

### FW-SN-008 [P2] — SEQ Field Increments Monotonically

**Procedure:**
1. Observe 10 consecutive RS-485 frames.
2. Extract SEQ value from each.
3. Verify: each SEQ = (previous SEQ + 1) % 256.

**Pass criteria:** Strict monotonic increment with correct 255→0 wraparound.

---

## 11. Module 9: RS-485 Protocol Engine

> **Covers:** CRC calculation and validation, frame parser, timeout/retry, direction control.
> **QA technique:** Protocol conformance testing, fault injection.

---

### FW-RS-001 [P1] — CRC16-Modbus: Matches Known Test Vector

**Procedure (bench mode, enable debug output):**
1. Capture a complete RS-485 frame from NodeMCU.
2. Extract the payload bytes (between STX and `CRC:` field).
3. Compute CRC16-Modbus independently (use Python: `crcmod.predefined.mkCrcFun('modbus')(payload.encode())`).
4. Compare computed CRC to CRC field in frame.

**Python verification:**
```python
import crcmod
crc_fun = crcmod.predefined.mkCrcFun('modbus')
payload = b"LVL:82;DIST:45.2;FLOW:8.30;ERR:0;LDSC:0;SEQ:142;"
print(hex(crc_fun(payload)))  # Must match frame's CRC field
```

**Pass criteria:** Computed CRC matches frame CRC for 10 consecutive frames.

---

### FW-RS-002 [P1] — CRC Failure: Frame Data Not Used

**Procedure:**
1. Capture a valid frame. Modify 1 byte in the payload (increment it by 1).
2. Do NOT update the CRC field (CRC now invalid).
3. Inject this corrupted frame via a test sketch that replaces normal NodeMCU response.
4. Observe: `water_level_percent` must NOT update to the corrupted LVL value.
5. `ultrasonic_cycles_timeout` or CRC error counter must increment.

**Pass criteria:** Level not updated from corrupted frame. Error counter increments.
**Fail criteria:** Level updated from corrupted frame. Defect category: PROTOCOL / SAFETY.

---

### FW-RS-003 [P1] — Retry: 3 Attempts Before Marking Sensor Offline

**Procedure:**
1. Silence NodeMCU (unplug power or run empty loop sketch that sends no response).
2. Monitor ESP32 Serial (debug mode) for retry log messages.
3. Count retry attempts before `remote_sensor_stable = false` in Firebase.

**Expected result:** Exactly 3 retry attempts logged. After 3rd timeout (3 × 250ms = 750ms), `remote_sensor_stable = false`.
**Pass criteria:** 3 retries confirmed in log. Timeout fires at 750ms total.

---

### FW-RS-004 [P1] — Timeout: 250ms Per Attempt

**Procedure:**
1. With NodeMCU silenced (FW-RS-003 conditions).
2. Timestamp each REQ sent and each timeout event in Serial log.
3. Compute interval from REQ to timeout.

**Expected result:** Each timeout ≈ 250ms ±20ms.
**Pass criteria:** All 3 timeouts within 230–270ms.

---

### FW-RS-005 [P2] — Partial Frame Stall Reset: rxPos Cleared After 20ms

**Requirement:** Bug M-03 fix.

**Procedure:**
1. Use a test sketch on a third Arduino to inject exactly 5 bytes of a valid frame, then stop transmitting.
2. Wait 25ms.
3. Send a complete valid frame.
4. Verify NodeMCU responds correctly to the complete valid frame.

**Pass criteria:** NodeMCU responds correctly after stall reset.
**Fail criteria:** NodeMCU ignores the valid frame (rxPos not reset). Defect category: PROTOCOL (M-03 regression).

---

### FW-RS-006 [P2] — LDSC Field Optional: Old-Format Frame Parsed Without Error

**Procedure:**
1. Create a test sketch that sends frames WITHOUT the LDSC field: `LVL:50;DIST:61.0;FLOW:5.00;ERR:0;SEQ:0;CRC:XXXX`.
2. Flash this to NodeMCU.
3. Observe ESP32 behavior: should NOT crash, should parse all other fields correctly.
4. `remote_level_discard_count` in Firebase should be 0 (default when field absent).

**Pass criteria:** ESP32 parses old-format frames without error. `remote_level_discard_count = 0`.
**Fail criteria:** Parse error, crash, or `is_sensor_error = true`. Defect category: PROTOCOL.

---

### FW-RS-007 [P2] — Direction Control: DE/RE Released Only After flush()

**Requirement:** `Serial2.flush()` must be called before `digitalWrite(RS485_DE_RE_PIN, LOW)` to prevent last byte truncation.

**Procedure (white-box):**
1. Read `02_rs485_comm.ino` (or equivalent file).
2. Locate `Serial2.print("REQ\n")` and the subsequent DE/RE toggle.
3. Verify `Serial2.flush()` is called BEFORE `digitalWrite(RS485_DE_RE_PIN, LOW)`.

**Pass criteria:** Code review confirms `flush()` before `LOW`. No logic path bypasses this.
**Fail criteria:** `flush()` absent or called after DE/RE toggle. Defect category: PROTOCOL.

---

### FW-RS-008 [P2] — Turnaround Guard: 80µs on ESP32, 60µs on NodeMCU

**Procedure (code review):**
1. In ESP32 RS-485 code: locate `delayMicroseconds(N)` after `flush()` before switching DE/RE to LOW. Verify N ≥ 80.
2. In NodeMCU RS-485 code: locate `delayMicroseconds(N)` before asserting DE/RE HIGH. Verify N ≥ 60.

**Pass criteria:** ESP32 guard ≥ 80µs. NodeMCU guard ≥ 60µs.

---

### FW-RS-009 [P2] — Frame Counters: ultrasonic_cycles_ok and _timeout Increment Correctly

**Procedure:**
1. Note `ultrasonic_cycles_ok` (A) and `ultrasonic_cycles_timeout` (B) at start.
2. Let system run for 30 poll cycles (90 seconds) with CAT6 connected and NodeMCU responding.
3. Read new values: A2, B2.
4. Compute: new_ok = A2 - A, new_timeout = B2 - B.

**Expected result:** `new_ok` ≈ 30 (±2). `new_timeout` ≈ 0 (good conditions).
**Ratio:** `new_ok / (new_ok + new_timeout) ≥ 0.97` (97% success in good conditions).

---

## 12. Module 10: Firebase Communication

> **Covers:** Status push fields, control read fields, config read fields, one-shot field reset, backoff.
> **QA technique:** Schema conformance testing, decision table.

---

### FW-FB-001 [P2] — All Status Fields Present in Every Push

**Procedure:**
1. Enable debug logging (gLogLevel = LOG_DEBUG).
2. Let system run 5 minutes.
3. Capture 5 complete Firebase status payloads.
4. Verify every field from the canonical schema is present in every payload.

**Required fields to check:**

```
water_level_percent (when valid)    is_running          flow_rate_lpm
run_mode                            pump_cooldown_remaining_sec
is_error                            is_sensor_error      is_flow_sensor_error
is_overflow_error                   is_idle_mode         is_sleeping
emergency_stop_latched              manual_desired       bypass_level_sensor
bypass_flow_sensor                  remote_sensor_stable level_fresh
manual_runtime_warning              countdown_remaining_sec
last_fault_code                     last_fault_message   level_sensor_health_pct
remote_level_discard_count          flow_volume_added_l  wifi_rssi
uptime_minutes                      last_boot_reason     debug_log_level
total_pump_cycles                   total_pump_run_min   ultrasonic_cycles_ok
ultrasonic_cycles_timeout           free_heap_bytes      min_free_heap_observed_bytes
firebase_consecutive_failures
```

**Pass criteria:** All fields present in 5/5 payloads with correct types.

---

### FW-FB-002 [P1] — One-Shot Fields Reset After Processing

**Test each one-shot field:**

| Field | Write value | Expected after 6s |
|---|---|---|
| `emergency_stop` | true | false |
| `reset_stop` | true | false |
| `clear_error` | true | false |
| `countdown_start` | true | false |
| `countdown_add_time` | true | false |

**For each:**
1. Write `true` to the field.
2. Wait 6s (2 poll cycles).
3. Read field value from Firebase.

**Pass criteria:** All fields reset to `false` after firmware processes them.
**Fail criteria:** Any field remains `true` after 6s. Defect category: FUNCTIONAL.

---

### FW-FB-003 [P2] — Status Push Interval: 3 Seconds ±0.5s

**Procedure:**
1. Use a Firebase onValue listener with timestamp logging.
2. Record 20 consecutive status updates.
3. Compute mean and standard deviation of intervals.

**Pass criteria:** Mean = 3.0s ±0.5s. No interval > 4.5s. No interval < 1.5s.

---

### FW-FB-004 [P2] — Firebase Write Error Backoff: Exponential with Cap

**Requirement:** On consecutive failures, backoff = min(1000ms × 2^n, 30000ms).

**Procedure:**
1. Block Firebase (disable WiFi or change Firebase URL temporarily).
2. Monitor Serial log for "Firebase write failed" messages.
3. Record timestamps of consecutive failure messages.
4. Compute intervals between them.

**Expected intervals:**
- 1st retry: ~1000ms
- 2nd retry: ~2000ms
- 3rd retry: ~4000ms
- 4th retry: ~8000ms
- 5th+: ~30000ms (capped)

**Pass criteria:** Interval pattern matches exponential formula (±20%).

---

### FW-FB-005 [P2] — debug_log_level Remote Control: Applied Within 30s

**Procedure:**
1. gLogLevel currently at 2 (INFO). No [D] messages in Serial.
2. Write `debug_log_level = 3` to Firebase config.
3. Wait 30s.
4. Observe Serial: [D] messages should appear.
5. Verify `status.debug_log_level = 3` in Firebase.

**Pass criteria:** [D] messages appear within 30s. Status field updated.

---

### FW-FB-006 [P2] — Firebase Status Continues During WiFi Reconnect (No Data Loss)

**Procedure:**
1. System running normally. Drop WiFi for 30s.
2. Restore WiFi.
3. Verify: `firebase_consecutive_failures` incremented during outage.
4. Verify: after reconnect, failures counter resets (or stops growing).
5. Verify: no missed status fields in first push after reconnect.

---

## 13. Module 11: Debug & Log System

> **Covers:** LOG() macro behavior, level filtering, format correctness, rate limiting.
> **QA technique:** Specification-based testing, equivalence partitioning.

---

### FW-LOG-001 [P3] — LOG_INFO Level: Boot and State Transition Messages Only

**Precondition:** `gLogLevel = 2` (LOG_INFO). Production build.

**Procedure:**
1. Boot system. Observe Serial Monitor for 5 minutes of normal AUTO operation.
2. Count message types: [I] = INFO, [W] = WARN, [E] = ERROR, [D] = DEBUG, [V] = VERBOSE.

**Expected result:** Only [I], [W], [E] messages appear. Zero [D] or [V] messages.
**Pass criteria:** Zero [D] or [V] in 5-minute log. Estimated total message count < 20 for 5 minutes of normal operation (80–90% reduction from verbose baseline).

---

### FW-LOG-002 [P3] — LOG_DEBUG Level: Per-Cycle Sensor Readings Appear

**Precondition:** `gLogLevel = 3` (LOG_DEBUG).

**Expected result:** [D] messages appear every ~3s showing sensor readings, RS-485 frames, etc.

---

### FW-LOG-003 [P3] — Log Format: [L][MODULE][MS] Matches Specification

**Procedure:**
1. Capture 20 log lines from Serial Monitor.
2. Verify format regex: `\[[EWIDV]\]\[[A-Z]{4,6}\]\[\d{10}\] .+`

**Examples of correct format:**
```
[E][PUMP][0045231] DRY_RUN lockout. flow=0.08LPM. Relay OFF.
[W][RS485][0046002] Frame timeout attempt 2/3.
[I][BOOT][0001240] NVS config loaded. mode=AUTO_STANDBY
[D][SENSOR][0047100] lvl=82% dist=45.2cm flow=8.30LPM
```

**Pass criteria:** 100% of log lines match the format. No bare `Serial.printf` or `Serial.println` output (except intentional boot banner).

---

### FW-LOG-004 [P3] — WARN Rate Limiting: Same Condition Logged Once Per 60s

**Procedure:**
1. Create a sustained warning condition (e.g., Firebase fails → LOG_WARN fires repeatedly).
2. Monitor Serial for 3 minutes.
3. Count occurrences of the same WARN message.

**Expected result:** Same WARN message appears at most once per 60s, not every poll cycle.
**Pass criteria:** ≤ 4 occurrences in 3 minutes (1 per 60s × 3 + 1 initial).

---

### FW-LOG-005 [P3] — NodeMCU #warning Directive Fires for DEBUG_USB_MODE=1

**Procedure:**
1. Compile NodeMCU firmware with `DEBUG_USB_MODE=1`.
2. Check compilation output.

**Expected result:** Arduino IDE / PlatformIO shows: `warning: DEBUG_USB_MODE=1: RS-485 DISABLED. Do not flash to deployed device.`
**Pass criteria:** Warning text appears during compilation.
**Fail criteria:** No warning. Developer may accidentally deploy debug build to production. Defect category: FUNCTIONAL.

---

## 14. Module 12: Fault Injection & Recovery

> **Covers:** System behavior under failure conditions. Hardware assumed good.
> **QA technique:** Fault injection, negative testing.

---

### FW-FI-001 [P1] — WiFi Loss During Pump Run: Local Logic Unaffected

**Precondition:** `[AUTO-RUN]`, pump running.
**Stimulus:** Disable router WiFi.
**Observable:** Relay state, pump physical operation, Serial log.

**Expected result:**
- Pump CONTINUES running based on local RS-485 sensor data.
- `firebase_consecutive_failures` increments in Serial log.
- Pump STOPS correctly when level reaches stop level (local logic, no Firebase needed).
- Pump DOES NOT stop due to WiFi loss alone.

**Pass criteria:** Pump runs and stops based on level even with WiFi down.
**Fail criteria:** Pump stops due to Firebase failure (should only stop for safety reasons). Defect category: RELIABILITY.

---

### FW-FI-002 [P1] — Firebase Auth Failure: System Continues Pump Operation

**Precondition:** System running normally.
**Stimulus:** Temporarily change Firebase email/password credentials in secrets.h and reflash (or simulate auth rejection via Firebase rules if possible).
**Observable:** Serial log for auth error messages. Relay state.

**Expected result:** Pump logic continues based on RS-485 sensor data. Error logged. Firebase writes fail gracefully (no crash).
**Note:** This is a degraded mode — no remote control possible, but pump still automates safely.

---

### FW-FI-003 [P2] — Corrupted NVS: System Falls Back to Compile-Time Defaults

**Procedure:**
1. Corrupt NVS by writing random bytes to a specific NVS key (use a test sketch).
2. Flash production firmware.
3. On boot: observe if firmware detects NVS corruption and falls back to defaults.

**Expected result:** Firmware does not crash. Falls back to compile-time defaults. Logs a warning about NVS read failure.

---

### FW-FI-004 [P2] — RS-485 CRC Errors: Up to 4 Consecutive Do Not Trigger Error Flag

**Requirement:** Single or few CRC errors are transient. Only consecutive failures above `sensor_failure_threshold` trigger the error flag.

**Procedure:**
1. Using a test sketch, inject 4 consecutive CRC-corrupted frames.
2. Observe `is_sensor_error` in Firebase.

**Expected result:** `is_sensor_error = false` after 4 bad frames (below threshold of 5).
**Then inject 5 bad frames:** `is_sensor_error = true`.

---

### FW-FI-005 [P2] — Rapidly Toggling manual_desired: System Remains Stable

**Procedure:**
1. Mode = MANUAL. Alternate `manual_desired: true/false` every 2s for 30s (15 toggles).
2. Monitor Serial log and Firebase for any error state or undefined behavior.

**Expected result:** Relay toggles cleanly 15 times. No stuck states, no unexpected errors, no crash.

---

### FW-FI-006 [P2] — Invalid Control Mode Value: Ignored or Sanitized

**Procedure:**
1. Write `mode: "INVALID_MODE_XYZ"` to `/pump_system/control/mode`.
2. Wait 6s.
3. Observe `run_mode` in Firebase status.

**Expected result:** Firmware ignores invalid mode string. System remains in current state. Logged at WARN level.
**Fail criteria:** Crash, undefined state, or pump starts/stops unexpectedly. Defect category: RELIABILITY.

---

### FW-FI-007 [P2] — Negative Countdown Duration: Sanitized or Ignored

**Procedure:**
1. Write `countdown_duration_min: -1` to Firebase control.
2. Write `countdown_start: true`.
3. Observe behavior.

**Expected result:** Firmware rejects negative duration. No countdown started. Logged at WARN.

---

### FW-FI-008 [P2] — Config Value Out of Range: Clamped or Ignored

**Test each invalid range:**

| Field | Invalid value | Expected behavior |
|---|---|---|
| `pump_start_level` | 150 | Clamped to 100 or ignored |
| `pump_stop_level` | -5 | Clamped to 0 or ignored |
| `dry_run_threshold_lpm` | 0 | Clamped to 0.1 or ignored |
| `dry_run_timeout_sec` | 0 | Clamped to 10 or ignored |
| `max_pump_runtime_min` | 999 | Clamped to 480 or ignored |

**Pass criteria:** No crash for any invalid value. Either clamped to valid range or ignored (previous value retained). No safety-critical behavior change.

---

### FW-FI-009 [P1] — Watchdog Recovery: System Returns to AUTO After Reset

**Procedure:**
1. Force a watchdog timeout (use a long delay in a test build, e.g., `delay(10000)` in main loop).
2. Observe: watchdog fires and resets ESP32.
3. On reboot: verify `last_boot_reason` contains watchdog-related string.
4. Verify system returns to `run_mode: AUTO_STANDBY` (not stuck in error state).

**Pass criteria:** System recovers cleanly after watchdog reset.

---

## 15. Module 13: Boundary Value & Equivalence Tests

> **Covers:** All configurable numeric thresholds tested at min, max, min-1, max+1, and typical values.
> **QA technique:** Boundary value analysis (BVA), equivalence partitioning (EP).

---

### 15.1 Dry-Run Threshold Equivalence Classes

For `dry_run_threshold_lpm = 1.0`:

| Partition | Representative value | Expected behavior |
|---|---|---|
| Below threshold | 0 LPM | Dry-run timer counting |
| At threshold exactly | 1.0 LPM | BVA: define if timer runs at exactly threshold |
| Above threshold | 1.5 LPM | No dry-run trigger |
| Sensor error zone | > 60 LPM | Flow sensor error flag |
| Invalid (sensor disconnected) | signal absent | Flow = 0 (treated as below threshold) |

### FW-BVA-001 [P2] — Flow at Exactly Threshold Value

**Test:** Set calibrated flow to exactly 1.0 LPM (verify with bucket). Observe dry-run timer behavior for 40 seconds.

**Document:** Does firmware use `< threshold` or `<= threshold`? What happens at exactly 1.0 LPM?
**Pass criteria:** Behavior is consistent with firmware condition operator. No oscillation.

---

### 15.2 Level Percentage Equivalence Classes

For pump_start_level=20, pump_stop_level=90:

| Partition | Representative | Expected AUTO behavior |
|---|---|---|
| Below start | 15% | Start pump |
| At start boundary | 20% | BVA: document |
| Between start and stop | 55% | No change (pump stays in current state) |
| At stop boundary | 90% | BVA: document |
| Above stop | 95% | Stop pump (if running) |
| Invalid / not yet valid | -1 | Do nothing |

### FW-BVA-002 [P2] — Level at Exact Start Boundary

**Test:** Set level to exactly `pump_start_level` (e.g., 20%). Pump is stopped. Observe.
**Document:** Does pump start at exactly 20%, or only at < 20%?

### FW-BVA-003 [P2] — Level at Exact Stop Boundary

**Test:** Set level to exactly `pump_stop_level` (e.g., 90%). Pump is running. Observe.
**Document:** Does pump stop at exactly 90%, or only at > 90%?

---

### 15.3 Countdown Duration Boundaries

| Value | Expected behavior |
|---|---|
| 0 | Rejected or treated as 1 |
| 1 | Minimum valid — 60s countdown |
| 120 | Maximum valid — 7200s countdown |
| 121 | Clamped to 120 or rejected |

### FW-BVA-004 [P2] — Countdown at Minimum (1 minute)

**Test:** Set `countdown_duration_min = 1`. Start countdown. Verify pump runs exactly 60s ±10s then stops.

### FW-BVA-005 [P2] — Countdown at Maximum (120 minutes)

**Test:** Set `countdown_duration_min = 120`. Verify `countdown_remaining_sec ≈ 7200` in Firebase. Do not wait full duration — cancel after 60s.

---

### 15.4 Sensor Failure Threshold Boundaries

For `sensor_failure_threshold = 5`:

| Consecutive failures | Expected behavior |
|---|---|
| 4 | `is_sensor_error = false` |
| 5 | `is_sensor_error = true` |
| 6+ | `is_sensor_error = true` (stays set) |
| 3 good frames after error | `is_sensor_error = false` (auto-clear) |

### FW-BVA-006 [P2] — Sensor Error at Exactly Threshold

**Test:** Inject exactly `sensor_failure_threshold - 1 = 4` bad frames. Verify `is_sensor_error = false`. Inject one more. Verify `is_sensor_error = true`.

---

## 16. Module 14: State Transition Coverage Matrix

> **Coverage target:** 100% of valid state transitions, 100% of guard condition checks.
> **Standard reference:** DO-178C MC/DC (Modified Condition/Decision Coverage) adapted for embedded.

### 16.1 Complete State Transition Coverage Table

For each row: verify the transition has been explicitly tested.

| From state | Event / Stimulus | To state | Guard condition | Test case |
|---|---|---|---|---|
| ANY | emergency_stop: true | STOPPED | None | FW-SM-003 |
| STOPPED | reset_stop: true, no error | AUTO_STANDBY | !is_error | FW-SM-005 |
| STOPPED | reset_stop: true, error active | STOPPED | is_error blocks | FW-SM-006 |
| STOPPED | mode: AUTO | STOPPED | latch blocks | FW-SM-004 |
| AUTO_STANDBY | level < start | AUTO | !is_error, !latch, level valid, !cooldown | FW-AUTO-001 |
| AUTO_STANDBY | level ≥ start | AUTO_STANDBY | No change | FW-AUTO-003 |
| AUTO_STANDBY | mode: MANUAL | MANUAL_OFF | — | FW-SM-001 |
| AUTO_STANDBY | mode: COUNTDOWN + start | COUNTDOWN | — | FW-COUNT-001 |
| AUTO | level ≥ stop | AUTO_COOLDOWN | — | FW-AUTO-002 |
| AUTO | overflow_timeout | AUTO_COOLDOWN + overflow_error | max_runtime exceeded | FW-SAF-006 |
| AUTO | dry_run_timeout | AUTO (pump OFF) + is_error | flow < threshold for timeout | FW-SAF-001 |
| AUTO | comm_loss_timeout | AUTO (pump OFF) | level stale | FW-SAF-009 |
| AUTO | mode: MANUAL | MANUAL_OFF | — | FW-SM-001 |
| AUTO_COOLDOWN | off_timer_expires + level < start | AUTO | cooldown done | FW-AUTO-004 |
| AUTO_COOLDOWN | off_timer_expires + level ≥ start | AUTO_STANDBY | cooldown done | FW-AUTO-004 |
| AUTO_COOLDOWN | mode: MANUAL | MANUAL_OFF | — | FW-SM-001 |
| MANUAL_OFF | manual_desired: true | MANUAL_ON | !is_error, !latch | FW-MAN-001 |
| MANUAL_OFF | manual_desired: false | MANUAL_OFF | Already off | — |
| MANUAL_OFF | mode: AUTO | AUTO_STANDBY | — | FW-SM-002 |
| MANUAL_ON | manual_desired: false | MANUAL_COOLDOWN | off_timer starts | FW-MAN-002 |
| MANUAL_ON | dry_run_timeout | MANUAL_ON (pump OFF) + is_error | — | FW-MAN-004 |
| MANUAL_ON | overflow_timeout | MANUAL_ON (pump continues) + warning | MANUAL: warn only | FW-MAN-005 |
| MANUAL_ON | mode: AUTO | AUTO (run or standby by level) | — | FW-SM-002 |
| MANUAL_COOLDOWN | off_timer_expires | MANUAL_OFF | — | FW-MAN-006 |
| COUNTDOWN | countdown_expires | AUTO_COOLDOWN | — | FW-COUNT-003 |
| COUNTDOWN | mode: AUTO | AUTO_STANDBY | — | FW-COUNT-005 |
| COUNTDOWN | dry_run_timeout | pump OFF + is_error | — | FW-COUNT-006 |
| COUNTDOWN | overflow_timeout | pump OFF + is_overflow_error | — | FW-COUNT-007 |
| ANY | is_error: true | [current state, pump OFF] | Error latches, mode unchanged | FW-SAF-004 |
| ERROR | clear_error: true | [current state, is_error: false] | — | FW-SAF-005 |
| ERROR | mode change | [new mode, pump still OFF] | Error persists | FW-SAF-004 |

**Coverage verification:** Each row's test case must have a PASS result before marking 100% transition coverage.

---

## 17. Module 15: Long-Duration Behavioral Tests

> **Covers:** Memory stability, counter accuracy, long-run logic correctness.
> **Minimum duration:** As stated per test. Run overnight where specified.

---

### FW-LD-001 [P2] — 4-Hour Heap Stability Test

**Procedure:**
1. Note `free_heap_bytes` and `min_free_heap_observed_bytes` at t=0 (after 5 minutes of operation).
2. Record values at t=1h, t=2h, t=3h, t=4h.
3. Compute total heap decrease over 4 hours.

**Pass criteria:** Total decrease < 5KB over 4 hours. `min_free_heap_observed_bytes` never drops below 100KB.
**Fail criteria:** Steady downward trend > 1KB/hour. Defect category: RELIABILITY (memory leak).

---

### FW-LD-002 [P2] — 4-Hour RS-485 Success Rate

**Procedure:**
1. Record `ultrasonic_cycles_ok` and `ultrasonic_cycles_timeout` at t=0.
2. Record at t=4h.
3. Compute: success_rate = ok_4h / (ok_4h + timeout_4h).

**Pass criteria:** success_rate ≥ 0.97 (97%) over 4 hours in good conditions.

---

### FW-LD-003 [P2] — 4-Hour AUTO Mode: Correct Pump Cycles

**Procedure:**
1. Note `total_pump_cycles` at t=0.
2. Count manual observations of pump start/stop events.
3. At t=4h, verify `total_pump_cycles` matches counted events.

**Pass criteria:** Counter matches observed cycles exactly.

---

### FW-LD-004 [P2] — uptime_minutes Accuracy Over 1 Hour

**Procedure:**
1. Note `uptime_minutes` at boot.
2. After exactly 1 hour (stopwatch): read `uptime_minutes`.

**Expected result:** `uptime_minutes` = 60 ±2 (accounting for float conversion).
**Pass criteria:** 58 ≤ uptime_minutes ≤ 62 at 1-hour mark.

---

### FW-LD-005 [P3] — 10 Power Cycles: Run_mode Always AUTO_STANDBY on Boot

**Procedure:**
1. Power cycle ESP32 10 times with 30s off each time.
2. For each boot: record the first `run_mode` value in Firebase.

**Expected result:** All 10 values = `"AUTO_STANDBY"`.
**Pass criteria:** 10/10 correct initial run_mode.

---

## 18. Regression Checklist

Run this minimal set after every firmware change to confirm no regression.

| ID | Test name | Component | Priority |
|---|---|---|---|
| FW-BOOT-001 | run_mode = AUTO_STANDBY on boot | Boot | P1 |
| FW-BOOT-002 | level absent before first valid frame | Boot | P1 |
| FW-BOOT-004 | Pump not start before sensor valid | Boot | P1 |
| FW-SM-003 | E-stop from any state | State machine | P1 |
| FW-SM-004 | STOPPED not exited by mode change | State machine | P1 |
| FW-SM-005 | STOPPED exits via reset_stop only | State machine | P1 |
| FW-AUTO-001 | AUTO pump starts when below start | AUTO logic | P1 |
| FW-AUTO-002 | AUTO pump stops when at stop | AUTO logic | P1 |
| FW-AUTO-003 | AUTO pump not start when full | AUTO logic | P1 |
| FW-AUTO-004 | Cooldown prevents immediate restart | AUTO logic | P1 |
| FW-AUTO-006 | Stale level stops pump | AUTO logic | P1 |
| FW-MAN-001 | MANUAL: start on manual_desired true | MANUAL logic | P1 |
| FW-MAN-002 | MANUAL: stop on manual_desired false | MANUAL logic | P1 |
| FW-MAN-003 | MANUAL: level has no effect | MANUAL logic | P1 |
| FW-MAN-004 | MANUAL: DRY_RUN still fires | MANUAL logic | P1 |
| FW-MAN-005 | MANUAL: overflow = warning, not stop | MANUAL logic | P1 |
| FW-SAF-001 | DRY_RUN fires at exact timeout | Safety | P1 |
| FW-SAF-002 | DRY_RUN timer resets on recovery | Safety | P1 |
| FW-SAF-003 | DRY_RUN: relay OFF immediately | Safety | P1 |
| FW-SAF-004 | DRY_RUN not cleared by mode change | Safety | P1 |
| FW-SAF-005 | DRY_RUN clears via clear_error only | Safety | P1 |
| FW-SAF-007 | OVERFLOW: does not stop in MANUAL | Safety | P1 |
| FW-SAF-009 | Sensor failure stops pump | Safety | P1 |
| FW-RS-001 | CRC16-Modbus matches test vector | Protocol | P1 |
| FW-RS-002 | CRC failure: frame data not used | Protocol | P1 |
| FW-FB-002 | One-shot fields reset after processing | Firebase | P1 |
| FW-NVS-001 | Config survives power cycle | NVS | P2 |
| FW-SN-002 | Level plausibility filter rejects bad readings | Sensor node | P2 |
| FW-SN-004 | Flow error hysteresis: 3s assert | Sensor node | P2 |
| FW-SN-005 | Flow error hysteresis: 5s clear | Sensor node | P2 |

---

## 19. Test Traceability Matrix

| Requirement | Bug/Feature | Test cases covering |
|---|---|---|
| runMode initialized to AUTO_STANDBY | Bug M-05 | FW-BOOT-001, FW-LD-005 |
| water_level_percent absent before valid data | Bug C-02 | FW-BOOT-002, FW-BOOT-004 |
| Overflow warning-only in MANUAL | Bug H-05 | FW-MAN-005, FW-SAF-007 |
| Crash counter: success-based clear | Bug H-06 | FW-BOOT-007 |
| AUTO_COOLDOWN run mode | Bug H-07 | FW-AUTO-002, FW-AUTO-004, FW-AUTO-005 |
| Level plausibility filter observability | Bug H-02 | FW-SN-002, FW-SN-003 |
| Flow discard uses local variable | Bug H-03 | FW-SN-006 |
| Flow error hysteresis | Bug H-04 | FW-SN-004, FW-SN-005 |
| RS-485 partial frame stall reset | Bug M-03 | FW-RS-005 |
| bypass_flow_sensor runtime control | Bug M-02 | FW-SAF-013, FW-NVS-005 |
| is_idle_mode in Firebase | Bug M-06 | FW-AUTO-009, FW-AUTO-010 |
| Level timestamp consolidation | Bug M-01 | FW-AUTO-006 |
| LDSC field in RS-485 frame | New field | FW-SN-007, FW-RS-006 |
| DRY_RUN safety lockout | Core safety | FW-SAF-001 through FW-SAF-005 |
| OVERFLOW protection | Core safety | FW-SAF-006, FW-SAF-007, FW-SAF-008 |
| Emergency stop | Core safety | FW-SM-003, FW-SM-004, FW-SM-005 |
| AUTO mode logic | Core function | FW-AUTO-001 through FW-AUTO-011 |
| MANUAL mode logic | Core function | FW-MAN-001 through FW-MAN-007 |
| COUNTDOWN mode | Core function | FW-COUNT-001 through FW-COUNT-007 |
| RS-485 CRC integrity | Protocol | FW-RS-001, FW-RS-002 |
| RS-485 timeout/retry | Protocol | FW-RS-003, FW-RS-004 |
| NVS persistence | Reliability | FW-NVS-001 through FW-NVS-005 |
| LOG() system | Observability | FW-LOG-001 through FW-LOG-005 |
| Firebase schema compliance | Data contract | FW-FB-001, FW-FB-002 |

---

*SmartFlow Firmware Software Quality Assurance Test Specification v1.0*
*Standards: IEEE 829-2008 (Software Test Documentation) · IEEE 1028-2008 (Software Reviews) ·
IEC 61508-3:2010 §7.4 (Software safety requirements, state machine testing) ·
IEC 61511-1:2016 (Functional safety, safety instrumented systems) ·
ISO/IEC 25010:2011 (Software quality — reliability, functional correctness, safety) ·
ISTQB Foundation Level v4.0 (EP, BVA, state transition, decision table techniques) ·
DO-178C Annex A (adapted: MC/DC coverage for state machines)*
