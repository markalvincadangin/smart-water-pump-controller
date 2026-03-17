# Smart Water Pump Controller — Master Technical Specification
## Firmware v5.0.0 · ESP32 DevKit V1 · Arduino Framework

> **Document Type:** Engineering Reference Manual — Operational Logic, State Architecture, Behavioral Analysis  
> **Hardware:** ESP32 DevKit V1 · JSN-SR04T Ultrasonic · YF-G1 Hall-Effect Flow · CJX2-2510 Contactor · LR2-D13 TOR  
> **Tank:** Bestank WT660 (660 L) · Deployment: Leon, Iloilo, Philippines (UTC+8)  
> **Author:** Mark Alvin Cadangin  
> **Firmware Source Files:** `smart_water_pump_controller.ino`, `01_config.ino`, `02_sensors.ino`, `03_safety_pump.ino`, `04_persistence.ino`, `05_connectivity_cloud.ino`, `smart_water_pump_controller_shared.h`

---

## Table of Contents

1. [System Architecture Overview](#1-system-architecture-overview)
2. [State Machine Architecture](#2-state-machine-architecture)
3. [Hierarchy of Command — Priority Model](#3-hierarchy-of-command--priority-model)
4. [Loop Execution Architecture](#4-loop-execution-architecture)
5. [Sensor Subsystem — Deep Analysis](#5-sensor-subsystem--deep-analysis)
6. [Safety Subsystem — All Checks Documented](#6-safety-subsystem--all-checks-documented)
7. [Master Scenario & Behavior Matrix](#7-master-scenario--behavior-matrix)
8. [Dashboard Interaction Mapping](#8-dashboard-interaction-mapping)
9. [Fail-Safe & Edge-Case Audit](#9-fail-safe--edge-case-audit)
10. [Network & Firebase Resilience](#10-network--firebase-resilience)
11. [NVS Persistence — Offline-First Architecture](#11-nvs-persistence--offline-first-architecture)
12. [Timing Reference](#12-timing-reference)
13. [Configuration Thresholds Reference](#13-configuration-thresholds-reference)
14. [Status Telemetry Reference — All Fields Published to Firebase](#14-status-telemetry-reference--all-fields-published-to-firebase)
15. [Control Key Reference — All Dashboard-Writable Keys](#15-control-key-reference--all-dashboard-writable-keys)
16. [Boot & Initialization Sequence](#16-boot--initialization-sequence)

---

## 1. System Architecture Overview

### 1.1 Three-Layer Communication Model

```
┌────────────────────────────────────────────────────────┐
│  LAYER 3 — CLOUD                                       │
│  Firebase RTDB  ←──write──  Next.js PWA Dashboard      │
│  Firebase RTDB  ──read──►   Next.js PWA Dashboard      │
└──────────────────────┬─────────────────────────────────┘
                       │  WiFi / Firebase-ESP-Client
┌──────────────────────▼─────────────────────────────────┐
│  LAYER 2 — FIRMWARE (ESP32, this document)             │
│  Reads:  /pump_system/control  (JSON, 1 round-trip)    │
│  Writes: /pump_system/status   (JSON, ~40 fields)      │
└──────┬──────────────────────────────────┬──────────────┘
       │ GPIO 5/18 (Ultrasonic)           │ GPIO 34 (Flow ISR)
┌──────▼──────────────────────────────────▼──────────────┐
│  LAYER 1 — HARDWARE                                    │
│  JSN-SR04T (Level)   YF-G1 (Flow)   GPIO 4 (Relay)    │
│  Relay → Contactor CJX2-2510 → Pump Motor (1.5HP)     │
└────────────────────────────────────────────────────────┘
```

### 1.2 Core Design Principles

| Principle | Implementation |
|-----------|---------------|
| **Offline-First** | All pump logic runs on local state in RAM + NVS. Firebase is a sync layer, not the control plane. Pump never waits for network. |
| **Non-Blocking Loop** | `loop()` uses `millis()` timers exclusively. No `delay()` in normal operation. WDT feeds every iteration. |
| **Hard Safety Precedence** | P1 safety checks execute every sensor cycle (1s) regardless of mode, network state, or dashboard commands. They cannot be overridden. |
| **Deterministic Execution** | Sensor → Safety → Countdown → Pump Logic executes in fixed order every cycle. Firebase sync is decoupled from pump logic. |
| **NVS as Authority** | Critical state (mode, error flags, bypass, telemetry) is persisted to NVS flash. System recovers to correct state after power cycle without Firebase. |

### 1.3 Hardware Pin Mapping

| GPIO | Direction | Function | Signal Level | Notes |
|------|-----------|----------|-------------|-------|
| **4** | OUTPUT | Relay control | Active LOW | `LOW` = Pump ON · `HIGH` = Pump OFF · Drives 5V opto-isolated relay module |
| **5** | OUTPUT | JSN-SR04T TRIG | 3.3V pulse | 10µs trigger pulse every sensor sample |
| **18** | INPUT | JSN-SR04T ECHO | 3.3V (via 1kΩ/2kΩ divider) | `pulseIn()` with 100ms timeout |
| **34** | INPUT (ISR) | YF-G1 flow pulses | 3.3V (via divider) | RISING edge ISR, `IRAM_ATTR`, 2ms debounce |

> **Active-Low Relay Note:** `setPump(true)` writes `GPIO 4 LOW` (relay coil energized → contactor closes → pump motor starts). `setPump(false)` writes `GPIO 4 HIGH` (relay opens → pump stops). Hardware TOR (LR2-D13 thermal overload relay) remains in circuit regardless of firmware — it is a hardware-level motor protection that the firmware cannot override.

---

## 2. State Machine Architecture (v5.0)

### 2.1 Primary Operational States

The firmware maintains two concurrent state variables:

- **`pumpMode`** (String) — **Policy mode**, set by the dashboard or by firmware on countdown expiry / safety events. Values: `"AUTO"`, `"MANUAL"`, `"COUNTDOWN"`, `"FORCE_OFF"`, `"FORCE_ON"`.
 - **`runMode`** (String) — **Operational state**, derived exclusively by `executePumpLogic()` from the current combination of `pumpMode`, sensor readings, and error flags. Never written by the dashboard. Values: `"OFF"`, `"AUTO_STANDBY"`, `"AUTO"`, `"MANUAL"`, `"MANUAL_OFF"`, `"COUNTDOWN"`, `"FORCE_ON"`.

### 2.2 Complete State Transition Diagram

```
                    ┌─────────────┐
         Power ON   │             │
         ─────────► │ INITIALIZING│
                    │  (setup())  │
                    └──────┬──────┘
                           │ GPIO init, NVS load, WiFi, Firebase, WDT
                           ▼
              ┌────────────────────────┐
              │         LOOP()         │◄────────────────────────────────┐
              └────────────────────────┘                                 │
                           │                                             │
           ┌───────────────▼───────────────┐                           │
           │        inSafeMode?            │                           │
           └───────────┬─────┬─────────────┘                           │
                   YES │     │ NO                                       │
                       │     │                                          │
           ┌───────────▼─┐   │                                         │
           │  SAFE_MODE  │   │                                         │
           │  (heartbeat │   │                                         │
           │  30s, WDT   │   │ After 1h → restart                     │
           │  only)      │   │                                         │
           └─────────────┘   ▼                                         │
                     ┌────────────────────────────────────────┐        │
                     │         SENSOR BLOCK (1s cycle)        │        │
                     │  readUltrasonicSensor()                │        │
                     │  checkLevelSensorFailure()             │        │
                     │  calculateFlowRate()                   │        │
                     │  updateFlowBasedEstimate()             │        │
                     │  checkSafetyCutoff()                   │        │
                     │  checkCountdownExpiry()                │        │
                     │  executePumpLogic()    ◄───────────────┼── pumpMode
                     └────────────┬───────────────────────────┘        │
                                  │                                     │
                     ┌────────────▼───────────────────────────┐        │
                     │       FIREBASE BLOCK (3s cycle)        │        │
                     │  readDeviceConfigFromFirebase (30s)    │        │
                     │  readFirebaseControl()  → pumpMode ────┼────────┘
                     │  pushFirebaseStatus()                  │
                     └────────────────────────────────────────┘
```

### 2.3 runMode State Derivation Table (v5.0)

`runMode` is re-derived at the **top of every `executePumpLogic()` call** before any relay action. This ensures the dashboard always receives an accurate state.

| `pumpMode` | Additional Condition | `runMode` Assigned | Pump Relay |
|-----------|---------------------|-------------------|-----------|
| `"FORCE_ON"` | — | `"FORCE_ON"` | ON (absolute) |
| any | `isDryRunError = true` OR `isOverflowError = true` | `"OFF"` | FORCED OFF (unless P0) |
| `"FORCE_OFF"` | — | `"OFF"` | FORCED OFF (held each cycle) |
| `"MANUAL"` | `isRunning = true` | `"MANUAL"` | ON |
| `"MANUAL"` | `isRunning = false` | `"MANUAL_OFF"` | OFF (MANUAL mode held; operator or safety stopped) |
| `"COUNTDOWN"` | `isCountdownActive = true` | `"COUNTDOWN"` | ON |
| `"COUNTDOWN"` | `isCountdownActive = false` | `"OFF"` | Transitioning |
| `"AUTO"` | `isRunning = true` | `"AUTO"` | ON |
| `"AUTO"` | `isRunning = false` | `"AUTO_STANDBY"` | OFF |
| any (catch-all) | `isRunning = false` | `"OFF"` | OFF |
| any (catch-all) | `isRunning = true` | `"AUTO"` | ON |

---

## 3. Hierarchy of Command — Priority Model (v5.0)

### 3.1 Six-Level Priority Cascade

`executePumpLogic()` evaluates levels **in strict top-down order**. A higher-priority condition terminates the function with `return` before any lower-priority logic executes.

```
╔══════════════════════════════════════════════════════════════════════╗
║  P0 — ABSOLUTE OVERRIDE (FORCE_ON)                                   ║
║  Relay ON unconditionally. Safety checks RUN but do NOT stop relay.  ║
║  Trigger: pumpMode == "FORCE_ON"                                     ║
║  Action:  setPump(true). return.                                     ║
╠══════════════════════════════════════════════════════════════════════╣
║  P1 — HARD SAFETY (DRY-RUN + OVERFLOW LOCKOUT)                       ║
║  Cannot be bypassed by any mode except P0.                           ║
║  Trigger: isDryRunError == true OR isOverflowError == true           ║
║  Action:  If pumpMode != \"FORCE_ON\": setPump(false). Cancel        ║
║           COUNTDOWN and revert to AUTO. In MANUAL, pump OFF but      ║
║           mode stays MANUAL (sticky). In FORCE_ON, only flags/       ║
║           telemetry are set; relay state is owned by P0.             ║
╠══════════════════════════════════════════════════════════════════════╣
║  P2 — EMERGENCY STOP (FORCE_OFF)                                     ║
║  Relay held OFF every cycle. Persistent through power cycles.        ║
║  Trigger: pumpMode == "FORCE_OFF"                                    ║
║  Action:  setPump(false). return.                                    ║
╠══════════════════════════════════════════════════════════════════════╣
║  P3 — MANUAL RUN (MANUAL mode — full safety active, sticky mode)     ║
║  Operator-initiated. Identical safety coverage to AUTO.              ║
║  Trigger: pumpMode == "MANUAL"                                       ║
║  Action:  Level sensor error (no bypass) → stop (mode stays MANUAL). ║
║           Tank ≥ stop level → stop; mode stays MANUAL (sticky).      ║
║           Otherwise: setPump(true).                                  ║
╠══════════════════════════════════════════════════════════════════════╣
║  P4 — COUNTDOWN TIMER (full safety active)                           ║
║  Timed operator-initiated run.                                       ║
║  Trigger: pumpMode == "COUNTDOWN" && isCountdownActive               ║
║  Action:  Tank ≥ stop level → early stop + AUTO.                     ║
║           Otherwise: setPump(true).                                  ║
╠══════════════════════════════════════════════════════════════════════╣
║  P5 — AUTO HYSTERESIS                                                ║
║  P5a: Sleep check — suppress new autonomous starts                   ║
║  P5b: Level sensor bypass — skip level checks; P1 only               ║
║  P5c: Level sensor error — fail-safe stop in AUTO                    ║
║  P5d: Standard hysteresis — start ≤30%, stop ≥100%                   ║
╚══════════════════════════════════════════════════════════════════════╝
```

> **Bypass note:** Level sensor bypass (`cfgBypassLevelSensor = true`) is implemented as a sub-check **within the P5 branch** — it only affects `AUTO` mode logic. It does NOT protect the pump if P1 fires; `isDryRunError` / `isOverflowError` still kill the relay when bypass is active. In FORCE_ON, all relay-level safety is bypassed (P0).

### 3.2 Conflict Resolution Matrix (v5.0)

| Dashboard Command / Event | Current System State | Priority Winner | Firmware Outcome |
|--------------------------|----------------------|-----------------|-----------------|
| `manual_start = true` | `isDryRunError = true` OR `isOverflowError = true` | **P1** | Command rejected. `"Manual run rejected: error lockout active."` Pump stays OFF. |
| `manual_start = true` | `pumpMode = "FORCE_OFF"` | **P2** | Command rejected. `"Manual run rejected: FORCE_OFF active."` Pump stays OFF. |
| `manual_start = true` | `isLevelSensorError = true`, bypass OFF | **P3** | `pumpMode = "MANUAL"` then immediate level-error check → `setPump(false)`, `pumpMode = "AUTO"`. Net: pump does not start. |
| `manual_start = true` | `isLevelSensorError = true`, bypass ON | **P3** | `pumpMode = "MANUAL"`, pump starts; P1 dry-run and overflow still guard. |
| `manual_start = true` | `isSleeping = true` | **P3** | Pump starts in MANUAL. Sleep only suppresses AUTO starts. |
| `manual_start = true` | COUNTDOWN active | **P3** | `pumpMode = "MANUAL"`, `isCountdownActive = false`, `countdownEndMs = 0`. Pump continues under MANUAL. |
| `mode = "FORCE_ON"` | `isDryRunError = true` OR `isOverflowError = true` | **P0** | Pump turns ON. Error flags remain set for monitoring (`is_error` / `is_overflow_error`), but relay stays ON. |
| `mode = "FORCE_ON"` | `isLevelSensorError = true` | **P0** | Pump turns ON. Level sensor error remains informational. No fail-safe stop. |
| `mode = "FORCE_OFF"` | Any active run (AUTO / MANUAL / COUNTDOWN / FORCE_ON) | **P2** | Control read detects `runActive`. `setPump(false)`, cancel COUNTDOWN if active, `pumpMode = "FORCE_OFF"`. |
| `manual_stop = true` | `pumpMode = "FORCE_ON"` OR `"FORCE_OFF"` | **P0/P2** | Ignored. `"Manual stop ignored: override/lockout active. Use mode selector."` |
| `manual_stop = true` | MANUAL running | **P3** | `setPump(false)`, `runMode = "MANUAL_OFF"`, `pumpMode` unchanged (sticky MANUAL). |
| `manual_stop = true` | COUNTDOWN active | **P4** | `setPump(false)`, `isCountdownActive = false`, `pumpMode = "AUTO"`, `pendingModeWriteback = true`. |
| `mode = "AUTO"` | `isDryRunError = true` | **P1** | `pumpMode = "AUTO"` accepted; P1 still prevents pump from running until `clear_error`. |
| `countdown_add_time = true` | `isDryRunError = true`, timer still active | **P1** | `countdownEndMs` extended; pump stays OFF. After `clear_error`, pump resumes if timer has remaining time. |
| `countdown_add_time = true` | Timer already expired | — | No effect. `isCountdownActive = false` → condition fails. Flag reset; operator must start a new countdown. |
| `bypass_level_sensor = true` | `isDryRunError = true` | **P1** | Bypass only affects P5. P1 still kills pump. |

---

## 4. Loop Execution Architecture

### 4.1 Complete Loop Execution Order

Every `loop()` call executes the following blocks in strict sequential order. No block is skipped unless `inSafeMode` is true (which causes an early `return` after the safe-mode handler).

```
loop() EXECUTION ORDER — runs as fast as FreeRTOS allows (~1ms granularity)
│
├─ [1] esp_task_wdt_reset()                         — Always first. 120s hardware WDT.
│
├─ [2] SAFE MODE CHECK
│       if (inSafeMode) → heartbeat log (30s), delay(100), return
│       └─ After 1 hour: clear NVS, ESP.restart()
│
├─ [3] WIFI RECOVERY (exponential backoff: 5s→10s→60s, ±2s jitter)
│       if (disconnected && wifiWasConnected) → wifiWasConnected=false, reset backoff
│       if (disconnected && retry window elapsed) → WiFi.disconnect(false), WiFi.begin()
│       if (just reconnected) → initFirebase (if never init'd) OR Firebase.refreshToken()
│                             → configTime() NTP re-sync
│
├─ [4] RSSI TELEMETRY UPDATE (every 60s, WiFi connected only)
│       wifiRssi = WiFi.RSSI()
│
├─ [5] HEAP DIAGNOSTICS (every 10 minutes)
│       Log free heap + update minFreeHeapObserved
│
├─ [6] SLEEP/IDLE STATE COMPUTATION
│       emergencyOverride = (waterLevelPct <= cfgSleepEmergencyLevel)
│       isSleeping = cfgSleepEnabled && ntpSynced && inWindow && !emergencyOverride
│       isIdleMode = (!isSleeping && !isRunning && level≥90% && stable≥5min)
│       → Compute sensorInterval and firebaseInterval
│
├─ [7] SENSOR BLOCK (every sensorInterval: 1s normal / 10s idle / 30s sleep)
│   ├─ readUltrasonicSensor()        — 5-sample median + EMA + rate-of-change guard
│   ├─ checkLevelSensorFailure()     — count timeouts, auto-bypass check
│   ├─ waterLevelPct update          — only if reading >= 0
│   ├─ calculateFlowRate()           — atomic ISR counter read, noise filter
│   ├─ updateFlowBasedEstimate()     — flow-based level estimate (bypass mode)
│   ├─ checkSafetyCutoff()           — P1: dry-run, flow stuck, overflow
│   ├─ checkCountdownExpiry()        — millis()-based timer check
│   └─ executePumpLogic()            — derive runMode, control relay via setPump()
│
├─ [8] FIREBASE BLOCK (every firebaseInterval OR 1s retry on push failure)
│   ├─ Gate: cooldown check, WiFi connected, Firebase.ready()
│   ├─ readDeviceConfigFromFirebase() — only every 30s
│   ├─ readFirebaseControl()          — single getJSON, all control keys atomic
│   └─ pushFirebaseStatus()           — single setJSON, ~40 fields
│
├─ [9] persistStateToNVS()           — on-change + wear-reduced writes
│
├─ [10] SENSOR TELEMETRY LOG (every 60s, if any events occurred)
│        Windowed counters: ok/timeout/discard/stuck
│
└─ [11] LIGHT SLEEP (if isSleeping)
         esp_sleep_enable_timer_wakeup(remainingMs)
         esp_light_sleep_start()
         esp_task_wdt_reset() on wake
```

### 4.2 Sensor Block vs Firebase Block Independence

These two blocks are **completely independent** via separate `millis()` timers (`lastSensorMs`, `lastFirebaseMs`). In any given `loop()` call, one, both, or neither may execute depending on elapsed time.

| Scenario | Sensor Block | Firebase Block | Outcome |
|---------|-------------|---------------|---------|
| First 3s after last sensor cycle | No | No | Loop exits via `delay(1)` |
| 1s elapsed, 3s not yet | Yes | No | Safety + pump logic runs, no Firebase |
| 3s elapsed | Yes (if 1s also elapsed) | Yes | Both blocks run in same loop() pass |
| Firebase failure retry (1s) | Possibly | Yes (retry) | Push retry without full control read |
| Sleep window | Every 30s | Every 30s | Pump idle, light sleep between cycles |

---

## 5. Sensor Subsystem — Deep Analysis

### 5.1 Ultrasonic Level Sensor Processing Pipeline

The level reading passes through a **4-stage processing pipeline** before being used by the pump state machine:

```
RAW PULSE → TIMEOUT FILTER → RANGE FILTER → MEDIAN FILTER → EMA SMOOTHING → RATE-OF-CHANGE GUARD → waterLevelPct
```

#### Stage 1 — Trigger and Raw Reading (`readSingleUltrasonic()`)

```
Trigger: GPIO5 HIGH for 10µs, then LOW
Echo:    pulseIn(GPIO18, HIGH, 100000µs timeout)
         duration == 0 → return -1.0f (TIMEOUT)
         distanceCm = duration / 58.0f
         distanceCm < 2.0cm OR > 200.0cm → return -1.0f (OUT OF RANGE)
```

| Condition | Value Returned | Meaning |
|-----------|---------------|---------|
| Echo pulse received in time | `distanceCm` (2.0–200.0 cm) | Valid reading |
| Echo times out (100ms) | `-1.0f` | Sensor timeout / obstruction |
| Echo returns < 2.0 cm | `-1.0f` | Too close — physically impossible |
| Echo returns > 200.0 cm | `-1.0f` | Too far — physically impossible |

#### Stage 2 — 5-Sample Median Filter (`readUltrasonicSensor()`)

Five readings are taken with 80ms settling delay between each. Valid samples are median-sorted (insertion sort) and the middle value selected. This eliminates single-pulse transients and cable-induced glitches.

```
Samples taken:    5 (ULTRASONIC_SAMPLES)
Inter-sample delay: 80ms (ULTRASONIC_SAMPLE_DELAY)
Median index:     readings[validCount / 2]
Total cycle time: up to 5 × 100ms timeout + 4 × 80ms = ~820ms worst case
                  typically ~400ms at normal distances
If validCount == 0: return -1 (total failure), ultrasonicCycleTimeoutCount++
If validCount > 0:  ultrasonicCycleOkCount++
```

#### Stage 3 — Distance-to-Percentage Conversion + EMA (`updateLevelFromReading()`)

```
distanceCm = constrain(distanceCm, cfgTankFullCm, cfgTankEmptyCm)
levelFloat = 100.0 × (cfgTankEmptyCm - distanceCm) / (cfgTankEmptyCm - cfgTankFullCm)
levelFloat = constrain(levelFloat, 0.0, 100.0)

EMA update:
  if (first reading after boot):  waterLevelEma = levelFloat     ← cold-start seed
  else:                           waterLevelEma = 0.5 × levelFloat + 0.5 × waterLevelEma

newLevel = round(waterLevelEma)   ← 0.5-based rounding
newLevel = constrain(newLevel, 0, 100)
```

**EMA Alpha = 0.5** — equal weight to new reading and historical average. Provides rapid response (tank fills in seconds) while suppressing single-cycle noise. Higher alpha = more responsive but noisier.

#### Stage 4 — Rate-of-Change Guard

```
delta = abs(newLevel - prevWaterLevelPct)
if (prevWaterLevelPct > 0 && delta > 15%):
    Log: "[SENSOR][WARN] Level jumped N%"
    return prevWaterLevelPct   ← HOLD PREVIOUS VALUE
else:
    return newLevel
```

A level change exceeding **15% in one sensor cycle (1 second)** is physically impossible for a 660L tank. Such readings indicate cable noise, mechanical shock, or sensor malfunction. The previous value is held until a plausible reading arrives.

### 5.2 Flow Rate Calculation Pipeline

```
ISR (IRAM_ATTR flowPulseISR()):
  Hardware interrupt on GPIO34 RISING edge
  Debounce: if (now - lastPulseUs > 2000µs) → lastPulseUs = now, pulseCount++
  Runs in IRAM — safe during cache miss / flash operations

calculateFlowRate() — called every sensor cycle (1s window):
  noInterrupts() → count = pulseCount; pulseCount = 0; interrupts()  ← atomic swap
  
  if (!isRunning && pumpOffStartMs > 0 && elapsed > 3000ms):
      return 0.0f  ← Force zero 3s after pump stops (drain-off suppression)
  
  lpm = count / cfgFlowCalibration  (default: count / 7.5)
  
  if (lpm > 100.0 LPM):
      flowDiscardMaxSaneCount++
      return flowRateLpm  ← Keep previous value — spike discarded
  
  return lpm
```

**YF-G1 Calibration:** `Q (L/min) = pulse_frequency_Hz / K-factor`. Default K = 7.5. At max ~100 LPM: 750 Hz → 1333µs between pulses → 2ms debounce accepts all real pulses while rejecting noise bursts.

### 5.3 Flow-Based Level Estimate

When `cfgBypassLevelSensor = true` and the pump is running with measurable flow, the firmware estimates the fill level by integrating flow volume:

```
updateFlowBasedEstimate():
  if (!isRunning OR flowRateLpm < cfgDryRunThresholdLpm):
      lastFlowEstimateMs = millis()  ← Keep timestamp current (so dtSec is correct on pump start)
      return

  dtSec = (millis() - lastFlowEstimateMs) / 1000.0
  lastFlowEstimateMs = millis()
  if (dtSec > 5.0s): return  ← Guard against large gaps (e.g., after idle interval)

  flowVolumeAddedL += flowRateLpm × (dtSec / 60.0)
  if (levelAnchorPct >= 0):
      estimatedLevelPct = constrain(levelAnchorPct + (flowVolumeAddedL / 660L × 100), 0, 100)
```

- **Anchor reset:** On sensor recovery, `levelAnchorPct = sensorReading` and `flowVolumeAddedL = 0`.
- **Accuracy:** ±5–10% depending on flow calibration accuracy and pipe characteristics.
- **Dashboard indicator:** `level_estimate_active = true` when this estimate is in use.

---

## 6. Safety Subsystem — All Checks Documented

`checkSafetyCutoff()` calls three independent safety functions in order: `checkDryRunProtection()` → `checkFlowSensorStuck()` → `checkOverflowProtection()`.

### 6.1 Dry-Run Protection (`checkDryRunProtection()`)

**Purpose:** Protect pump motor from running without water flow (cavitation damage, motor burnout).

```
if (!isRunning): reset timer, return

if (flowRateLpm < cfgDryRunThresholdLpm):          ← Default: 0.5 L/min
    if (!dryRunTimerActive):
        dryRunTimerActive = true
        dryRunStartMs = millis()
        Log: "[SAFETY][WARN] Dry-run condition detected. Timer started."
    else if (millis() - dryRunStartMs >= cfgDryRunTimeoutSec × 1000):  ← Default: 30s
        isDryRunError = true
        setPump(false)                              ← IMMEDIATE relay OFF
        Log: "[SAFETY][ERROR] DRY-RUN LOCKOUT."
else:
    dryRunTimerActive = false, dryRunStartMs = 0
```

**Flow states and timer behavior:**

| Flow Rate | Pump State | Timer | Error | Result |
|-----------|-----------|-------|-------|--------|
| ≥ 0.5 LPM | ON | Not started / Reset | — | Normal operation |
| < 0.5 LPM, < 30s | ON | Running | — | Warning pending |
| < 0.5 LPM, ≥ 30s | ON | Expired | `isDryRunError=true` | Relay OFF immediately |
| Any | OFF | Reset | — | No check performed |

**Recovery:** Only via `clear_error = true` from the dashboard. Firmware sets `clear_error = false` after applying.

### 6.2 Flow Sensor Stuck-High Detection (`checkFlowSensorStuck()`)

**Purpose:** Detect a failed/jammed flow sensor reporting non-zero flow while the pump is OFF.

```
if (!isRunning && flowRateLpm > 2.0 LPM):          ← FLOW_STUCK_THRESHOLD_LPM
    if (!flowStuckTimerActive):
        flowStuckTimerActive = true
        flowStuckStartMs = millis()
    else if (millis() - flowStuckStartMs >= 5000ms): ← FLOW_STUCK_TIMEOUT_MS
        isFlowSensorError = true
        lastFaultCode = "FLOW_SENSOR"
        flowStuckHighEventCount++                  ← Lifetime + windowed telemetry
else:
    if (isFlowSensorError): Log: "Flow sensor recovered."
    isFlowSensorError = false
    flowStuckTimerActive = false
```

> **Important:** `isFlowSensorError = true` does **not** stop the pump. It is a **diagnostic/informational flag** only. It does NOT trigger P1 lockout. The dashboard displays a warning. Auto-recovery occurs when flow returns to ≤ 2.0 LPM.

### 6.3 Overflow / Max Runtime Protection (`checkOverflowProtection()`)

**Purpose:** Prevent tank overflow or pump burnout if the level sensor fails to trigger a stop.

```
if (!isRunning OR pumpMode NOT IN {"AUTO", "COUNTDOWN", "MANUAL"}):
    pumpAutoStartTracking = false
    pumpAutoStartMs = 0
    return   ← Tracks continuous runtime in AUTO, COUNTDOWN, and MANUAL

if (!pumpAutoStartTracking):
    pumpAutoStartTracking = true
    pumpAutoStartMs = millis()
    return

elapsed = millis() - pumpAutoStartMs
if (elapsed >= cfgMaxPumpRuntimeMin × 60000):   ← Default: 120 minutes
    isOverflowError = true
    setPump(false)                              ← IMMEDIATE relay OFF
    Log: "[SAFETY][ERROR] Max runtime exceeded."
```

**Coverage (v4.0):** Applies to `AUTO`, `COUNTDOWN`, and `MANUAL` modes. `FORCE_ON` (P0 absolute override) is intentionally **not covered** — operator assumes responsibility for duration; an optional FORCE_ON auto-timeout (`cfgForceOnMaxMin`) provides an additional guard.

---

## 7. Master Scenario & Behavior Matrix

> Format: `Trigger Condition → Internal Firmware Logic → Hardware Output → Dashboard Telemetry`

### 7.1 AUTO Mode — Normal Automation

| ID | Trigger Condition | Internal Firmware Logic | Hardware Output (`GPIO 4`) | Dashboard Telemetry (`/pump_system/status`) |
|----|------------------|------------------------|--------------------------|---------------------------------------------|
| **A-01** | `pumpMode = "AUTO"` · `waterLevelPct > 30%` · `isRunning = false` | `executePumpLogic()` P5: `!isRunning && level ≤ 30%` → false → no action | `HIGH` (OFF, unchanged) | `run_mode: "AUTO_STANDBY"` · `is_running: false` · `water_level_percent: current` |
| **A-02** | `pumpMode = "AUTO"` · `waterLevelPct ≤ 30%` · `isRunning = false` | P5 hysteresis: `!isRunning && level ≤ cfgPumpStartLevel` → `setPump(true)` · `totalPumpCycles++` · `pumpOnSinceMs = millis()` | `LOW` (ON) | `run_mode: "AUTO"` · `is_running: true` · `total_pump_cycles: N` |
| **A-03** | `pumpMode = "AUTO"` · `waterLevelPct ≥ 100%` · `isRunning = true` | P5: `isRunning && level ≥ cfgPumpStopLevel` → `setPump(false)` · `totalPumpRunSec += elapsed` | `HIGH` (OFF) | `run_mode: "AUTO_STANDBY"` · `is_running: false` · `total_pump_run_min: updated` |
| **A-04** | `pumpMode = "AUTO"` · `30% < level < 100%` · `isRunning = true` | P5: Both thresholds false → hysteresis hold, no action | `LOW` (ON, unchanged) | `run_mode: "AUTO"` · `is_running: true` · Level rising visible per cycle |
| **A-05** | `pumpMode = "AUTO"` · Level ≥ 90% for ≥ 5 min · Pump OFF | `isIdleMode = true` · `sensorInterval = cfgIdleSensorIntervalMs (10s)` · `firebaseInterval = cfgIdleFirebaseIntervalMs (30s)` | Unchanged | `run_mode: "AUTO_STANDBY"` · Update frequency drops to 30s |

### 7.2 Safety Events — P1 (Highest Priority)

| ID | Trigger Condition | Internal Firmware Logic | Hardware Output | Dashboard Telemetry |
|----|------------------|------------------------|----------------|---------------------|
| **S-01** | Pump ON · `flowRateLpm < 0.5 LPM` · Timer < 30s | `checkDryRunProtection()`: `dryRunTimerActive = true` | No change (ON) | `flow_rate_lpm: < 0.5` · No error flag yet |
| **S-02** | Pump ON · `flowRateLpm < 0.5 LPM` · Timer ≥ 30s | `isDryRunError = true` · `setPump(false)` · P1 in `executePumpLogic()`: `lastFaultCode = "DRY_RUN"`. If COUNTDOWN active: `isCountdownActive = false` · `pumpMode = "AUTO"` · `pendingModeWriteback = true` | `HIGH` (OFF, immediate) | `is_error: true` · `run_mode: "OFF"` · `last_fault_code: "DRY_RUN"` · `last_fault_message: "Dry-run lockout..."` · Dashboard: red lockout banner |
| **S-03** | `isDryRunError = true` · `clear_error = true` received · `pumpMode = "AUTO"` | `isDryRunError = false` · `isOverflowError = false` · `dryRunTimerActive = false` · `pumpAutoStartTracking = false` · `lastFaultCode = ""` · `Firebase.setBool("clear_error", false)` | Unchanged (OFF) | `is_error: false` · `run_mode: "AUTO_STANDBY"` · Error banner clears. Pump remains stopped until AUTO hysteresis restarts it based on level. |
| **S-03b** | `isDryRunError = true` · `clear_error = true` received · `pumpMode = "MANUAL"` | Same as S-03 (errors cleared, timers reset). On next P3 evaluation, `pumpMode == "MANUAL"` causes `setPump(true)` (subject to level sensor/bypass checks). | `HIGH` (OFF) at clear, then `LOW` (ON) on next sensor cycle | `is_error: false` · `run_mode: "MANUAL"` · Pump auto-restarts in MANUAL after error clear (sticky MANUAL). |
| **S-04** | `pumpMode = "AUTO"/"COUNTDOWN"` · Pump ON · Runtime ≥ 120 min | `isOverflowError = true` · `setPump(false)` · P1: `lastFaultCode = "OVERFLOW"` | `HIGH` (OFF, immediate) | `is_overflow_error: true` · `run_mode: "OFF"` · `last_fault_code: "OVERFLOW"` |
| **S-05** | Pump OFF · `flowRateLpm > 2.0 LPM` · Duration ≥ 5s | `isFlowSensorError = true` · `lastFaultCode = "FLOW_SENSOR"` · `flowStuckHighEventCount++` | Unchanged (OFF) | `is_flow_sensor_error: true` · `flow_stuck_high_events: N` · Dashboard: flow sensor warning (informational, no lockout) |
| **S-06** | `isFlowSensorError = true` · `flowRateLpm` returns to ≤ 2.0 LPM | `isFlowSensorError = false` · Auto-recovery, no dashboard action required | Unchanged | `is_flow_sensor_error: false` · Warning clears |

### 7.3 Level Sensor Failure States

| ID | Trigger Condition | Internal Firmware Logic | Hardware Output | Dashboard Telemetry |
|----|------------------|------------------------|----------------|---------------------|
| **L-01** | Ultrasonic returns `-1` · `levelSensorFailCount < 5` | `checkLevelSensorFailure()`: `levelSensorFailCount++` · `level_sensor_health_pct -= 20` · `waterLevelPct` holds last valid value | Unchanged | `level_sensor_health_pct: degrading` · `level_last_valid_age_sec: increasing` · No error flag |
| **L-02** | Ultrasonic returns `-1` · `levelSensorFailCount ≥ 5` | `isLevelSensorError = true` · In P5 AUTO: if running → `setPump(false)` fail-safe · `lastFaultCode = "LEVEL_SENSOR"` | `HIGH` (OFF if was ON) | `is_level_sensor_error: true` · `last_fault_code: "LEVEL_SENSOR"` · `run_mode: "OFF"` |
| **L-03** | Valid ultrasonic reading received after `isLevelSensorError = true` | `isLevelSensorError = false` · `levelSensorFailCount = 0` · `levelAnchorPct = newReading` · `flowVolumeAddedL = 0` · If auto-bypass: `cfgBypassLevelSensor = false` · `autoBypassActive = false` | May restart (AUTO evaluates level) | `is_level_sensor_error: false` · `level_sensor_health_pct: 100` · `auto_bypass_active: false` |
| **L-04** | `cfgAutoBypassOnSensorFail = true` · `isLevelSensorError = true` · Duration ≥ `cfgAutoBypassDelaySec` (60s) | `cfgBypassLevelSensor = true` · `autoBypassWasEngaged = true` · `autoBypassActive = true` · Level estimate via flow activates | No relay change | `bypass_level_sensor: true` · `auto_bypass_active: true` · `level_estimate_active: true` · Dashboard: Auto-Maintenance banner |
| **L-05** | Manual: `bypass_level_sensor = true` written by admin | `cfgBypassLevelSensor = true` persisted to NVS · P5 bypass branch returns early | No relay change | `bypass_level_sensor: true` · `auto_bypass_active: false` · Dashboard: Maintenance Mode banner |

### 7.4 Manual Dashboard Commands (v5.0)

| ID | Trigger Condition | Internal Firmware Logic | Hardware Output | Dashboard Telemetry |
|----|------------------|------------------------|----------------|---------------------|
| **M-01** | `manual_start = true` · no P1 lockout · not in `FORCE_OFF` | `pumpMode = "MANUAL"` · `isManualRun = true` · `runStartMs = millis()` · P3: `setPump(true)` | `LOW` (ON) | `run_mode: "MANUAL"` · `is_running: true` |
| **M-02** | `manual_start = true` · `isDryRunError = true` OR `isOverflowError = true` | Command rejected. `"Manual run rejected: error lockout active."` · No state change | Unchanged (OFF) | Error banner stays. |
| **M-03** | `manual_start = true` · `pumpMode = "FORCE_OFF"` | Command rejected. `"Manual run rejected: FORCE_OFF active."` | `HIGH` (OFF) | FORCE_OFF badge; hint to exit FORCE_OFF. |
| **M-04** | `manual_stop = true` · MANUAL running | `setPump(false)` · `isManualRun = false` · `runMode = "MANUAL_OFF"` · `runStartMs = 0` · `pumpMode` unchanged (sticky MANUAL) | `HIGH` (OFF) | `run_mode: "MANUAL_OFF"` · `is_running: false` · `control.mode: "MANUAL"` |
| **M-04b** | `manual_stop = true` · COUNTDOWN active | `setPump(false)` · `isManualRun = false` · `isCountdownActive = false` · `countdownEndMs = 0` · `pumpMode = "AUTO"` · `pendingModeWriteback = true` · dashboard sees mode AUTO | `HIGH` (OFF) | `run_mode: "AUTO_STANDBY"` · `is_running: false` · `mode` reverts to AUTO |
| **M-05** | `manual_stop = true` · `pumpMode = "FORCE_ON"` | Ignored. `"Manual stop ignored: FORCE_ON active. Use mode selector."` | Unchanged | FORCE_ON banner; instructions to use mode selector. |
| **M-06** | `mode = "FORCE_OFF"` · Run active (`runMode = "MANUAL"` OR COUNTDOWN active OR FORCE_ON) | Control read run-active intercept: `setPump(false)` · `isManualRun = false` · `isCountdownActive = false` · `pumpMode = "FORCE_OFF"` | `HIGH` (OFF, held) | `run_mode: "OFF"` · FORCE_OFF badge |
| **M-07** | `mode = "AUTO"` after `FORCE_OFF` | `pumpMode = "AUTO"` · `isManualRun = false` · P5 evaluates level on next sensor cycle | Per level (may turn ON) | `run_mode: "AUTO_STANDBY"` or `"AUTO"` |
| **M-08** | `reboot_request_id ≠ lastRebootRequestId` | Persists new ID to NVS · Writes ACK to status · `delay(100)` · `ESP.restart()` | All GPIO reset by `setup()` | Controller offline → uptime resets to 0 on reconnect |

### 7.5 Countdown Mode Scenarios

| ID | Trigger Condition | Internal Firmware Logic | Hardware Output | Dashboard Telemetry |
|----|------------------|------------------------|----------------|---------------------|
| **C-01** | `mode = "COUNTDOWN"` · `countdown_duration_min = N` · No lockout | `countdownEndMs = millis() + N×60000` · `isCountdownActive = true` · `countdownConsumed = true` · `lastAddTime = false` · P4: `setPump(true)` | `LOW` (ON) | `run_mode: "COUNTDOWN"` · `countdown_remaining_sec: N×60` · `is_running: true` |
| **C-02** | `mode = "COUNTDOWN"` · Firebase unavailable for `countdown_duration_min` | Uses `cfgLastCountdownDurationMin` (NVS-persisted, default 15 min) | `LOW` (ON) | Countdown starts with last-known duration. Serial: `"(offline — using last known duration)"` |
| **C-03** | `countdown_add_time = true` · `countdown_add_min = N` (or default 5) · Countdown active | `countdownEndMs += N×60000` · capped at `millis() + 7200000` · `Firebase.setBool("countdown_add_time", false)` (firmware reset) | Unchanged (ON) | `countdown_remaining_sec: increases` · `countdown_add_time` resets to false in Firebase |
| **C-04** | `millis() ≥ countdownEndMs` | `checkCountdownExpiry()`: `isCountdownActive = false` · `pumpMode = "AUTO"` · `pendingModeWriteback = true` · `pendingModeWritebackSentMs = 0` · P5 handles relay on next cycle | `HIGH` (OFF, next sensor cycle) | `run_mode: "AUTO_STANDBY"` · `countdown_remaining_sec: 0` · Mode reverts to AUTO |
| **C-05** | COUNTDOWN active · `waterLevelPct ≥ 100%` · `cfgBypassLevelSensor = false` | P4 early stop: `setPump(false)` · `isCountdownActive = false` · `pumpMode = "AUTO"` · `pendingModeWriteback = true` · `pendingModeWritebackSentMs = 0` | `HIGH` (OFF) | `run_mode: "AUTO_STANDBY"` · Tank-full early stop |
| **C-06** | COUNTDOWN active · `isDryRunError = true` fires | P1 fires before P4 · `setPump(false)` · `isCountdownActive = false` · `pumpMode = "AUTO"` · `pendingModeWriteback = true` | `HIGH` (OFF, immediate) | `is_error: true` · `run_mode: "OFF"` · Countdown cancelled |
| **C-07** | COUNTDOWN active · `isOverflowError = true` fires | Same as C-06 but `isOverflowError` | `HIGH` (OFF, immediate) | `is_overflow_error: true` · `run_mode: "OFF"` |

### 7.6 Sleep & Idle Mode Scenarios

| ID | Trigger Condition | Internal Firmware Logic | Hardware Output | Dashboard Telemetry |
|----|------------------|------------------------|----------------|---------------------|
| **Sl-01** | `cfgSleepEnabled = true` · `ntpSynced = true` · `currentHour` within window · `level > cfgSleepEmergencyLevel` | `isSleeping = true` · `sensorInterval = 30s` · `firebaseInterval = 30s` · P5: `isSleeping` branch — no new AUTO starts | `HIGH` (OFF, no new starts) | `is_sleeping: true` · Status updates every 30s |
| **Sl-02** | `isSleeping = true` · `waterLevelPct ≤ cfgSleepEmergencyLevel` (default 5%) | `emergencyOverride = true` → `isSleeping = false` · Normal intervals resume · P5 evaluates level next cycle | Per level (may turn ON) | `is_sleeping: false` · Normal 3s updates resume |
| **Sl-03** | `isSleeping = true` · `manual_start` or `mode = FORCE_ON` received | P3b fires (evaluated before P5 sleep check) · `setPump(true)` | `LOW` (ON) | `run_mode: "MANUAL"` · `is_sleeping: true` (still in window) |
| **Sl-04** | `isSleeping = true` · Pump running (from previous FORCE_ON) · `level ≥ 100%` | P5 sleep branch: `isRunning && level ≥ cfgPumpStopLevel` → `setPump(false)` | `HIGH` (OFF) | `run_mode: "AUTO_STANDBY"` |
| **Sl-05** | Pump OFF · Level ≥ 90% · Stable for ≥ 5 minutes | `isIdleMode = true` · `sensorInterval = cfgIdleSensorIntervalMs (10s)` · `firebaseInterval = cfgIdleFirebaseIntervalMs (30s)` | Unchanged | Status cadence slows to 30s. `run_mode: "AUTO_STANDBY"` |
| **Sl-06** | `isIdleMode = true` · Level drops or pump starts | `isIdleMode = false` · `idleStartMs = 0` · Intervals reset to 1s/3s | Per pump logic | Normal update cadence resumes |

### 7.7 Network & Firebase Scenarios

| ID | Trigger Condition | Internal Firmware Logic | Hardware Output | Dashboard Telemetry |
|----|------------------|------------------------|----------------|---------------------|
| **N-01** | WiFi connected · `Firebase.ready() = true` · 3s elapsed | `readFirebaseControl()` (single JSON) · `pushFirebaseStatus()` (40+ fields) | No change | All fields updated. `firebase_consecutive_failures: 0` |
| **N-02** | Firebase read/write fails · `firebaseConsecutiveFailCount < 3` | Count increments. `statusPushRetryCount++` · Retry in 1s via `statusRetryDue` path. `lastFirebaseMs` NOT reset on retry | No change | 3–6s possible gap. Pump logic unaffected. |
| **N-03** | Firebase read/write fails · `firebaseConsecutiveFailCount ≥ 3` · Timeout error | 30s cooldown: `firebaseCooldownUntilMs = now + 30000` · No Firebase calls during cooldown | No change | Dashboard shows controller "offline" after 20s. Pump continues on local state. |
| **N-04** | Firebase token error (`"token is not ready"` / `"revoked"`) | `firebaseCooldownUntilMs = now + 30000UL` (30s) · `Firebase.refreshToken()` called | No change | 30s gap. Token refreshes automatically. |
| **N-05** | WiFi drops mid-operation | `wifiWasConnected = false` · Pump logic runs unaffected · WiFi retry: 5s → 10s → 60s backoff | No change (pump continues) | Dashboard shows offline. Pump status maintained locally. |
| **N-06** | WiFi reconnects | `Firebase.refreshToken()` · 10s cooldown · `configTime()` NTP re-sync | No change | Status pushes resume. Correct state delivered to dashboard. |
| **N-07** | `pendingModeWriteback = true` · Firebase still shows old mode | Mode read suppressed. If `pendingModeWritebackSentMs == 0` OR 5s elapsed: re-send `Firebase.setString("mode", pumpMode)` · `pendingModeWritebackSentMs = millis()` | No change | Correct mode eventually confirmed. `countdownConsumed` stays protected during propagation lag. |
| **N-08** | Admin changes device config via dashboard · `readDeviceConfigFromFirebase()` reads change | Validates all fields. Applies in-memory. `saveDeviceConfigToNVS()` called. | New thresholds active next cycle | All affected thresholds update immediately. |

### 7.8 Boot & Resilience Scenarios

| ID | Trigger Condition | Internal Firmware Logic | Hardware Output | Dashboard Telemetry |
|----|------------------|------------------------|----------------|---------------------|
| **B-01** | Normal power-on · NVS valid · WiFi connects | GPIO init → setPump(false) → checkCrashLoop → loadDeviceConfigFromNVS → loadStateFromNVS → 5s delay → connectWiFi → NTP → initFirebase → WDT | `HIGH` (OFF on boot) | Controller online within ~10s. `last_boot_reason: "Power-on"` |
| **B-02** | Power-on · WiFi unavailable | Firebase skipped · Loop starts · WiFi retry begins · Pump logic runs from NVS state | Per NVS mode | Dashboard offline. Pump state determined by NVS. |
| **B-03** | Boot · NVS `mode = "COUNTDOWN"` restored | `pumpMode = "COUNTDOWN"` · `countdownConsumed = false` · First Firebase sync: countdown re-arms with `cfgLastCountdownDurationMin` | Per countdown state | Countdown resumes. `run_mode: "COUNTDOWN"` |
| **B-04** | Boot · NVS `isDryRunError = true` | P1 fires every sensor cycle · Pump stays OFF · `lastFaultCode = "DRY_RUN"` set in P1 | `HIGH` (OFF) | `is_error: true` on first push. Dashboard shows error banner. |
| **B-05** | Boot · NVS `cfgBypassLevelSensor = true` | Bypass restored · P5 bypass branch active · Level sensor not used for start/stop | Per bypass logic | `bypass_level_sensor: true` |
| **B-06** | ≥ 5 reboots within 5 minutes | `inSafeMode = true` · `setup()` returns early · `loop()` heartbeat + WDT only · No sensors, WiFi, Firebase, relay | `HIGH` (OFF, unconditional) | Controller offline. No status pushed. Serial: `"SAFE MODE"` |
| **B-07** | Power cycle while in safe mode | `millis() < 5000` at `checkCrashLoop()` → `safe_mode_ms = 0` · `boot_count = 0` · Normal boot | Normal boot | Controller comes online. |
| **B-08** | Safe mode auto-timeout (1 hour) | `inSafeMode = true` · `now - safeModeEnteredMs ≥ 3600000` → clear NVS · `ESP.restart()` | Normal boot | Controller recovers automatically. |
| **B-09** | NVS config corrupted / out of range | `loadDeviceConfigFromNVS()` validation fails · Compiled defaults used · Log: `"Stored config invalid"` | Firmware defaults apply | Calibration values revert to compiled defaults. Dashboard config may mismatch. |
| **B-10** | WDT triggers (loop blocked > 120s) | Hardware watchdog fires → panic + reboot · `last_boot_reason = "Task watchdog"` or `"Interrupt watchdog"` | GPIO reset on `setup()` | `last_boot_reason: "Task watchdog"` · `uptime_minutes: 0` |

---

## 8. Dashboard Interaction Mapping

### 8.1 Bidirectional Data Flow

```
PHYSICAL EVENT                    FIRMWARE PROCESSING              DASHBOARD UPDATE
     │                                    │                              │
Sensor reads level change                 │                              │
     ↓                                    │                              │
readUltrasonicSensor()                    │                              │
→ waterLevelPct updated (1s)             │                              │
→ executePumpLogic() runs               │                              │
→ setPump(true/false)                    │                              │
     │                           Every 3s: pushFirebaseStatus()         │
     │                           → /pump_system/status JSON             │
     │                                    ↓                              │
     │                           Firebase RTDB update                   │
     │                                    ↓                              │
     │                           onValue() listener in dashboard        │
     │                                    ↓                              │
     │                           TankVisual, StatCards, AlertBanners ←──┘
     │
     │
DASHBOARD ACTION                 FIREBASE WRITE               FIRMWARE PROCESSING
     │                                 │                              │
User taps Manual ON                    │                              │
     ↓                                 │                              │
usePumpData.startManualRun()           │                              │
→ set(control/manual_start, true)      │                              │
     │                 Firebase RTDB ──►                              │
     │                                                readFirebaseControl()
     │                                                → manual_start edge detected
     │                                                → pumpMode = "MANUAL"
     │                                                → P3 evaluates and calls setPump(true)
     │                                 ◄── /status: run_mode="MANUAL", is_running=true
     ↓                                                               │
Dashboard: run_mode="MANUAL"                                         │
  Run Controls shows MANUAL ON/OFF toggle                            │
  Status card: "ON"                                                  │
```

### 8.2 Dashboard UI Element → Firmware Function Mapping (v5.0)

| UI Element | Dashboard Action | Firebase Write | Firmware Function | Firmware Outcome |
|-----------|-----------------|----------------|-------------------|-----------------|
| **Quick Start button** | `startManualRun()` | `control/manual_start = true` (5s reset) | `readFirebaseControl()` rising-edge detect → `pumpMode = "MANUAL"`, `isManualRun = true` | `setPump(true)`, `run_mode: "MANUAL"` |
| **Stop button** | `stopRun()` | `control/manual_stop = true` (5s reset) | `readFirebaseControl()` edge → `setPump(false)`, `pumpMode = "AUTO"`, `pendingModeWriteback = true` | Relay OFF, mode reverts to AUTO |
| **Start Countdown button** | `startCountdown(N)` | `control/countdown_duration_min = N` + `control/mode = "COUNTDOWN"` | Mode read → P4 activated, `countdownEndMs = millis() + N×60000` | `setPump(true)`, `run_mode = "COUNTDOWN"` |
| **Add N min button** | `addCountdownTime()` | `control/countdown_add_time = true`, `control/countdown_add_min = N` | Edge detect → `countdownEndMs += N×60000`, `Firebase.setBool(add_time, false)` | Timer extended |
| **AUTO mode button** | `setModeAuto()` | `control/mode = "AUTO"` | Mode read → `pumpMode = "AUTO"`, P5 evaluates | Level-based automation resumes |
| **Stopped (FORCE_OFF) segment** | `setModeForceOff()` | `set(control/mode, "FORCE_OFF")` | Mode read → `pumpMode = "FORCE_OFF"`, P2: `setPump(false)` every cycle | Relay held OFF |
| **Override (FORCE_ON) segment** | `setModeForceOn()` (admin) | `set(control/mode, "FORCE_ON")` | Mode read → `pumpMode = "FORCE_ON"` (P0). `executePumpLogic()` P0: `setPump(true)` regardless of errors. | Relay held ON (absolute override) |
| **Clear Error button** | `clearError()` | `control/clear_error = true` | `readFirebaseControl()`: `isDryRunError = false`, `isOverflowError = false`, `Firebase.setBool(clear_error, false)` | P1 lockout removed |
| **Enable Bypass toggle** | `setBypassLevelSensor(true)` (admin) | `control/bypass_level_sensor = true` | `cfgBypassLevelSensor = true`, NVS write | P5 bypass branch active |
| **Disable Bypass toggle** | `setBypassLevelSensor(false)` (admin) | `control/bypass_level_sensor = false` | `cfgBypassLevelSensor = false`, `autoBypassActive = false`, `autoBypassWasEngaged = false` | Level sensor re-enabled |
| **Restart Controller button** | `requestReboot()` (admin) | `control/reboot_request_id = N+1` | `readFirebaseControl()`: new ID detected → NVS persist → ACK push → `ESP.restart()` | ESP32 reboots |
| **Device Config — Save button** | `saveConfig(config)` (admin) | `config/device = {all fields}` | `readDeviceConfigFromFirebase()` every 30s: validate, apply, `saveDeviceConfigToNVS()` | All thresholds update |
| **Tank Visual** | Display only | Reads `water_level_percent` | N/A (read only) | Displays current % with fill animation |
| **Flow Rate card** | Display only | Reads `flow_rate_lpm` | N/A | Shows LPM + low-flow warning if near dry-run threshold |
| **Pump Status card** | Display only | Reads `run_mode`, `is_running` | N/A | Shows ON/OFF/STANDBY/LOCKED OUT |
| **Alert banners (ranked)** | Display only | Reads error flags | N/A | Priority order: offline > dry-run > overflow > auto-bypass > bypass > level error > flow error > sleeping |
| **Countdown display (MM:SS)** | Display only | Reads `countdown_remaining_sec` | N/A | Displays live countdown from status field |
| **History chart** | Display only | Rolling buffer from status | N/A | Last ~3 min at 3s resolution (client-side buffer) |
| **System Info panel** | Display only | Reads diagnostics fields | N/A | RSSI, heap, uptime, ultrasonic stats, flow discard count |
| **Activity Log** | Display only | Reads `/audit/events` | N/A | Operator actions: mode changes, starts, stops, config saves |

### 8.3 Sensor Change → Dashboard Update Flow

```
waterLevelPct changes (every 1s sensor cycle)
     │
     ▼
executePumpLogic() → may change isRunning, runMode
     │
     ▼ (every 3s Firebase cycle)
pushFirebaseStatus()
  statusJson.set("water_level_percent", waterLevelPct)    ← TankVisual height
  statusJson.set("run_mode",            runMode)           ← Status card + mode badge
  statusJson.set("is_running",          isRunning)         ← Relay state indicator
  statusJson.set("flow_rate_lpm",       flowRateLpm)       ← Flow card
  statusJson.set("countdown_remaining_sec", ...)           ← Countdown MM:SS
  ... (40+ fields total)
     │
     ▼
Firebase RTDB update → onValue() triggers in dashboard
     │
     ▼
React state update → All consuming components re-render
  TankVisual (level %, glow state)
  StatCards (level, flow, pump status)
  AlertBanners (getRankedAlerts() from status fields)
  RunControls (button states based on run_mode + lockout flags)
  ModeControls (active segment based on pumpMode)
  CountdownDisplay (countdown_remaining_sec)
```

---

## 9. Fail-Safe & Edge-Case Audit

### 9.1 Sensor Anomalies

| Anomaly | Detection Point | Firmware Response | Safe Outcome |
|---------|----------------|-------------------|-------------|
| **All 5 ultrasonic samples timeout** | `validCount == 0` in `readUltrasonicSensor()` | Returns `-1`. `waterLevelPct` holds last valid value. `ultrasonicCycleTimeoutCount++` | Pump continues on stale level. After `cfgLevelSensorFailureThreshold` (5) consecutive cycles: `isLevelSensorError = true` → fail-safe pump OFF (AUTO only) |
| **Single ultrasonic reading < 2 cm or > 200 cm** | Range check in `readSingleUltrasonic()` | Reading discarded, returns `-1.0f`. Remaining samples still collected | Spike removed before median filter |
| **Level jump > 15% in 1 cycle** | `delta > LEVEL_RATE_OF_CHANGE_MAX` in `updateLevelFromReading()` | Previous value returned. EMA not updated. Logged as `[SENSOR][WARN]` | False auto-start or auto-stop prevented |
| **Flow reading > 100 LPM** | `lpm > FLOW_MAX_SANE_LPM` in `calculateFlowRate()` | Reading discarded. `flowRateLpm` holds previous value. `flowDiscardMaxSaneCount++` | Noise spike cannot trigger dry-run timer |
| **Flow pulses after pump stops** | `!isRunning && elapsed > FLOW_PUMP_OFF_ZERO_MS (3s)` | `calculateFlowRate()` returns `0.0f` | Pipe drain-off after pump stop does not cause stuck-high false positive |
| **NVS config corrupted** | Range validation in `loadDeviceConfigFromNVS()` | All NVS values rejected, compiled defaults used | System operates on known-safe parameters |
| **NVS schema from newer firmware** | `schemaVer > NVS_SCHEMA_VERSION` | Entire NVS config rejected, compiled defaults used | No attempt to interpret unknown fields |

### 9.2 Network Loss Scenarios

| Scenario | Duration | Firmware Behavior | Pump Behavior |
|---------|---------|-------------------|--------------|
| **WiFi drops (brief, < 60s)** | < 60s | Pump loop continues unchanged. WiFi retry starts at 5s. | Unaffected. Local state machine runs. |
| **WiFi drops (extended, > 60s)** | > 60s | Backoff up to 60s between retries. Firebase cooldown. All state runs locally. | Unaffected. Config from NVS. |
| **Firebase auth token expires** | Up to 30s | 30s cooldown, `refreshToken()` called. Status gap. | Unaffected. |
| **Firebase timeout (1–2 times)** | 3–6s | Retry via `statusRetryDue` at 1s. `lastFirebaseMs` NOT reset on retry. | Unaffected. |
| **Firebase timeout (3+ times)** | 30s cooldown | Dashboard shows offline after 20s. Pump runs on last known mode + NVS. | Unaffected. |
| **Dashboard sends command while offline** | Delivered on reconnect | Edge-detect on `manual_start`/`manual_stop` prevents re-fire if flag was already `true` at last read. | Command processes normally on reconnect. |
| **Power failure during countdown** | — | NVS has `mode = "COUNTDOWN"`, `cfgLastCountdownDurationMin` persisted. On boot: countdown re-arms with last-known duration. | Countdown resumes (may differ from remaining time before failure). |

### 9.3 Hardware Overrun Protection (v4.0)

| Scenario | Protection Mechanism | Threshold | Response |
|---------|---------------------|-----------|---------|
| **Pump runs too long without filling** | `checkOverflowProtection()` — P1 | Default 120 min (configurable 30–480 min) | `isOverflowError = true` · Relay OFF · Requires `clear_error` from dashboard. Applies in `AUTO`, `COUNTDOWN`, and `MANUAL` modes; `FORCE_ON` excluded. |
| **No water in pipe (dry run)** | `checkDryRunProtection()` — P1 | Flow < 0.5 LPM for 30s (configurable) | `isDryRunError = true` · Relay OFF · Requires `clear_error` |
| **Motor thermal overload** | Hardware TOR (LR2-D13) — firmware-independent | Motor current threshold (set on TOR dial) | TOR trips contactor open. Firmware continues operating as if pump is ON but relay circuit is broken. Firmware may detect via dry-run (no flow). |
| **ESP32 firmware crash / hang** | Hardware WDT (esp_task_wdt) | 120 seconds without `esp_task_wdt_reset()` | Panic + automatic reboot. Boot reason: `"Task watchdog"`. Crash loop detection tracks frequency. |
| **Repeated crash loops** | `checkCrashLoop()` | ≥ 5 reboots in 5 minutes | `inSafeMode = true`. All outputs disabled. No pump, no Firebase. Auto-clears after 1 hour or power cycle. |
| **Brownout / undervoltage** | ESP32 brownout detector | Hardware level (≈2.5V) | Controlled reset. Boot reason: `"Brownout"`. |
| **Flow sensor stuck open** | `checkFlowSensorStuck()` | Flow > 2.0 LPM for 5s with pump OFF | `isFlowSensorError = true` (informational). Pump not stopped. Operator alerted via dashboard. |

---

## 10. Network & Firebase Resilience

### 10.1 Firebase Retry and Cooldown Architecture

```
Firebase Cycle (every 3s or 1s retry):
│
├─ firebaseCooldownUntilMs check:
│   If (now < cooldownUntilMs): skip all Firebase ops
│   If cooldown elapsed: firebaseCooldownUntilMs = 0
│
├─ readFirebaseControl() — single getJSON (1 network round-trip, atomic):
│   ├─ SUCCESS: firebaseConsecutiveFailCount = 0
│   └─ FAILURE:
│       firebaseConsecutiveFailCount++
│       if (auth error): cooldown 30s + refreshToken()
│       if (timeout) AND (count >= 3): cooldown 30s
│       if (timeout) AND (count < 3):  log retry, no cooldown
│
└─ pushFirebaseStatus() — single setJSON:
    ├─ SUCCESS: firebaseConsecutiveFailCount = 0 · statusPushRetryCount = 0
    └─ FAILURE:
        statusPushRetryCount++
        statusPushRetryMs = millis()
        ← 1s retry via statusRetryDue in loop
        if (auth error): cooldown 30s + refreshToken()
        if (timeout) AND (count >= 3): cooldown 30s
        if (timeout) AND (count < 3): log retry
```

### 10.2 `pendingModeWriteback` — Stale Mode Overwrite Protection

This mechanism prevents Firebase propagation lag from causing countdown restart / stop-then-restart behavior:

```
When firmware locally changes pumpMode (countdown expiry, manual_stop, P1+countdown, P4 early stop):
  pendingModeWriteback = true
  pendingModeWritebackSentMs = 0  (or millis() if write sent immediately)

In readFirebaseControl(), mode read block:
  if (pendingModeWriteback):
    if (Firebase reports same mode as pumpMode):
      → pendingModeWriteback = false  ← confirmed
      → if (pumpMode == "AUTO"): countdownConsumed = false
    elif (time since last write >= 5s):
      → Firebase.setString("mode", pumpMode)  ← rate-limited retry
      → pendingModeWritebackSentMs = millis()
    [mode read does NOT overwrite local pumpMode while pendingModeWriteback is true]
  else:
    pumpMode = Firebase value  ← normal operation
```

---

## 11. NVS Persistence — Offline-First Architecture

### 11.1 State Persisted to NVS (Namespace: `pump_state`)

| NVS Key | C++ Variable | Trigger for Write | Behavior on Boot Restore |
|---------|-------------|-------------------|--------------------------|
| `mode` | `pumpMode` (String) | On change | Pump resumes in last mode. `COUNTDOWN` triggers re-arm. |
| `dry_run_err` | `isDryRunError` (bool) | On change | P1 lockout persists through power cycle. Operator must clear. |
| `bypass_lvl` | `cfgBypassLevelSensor` (bool) | On change | Bypass state restored. NVS prevents accidental level-stop on bypass devices. |
| `pump_cycles` | `totalPumpCycles` (uint32) | On each pump cycle | Lifetime counter restored. |
| `pump_run_sec` | `totalPumpRunSec` (ulong) | On each pump cycle | Lifetime runtime restored. |
| `last_reboot_id` | `lastRebootRequestId` (int) | On reboot request | Prevents duplicate reboot on same ID. |
| `cd_dur_min` | `cfgLastCountdownDurationMin` (int) | On countdown start (if duration changes) | Offline fallback duration for next countdown. |
| `level_pct` | `waterLevelPct` (int) | Delta ≥ 5% OR every 5 min | Diagnostic only (not used in boot logic). |
| `last_boot_ms` | (uptime tracker) | Every 60s | Crash loop detection accuracy. |

### 11.2 Config Persisted to NVS (Namespace: `pump_cfg`)

All device config values from `readDeviceConfigFromFirebase()` are saved when any value changes. Loaded on boot as offline fallback.

| Group | NVS Keys | Dashboard Editable |
|-------|---------|-------------------|
| Tank calibration | `tank_empty`, `tank_full` | Yes |
| Pump thresholds | `pump_start`, `pump_stop` | Yes |
| Dry-run protection | `dry_run_lpm`, `dry_run_sec` | Yes |
| Flow calibration | `flow_cal` | Yes |
| Max runtime | `max_runtime` | Yes |
| Sleep schedule | `slp_en`, `slp_start`, `slp_end`, `slp_emerg` | Yes |
| Advanced (sensor) | `sens_thresh`, `idle_sens_ms`, `idle_fb_ms` | Yes |
| Advanced (bypass) | `auto_bypass_en`, `auto_bypass_sec` | Yes |
| Schema version | `schema_ver` | No (firmware-managed) |

### 11.3 NVS Write Strategy (Flash Wear Reduction)

| Data | Write Trigger | Rationale |
|------|-------------|-----------|
| `pumpMode` | Immediate on change | Critical — must survive power cut |
| `isDryRunError` | Immediate on change | Safety state — must survive |
| `cfgBypassLevelSensor` | Immediate on change | Bypass state critical for next boot |
| `totalPumpCycles/RunSec` | On each pump cycle | Telemetry — acceptable wear rate |
| `waterLevelPct` | Delta ≥ 5% OR 5 min elapsed | Reduces writes during rapid fill |
| `last_boot_ms` (uptime) | Every 60s | Crash loop accuracy requires frequent update |
| Device config | Only when Firebase delivers changed values | Prevents wear on stable deployments |

---

## 12. Timing Reference

### 12.1 All System Intervals

| Operation | Normal | Idle Mode | Sleep Mode | Configurable |
|-----------|--------|-----------|-----------|-------------|
| Sensor read (level + flow) | **1,000 ms** | `cfgIdleSensorIntervalMs` (5,000–60,000 ms, default 10,000) | **30,000 ms** | Idle: Yes |
| Safety checks (P1) | **1,000 ms** | Same as sensor | Same | — |
| Countdown expiry check | **1,000 ms** | Same as sensor | Same | — |
| Pump state machine | **1,000 ms** | Same as sensor | Same | — |
| Firebase control read | **3,000 ms** | `cfgIdleFirebaseIntervalMs` (default 30,000) | **30,000 ms** | Idle: Yes |
| Firebase status push | **3,000 ms** | Same as Firebase | Same | — |
| Status push retry | **1,000 ms** (up to 3×) | Same | Same | — |
| Device config read | **30,000 ms** | Same | Same | — |
| Firebase auth cooldown | **30,000 ms** | — | — | — |
| WiFi retry initial | **5,000 ms** | — | — | — |
| WiFi retry max | **60,000 ms** | — | — | — |
| WiFi jitter | ±2,000 ms | — | — | — |
| Idle stable time threshold | 300,000 ms (5 min) | — | — | No |
| RSSI update | 60,000 ms | — | — | — |
| NVS uptime write | 60,000 ms | — | — | — |
| Heap diagnostics | 600,000 ms (10 min) | — | — | — |
| Sensor telemetry window | 60,000 ms | — | — | — |
| Safe mode heartbeat | 30,000 ms | — | — | — |
| Watchdog timeout | 120,000 ms | — | — | — |
| Safe mode auto-clear | 3,600,000 ms (1 hr) | — | — | — |

### 12.2 Timing Accuracy Notes

- All intervals use `millis()` (unsigned long, 1ms resolution, wraps at ~49.7 days).
- Uptime for dashboard uses `esp_timer_get_time()` (µs precision, no rollover issue).
- Countdown timer uses `millis()` exclusively — immune to network outages. Timer accuracy: ±1 sensor cycle (1s at normal intervals).
- Light sleep wakeup uses `esp_sleep_enable_timer_wakeup()` — accurate to ~100ms minimum.

---

## 13. Configuration Thresholds Reference

### 13.1 Compiled Defaults (all overridable via dashboard Device Config)

| Parameter | Constant | Default | Valid Range | Impact of Change |
|-----------|---------|---------|------------|-----------------|
| `pump_start_level` | `PUMP_START_LEVEL` | **30%** | 0–99% | AUTO starts pump at/below this level |
| `pump_stop_level` | `PUMP_STOP_LEVEL` | **100%** | 1–100% | AUTO/COUNTDOWN stops at/above this level |
| `tank_empty_cm` | `TANK_EMPTY_CM` | **122 cm** | 5–200 cm | 0% reference distance from sensor |
| `tank_full_cm` | `TANK_FULL_CM` | **8 cm** | 1–(tank_empty-1) cm | 100% reference distance from sensor |
| `dry_run_threshold_lpm` | `DRY_RUN_THRESHOLD_LPM` | **0.5 L/min** | 0.1–10.0 | Below this while pump ON: dry-run timer starts |
| `dry_run_timeout_sec` | `DRY_RUN_TIMEOUT_MS/1000` | **30 s** | 10–300 s | Duration of low flow before P1 lockout |
| `max_pump_runtime_min` | `MAX_PUMP_RUNTIME_MIN` | **120 min** | 30–480 min | Max continuous AUTO/COUNTDOWN/MANUAL runtime |
| `flow_calibration_factor` | `FLOW_CALIBRATION_FACTOR` | **7.5** | 0.1–20.0 | `Q = pulseHz / factor` (YF-G1 K-factor) |
| `level_sensor_failure_threshold` | `SENSOR_FAILURE_THRESHOLD` | **5 failures** | 3–20 | Consecutive timeouts before error flag |
| `auto_bypass_delay_sec` | `AUTO_BYPASS_FAILURE_SEC_DEF` | **60 s** | 10–300 s | Time after error before auto-bypass (if enabled) |
| `auto_bypass_on_sensor_fail` | (bool) | **false** | bool | If true: auto-bypass on sustained sensor failure |
| `idle_sensor_interval_ms` | `IDLE_SENSOR_INTERVAL_MS_DEF` | **10,000 ms** | 5,000–60,000 | Slow-poll rate when tank full + pump off |
| `idle_firebase_interval_ms` | `IDLE_FIREBASE_INTERVAL_MS_DEF` | **30,000 ms** | 10,000–120,000 | Firebase interval in idle mode |
| `sleep_enabled` | `SLEEP_DEFAULT_ENABLED` | **false** | bool | Enable scheduled sleep window |
| `sleep_start_hour` | `SLEEP_DEFAULT_START_HOUR` | **23 (11PM)** | 0–23 | Sleep window start (PHT, UTC+8) |
| `sleep_end_hour` | `SLEEP_DEFAULT_END_HOUR` | **5 (5AM)** | 0–23 | Sleep window end (PHT, UTC+8) |
| `sleep_emergency_level` | `SLEEP_DEFAULT_EMERGENCY_LVL` | **5%** | 0–100% | Level below which sleep is overridden |

### 13.2 Compile-Time Only Constants (cannot be changed without reflash)

| Constant | Value | Purpose |
|---------|-------|---------|
| `RELAY_PIN` | GPIO 4 | Pump relay output |
| `TRIG_PIN` | GPIO 5 | Ultrasonic trigger |
| `ECHO_PIN` | GPIO 18 | Ultrasonic echo |
| `FLOW_SENSOR_PIN` | GPIO 34 | Flow pulse interrupt |
| `TANK_CAPACITY_L` | 660 L | Used for flow-based estimate |
| `ULTRASONIC_SAMPLES` | 5 | Median filter depth |
| `ULTRASONIC_SAMPLE_DELAY` | 80 ms | Inter-sample settling |
| `ULTRASONIC_EMA_ALPHA` | 0.5 | EMA smoothing factor |
| `ULTRASONIC_TIMEOUT_MS` | 100 ms | Single reading timeout |
| `LEVEL_RATE_OF_CHANGE_MAX` | 15% | Max level jump per cycle |
| `FLOW_DEBOUNCE_US` | 2,000 µs | ISR debounce window |
| `FLOW_MAX_SANE_LPM` | 100.0 L/min | Max physically possible flow |
| `FLOW_PUMP_OFF_ZERO_MS` | 3,000 ms | Force zero after pump stop |
| `FLOW_STUCK_THRESHOLD_LPM` | 2.0 L/min | Stuck-high detection threshold |
| `FLOW_STUCK_TIMEOUT_MS` | 5,000 ms | Stuck-high persistence before flag |
| `COUNTDOWN_ADD_TIME_MIN` | 5 min | Default add-time increment |
| `COUNTDOWN_MAX_DURATION_MIN` | 120 min | Max countdown / add-time ceiling |
| `CRASH_LOOP_THRESHOLD` | 5 reboots | Reboots triggering safe mode |
| `CRASH_LOOP_WINDOW_SEC` | 300 s (5 min) | Window for crash loop detection |
| `WDT_TIMEOUT_SEC` | 120 s | Hardware watchdog timeout |
| `NVS_SCHEMA_VERSION` | 1 | NVS layout version |
| `NVS_LEVEL_DELTA_THRESHOLD` | 5% | Minimum level change for NVS write |
| `STATUS_PUSH_RETRY_MAX` | 3 | Max retry attempts before cooldown |
| `STATUS_PUSH_RETRY_MS` | 1,000 ms | Retry interval |
| `WIFI_BACKOFF_INITIAL_MS` | 5,000 ms | First retry delay |
| `WIFI_BACKOFF_MAX_MS` | 60,000 ms | Maximum retry delay |
| `WIFI_JITTER_MS` | ±2,000 ms | Backoff jitter |
| `FIREBASE_AUTH_COOLDOWN_MS` | 30,000 ms | Cooldown after auth error |
| `MIN_PUMP_OFF_TIME_MS` | 30,000 ms | Minimum off-time between pump starts (AUTO / MANUAL / COUNTDOWN) |

---

## 14. Status Telemetry Reference — All Fields Published to Firebase

`pushFirebaseStatus()` writes a single JSON object to `/pump_system/status` on every Firebase cycle.

### 14.1 Core State Fields (v5.0)

| Field | Type | Source | Description |
|-------|------|--------|-------------|
| `water_level_percent` | int (0–100) | `waterLevelPct` | Current tank fill level. From ultrasonic after median + EMA. Holds last valid on timeout. |
| `run_mode` | string | `runMode` | Operational state: `"OFF"` · `"AUTO_STANDBY"` · `"AUTO"` · `"MANUAL"` · `"MANUAL_OFF"` · `"COUNTDOWN"` · `"FORCE_ON"` |
| `is_running` | bool | `isRunning` | `true` when relay is energized (GPIO4 LOW) |
| `flow_rate_lpm` | float | `flowRateLpm` | L/min from YF-G1. Zeroed 3s after pump off. Spikes > 100 LPM discarded. |
| `countdown_remaining_sec` | int | `countdownEndMs - millis()` | Seconds remaining in COUNTDOWN. 0 when not active. |

### 14.2 Error & Safety State Fields

| Field | Type | Source | Description |
|-------|------|--------|-------------|
| `is_error` | bool | `isDryRunError` | `true` = Dry-run lockout active. Pump will not run. |
| `is_overflow_error` | bool | `isOverflowError` | `true` = Max runtime exceeded. Requires `clear_error`. |
| `is_level_sensor_error` | bool | `isLevelSensorError` | `true` = ≥5 consecutive ultrasonic timeouts. |
| `is_flow_sensor_error` | bool | `isFlowSensorError` | `true` = Stuck-high detection (diagnostic, no lockout). |
| `bypass_level_sensor` | bool | `cfgBypassLevelSensor` | `true` = Level sensor bypassed (manual or auto). |
| `auto_bypass_active` | bool | `autoBypassActive` | `true` = Auto-bypass engaged by firmware (not manual). |
| `is_sleeping` | bool | `isSleeping` | `true` = Scheduled sleep window active. |
| `last_fault_code` | string | `lastFaultCode` | `"DRY_RUN"` · `"OVERFLOW"` · `"LEVEL_SENSOR"` · `"FLOW_SENSOR"` · `"SAFE_MODE"` · `""` |
| `last_fault_message` | string | `lastFaultMessage` | Human-readable fault description. |

### 14.3 Sensor Diagnostics Fields

| Field | Type | Source | Description |
|-------|------|--------|-------------|
| `level_sensor_health_pct` | int (0–100) | Computed in push | `100 - (failCount × 20) - (20 if >30s stale)`. Dashboard health indicator. |
| `level_last_valid_age_sec` | int | `millis() - levelLastValidMs` | Seconds since last valid ultrasonic reading. Dashboard: show stale warning > 30s. |
| `estimated_level_pct` | int (0–100) | `estimatedLevelPct` | Flow-based estimate. Only present when ≥ 0.0f. |
| `level_estimate_active` | bool | `estimatedLevelPct ≥ 0 && cfgBypassLevelSensor` | `true` = Flow estimate in use. Dashboard: label level as "~Estimated (±5%)". |
| `flow_volume_added_l` | float | `flowVolumeAddedL` | Litres added since last level anchor. Diagnostic. |
| `ultrasonic_cycles_ok` | int | `ultrasonicCycleOkCount` | Lifetime count of successful cycles. |
| `ultrasonic_cycles_timeout` | int | `ultrasonicCycleTimeoutCount` | Lifetime count of all-timeout cycles. |
| `ultrasonic_last_good_cm` | float | `ultrasonicLastGoodCmX10 / 10.0` | Last valid distance reading in cm (×10 stored for precision). |
| `flow_discard_max_sane` | int | `flowDiscardMaxSaneCount` | Lifetime count of flow readings discarded as physically impossible. |
| `flow_stuck_high_events` | int | `flowStuckHighEventCount` | Lifetime count of stuck-high events. |

### 14.4 Telemetry & Connectivity Fields

| Field | Type | Source | Description |
|-------|------|--------|-------------|
| `total_pump_cycles` | int | `totalPumpCycles` (NVS) | Lifetime pump start count. |
| `total_pump_run_min` | int | `totalPumpRunSec / 60` (NVS) | Lifetime runtime in minutes. |
| `wifi_rssi` | int | `wifiRssi` | WiFi signal dBm. Updated every 60s. |
| `uptime_minutes` | uint32 | `esp_timer_get_time() / 60M` | Minutes since last boot. No rollover. |
| `last_boot_reason` | string | `bootReasonStr` | `"Power-on"` · `"Watchdog"` · `"Brownout"` · `"Software reset"` · etc. |
| `free_heap_bytes` | int | `ESP.getFreeHeap()` | Current free heap. |
| `min_free_heap_bytes` | int | `ESP.getMinFreeHeap()` | Minimum free heap since boot (ESP32 SDK). |
| `max_alloc_heap_bytes` | int | `ESP.getMaxAllocHeap()` | Maximum contiguous allocatable block. |
| `min_free_heap_observed_bytes` | int | `minFreeHeapObserved` | Firmware-tracked minimum (cross-checked with SDK). |
| `firebase_consecutive_failures` | int | `firebaseConsecutiveFailCount` | Current consecutive failure count. Resets on success. |
| `firebase_last_error` | string | `firebaseLastError` | Error string from most recent Firebase failure. |

---

## 15. Control Key Reference — All Dashboard-Writable Keys (v5.0)

All keys under `/pump_system/control/` are read as a **single atomic JSON** per Firebase cycle.

| Key | Type | One-Shot? | Reset By | Behavior |
|-----|------|----------|---------|---------|
| `mode` | string | No (persistent) | Dashboard | `"AUTO"` · `"MANUAL"` · `"COUNTDOWN"` · `"FORCE_OFF"` · `"FORCE_ON"`. Applied each cycle unless `pendingModeWriteback` is active. |
| `manual_start` | bool | **Yes (edge-detect)** | Dashboard (5s reset) | Rising edge (`false→true`): `pumpMode = "MANUAL"`, `isManualRun = true`. Rejected if `isDryRunError` / `isOverflowError` are true, `pumpMode = "FORCE_OFF"`, or minimum off-time (`MIN_PUMP_OFF_TIME_MS`) has not elapsed since the last stop. |
| `manual_stop` | bool | **Yes (edge-detect)** | Dashboard (5s reset) | Rising edge: if `pumpMode = "FORCE_ON"` or `"FORCE_OFF"` → ignored; if MANUAL (pumpMode == "MANUAL") → `setPump(false)`, `runMode = "MANUAL_OFF"`, `pumpMode` unchanged; if COUNTDOWN active → stop pump, clear countdown, set `pumpMode = "AUTO"`. |
| `countdown_duration_min` | int (1–120) | No | Persistent | Duration for next COUNTDOWN start. Read once per countdown activation. |
| `countdown_add_time` | bool | **Yes (edge-detect)** | **Firmware** | Rising edge: extend `countdownEndMs` by `countdown_add_min` minutes. Firmware sets `false` after processing. |
| `countdown_add_min` | int (1–120) | No | Persistent | Add-time increment override. Falls back to 5 min if absent. |
| `bypass_level_sensor` | bool | No | Persistent (admin) | `true` = bypass active; `false` = restore + clear auto-bypass. Persisted to NVS. |
| `clear_error` | bool | **Yes** | **Firmware** | Clears `isDryRunError`, `isOverflowError`, all timers, `lastFaultCode`. Firmware sets `false` after applying. |
| `reboot_request_id` | int | **Yes (ID-based)** | Persistent | New non-zero value != `lastRebootRequestId` triggers reboot. ID persisted to NVS. |

### 15.1 One-Shot Command Timing

| Command | Reset Mechanism | Guard Against Re-fire | Latency |
|---------|----------------|----------------------|---------|
| `manual_start` | Dashboard resets to `false` after 5s | `lastManualStart` static — fires only on `false→true` edge | ~3s (next Firebase cycle) |
| `manual_stop` | Dashboard resets to `false` after 5s | `lastManualStop` static — fires only on `false→true` edge | ~3s |
| `countdown_add_time` | **Firmware** resets to `false` after processing | `lastAddTime` static — fires only on `false→true` edge. `pendingModeWritebackSentMs = 0` on new countdown start clears carry-over | ~3s + Firebase propagation |
| `clear_error` | **Firmware** sets `false` after applying | Checked only when `isDryRunError OR isOverflowError` is true | ~3s |

---

## 16. Boot & Initialization Sequence

### 16.1 `setup()` — Complete Annotated Sequence

```
setup() — runs once at power-on or reset
│
├─ Serial.begin(115200)
├─ Log: "Smart Water Pump Controller v5.0.0"
│
├─ bootReasonStr = getBootReasonString()
│   ← esp_reset_reason(): Power-on / Watchdog / Brownout / Software / etc.
│
├─ GPIO initialization:
│   pinMode(RELAY_PIN=4, OUTPUT)    — Relay
│   pinMode(TRIG_PIN=5, OUTPUT)     — Ultrasonic trigger
│   pinMode(ECHO_PIN=18, INPUT)     — Ultrasonic echo
│   pinMode(FLOW_SENSOR_PIN=34, INPUT) — Flow pulse
│
├─ setPump(false)                   — Relay HIGH (OFF) immediately. SAFETY CRITICAL.
├─ digitalWrite(TRIG_PIN, LOW)      — Ensure no spurious trigger
│
├─ checkCrashLoop()
│   ├─ Opens NVS "pump_state"
│   ├─ Reads: last_boot_ms, boot_count, safe_mode_ms
│   ├─ If fresh power cycle (millis() < 5000) AND safe_mode_ms > 0:
│   │   → Clear safe_mode_ms, boot_count=0, return  ← Safe mode cleared
│   ├─ If lastBootTime > 300s: bootCount = 0  ← Stable prior run, not crash loop
│   ├─ bootCount++, save to NVS
│   └─ If bootCount ≥ 5: inSafeMode = true, lastFaultCode = "SAFE_MODE"
│
├─ if (inSafeMode): return  ← EXITS SETUP EARLY. loop() handles safe mode only.
│
├─ attachInterrupt(GPIO34, flowPulseISR, RISING)
│   ← ISR in IRAM, 2ms debounce via esp_timer_get_time()
│
├─ loadDeviceConfigFromNVS()
│   ← Reads "pump_cfg" namespace. Validates all fields. Falls back to compiled defaults.
│
├─ loadStateFromNVS()
│   ← Reads "pump_state": mode, dry_run_err, bypass_lvl, pump_cycles, pump_run_sec,
│     last_reboot_id, cd_dur_min
│
├─ delay(5000)  ← Startup stabilization: sensors settle, capacitors charge
│
├─ connectWiFi()
│   ← WiFi.mode(STA), WiFi.persistent(false), WiFi.begin()
│   ← Blocks up to 20s (40 × 500ms attempts)
│
├─ if (WiFi connected):
│   ├─ configTime(UTC+8, 0, "pool.ntp.org", "time.nist.gov")
│   ├─ getLocalTime(..., 5000ms) → ntpSynced = true if successful
│   └─ initFirebase()
│       ← config: api_key, database_url
│       ← auth: email/password
│       ← setReadTimeout(10000ms), setwriteSizeLimit("medium"), setResponseSize(1024)
│       ← Firebase.begin(), Firebase.reconnectWiFi(true)
│
├─ WDT initialization:
│   esp_task_wdt_deinit()  ← Clear any existing WDT config
│   esp_task_wdt_init(120s, panic=true)
│   esp_task_wdt_add(NULL)  ← Register loopTask
│
└─ Initialize all millis() timers:
    lastSensorMs = lastFirebaseMs = lastRssiLogMs = lastLevelWriteMs
    = lastUptimeWriteMs = lastHeapDiagMs = millis()
    lastDeviceConfigMs = lastWifiRetryMs = 0
    minFreeHeapObserved = ESP.getFreeHeap()
```

### 16.2 First Firebase Cycle After Boot

On the first Firebase cycle after boot, `lastDeviceConfigMs = 0` causes `readDeviceConfigFromFirebase()` to execute immediately (before the normal 30s interval). This ensures cloud-configured thresholds override NVS/compiled defaults as quickly as possible after network connection.

### 16.3 Post-Boot State

After `setup()` completes:
- Relay is OFF (GPIO4 HIGH)
- `pumpMode` is restored from NVS (or default `"AUTO"`)
- `isDryRunError` is restored from NVS
- `cfgBypassLevelSensor` is restored from NVS
- All config thresholds are from NVS (will be overridden by Firebase on first config read)
- WiFi may or may not be connected
- Pump loop begins running immediately on first `loop()` call

---

*End of Document — Smart Water Pump Controller Firmware v5.0.0 Master Technical Specification*

*All behavior described in this document is derived directly from the firmware source code via static analysis. No behavior is inferred or assumed.*
