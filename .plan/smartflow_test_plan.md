# SmartFlow — System Verification & Validation Master Test Plan
### Quality Assurance Specification v1.0

**Project:** SmartFlow IoT Pump Controller
**Location:** Leon, Iloilo, Philippines
**Classification:** Safety-related embedded IoT system — 220V AC motor control
**Standard references:** IEC 61508 (functional safety), IEC 62443 (IoT security),
IEEE 829 (software test documentation), ISO/IEC 25010 (software quality),
ISTQB Foundation Level syllabus, OWASP IoT Top 10

**Document purpose:** Complete verification and validation test suite covering firmware,
communication protocol, cloud integration, dashboard, security, reliability, and safety.
Every test is traceable to a system requirement. The system must pass all CRITICAL and
HIGH priority tests before deployment.

---

> **Tester notice**
> This document assumes the refactor described in SmartFlow System Refactor Plan v2.0
> is complete. Read the system architecture section before executing any test. Tests
> marked [SAFETY] affect the pump relay directly — follow all safety precautions before
> executing them. Never execute safety tests with the pump running unless the test
> explicitly requires it.

---

## Safety Precautions — Read Before Testing

```
⚠ HIGH VOLTAGE WARNING
The pump operates on 220V AC single-phase. The TOR (LR2-D13) protects the motor
but does not protect the tester.

Before any test involving the relay or contactor:
1. Confirm a qualified person has verified all 220V wiring.
2. The MCB must be accessible and reachable during the entire test session.
3. Never defeat or bypass the TOR during testing.
4. Have a clear path to the MCB for immediate power cut.
5. Tests marked [DRY-RUN-SAFE] may be run with pump disconnected from the relay.
   All others require a functioning pump and water supply.
```

---

## Table of Contents

1. [System Architecture Overview](#1-system-architecture-overview)
2. [Test Infrastructure Requirements](#2-test-infrastructure-requirements)
3. [Test ID Schema & Conventions](#3-test-id-schema--conventions)
4. [Layer 1 — Hardware & Physical Tests](#4-layer-1--hardware--physical-tests)
5. [Layer 2 — Sensor Node Tests (NodeMCU V2)](#5-layer-2--sensor-node-tests-nodemcu-v2)
6. [Layer 3 — RS-485 Communication Protocol Tests](#6-layer-3--rs-485-communication-protocol-tests)
7. [Layer 4 — ESP32 Firmware Logic Tests](#7-layer-4--esp32-firmware-logic-tests)
8. [Layer 5 — Firebase Integration Tests](#8-layer-5--firebase-integration-tests)
9. [Layer 6 — Dashboard Functional Tests](#9-layer-6--dashboard-functional-tests)
10. [Layer 7 — End-to-End System Tests](#10-layer-7--end-to-end-system-tests)
11. [Layer 8 — Safety & Fault Injection Tests](#11-layer-8--safety--fault-injection-tests)
12. [Layer 9 — Edge Case & Boundary Tests](#12-layer-9--edge-case--boundary-tests)
13. [Layer 10 — Security Tests](#13-layer-10--security-tests)
14. [Layer 11 — Reliability & Soak Tests](#14-layer-11--reliability--soak-tests)
15. [Layer 12 — Accessibility & PWA Tests](#15-layer-12--accessibility--pwa-tests)
16. [Regression Test Suite](#16-regression-test-suite)
17. [Test Execution Checklist](#17-test-execution-checklist)
18. [Pass/Fail Criteria Summary](#18-passfail-criteria-summary)

---

## 1. System Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│  LAYER 3 — CLOUD / DASHBOARD                                    │
│  Next.js 14 PWA ←→ Firebase RTDB ←→ ESP32 (every 3s)           │
│  Route: /pump_system/{status, control, config}                  │
├─────────────────────────────────────────────────────────────────┤
│  LAYER 2 — ESP32 FIRMWARE (master controller)                   │
│  Mode state machine: AUTO / MANUAL / COUNTDOWN / STOPPED        │
│  Safety: dry-run lockout, overflow cutoff, sensor failure gate  │
│  RS-485 master — polls NodeMCU every 3s                        │
├─────────────────────────────────────────────────────────────────┤
│  LAYER 2B — NodeMCU V2 FIRMWARE (sensor slave)                  │
│  JSN-SR04T ultrasonic → level % + distance cm                   │
│  YF-G1 hall-effect → flow rate LPM                              │
│  RS-485 slave — responds to ESP32 REQ frames                    │
├─────────────────────────────────────────────────────────────────┤
│  LAYER 1 — HARDWARE (always active, software-independent)       │
│  MCB (20A) → CJX2-2510 Contactor → LR2-D13 TOR → 1.5HP Pump   │
│  Manual bypass switch (parallel with relay NO-COM path)         │
└─────────────────────────────────────────────────────────────────┘

Key thresholds (production defaults):
  Tank: empty=200cm, full=10cm (adjust per calibration)
  Pump start: 20%  |  Pump stop: 90%
  Dry-run threshold: 1.0 LPM  |  Dry-run timeout: 30s
  Max pump runtime: 120 min
  RS-485 frame timeout: 250ms  |  Max retries: 3
  Sensor failure threshold: (verify in firmware)
  Firebase poll: status every 3s, config every 30s
```

---

## 2. Test Infrastructure Requirements

### 2.1 Required Equipment

| Item | Specification | Purpose |
|------|--------------|---------|
| Full hardware rig | ESP32 + NodeMCU + relay + contactor + TOR + pump | Integration tests |
| Water supply | Connected to pump inlet, tank reachable | Fill/drain tests |
| Multimeter | True RMS, CAT II 300V minimum | Voltage, continuity |
| USB-TTL adapter | 3.3V logic, 115200 baud | NodeMCU GPIO2 debug |
| Logic analyzer | 2-channel, 115200 baud decode | RS-485 frame analysis |
| Laptop with Arduino IDE + PlatformIO | - | Serial Monitor |
| Browser (Chrome) | For dashboard testing | Dashboard tests |
| Chrome DevTools | Network, Memory, Lighthouse | Performance/security |
| Android phone | Chrome browser | PWA + mobile tests |
| Stopwatch | For timing tests | Latency verification |
| Bucket (10L calibration) | For flow sensor calibration | Flow accuracy |

### 2.2 Software Prerequisites

- SmartFlow firmware flashed (production build, `SMARTFLOW_DEBUG` undefined or `LOG_COMPILE_FLOOR=LOG_INFO`)
- Dashboard deployed to Vercel (or running on `localhost:3000` for dashboard-only tests)
- Firebase project live, rules deployed
- `secrets.h` correct on ESP32
- All test sketches from Phase 5 compiled and ready

### 2.3 Test Environment States

Tests reference these named states:

| State name | Description |
|------------|-------------|
| COLD_BOOT | Both nodes powered off for ≥ 60s before power-on |
| WARM_BOOT | Power cycled with NVS intact |
| BLANK_NVS | ESP32 flashed with NVS erased (Arduino IDE: Erase Flash → All Flash Contents) |
| TANK_LOW | Physical tank level < pump_start_level (20%) |
| TANK_MID | Physical tank level between 20% and 90% |
| TANK_HIGH | Physical tank level > pump_stop_level (90%) |
| TANK_FULL | Physical tank level at 100% (sensor reads ≤ tank_full_cm) |
| CAT6_CONNECTED | CAT6 cable between ESP32 and NodeMCU is intact |
| CAT6_DISCONNECTED | CAT6 physically unplugged at the NodeMCU end |
| PUMP_RUNNING | `is_running: true` confirmed in Firebase |
| PUMP_STOPPED | `is_running: false` confirmed in Firebase |
| WIFI_CONNECTED | ESP32 has WiFi and Firebase connection |
| WIFI_DOWN | Router powered off or SSID blocked |
| FLOW_BLOCKED | Flow sensor physically blocked (valve closed or pipe clamped) |
| FLOW_OPEN | Normal water flow through sensor |

---

## 3. Test ID Schema & Conventions

```
SF-[LAYER]-[NUMBER]

Layers:
  HW   Hardware & Physical
  SN   Sensor Node (NodeMCU)
  RS   RS-485 Protocol
  FW   ESP32 Firmware Logic
  FB   Firebase Integration
  DB   Dashboard
  E2E  End-to-End System
  SAF  Safety & Fault Injection
  EDG  Edge Cases & Boundaries
  SEC  Security
  REL  Reliability & Soak
  PWA  Accessibility & PWA

Priority:
  [CRITICAL] System cannot be deployed without passing
  [HIGH]     Strongly recommended before deployment
  [MEDIUM]   Should be tested; failure documented if known
  [LOW]      Best-effort; document if resources allow

Annotations:
  [SAFETY]         Involves relay activation or 220V
  [DRY-RUN-SAFE]   Can run with pump disconnected from relay
  [MANUAL]         Requires a human observer/operator
  [AUTOMATED]      Can be scripted / run without human input
  [DESTRUCTIVE]    Erases persistent state (NVS, Firebase)
```

---

## 4. Layer 1 — Hardware & Physical Tests

### SF-HW-001 [CRITICAL] [MANUAL] — TOR Continuity & Setting

**Objective:** Verify the thermal overload relay is correctly calibrated and will trip independently of firmware.

**Precondition:** Multimeter available, pump wiring complete.

**Steps:**
1. With MCB OFF, use multimeter in continuity mode.
2. Probe TOR terminals L1→T1, L2→T2. Confirm continuity (TOR in normal closed state).
3. Read the TOR dial setting. Confirm it is set to motor FLA: **8–9A** for 1.5HP 220V single-phase.
4. Confirm TOR L3/T3 terminals are capped with terminal covers.
5. Confirm TOR reset button is accessible (not obstructed by wiring).

**Expected result:**
- Continuity confirmed on L1→T1 and L2→T2
- TOR dial confirmed at 8–9A
- L3/T3 capped

**Pass criteria:** All three conditions met.
**Fail criteria:** Any discrepancy. Do not power on until resolved.

---

### SF-HW-002 [CRITICAL] [MANUAL] — Earth Continuity

**Objective:** Verify protective earth is continuous from DIN rail to pump motor casing.

**Steps:**
1. Set multimeter to resistance (Ω), 200Ω range.
2. Probe between DIN rail grounding lug and pump motor casing (external metal body).
3. Record resistance.

**Expected result:** < 1 Ω (IEC 60364-6 commissioning requirement).

**Pass criteria:** Measured resistance < 1 Ω.
**Fail criteria:** > 1 Ω. Do not energize until corrected.

---

### SF-HW-003 [CRITICAL] [MANUAL] [DRY-RUN-SAFE] — Relay GPIO Logic Level

**Objective:** Confirm relay module is active-LOW and GPIO 4 controls it correctly.

**Precondition:** ESP32 powered, production firmware running. Pump disconnected from contactor coil (contactor coil wire removed for safety — this test does not need the coil energized).

**Steps:**
1. Open Serial Monitor at 115200 baud.
2. From dashboard, switch to FORCE_OFF or ensure pump is STOPPED.
3. Measure DC voltage between Relay module IN and GND.
4. Expected when relay should be OFF (pump not running): ~3.3V (active-LOW — HIGH = relay OFF).
5. From dashboard, switch to MANUAL mode, send manual_desired: true.
6. Measure voltage again. Expected when relay should be ON: ~0V.
7. Visually confirm relay LED follows the expected state (if relay module has an LED).

**Expected result:** GPIO 4 HIGH (3.3V) = relay coil de-energized; GPIO 4 LOW (0V) = relay coil energized.

**Pass criteria:** Voltage readings match expected logic levels within ±0.3V.

---

### SF-HW-004 [HIGH] [MANUAL] — Voltage Dividers on GPIO 34 and GPIO 18

**Objective:** Verify voltage dividers (1kΩ + 2kΩ) protect ESP32 inputs from 5V sensor signals.

**Steps:**
1. Apply 5V to the sensor signal line (simulate sensor output at maximum).
2. Measure voltage at GPIO 34 (flow sensor) and GPIO 18 (ultrasonic ECHO).
3. Expected: ~3.33V = 5V × (2kΩ / (1kΩ + 2kΩ)).

**Expected result:** 3.2–3.4V measured at each GPIO when sensor outputs 5V.

**Pass criteria:** Both readings within 3.2–3.4V range.

---

### SF-HW-005 [CRITICAL] [MANUAL] — Manual Bypass Switch Isolation

**Objective:** Confirm the manual bypass switch bypasses the relay and operates the contactor directly, and that this is documented.

**Precondition:** [SAFETY] 220V powered, qualified person present, clear path to MCB.

**Steps:**
1. Confirm ESP32 relay is in OFF state (pump should be stopped from firmware perspective).
2. Flip the manual bypass switch ON.
3. Verify contactor energizes (audible click, green indicator if present).
4. Flip bypass switch OFF.
5. Verify contactor de-energizes.

**Expected result:** Contactor responds to bypass switch independently of ESP32 relay state.

**Pass criteria:** Contactor cycles correctly with bypass switch.

**⚠ Critical note:** With bypass switch ON, all software protections (dry-run, tank level, overflow) are bypassed. TOR is the only remaining protection. Document this clearly.

---

### SF-HW-006 [HIGH] [MANUAL] — CAT6 Cable Integrity

**Objective:** Verify CAT6 cable between enclosure and NodeMCU carries correct signals.

**Steps:**
1. With system running, measure DC voltage between RS-485 A and B lines at the enclosure end.
2. In idle (no transmission): should be ~2.0V differential (bias resistors in MAX485 hold lines).
3. Open Serial Monitor for NodeMCU (GPIO2 / USB-TTL adapter).
4. Confirm debug log messages appear (node is communicating).
5. Tug test: pull each conductor at both enclosure and tank ends. Confirm no intermittent dropouts in Serial Monitor.

**Expected result:** Stable RS-485 differential, no tug-induced frame errors.

---

## 5. Layer 2 — Sensor Node Tests (NodeMCU V2)

### SF-SN-001 [CRITICAL] [DRY-RUN-SAFE] — Ultrasonic Sensor Accuracy

**Objective:** Verify JSN-SR04T readings are within acceptable accuracy of actual tank level.

**Precondition:** Sensor probe installed in tank, NodeMCU running test sketch TC-S-02 OR production firmware with debug output.

**Steps:**
1. Fill tank to a known level. Measure actual water surface to sensor distance with a tape measure (cm). Record: `actual_cm`.
2. Read sensor output: median of 10 readings from Serial Monitor or Firebase `ultrasonic_last_good_cm`. Record: `sensor_cm`.
3. Repeat at three different tank levels: low (~25%), mid (~50%), high (~85%).

**Expected result:** `|sensor_cm - actual_cm| ≤ 3.0 cm` at all three levels.

**Pass criteria:** All three measurements within ±3 cm tolerance.
**Fail criteria:** Any reading exceeds ±3 cm. Recheck sensor mounting angle and wiring.

---

### SF-SN-002 [HIGH] [DRY-RUN-SAFE] — Level Percentage Calculation

**Objective:** Verify the ESP32 correctly converts ultrasonic distance to water level percentage.

**Formula:** `level% = (tank_empty_cm - measured_cm) / (tank_empty_cm - tank_full_cm) × 100`

**Steps:**
1. Note configured `tank_empty_cm` and `tank_full_cm` from `/pump_system/config/device`.
2. At physical level matching tank_empty_cm (tank truly empty): confirm `water_level_percent = 0%` in Firebase.
3. At physical level matching tank_full_cm (tank truly full): confirm `water_level_percent = 100%` in Firebase.
4. At midpoint distance: verify percentage matches formula.

**Expected result:** Level % matches formula within ±2%.

---

### SF-SN-003 [HIGH] [MANUAL] — Flow Sensor Calibration Verification

**Objective:** Verify YF-G1 flow rate readings match actual flow rate within tolerance.

**Engineering basis:** YF-G1 specifications: working range 1–60 LPM, pulse factor ≈ 7.5 pulses/liter (calibration factor may vary ±10% per unit).

**Steps:**
1. Set pump to MANUAL ON.
2. Collect output water in a measured 10L bucket.
3. Time how long it takes to fill the bucket (seconds). Record: `fill_time_s`.
4. Compute actual flow: `actual_lpm = (10 / fill_time_s) × 60`.
5. Read `flow_rate_lpm` from Firebase during the collection period (average of 3 readings).
6. Compare: `|firebase_lpm - actual_lpm| / actual_lpm × 100` = percentage error.

**Expected result:** Flow rate measurement within ±15% of actual (YF-G1 stated accuracy: ±10%, plus ISR timing noise budget of ±5%).

**Pass criteria:** Error ≤ 15%.
**If fail:** Adjust `flow_calibration_factor` in settings and repeat. Document calibrated value.

---

### SF-SN-004 [CRITICAL] [DRY-RUN-SAFE] — Sensor Error Flag Behavior

**Objective:** Verify `is_sensor_error` / `ERR` bit 0 is set when ultrasonic sensor fails.

**Steps:**
1. With NodeMCU running production firmware and RS-485 connected:
2. Disconnect the JSN-SR04T TRIG wire from NodeMCU.
3. Wait for sensor error to propagate (≤ 10s).
4. Check Firebase: `is_sensor_error` should become `true`.
5. Reconnect TRIG wire.
6. Confirm `is_sensor_error` auto-clears within 10s of stable readings.

**Expected result:**
- Error detected and reported within 10s of sensor failure
- Error auto-clears within 10s of recovery

---

### SF-SN-005 [HIGH] [DRY-RUN-SAFE] — Flow Sensor Error Flag Hysteresis

**Objective:** Verify `snFlowError` does not oscillate — requires 3 consecutive bad seconds to assert, 5 consecutive clean seconds to clear.

**Steps:**
1. With production firmware: observe `is_flow_sensor_error` in Firebase at rest (should be false, no flow).
2. Inject brief noise on the flow sensor line (momentarily ground the signal wire 2-3 times rapidly).
3. Observe: flag should NOT immediately become true unless noise persists ≥ 3 consecutive polling cycles.
4. Sustain noise for 5s. Confirm flag becomes true.
5. Remove noise. Confirm flag does NOT immediately clear — should take ≥ 5s.

**Expected result:** 3-second assert dwell, 5-second clear dwell observed.

---

### SF-SN-006 [HIGH] [DRY-RUN-SAFE] — Level Plausibility Filter

**Objective:** Verify extreme level jumps are discarded and `remote_level_discard_count` increments.

**Steps:**
1. With NodeMCU in debug mode (`DEBUG_USB_MODE=1`):
2. Temporarily inject an extreme reading (>LEVEL_MAX_DELTA_PCT change from last good value) by covering the sensor with hand momentarily.
3. Observe Serial output: should log a warning about discard.
4. Check `remote_level_discard_count` in Firebase — should increment.
5. Confirm `water_level_percent` does NOT jump to the bad reading.

**Expected result:** Plausibility filter rejects extreme readings; discard counter increments; level display stable.

---

## 6. Layer 3 — RS-485 Communication Protocol Tests

### SF-RS-001 [CRITICAL] [DRY-RUN-SAFE] — Frame CRC Integrity

**Objective:** Verify all RS-485 frames have valid CRC16-Modbus and the master rejects frames with bad CRC.

**Test A — Valid frame:**
1. Run test sketch TC-S-04 (echo server) on NodeMCU. Run TC-M-02 (RS-485 poll) on ESP32.
2. Observe: 30 frames, ≥ 90% valid CRC.
3. Check `ultrasonic_cycles_ok` increments.

**Test B — CRC corruption injection:**
1. Using a logic analyzer or test sketch: capture a valid frame, modify one payload byte, recompute nothing.
2. Inject the corrupted frame.
3. Observe: ESP32 must increment `ultrasonic_cycles_timeout` or CRC error counter.
4. Confirm `water_level_percent` is NOT updated from a corrupted frame.

**Expected result:** Valid frames accepted; corrupted frames rejected with no data update.

---

### SF-RS-002 [CRITICAL] [DRY-RUN-SAFE] — Frame Timeout & Retry

**Objective:** Verify ESP32 retries up to 3 times (250ms each) before marking sensor offline.

**Steps:**
1. Disconnect NodeMCU power (sensor node offline).
2. Observe Serial Monitor on ESP32 (debug mode): should show retry attempts.
3. After 3 retries × 250ms = 750ms: `remote_sensor_stable` should become `false`.
4. Reconnect NodeMCU power.
5. Confirm `remote_sensor_stable` returns to `true` within 15s (3 × 3s poll cycle = first 3 successful frames).

**Expected result:** 3 retries confirmed in log, offline detected at 750ms, stable after 15s reconnection.

---

### SF-RS-003 [HIGH] [DRY-RUN-SAFE] — Partial Frame Stall Recovery

**Objective:** Verify the NodeMCU receiver resets when a partial frame stalls (Bug M-03 fix).

**Steps:**
1. Using a logic analyzer or UART injector: send only the first 5 bytes of a valid frame to the NodeMCU, then stop.
2. Wait 25ms (> 20ms inter-byte stall threshold).
3. Send a complete valid REQ frame.
4. Observe: NodeMCU should respond correctly to the next valid REQ.

**Expected result:** Stale partial frame discarded after 20ms; subsequent valid frame processed correctly.

---

### SF-RS-004 [HIGH] [DRY-RUN-SAFE] — Sequence Number Monotonicity

**Objective:** Verify SEQ field increments from 0–255 and wraps correctly.

**Steps:**
1. Monitor RS-485 traffic with logic analyzer or debug Serial output.
2. Observe SEQ field across 260 consecutive frames.
3. Verify: SEQ increments by 1 each frame, wraps from 255→0.

**Expected result:** Strict monotonic increment with correct wraparound.

---

### SF-RS-005 [HIGH] [DRY-RUN-SAFE] — LDSC Field Backward Compatibility

**Objective:** Verify ESP32 parser handles frames with and without the LDSC field.

**Test A — Frame with LDSC:**
1. NodeMCU sends frame including `LDSC:3;` field.
2. Confirm ESP32 parses `remote_level_discard_count: 3` in Firebase.

**Test B — Frame without LDSC (old firmware simulation):**
1. Temporarily modify test sketch TC-S-04 to omit the LDSC field.
2. Send 10 frames without LDSC.
3. Confirm ESP32 does NOT crash, does NOT show parse error.
4. Confirm `remote_level_discard_count` defaults to 0.

**Expected result:** Both frame variants parsed without error.

---

### SF-RS-006 [CRITICAL] [DRY-RUN-SAFE] — RS-485 Bus Direction Control Timing

**Objective:** Verify DE/RE pin toggling does not corrupt frames (Serial2.flush() before DE LOW).

**Steps:**
1. Attach logic analyzer to RS-485 A/B lines.
2. Capture 20 request-response exchanges.
3. Verify: DE/RE is held HIGH through the complete transmission + flush.
4. Verify: First byte of each response is not truncated or corrupted.
5. Check: no bus contention (both nodes driving at same time).

**Expected result:** Clean waveforms, no truncation, no bus contention.

---

## 7. Layer 4 — ESP32 Firmware Logic Tests

### SF-FW-001 [CRITICAL] [SAFETY] — AUTO Mode: Pump Starts When Tank Below Start Level

**Objective:** Verify pump automatically starts when tank drops below configured start level.

**Precondition:** State = COLD_BOOT, TANK_LOW (physical level < 20%), MODE = AUTO.

**Steps:**
1. Set mode to AUTO from dashboard.
2. Power cycle ESP32 (cold boot).
3. Wait for system to initialize (Firebase connected, sensor stable — ≤ 30s).
4. Observe: within one poll cycle (≤ 3s after sensor stable), `is_running` should become `true`.
5. Verify contactor energizes (audible click).
6. Verify `run_mode: AUTO` in Firebase.

**Expected result:** Pump starts automatically, `is_running: true` within 3s of sensor stabilization.

**Pass criteria:** Pump starts within 3s of sensor stabilization in TANK_LOW state.

---

### SF-FW-002 [CRITICAL] [SAFETY] — AUTO Mode: Pump Stops When Tank Reaches Stop Level

**Objective:** Verify pump automatically stops when tank level reaches configured stop level.

**Precondition:** State = PUMP_RUNNING (in AUTO), pump filling the tank.

**Steps:**
1. Let pump run in AUTO mode with tank filling.
2. When `water_level_percent` reaches 90% (pump_stop_level) in Firebase:
3. Verify `is_running` becomes `false` within one poll cycle (≤ 3s).
4. Verify contactor de-energizes (audible click).
5. Verify `run_mode` transitions to `AUTO_STANDBY`.
6. Physically verify water level has stabilized near stop level.

**Expected result:** Pump stops at exactly stop level, `is_running: false` within 3s.

---

### SF-FW-003 [CRITICAL] [SAFETY] — AUTO Mode: Pump Does NOT Start When Tank Is Full

**Objective:** Verify pump does not start when tank level is already above stop level.

**Precondition:** State = COLD_BOOT, TANK_HIGH (level > 90%), MODE = AUTO.

**Steps:**
1. With TANK_HIGH condition, power on ESP32.
2. Wait 30s for full initialization.
3. Confirm `is_running: false` throughout initialization and for 60s after.
4. Confirm `run_mode: AUTO_STANDBY`.

**Expected result:** Pump remains off. No spurious start.

**Pass criteria:** `is_running` stays false for full 60s observation window.

---

### SF-FW-004 [CRITICAL] [SAFETY] — MANUAL Mode: Pump Starts on manual_desired True

**Objective:** Verify pump starts immediately when MANUAL mode and `manual_desired: true`.

**Precondition:** State = PUMP_STOPPED, MODE = MANUAL (from dashboard).

**Steps:**
1. Set mode to MANUAL via dashboard. Confirm `run_mode: MANUAL_OFF`.
2. Write `manual_desired: true` to `/pump_system/control/manual_desired` via dashboard.
3. Measure time from write to relay activation (audible click or multimeter on relay output).
4. Confirm `is_running: true` and `run_mode: MANUAL_ON` in Firebase.

**Expected result:** Pump starts within 6s of `manual_desired: true` (≤ 2 Firebase poll cycles of 3s each).

**Pass criteria:** Relay activates within 6s.

---

### SF-FW-005 [CRITICAL] [SAFETY] — MANUAL Mode: Pump Stops on manual_desired False

**Objective:** Verify pump stops immediately when `manual_desired: false` in MANUAL mode.

**Precondition:** State = PUMP_RUNNING in MANUAL mode.

**Steps:**
1. With pump running in MANUAL (SF-FW-004 passed).
2. Write `manual_desired: false` to control path via dashboard.
3. Measure time from write to relay de-activation.
4. Confirm `is_running: false` and `run_mode: MANUAL_OFF` in Firebase.

**Expected result:** Pump stops within 6s of `manual_desired: false`.

---

### SF-FW-006 [CRITICAL] [SAFETY] — MANUAL Mode: Tank Level Does NOT Control Pump

**Objective:** Verify tank level thresholds have NO effect on pump in MANUAL mode.

**Test A — Tank full, pump should keep running in MANUAL:**
1. Set mode to MANUAL, manual_desired: true, pump running.
2. Physically fill tank above 90% (stop level).
3. Confirm pump continues running despite level exceeding stop threshold.
4. `is_running` must remain `true`.

**Test B — Tank empty, pump should stay off in MANUAL if manual_desired is false:**
1. Set mode to MANUAL, manual_desired: false.
2. Drain tank below 20% (start level).
3. Confirm pump does NOT auto-start.
4. `is_running` must remain `false`.

**Expected result:** Tank level has zero influence on pump state in MANUAL mode.

---

### SF-FW-007 [CRITICAL] [SAFETY] — MANUAL Mode: Dry-Run Protection Still Active

**Objective:** Verify dry-run lockout DOES fire in MANUAL mode even though tank level doesn't control the pump.

**Engineering basis:** IEC 61508 — safety functions must not be defeatable by normal operation mode changes.

**Precondition:** MANUAL mode, pump running, FLOW_BLOCKED (valve closed or inlet plugged).

**Steps:**
1. Set mode to MANUAL, manual_desired: true (pump running).
2. Close the flow valve / block the pipe to the flow sensor.
3. Wait for dry-run timeout (default 30s).
4. Observe: `is_error: true`, `last_fault_code: DRY_RUN`, relay OFF within 1s of fault.
5. Confirm dashboard shows DRY_RUN error.
6. Attempt to restart pump via manual_desired: true. Confirm pump does NOT start (error latch is active).

**Expected result:** Dry-run lockout fires in MANUAL mode within 30s of blocked flow; cannot be overridden by manual command.

**Pass criteria:** Error triggers within 30s + 3s poll tolerance; restart attempt fails while error is latched.

---

### SF-FW-008 [CRITICAL] [SAFETY] — Emergency Stop: Immediate Pump Shutdown

**Objective:** Verify emergency stop command stops pump within one poll cycle from any mode.

**Test from each mode:**
1. From AUTO mode (pump running): write `emergency_stop: true` via dashboard.
2. From MANUAL mode (pump running): same.
3. From COUNTDOWN mode (pump running): same.

**For each test:**
- Measure time from dashboard write to relay de-activation (≤ 6s per 2 poll cycles).
- Confirm `emergency_stop_latched: true` in Firebase.
- Confirm `run_mode: STOPPED` in Firebase.
- Confirm relay does NOT re-activate when mode changes while latch is active.

**Expected result:** Pump stops within 6s from any mode; latch prevents restart.

---

### SF-FW-009 [CRITICAL] [SAFETY] — Emergency Stop: Latch Cannot Be Overridden by Mode Change

**Objective:** Verify changing mode does not clear the emergency stop latch.

**Precondition:** Emergency stop latched (SF-FW-008 completed).

**Steps:**
1. Change mode to AUTO via dashboard while latch is active.
2. Confirm pump does NOT start.
3. Change mode to MANUAL and set manual_desired: true.
4. Confirm pump does NOT start.
5. Write clear_error: true. Confirm pump does NOT start (clear_error does not reset E-stop).
6. Write reset_stop: true. Confirm pump can now restart normally.

**Expected result:** Only `reset_stop: true` clears the emergency stop latch. No other command does.

---

### SF-FW-010 [CRITICAL] [SAFETY] — Dry-Run Lockout: Cannot Be Bypassed by Mode Switch

**Objective:** Verify DRY_RUN error latch survives mode transitions.

**Precondition:** `is_error: true`, `last_fault_code: DRY_RUN`, pump stopped.

**Steps:**
1. Switch to MANUAL mode, write manual_desired: true.
2. Confirm pump does NOT start.
3. Switch to AUTO mode.
4. Confirm pump does NOT start (even if tank is below start level).
5. Write `clear_error: true`.
6. Confirm `is_error: false` and pump can restart normally.

**Expected result:** DRY_RUN latch survives mode transitions; only `clear_error: true` clears it.

---

### SF-FW-011 [HIGH] [SAFETY] — Overflow Protection in AUTO Mode

**Objective:** Verify pump stops when runtime exceeds `max_pump_runtime_min` in AUTO mode.

**Note:** This test is time-intensive. Can be simulated by temporarily reducing `max_pump_runtime_min` to 2 minutes via settings.

**Steps:**
1. Set `max_pump_runtime_min = 2` (for testing).
2. In AUTO mode with tank below start level, let pump run.
3. After 2 minutes:
4. Confirm `is_overflow_error: true`, `is_running: false`.
5. Confirm relay de-activated.
6. Restore `max_pump_runtime_min` to original value after test.

**Expected result:** Overflow cutoff fires at exactly 2 minutes (±6s for poll cycle).

---

### SF-FW-012 [HIGH] [SAFETY] — Overflow Protection in MANUAL Mode: Warning Only

**Objective:** Verify overflow protection in MANUAL mode issues a WARNING but does NOT stop the pump (Bug H-05 fix).

**Steps:**
1. Set `max_pump_runtime_min = 2` (for testing). Set mode to MANUAL.
2. Write manual_desired: true. Let pump run.
3. After 2 minutes:
4. Confirm `manual_runtime_warning: true` in Firebase.
5. Confirm pump is STILL RUNNING (`is_running: true`).
6. Confirm `is_overflow_error: false`.
7. Stop pump manually.

**Expected result:** Warning flag set, pump continues running in MANUAL mode.

---

### SF-FW-013 [HIGH] [SAFETY] — Countdown Mode: Pump Runs for Specified Duration

**Objective:** Verify COUNTDOWN mode runs the pump for exactly the configured duration then stops and returns to AUTO.

**Steps:**
1. Set mode to COUNTDOWN via dashboard.
2. Set countdown_duration_min = 5.
3. Write countdown_start: true.
4. Confirm pump starts, `run_mode: COUNTDOWN`.
5. Confirm `countdown_remaining_sec` decrements in Firebase (visible in diagnostics).
6. After 5 minutes (±10s): confirm pump stops, `run_mode: AUTO_STANDBY`.

**Expected result:** Pump runs exactly 5 minutes, countdown visible, returns to AUTO.

---

### SF-FW-014 [HIGH] [SAFETY] — Cooldown: Pump Does Not Restart Immediately After Stop

**Objective:** Verify AUTO_COOLDOWN prevents immediate pump restart when level drops back below start within the cooldown window.

**Precondition:** Pump just stopped at stop level (90%), tank level dropping.

**Steps:**
1. After AUTO stop event: confirm `run_mode: AUTO_COOLDOWN`, `pump_cooldown_remaining_sec > 0`.
2. Artificially drain tank below start level (20%) while cooldown is active.
3. Confirm pump does NOT start during cooldown period.
4. After cooldown expires: confirm pump starts normally.

**Expected result:** Cooldown enforced — pump stays off during cooldown regardless of level drop.

---

### SF-FW-015 [CRITICAL] [DRY-RUN-SAFE] — NVS First Boot: Defaults Load Correctly

**Objective:** Verify all config defaults are correctly loaded on first boot with blank NVS (Bug C-02 fix).

**Precondition:** State = BLANK_NVS (flash with erase all).

**Steps:**
1. Flash production firmware with "Erase Flash → All Flash Contents".
2. Power on. Wait 30s for full initialization.
3. Read `/pump_system/config/device` from Firebase.
4. Verify each field matches documented defaults:
   - `dry_run_threshold_lpm: 1.0`
   - `dry_run_timeout_sec: 30`
   - `pump_start_level: 20`
   - `pump_stop_level: 90`
   - `max_pump_runtime_min: 120`
5. Read `/pump_system/status`.
6. Verify `run_mode: AUTO_STANDBY` (not "OFF") — Bug M-05 fix.

**Expected result:** All defaults loaded from compile-time constants; run_mode initialized to AUTO_STANDBY.

---

### SF-FW-016 [CRITICAL] [DRY-RUN-SAFE] — water_level_percent Omitted Before First Valid Frame

**Objective:** Verify Firebase does not receive `water_level_percent: 0` on boot before the first valid RS-485 frame (Bug C-02 fix).

**Steps:**
1. State = BLANK_NVS or COLD_BOOT with CAT6 DISCONNECTED.
2. Power on ESP32. Start monitoring Firebase `/pump_system/status` in real time.
3. For the first 30s (before sensor stable):
4. Verify `water_level_percent` is ABSENT from the status payload (not present, not 0).
5. Reconnect CAT6. Confirm `water_level_percent` appears after first valid RS-485 frame.

**Expected result:** `water_level_percent` field absent until valid sensor data received.

---

### SF-FW-017 [HIGH] [DRY-RUN-SAFE] — ISR Safety: Flow Pulses Read Atomically

**Objective:** Verify the flow pulse counter ISR is protected against torn reads.

**Steps:**
1. Enable debug mode (`SMARTFLOW_DEBUG` or `gLogLevel = LOG_DEBUG`).
2. Run pump with normal water flow.
3. Observe Serial output: flow rate readings should be smooth, not showing occasional impossible spikes (e.g., sudden 100 LPM then normal).
4. Run for 30 minutes. Log all flow rate readings to file.
5. Compute standard deviation. Flag any reading > 3σ from mean as a potential ISR race condition artifact.

**Expected result:** Flow readings smooth and within ±15% of actual; no impossible spikes.

---

### SF-FW-018 [HIGH] [DRY-RUN-SAFE] — Sensor Comm Loss: Pump Fails Safe

**Objective:** Verify pump stops when RS-485 communication is lost while running in AUTO mode.

**Precondition:** PUMP_RUNNING in AUTO, CAT6_CONNECTED.

**Steps:**
1. While pump is running in AUTO mode: disconnect CAT6.
2. Observe: `remote_sensor_stable` becomes false.
3. Observe: within `REMOTE_SENSOR_OFFLINE_MS` timeout (verify value in firmware, expected ≤ 10s):
4. Pump should stop (`is_running: false`).
5. Reconnect CAT6. Confirm system recovers to normal operation within 15s.

**Expected result:** Pump stops on sensor comm loss, recovers automatically on reconnect.

---

### SF-FW-019 [HIGH] [DRY-RUN-SAFE] — Bypass Level Sensor: Flow Guard Only

**Objective:** Verify that when level sensor is bypassed, pump operation is governed by flow guard only.

**Steps:**
1. Set `bypass_level_sensor: true` via dashboard.
2. Confirm `bypass_level_sensor: true` in Firebase status.
3. With TANK_FULL: confirm pump CAN start in AUTO when bypass is active (tank level ignored).
4. With flow sensor functional: dry-run protection still fires if flow drops below threshold.
5. Set `bypass_level_sensor: false`. Confirm pump respects tank level thresholds again.

**Expected result:** Level bypass fully disables tank level control; flow protection remains.

---

### SF-FW-020 [HIGH] [DRY-RUN-SAFE] — Bypass Flow Sensor: Level Guard Only

**Objective:** Verify that when flow sensor is bypassed, dry-run protection is disabled.

**⚠ Warning:** This test bypasses dry-run protection. Only run with water present in the pump.

**Steps:**
1. Set `bypass_flow_sensor: true` via dashboard.
2. Confirm `bypass_flow_sensor: true` in Firebase status.
3. With FLOW_BLOCKED: verify pump does NOT trigger DRY_RUN fault.
4. `is_error` must remain `false` despite no flow.
5. Set `bypass_flow_sensor: false`. Repeat FLOW_BLOCKED test. Verify DRY_RUN fires correctly.

**Expected result:** Flow bypass disables dry-run protection; re-enabling restores it.

---

### SF-FW-021 [CRITICAL] [DRY-RUN-SAFE] — Crash Loop Detection

**Objective:** Verify repeated crash/restart cycles are detected and the system enters safe mode.

**Steps:**
1. Note current `crash_loop_count` in NVS (read from Serial log on boot).
2. Force a simulated crash by triggering a watchdog reset (or temporarily using `ESP.restart()` in a test build).
3. Repeat 3-4 times within the crash detection window.
4. Observe: system should log SAFE_MODE detection.
5. In safe mode: pump should NOT start automatically.

**Expected result:** Crash loop detected; safe mode entered; pump stays off.

---

### SF-FW-022 [HIGH] [DRY-RUN-SAFE] — Idle Mode Activation

**Objective:** Verify idle mode activates when tank is full and pump has been off for the idle activation period.

**Steps:**
1. Fill tank to 100%.
2. Confirm pump stops (AUTO stop condition met).
3. Wait for idle activation period (verify `idle_activation_timeout_ms` in firmware).
4. Confirm `is_idle_mode: true` in Firebase status.
5. Confirm sensor and Firebase poll intervals increase (observable in Serial log or Firebase update frequency).

**Expected result:** Idle mode activates when conditions met; poll frequency reduces.

---

### SF-FW-023 [CRITICAL] [DRY-RUN-SAFE] — NVS Config Persistence Across Restart

**Objective:** Verify settings changes persist across ESP32 power cycles.

**Steps:**
1. Change `pump_start_level` to 25% via settings page.
2. Save settings. Confirm `pump_start_level: 25` in Firebase config.
3. Power cycle the ESP32.
4. After reboot, read `/pump_system/config/device` from Firebase.
5. Confirm `pump_start_level: 25` (loaded from NVS, not default).

**Expected result:** Changed settings survive power cycle.

---

### SF-FW-024 [HIGH] [DRY-RUN-SAFE] — run_mode Initialized to AUTO_STANDBY

**Objective:** Verify first Firebase push after boot shows AUTO_STANDBY, not OFF (Bug M-05 fix).

**Steps:**
1. State = COLD_BOOT.
2. Monitor Firebase in real time from boot.
3. First `run_mode` value received after boot: must be `AUTO_STANDBY`, not `OFF`.

**Expected result:** `run_mode: AUTO_STANDBY` on first push.

---

## 8. Layer 5 — Firebase Integration Tests

### SF-FB-001 [CRITICAL] [DRY-RUN-SAFE] — ESP32 Status Push Frequency

**Objective:** Verify status updates arrive at Firebase every 3 seconds (±0.5s tolerance).

**Steps:**
1. Monitor `/pump_system/status` in Firebase console or Realtime Database client.
2. Record timestamp of 20 consecutive status updates.
3. Compute interval between each consecutive pair.

**Expected result:** Mean interval = 3.0s; no interval > 4.0s; no interval < 2.0s.

---

### SF-FB-002 [CRITICAL] [DRY-RUN-SAFE] — Control Path Latency: Dashboard to Pump

**Objective:** Measure end-to-end latency from dashboard write to pump state change.

**Steps:**
1. Pump stopped. Mode = MANUAL.
2. Use stopwatch: simultaneously write `manual_desired: true` and start timer.
3. Stop timer when relay audibly clicks (or `is_running: true` appears in Firebase).
4. Repeat 5 times. Record all latencies.

**Expected result:**
- P50 (median) latency: ≤ 4s
- P95 latency: ≤ 6s
- P100 (max observed): ≤ 9s (3 Firebase poll cycles max)
- No instance > 9s

---

### SF-FB-003 [HIGH] [DRY-RUN-SAFE] — Firebase Config Poll: Changes Applied Within 30s

**Objective:** Verify firmware reads and applies config changes within 30 seconds of dashboard save.

**Steps:**
1. Change `pump_start_level` from 20 to 30 via settings.
2. Start stopwatch at save click.
3. Monitor Serial log (debug mode) for config read confirmation.
4. Stop timer when new value applied (Serial shows new threshold).

**Expected result:** Config change applied within 30s.

---

### SF-FB-004 [HIGH] [DRY-RUN-SAFE] — Firebase Write Failure: Graceful Degradation

**Objective:** Verify system continues operating when Firebase writes fail.

**Steps:**
1. Block Firebase API calls by temporarily disabling WiFi on the router (or changing router password).
2. Observe system behavior:
   - Pump logic should continue executing based on RS-485 sensor data.
   - Firebase `consecutive_failures` counter should increment in Serial log.
   - System should NOT crash or enter undefined state.
3. Restore WiFi. Confirm system reconnects and resumes Firebase writes.
4. Confirm exponential backoff: write intervals should increase on consecutive failures.

**Expected result:** Pump logic unaffected by Firebase outage; exponential backoff applied; auto-recovery on reconnect.

---

### SF-FB-005 [CRITICAL] [DRY-RUN-SAFE] — One-Shot Control Fields Reset

**Objective:** Verify one-shot control fields (`emergency_stop`, `clear_error`, `reset_stop`, `countdown_start`) are reset to `false` by firmware after processing.

**Steps for each field:**
1. Write `emergency_stop: true` to Firebase.
2. Wait ≤ 6s (2 poll cycles).
3. Read `/pump_system/control/emergency_stop` from Firebase.
4. Confirm value is now `false` (firmware reset it after processing).

**Repeat for:** `clear_error`, `reset_stop`, `countdown_start`, `countdown_add_time`.

**Expected result:** Each one-shot field auto-resets to `false` after firmware processes it.

---

### SF-FB-006 [CRITICAL] [DRY-RUN-SAFE] — debug_log_level Remote Control

**Objective:** Verify log level set from dashboard actually changes firmware behavior.

**Steps:**
1. Set `debug_log_level: 3` (DEBUG) via diagnostics panel.
2. Wait ≤ 30s (config poll cycle).
3. Observe Serial Monitor: `[D]` level messages should appear.
4. Confirm `status.debug_log_level: 3` in Firebase.
5. Set `debug_log_level: 2` (INFO). Wait 30s.
6. Confirm `[D]` messages disappear from Serial output.

**Expected result:** Log level change reflected in both Firebase status and Serial behavior.

---

### SF-FB-007 [HIGH] [DRY-RUN-SAFE] — Firebase Authentication: Unauthorized Write Rejected

**Objective:** Verify Firebase security rules reject writes from unauthenticated clients.

**Steps:**
1. In a browser console (not logged into the dashboard): attempt to write directly to `/pump_system/control/emergency_stop: true` using the Firebase JS SDK with no auth token.
2. Check Firebase console security rules test or browser console for permission denied error.

**Expected result:** Write rejected with PERMISSION_DENIED; pump unaffected.

---

## 9. Layer 6 — Dashboard Functional Tests

### SF-DB-001 [CRITICAL] [DRY-RUN-SAFE] — Cold Load: Data Populates Within 5s

**Objective:** Verify dashboard loads data and replaces all skeleton loaders within 5s on a stable connection.

**Steps:**
1. Open dashboard in fresh browser tab (clear cache + cookies).
2. Start timer at URL entry.
3. Observe all skeleton loaders; stop timer when all replaced with real data.

**Expected result:** All skeleton loaders replaced within 5s on ≥ 3G connection.

---

### SF-DB-002 [CRITICAL] [DRY-RUN-SAFE] — Offline Banner: Appears on Disconnect, Disappears on Reconnect

**Objective:** Verify offline banner behavior.

**Steps:**
1. Dashboard open with live data.
2. Disable device WiFi.
3. Within 10s: amber offline banner should appear ("Reconnecting to SmartFlow...").
4. Last-known data should remain visible (not blank).
5. Re-enable WiFi.
6. Banner should disappear within 10s of reconnect.

**Expected result:** Banner appears promptly; data preserved; banner clears on reconnect.

---

### SF-DB-003 [CRITICAL] [SAFETY] — Emergency Stop: Confirmation Required, Then Fires

**Objective:** Verify E-stop requires confirmation before sending to Firebase, and fires correctly.

**Steps:**
1. Click Emergency Stop button.
2. Confirm: inline confirmation popover appears ("Stop the pump now? [Stop] [Cancel]").
3. Click Cancel. Confirm: no Firebase write occurred (pump state unchanged).
4. Click Emergency Stop again → Confirm: click Stop.
5. Confirm: `emergency_stop: true` written to Firebase, pump stops within 6s.

**Expected result:** Confirmation required; Cancel prevents write; Confirm fires correctly.

---

### SF-DB-004 [CRITICAL] [SAFETY] — Mode Selector: Pending State Blocks Double-Click

**Objective:** Verify mode changes have a pending state that prevents double-firing.

**Steps:**
1. Click MANUAL in mode selector.
2. Immediately click AUTO.
3. Confirm only one Firebase write occurred (pending state blocked the second click).
4. Confirm no race condition: final mode reflects last intended mode.

**Expected result:** Controls disabled during write; no double-write.

---

### SF-DB-005 [HIGH] [DRY-RUN-SAFE] — Settings: Invalid Values Blocked with Inline Errors

**Objective:** Verify all validation rules are enforced with inline error messages.

**Test each invalid case:**

| Case | Input | Expected error |
|------|-------|----------------|
| Start ≥ stop | start=90, stop=20 | "Start level must be less than stop level" |
| Tank full ≥ empty | full=200, empty=10 | "Full distance must be less than empty distance" |
| DRY_RUN threshold too low | 0.05 | "Dry-run threshold must be 0.1–10.0 LPM" |
| DRY_RUN timeout too short | 5 | "Dry-run timeout must be 10–300 seconds" |
| Max runtime too short | 10 | "Max runtime must be 30–480 minutes" |
| Empty required field | clear pump_start | Error appears on that specific field |

**For each case:**
1. Enter invalid value.
2. Click Save.
3. Confirm inline error appears next to the specific field.
4. Confirm no Firebase write occurred (settings not saved).

---

### SF-DB-006 [HIGH] [DRY-RUN-SAFE] — Settings: Success Message After Valid Save

**Steps:**
1. Enter valid settings.
2. Click Save.
3. Confirm "Settings saved. Firmware will apply within 30 seconds." message appears.
4. Confirm message disappears after 5s.

---

### SF-DB-007 [HIGH] [DRY-RUN-SAFE] — Cooldown Countdown: Client-Side Smooth Decrement

**Objective:** Verify cooldown countdown decrements smoothly (not Firebase poll rate jitter).

**Steps:**
1. Trigger a cooldown state (pump stops after AUTO cycle, off-timer active).
2. Observe the cooldown chip (e.g., "AUTO — Cooldown 47s").
3. Confirm countdown decrements by 1 every second, not jumping by 3 every 3s.

**Expected result:** Smooth 1-second decrement using client-side `setInterval`.

---

### SF-DB-008 [HIGH] [DRY-RUN-SAFE] — Idle Mode Badge: Appears and Disappears

**Steps:**
1. Fill tank to 100%, ensure pump is off.
2. Wait for idle mode activation.
3. Confirm "Idle" badge appears in header.
4. Drain tank below start level. Pump starts. Confirm idle badge disappears.

---

### SF-DB-009 [HIGH] [DRY-RUN-SAFE] — Alert Priority Order

**Objective:** Verify alerts appear in correct priority order (emergency_stop > is_error > sensor errors > warnings).

**Steps:**
1. Trigger multiple alerts simultaneously: E-stop latched + DRY_RUN error + sensor error.
2. Verify E-stop alert appears at top.
3. Verify DRY_RUN error below E-stop.
4. Verify sensor error below DRY_RUN.

---

### SF-DB-010 [HIGH] [DRY-RUN-SAFE] — Firebase Listener Memory Leak Test

**Objective:** Verify no memory leaks from Firebase listeners on component mount/unmount cycles.

**Steps:**
1. Open dashboard in Chrome.
2. Open DevTools → Memory tab.
3. Take heap snapshot (Snapshot 1).
4. Navigate to /settings and back to / ten times (triggers component unmount/remount).
5. Take heap snapshot (Snapshot 2).
6. Compare: heap size should not grow significantly (< 5MB growth is acceptable).

**Expected result:** No unbounded heap growth from listener leaks.

---

## 10. Layer 7 — End-to-End System Tests

### SF-E2E-001 [CRITICAL] [SAFETY] — Full AUTO Cycle: Empty to Full

**Objective:** Verify complete automatic fill cycle works end-to-end.

**Steps:**
1. Set mode to AUTO. Drain tank below 20% (start level).
2. Observe from dashboard:
   - Pump starts within 3s of sensor stabilization.
   - Level percentage climbs correctly as tank fills.
   - Pump stops automatically when level reaches 90%.
   - `run_mode` transitions: AUTO → AUTO (running) → AUTO_COOLDOWN → AUTO_STANDBY.
3. Time total fill cycle. Verify reasonable fill rate.

**Expected result:** Complete autonomous fill cycle with no manual intervention.

---

### SF-E2E-002 [CRITICAL] [SAFETY] — Complete MANUAL Cycle

**Objective:** Verify complete manual control flow: switch to MANUAL, start, stop, switch back to AUTO.

**Steps:**
1. Mode = AUTO, pump stopped.
2. Switch to MANUAL from dashboard.
3. Write manual_desired: true. Confirm pump starts.
4. Run for 60s. Monitor flow rate and level in dashboard.
5. Write manual_desired: false. Confirm pump stops.
6. Switch back to AUTO. Confirm mode change.
7. Verify AUTO resumes normal operation (starts when below start level).

**Expected result:** Each step executes cleanly with correct Firebase state at each stage.

---

### SF-E2E-003 [HIGH] [SAFETY] — Countdown: Set, Run, Revert

**Steps:**
1. Set COUNTDOWN mode, 3 minutes.
2. Start countdown. Observe pump starts, countdown chip active.
3. Extend countdown by +2 min using add-time function.
4. Total expected runtime: 5 minutes.
5. Verify auto-revert to AUTO_STANDBY at end.

---

### SF-E2E-004 [CRITICAL] [SAFETY] — E-Stop → Reset → Resume AUTO

**Steps:**
1. AUTO mode, pump running.
2. Trigger E-stop from dashboard (with confirmation).
3. Verify pump stops, `run_mode: STOPPED`.
4. Attempt mode change — confirm blocked.
5. Click Reset (with confirmation).
6. Verify system resumes AUTO mode normally.

---

### SF-E2E-005 [HIGH] [DRY-RUN-SAFE] — Full Boot with Existing Config

**Objective:** Verify warm boot preserves all settings and resumes operation correctly.

**Steps:**
1. Set non-default values: start=25%, stop=85%, dry_run_timeout=45s.
2. Power cycle ESP32.
3. After boot, verify all custom values persist in Firebase config.
4. Verify mode returns to AUTO_STANDBY (not OFF).

---

---

## 11. Layer 8 — Safety & Fault Injection Tests

> These tests deliberately create failure conditions. Execute with extreme care.
> Always have the MCB accessible. Monitor TOR and motor temperature.

### SF-SAF-001 [CRITICAL] [SAFETY] — Dry-Run Lockout: Timed to Spec

**Objective:** Verify dry-run triggers at exactly `dry_run_timeout_sec` after flow drops below threshold.

**Steps:**
1. Set `dry_run_timeout_sec = 30` (default). PUMP_RUNNING in AUTO.
2. Close valve. Start stopwatch at valve close.
3. Measure time to: `is_error: true` appears in Firebase.
4. Expected: 30s ± (poll_interval × 2) = 30 ± 6s.
5. Verify relay de-activates within 1s of `is_error` set.

**Expected result:** Fault triggers at 30s ±6s; relay off within 1s of fault.

---

### SF-SAF-002 [CRITICAL] [SAFETY] — Dry-Run: Flow Must Be Below Threshold Continuously

**Objective:** Verify dry-run timer RESETS if flow recovers above threshold mid-count.

**Steps:**
1. Pump running. Briefly block flow for 15s (half the timeout), then unblock for 5s, then block again.
2. Total blocked time would exceed timeout if accumulative, but the timer should reset on recovery.
3. Confirm: `is_error` does NOT fire after first 15s block.
4. After unblocking and re-blocking: timer restarts from 0.

**Expected result:** Dry-run timer is not accumulative — resets on flow recovery.

---

### SF-SAF-003 [CRITICAL] [SAFETY] — Watchdog Reset Recovery

**Objective:** Verify system recovers gracefully from a watchdog timer reset.

**Steps:**
1. Note `last_boot_reason` in Firebase (should be "Power-on").
2. Force a watchdog reset (e.g., insert a long `delay()` in a test build, or use `ESP.restart()` as proxy).
3. After reboot: verify `last_boot_reason: "Task watchdog"` or equivalent in Firebase.
4. Verify system boots cleanly, returns to AUTO_STANDBY.
5. Verify crash_loop_count incremented in NVS (visible in Serial log).

**Expected result:** Watchdog reset detected, system recovers, boot reason reported.

---

### SF-SAF-004 [HIGH] [SAFETY] — Sensor Node Power Loss Mid-Run

**Objective:** Verify system response when NodeMCU loses power while pump is running.

**Steps:**
1. Pump running in AUTO.
2. Unplug NodeMCU power (simulate sensor node failure).
3. Observe: `remote_sensor_stable` becomes false within 10s.
4. Observe: pump stops (fails safe) within REMOTE_SENSOR_OFFLINE_MS.
5. Restore NodeMCU power. Confirm system recovers.

**Expected result:** Pump stops on sensor node failure; auto-recovers when node returns.

---

### SF-SAF-005 [HIGH] [SAFETY] — WiFi Loss During Pump Run

**Objective:** Verify pump continues running correctly (local logic unaffected) during WiFi outage.

**Steps:**
1. Pump running in AUTO.
2. Disable WiFi on router.
3. Observe: Firebase updates stop. Firebase shows last known state (stale).
4. Observe: pump CONTINUES running based on local RS-485 sensor data.
5. Observe: pump STOPS correctly when level reaches stop threshold (local logic).
6. Restore WiFi. Confirm Firebase resumes updates.

**Expected result:** Pump logic operates independently of Firebase/WiFi; resumes reporting on reconnect.

---

### SF-SAF-006 [HIGH] [SAFETY] — TOR Trip Simulation

**Objective:** Verify the TOR trips independently and firmware detects the pump stopped (relay still on, but pump off).

**Note:** This test verifies that TOR acts as a final hardware guard. Actual TOR trip requires sustained overcurrent — simulate by briefly disconnecting the pump motor leads after the contactor, which simulates "contactor energized but no motor" scenario.

**Observable:** After TOR trips, relay remains ON (firmware hasn't changed it), but pump motor is off. The flow sensor drops to zero. Firmware should detect dry-run condition and eventually lock out.

**Expected result:** TOR protection operates independently of firmware; firmware detects flow loss and latches DRY_RUN.

---

### SF-SAF-007 [HIGH] [DRY-RUN-SAFE] — Factory Reset via Blank NVS Does Not Break Safety

**Objective:** Verify that after a full NVS erase (factory reset), all safety defaults are loaded and the system is safe.

**Precondition:** State = BLANK_NVS.

**Steps:**
1. Flash with full erase.
2. Boot. Verify no crash, no error state.
3. Verify dry_run_threshold_lpm = 1.0 (safe default).
4. Verify dry_run_timeout_sec ≥ 10.
5. Verify pump does not start with `water_level_percent` unknown (C-02 fix).

**Expected result:** Safe defaults loaded; pump stays off until valid sensor data received.

---

## 12. Layer 9 — Edge Case & Boundary Tests

### SF-EDG-001 [HIGH] [DRY-RUN-SAFE] — Level Exactly at Start Level Boundary (20%)

**Objective:** Verify pump behavior when level is exactly equal to start threshold.

**Engineering note:** Integer comparison. Level of exactly 20% with pump_start_level = 20: should pump start or not? Verify the firmware uses `<=` (start if level ≤ start) or `<` (start if level < start). Document the actual behavior — it must be consistent and documented.

**Steps:**
1. Physically position water level to exactly 20% (verify with sensor reading).
2. In AUTO mode: observe if pump starts.
3. Repeat at 19%, 21% to bracket the boundary.

**Expected result:** Consistent behavior at boundary, documented in test record. Typically: pump starts at ≤ 20%.

---

### SF-EDG-002 [HIGH] [DRY-RUN-SAFE] — Level Exactly at Stop Level Boundary (90%)

**Same as SF-EDG-001 but for the stop threshold.**

**Expected result:** Consistent behavior at 90% boundary. Typically: pump stops at ≥ 90%.

---

### SF-EDG-003 [HIGH] [DRY-RUN-SAFE] — Start Level = Stop Level (misconfiguration)

**Objective:** Verify system does not crash or behave unpredictably when start == stop level.

**Note:** The dashboard should prevent this, but test firmware behavior if it occurs (e.g., via direct Firebase write).

**Steps:**
1. Directly write `pump_start_level: 50, pump_stop_level: 50` to Firebase config.
2. Wait 30s for firmware to read config.
3. Observe system behavior: should either refuse the config or default to safe behavior.
4. Pump must NOT enter an infinite start/stop loop.

**Expected result:** System either rejects config or fails safe. No pump oscillation.

---

### SF-EDG-004 [HIGH] [DRY-RUN-SAFE] — Firebase Data Stale: Level > 2500ms Old

**Objective:** Verify `level_fresh: false` is reported when sensor data ages beyond the staleness threshold.

**Steps:**
1. CAT6_DISCONNECTED. Wait 10s.
2. Confirm `level_fresh: false` in Firebase status.
3. Confirm `remote_sensor_stable: false`.
4. Reconnect CAT6. Confirm both flags return to true within 15s.

**Expected result:** Staleness correctly detected and reported.

---

### SF-EDG-005 [MEDIUM] [DRY-RUN-SAFE] — Rapid Mode Switching

**Objective:** Verify no state corruption when mode is switched rapidly.

**Steps:**
1. In 10 seconds, cycle: AUTO → MANUAL → COUNTDOWN → AUTO → MANUAL → AUTO.
2. After cycling: verify system is in consistent state (final mode matches last write).
3. Pump behavior must be predictable — no ghost states.

**Expected result:** Final state consistent with last mode written; no state machine corruption.

---

### SF-EDG-006 [HIGH] [DRY-RUN-SAFE] — Flow Rate at Sensor Minimum (1.0 LPM)

**Objective:** Verify dry-run detection at exactly the threshold.

**Engineering basis:** YF-G1 minimum working range is 1.0 LPM. Below this, readings are unreliable.

**Steps:**
1. Throttle flow to approximately 1.0 LPM using valve (verify with bucket measurement).
2. Run pump. Observe dry-run timer behavior.
3. At exactly 1.0 LPM, the timer should NOT trigger (firmware: `flow < threshold`, not `≤`).
4. Reduce to 0.9 LPM. Confirm timer starts.

**Expected result:** Dry-run timer activates at < 1.0 LPM, not at = 1.0 LPM.

---

### SF-EDG-007 [MEDIUM] [DRY-RUN-SAFE] — SEQ Counter Rollover (255 → 0)

**Objective:** Verify ESP32 handles SEQ wraparound without treating 0 as a discontinuity.

**Steps:**
1. Monitor RS-485 traffic across the SEQ=255 → SEQ=0 boundary.
2. Confirm no false sequence error logged at the rollover.
3. Confirm data continues to be parsed and accepted normally.

---

### SF-EDG-008 [MEDIUM] [DRY-RUN-SAFE] — Maximum Countdown Duration

**Objective:** Verify system correctly handles maximum allowed countdown (120 minutes).

**Steps:**
1. Set countdown_duration_min = 120 (max allowed).
2. Start countdown. Confirm `run_mode: COUNTDOWN`.
3. Do NOT wait full duration — verify countdown_remaining_sec starts at ~7200.
4. Cancel countdown by switching to AUTO mode.

**Expected result:** Max value accepted; countdown started; mode switch cancels cleanly.

---

### SF-EDG-009 [HIGH] [DRY-RUN-SAFE] — Heap Stability Over Time

**Objective:** Verify free heap does not decrease monotonically over 24 hours (memory leak check).

**Steps:**
1. Record `free_heap_bytes` at boot (baseline).
2. Record at 1h, 4h, 8h, 24h intervals from Firebase status.
3. Plot values. Confirm no steady downward trend.

**Pass criteria:** `free_heap_bytes` at 24h is within 5KB of boot value.
**Fail criteria:** Steady decrease > 1KB/hour (indicates leak).

---

### SF-EDG-010 [MEDIUM] [DRY-RUN-SAFE] — Simultaneous Dashboard + Firmware Firebase Access

**Objective:** Verify no race conditions when dashboard writes control simultaneously with firmware reads.

**Steps:**
1. During a rapid pump start/stop sequence, simultaneously change mode from dashboard.
2. Open two browser tabs with the dashboard. In rapid succession, write conflicting modes.
3. Observe: final Firebase state should be consistent with one of the writes (Firebase Last-Write-Wins).
4. Pump state should converge to consistent state within 6s.

**Expected result:** No undefined pump state; system converges within 2 poll cycles.

---

## 13. Layer 10 — Security Tests

> These tests verify the system resists unauthorized access and data manipulation.
> Reference: OWASP IoT Top 10, IEC 62443-4-2 (IoT Security)

### SF-SEC-001 [CRITICAL] — Firebase Auth: Dashboard Requires Google Login

**Objective:** Verify unauthenticated users cannot access the dashboard.

**Steps:**
1. Open dashboard in fresh browser (no cached auth).
2. Confirm redirect to `/login` page (or Google OAuth prompt).
3. Attempt to access `/` directly without logging in.
4. Confirm redirect occurs.

---

### SF-SEC-002 [CRITICAL] — Firebase Rules: Unauthorized Control Write Rejected

**Objective:** Verify only authenticated users (with correct UID) can write to `/pump_system/control/`.

**Steps:**
1. In browser DevTools console, with no auth token, attempt:
   ```javascript
   firebase.database().ref('/pump_system/control/emergency_stop').set(true);
   ```
2. Expected: PERMISSION_DENIED error.
3. With wrong Google account (not the authorized UID): attempt same write.
4. Expected: PERMISSION_DENIED.

**Expected result:** Both unauthorized attempts rejected. Pump unaffected.

---

### SF-SEC-003 [CRITICAL] — ESP32 Credentials: secrets.h Not in Repository

**Objective:** Verify WiFi and Firebase credentials are not committed to the git repository.

**Steps:**
1. Run: `git log --all --full-history -- firmware/*/secrets.h`
2. Expected: no results (file was never committed).
3. Run: `grep -r "WIFI_SSID\|FIREBASE_API_KEY\|password" .git/` (searches commit history).
4. Expected: no credentials found in git history.

**Expected result:** Zero credential exposure in repository history.

---

### SF-SEC-004 [HIGH] — Firebase Rules: ESP32 Cannot Write to Control Path

**Objective:** Verify the ESP32 Firebase service account cannot write to `/pump_system/control/` (it should only write to `/status/`).

**Steps:**
1. Review `database.rules.json`. Verify ESP32 UID has write permission to `/pump_system/status/` only.
2. Simulate: attempt to write to `/pump_system/control/mode` with the ESP32 auth token.
3. Expected: PERMISSION_DENIED.

**Expected result:** ESP32 write to control path rejected.

---

### SF-SEC-005 [HIGH] — Dashboard: No Sensitive Data in Browser Local Storage

**Objective:** Verify Firebase auth tokens and API keys are handled securely.

**Steps:**
1. Open dashboard → DevTools → Application → Local Storage.
2. Confirm no raw API keys, Firebase service account credentials, or auth tokens stored in readable form.
3. (Note: Firebase SDK stores auth tokens in IndexedDB internally — this is acceptable as it's encrypted by the browser.)

**Expected result:** No plaintext credentials in Local Storage.

---

### SF-SEC-006 [HIGH] — HTTPS Enforcement

**Objective:** Verify dashboard is only served over HTTPS (required for PWA and Firebase Auth).

**Steps:**
1. Attempt to access dashboard over HTTP: `http://your-vercel-url.vercel.app`.
2. Confirm automatic redirect to HTTPS.
3. Verify TLS certificate is valid and not expired.

---

### SF-SEC-007 [MEDIUM] — Content Security Policy Headers

**Objective:** Verify dashboard has basic CSP headers to prevent XSS.

**Steps:**
1. In browser DevTools → Network → Headers for the dashboard main document.
2. Check for `Content-Security-Policy` header.
3. Verify it does not contain `unsafe-inline` or `unsafe-eval` in script-src (if present).

---

### SF-SEC-008 [MEDIUM] — Firebase Data Validation: Integer Overflow Prevention

**Objective:** Verify firmware does not crash when Firebase config fields contain extreme values.

**Steps:**
1. Write extreme but valid-looking values directly to Firebase config:
   - `pump_start_level: 999`
   - `dry_run_timeout_sec: -1`
   - `max_pump_runtime_min: 0`
2. Wait 30s for firmware to read config.
3. Confirm firmware either clamps to valid range or ignores invalid values.
4. Confirm firmware does NOT crash.

**Expected result:** Invalid config values clamped or ignored; no firmware crash.

---

## 14. Layer 11 — Reliability & Soak Tests

> These tests require extended time periods. Run them in parallel with other work.

### SF-REL-001 [CRITICAL] [SAFETY] — 24-Hour Continuous AUTO Soak

**Objective:** Verify system operates continuously for 24 hours without crash, memory leak, or logic error.

**Monitoring:**
- `uptime_minutes` must reach 1440 (24h × 60min)
- `free_heap_bytes` trend: < 5KB total decrease over 24h
- `firebase_consecutive_failures`: must stay < 3 on average
- `ultrasonic_cycles_timeout / ultrasonic_cycles_ok`: ratio must be < 5%
- Pump must complete at least 3 full fill cycles
- No `is_error: true` without physical cause
- No watchdog resets (`last_boot_reason` unchanged)

**Pass criteria:** All monitoring criteria met at 24h mark.

---

### SF-REL-002 [HIGH] — WiFi Reconnection After Router Restart

**Objective:** Verify system reconnects automatically after WiFi infrastructure restarts.

**Steps:**
1. System running normally. Power cycle the router.
2. Router restart time: typically 60–90s.
3. Observe: ESP32 attempts reconnection (Serial log), eventually reconnects.
4. Expected: reconnected within 120s of router becoming available.
5. Verify `firebase_consecutive_failures` resets after reconnect.
6. Verify Firebase updates resume.

**Expected result:** Auto-reconnect within 2 minutes; no manual intervention required.

---

### SF-REL-003 [HIGH] — RS-485 Reliability Over 8 Hours

**Objective:** Verify RS-485 communication remains reliable over an extended period.

**Monitoring over 8 hours:**
- `ultrasonic_cycles_ok / (ultrasonic_cycles_ok + ultrasonic_cycles_timeout)` > 95%
- CRC error rate < 5%
- No `remote_sensor_stable: false` events without physical cause

**Expected result:** ≥ 95% RS-485 frame success rate over 8 hours.

---

### SF-REL-004 [MEDIUM] — 10 Power Cycles: Boot Reliability

**Objective:** Verify reliable boot sequence across 10 cold power cycles.

**Steps:**
1. Power cycle ESP32 + NodeMCU simultaneously, 10 times with 30s between cycles.
2. For each boot: record time to Firebase connected, sensor stable, first valid status push.
3. Verify no boot takes > 90s.
4. Verify no boot results in error state without physical cause.

**Expected result:** 10/10 clean boots within 90s each.

---

### SF-REL-005 [HIGH] — Long-Run Pump Cycle Count

**Objective:** Verify pump cycle counters are accurately maintained across power cycles.

**Steps:**
1. Note `total_pump_cycles` at start.
2. Run 10 complete pump cycles (fill tank from 20% to 90% each time).
3. Power cycle ESP32.
4. Verify `total_pump_cycles` incremented by exactly 10 and survived the power cycle.

**Expected result:** Accurate cycle count, NVS-persisted across reboots.

---

## 15. Layer 12 — Accessibility & PWA Tests

### SF-PWA-001 [HIGH] — Lighthouse Accessibility Score ≥ 95

**Steps:**
1. Open deployed dashboard in Chrome.
2. DevTools → Lighthouse → Run audit (Mobile preset, clear storage).
3. Record Accessibility score.

**Expected result:** ≥ 95.

---

### SF-PWA-002 [HIGH] — Lighthouse PWA Score ≥ 80

**Expected result:** ≥ 80. Verify installable, HTTPS, service worker, manifest.

---

### SF-PWA-003 [CRITICAL] — Emergency Stop: Visible on 375px Without Scrolling

**Steps:**
1. Chrome DevTools → Device toolbar → 375×667 (iPhone SE).
2. Open dashboard.
3. Confirm Emergency Stop button or mobile-pinned E-stop bar is visible without scrolling.

**Expected result:** E-stop visible without any scrolling on 375px viewport.

---

### SF-PWA-004 [HIGH] — Keyboard Navigation: All Controls Reachable

**Steps:**
1. Open dashboard. Click outside any interactive element.
2. Press Tab repeatedly. Confirm every interactive element receives visible focus ring.
3. Verify: Mode selector, E-stop button, countdown input, settings fields all reachable by keyboard.
4. Verify: No keyboard trap (pressing Tab always advances focus, never gets stuck).

---

### SF-PWA-005 [HIGH] — PWA Install on Android Chrome

**Steps:**
1. Open dashboard on Android Chrome over HTTPS.
2. Confirm "Add to Home Screen" banner or install prompt appears.
3. Install the PWA.
4. Open from home screen. Confirm it opens without browser chrome (standalone mode).
5. Confirm theme_color (#185FA5) applied to status bar.

---

### SF-PWA-006 [HIGH] — ARIA Labels on Safety Controls

**Steps:**
1. Inspect E-stop button in DevTools. Confirm `aria-label` includes description of action.
2. Inspect mode selector. Confirm buttons/segments have descriptive labels.
3. Inspect bypass toggles. Confirm each has aria-label explaining what it bypasses.

---

### SF-PWA-007 [MEDIUM] — Screen Reader Announcements on Pump State Change

**Steps:**
1. Enable VoiceOver (iOS) or TalkBack (Android) or NVDA (Windows).
2. Trigger a pump start from AUTO mode.
3. Confirm screen reader announces the state change (from `role="status" aria-live="polite"` region).

---

## 16. Regression Test Suite

The following tests must be re-run after every code change to firmware or dashboard.
They represent the minimum set to confirm no regression.

| Test ID | Name | Layer |
|---------|------|-------|
| SF-FW-001 | AUTO start when tank low | Firmware |
| SF-FW-002 | AUTO stop when tank full | Firmware |
| SF-FW-004 | MANUAL start on manual_desired true | Firmware |
| SF-FW-005 | MANUAL stop on manual_desired false | Firmware |
| SF-FW-007 | Dry-run fires in MANUAL mode | Firmware |
| SF-FW-008 | E-stop from any mode | Firmware |
| SF-FW-009 | E-stop latch survives mode change | Firmware |
| SF-FW-010 | DRY_RUN latch survives mode change | Firmware |
| SF-FW-015 | NVS first boot defaults | Firmware |
| SF-FW-016 | water_level_percent absent before first frame | Firmware |
| SF-RS-001 | CRC integrity | Protocol |
| SF-FB-001 | Status push frequency | Firebase |
| SF-FB-005 | One-shot fields reset | Firebase |
| SF-DB-001 | Cold load < 5s | Dashboard |
| SF-DB-002 | Offline banner | Dashboard |
| SF-DB-003 | E-stop confirmation | Dashboard |
| SF-DB-005 | Settings validation blocks invalid saves | Dashboard |
| SF-E2E-001 | Full AUTO cycle | E2E |
| SF-SAF-001 | Dry-run timing | Safety |
| SF-PWA-003 | E-stop visible on 375px | PWA |

---

## 17. Test Execution Checklist

### Pre-Test Setup

- [ ] Production firmware flashed (verify `LOG_COMPILE_FLOOR = LOG_INFO` or `LOG_DEBUG` for tracing)
- [ ] All hardware wired per `hardware/wiring_notes.md`
- [ ] `secrets.h` populated on ESP32
- [ ] Firebase project live, security rules deployed
- [ ] Dashboard deployed to Vercel (HTTPS) or running on localhost
- [ ] USB-TTL adapter connected to NodeMCU GPIO2 for debug output
- [ ] Serial Monitor open at 115200 baud
- [ ] Chrome DevTools open (dashboard tests)
- [ ] Stopwatch ready (latency tests)
- [ ] Multimeter calibrated and ready
- [ ] Clear path to MCB confirmed
- [ ] Water supply confirmed available for pump tests
- [ ] Phase 5 test sketches compiled and ready (TC-S-xx, TC-M-xx)

### Execution Order

1. SF-HW-001 through SF-HW-006 (hardware verification first — do not proceed with any other test if HW tests fail)
2. SF-SN-001 through SF-SN-006 (sensor node)
3. SF-RS-001 through SF-RS-006 (protocol) — using test sketches TC-S-04 and TC-M-02
4. SF-FW-015, SF-FW-016, SF-FW-024 (boot and init, run before all other firmware tests)
5. SF-FW-001 through SF-FW-014, SF-FW-017 through SF-FW-023 (firmware logic)
6. SF-FB-001 through SF-FB-007 (Firebase integration)
7. SF-DB-001 through SF-DB-010 (dashboard)
8. SF-E2E-001 through SF-E2E-005 (end-to-end)
9. SF-SAF-001 through SF-SAF-007 (safety and fault injection)
10. SF-EDG-001 through SF-EDG-010 (edge cases)
11. SF-SEC-001 through SF-SEC-008 (security)
12. SF-PWA-001 through SF-PWA-007 (PWA + accessibility)
13. SF-REL-001 through SF-REL-005 (reliability — run overnight)

### Test Record Template

For each test executed:

```
Test ID:       SF-XX-NNN
Date:          YYYY-MM-DD
Tester:        [name]
Firmware:      v[x.x] / commit [hash]
Dashboard:     v[x.x] / commit [hash]
Environment:   [hardware setup description]
Result:        PASS / FAIL / SKIP
Actual result: [what actually happened]
Notes:         [deviations, observations, next action]
```

---

## 18. Pass/Fail Criteria Summary

### Deployment Gate

The system may NOT be deployed to production until:

| Requirement | Count |
|-------------|-------|
| All [CRITICAL] tests pass | 100% |
| All [HIGH] tests pass or have documented waivers | 100% |
| All [MEDIUM] tests pass or documented | ≥ 80% |
| Zero safety-critical failures (SF-SAF-*, SF-FW-007, SF-FW-008, SF-FW-009, SF-FW-010) | 100% |
| 24-hour soak test (SF-REL-001) passes | Required |
| Lighthouse Accessibility ≥ 95 | Required |
| Firebase auth and rules pass (SF-SEC-001, SF-SEC-002) | Required |

### Risk Classification

| Failure class | Risk level | Action required |
|---------------|-----------|-----------------|
| Pump starts unexpectedly | CRITICAL — DO NOT DEPLOY | Root cause analysis, full retest |
| Safety lockout can be bypassed | CRITICAL — DO NOT DEPLOY | Root cause analysis, full retest |
| Pump does not stop when commanded | CRITICAL — DO NOT DEPLOY | Root cause analysis, full retest |
| Dashboard shows wrong state | HIGH | Fix before deployment |
| Config change not applied | HIGH | Fix before deployment |
| Memory leak detected | HIGH | Fix before deployment |
| Visual/UX issue only | MEDIUM | Can deploy with documented known issue |
| Accessibility score < 95 | MEDIUM | Fix for next sprint |

---

*SmartFlow System Verification & Validation Master Test Plan v1.0*
*Engineering standards: IEC 61508 (functional safety of E/E/PE safety-related systems),
IEC 62443-4-2 (IoT security — component requirements), IEEE 829-2008 (software test documentation),
ISO/IEC 25010 (software quality model — reliability, maintainability, security, usability),
ISTQB Foundation Level Testing Syllabus v4.0, OWASP IoT Top 10 2018,
WCAG 2.1 Level AA (W3C, 2018), Google PWA Checklist 2024.*
*All test cases are traceable to SmartFlow System Refactor Plan v2.0 and Dashboard Refactor Plan v1.0.*
