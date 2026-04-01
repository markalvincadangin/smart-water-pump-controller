# SmartFlow — System Refactor Plan
### Engineering Specification v2.0

**Project:** SmartFlow (formerly Smart Water Pump Controller)
**Location:** Leon, Iloilo, Philippines
**Classification:** Embedded IoT — ESP32 Master + NodeMCU V2 Slave + Next.js Firebase Dashboard
**Plan type:** Systematic refactor with research-first methodology
**Document version:** 2.0 (merged from v1.0-A and v1.0-B)
**Optimized for:** AI agent execution

---

> **Agent notice**
> This document is the authoritative specification for the SmartFlow refactor. It is
> structured for AI agent execution. Complete Phase 0 in full before writing a single
> line of code. Every subsequent phase references findings from Phase 0. All assumptions
> in this document are unverified until Phase 0 confirms or contradicts them — Phase 0
> findings supersede everything written here.

---

## Governing Principles

All decisions in this plan follow this hierarchy, in order of precedence:

1. **Safety first.** The pump must fail safe. Any ambiguity in logic resolves toward pump OFF, never pump ON. The hardware TOR layer is never bypassed by any firmware change.
2. **Do not break what works.** Touch only what is confirmed incorrect, missing, or unnecessarily complex. Working subsystems are left alone.
3. **Feasibility over completeness.** Scope is constrained to what can be implemented, tested, and deployed without introducing new failure modes.
4. **Observability before optimization.** If the system cannot be observed, it cannot be debugged. Debug infrastructure comes before all other changes.
5. **Engineering basis required.** Every threshold, timeout, and behavioral rule must cite a source — datasheet, industry standard, or documented empirical measurement from the installed system.
6. **No scope creep.** If something looks fixable but is not in this plan, document it and leave it alone.

---

## Scope Definition

### In Scope

| # | Area | Work |
|---|---|---|
| S1 | Firmware — ESP32 master | Bug fixes confirmed in Phase 0 audit |
| S2 | Firmware — ESP32 master | 5-level structured log system |
| S3 | Firmware — ESP32 master | Run mode state machine corrections |
| S4 | Firmware — NodeMCU V2 slave | Bug fixes confirmed in Phase 0 audit |
| S5 | Firmware — NodeMCU V2 slave | Debug transport architecture (GPIO2/Serial1) |
| S6 | Firmware — both nodes | RS-485 protocol hardening and LDSC frame field |
| S7 | Firebase RTDB | Additive schema fields only (backward compatible) |
| S8 | Test firmware | Standalone test sketches for master and slave |
| S9 | Dashboard | Visual redesign under SmartFlow brand |
| S10 | Dashboard | New components for all new Firebase fields |
| S11 | All files | SmartFlow rebranding string substitution |
| S12 | Docs | Audit report, protocol spec, state machine doc |

### Out of Scope

| # | Area | Reason |
|---|---|---|
| O1 | Hardware changes | BOM, pinout, and enclosure are fixed |
| O2 | Firebase security rules | Separate sprint |
| O3 | OTA firmware update | Future phase |
| O4 | Mobile native app | PWA remains the delivery mechanism |
| O5 | Additional sensor types | No new hardware |
| O6 | Cloud Functions rewrite | Not a source of known bugs |
| O7 | Multi-user authentication | Single-user system |

---

## Phase Structure

```
Phase 0 — Research & Audit          ← Non-skippable prerequisite
Phase 1 — Debug Infrastructure      ← Prerequisite for all firmware phases
Phase 2 — Slave Node Bug Fixes      ← NodeMCU V2 firmware (requires Phase 1)
Phase 3 — Master Node Bug Fixes     ← ESP32 firmware (requires Phase 1)
Phase 4 — Protocol & Schema         ← Formalizes RS-485 + Firebase contract
Phase 5 — Test Firmware Suite       ← Standalone test sketches
Phase 6 — Dashboard Redesign        ← UI/UX + SmartFlow branding
Phase 7 — Integration & Validation  ← Full system sign-off
```

Phases 2–6 may proceed in parallel after Phase 1 is complete. Phase 7 gates deployment.

---

---

# Phase 0 — Research & System Audit

## Objective

Establish an accurate, current baseline before any modification. Every assumption from previous reviews is treated as unverified. Do not skip any sub-section.

## 0.1 Firmware Source Inventory

**Deliverable:** Complete file manifest for both firmware projects.

Read every source file in:
- `firmware/arduino_smart_water_pump_controller/` (ESP32 Arduino project)
- `firmware/platformio_smart_water_pump_controller/src/` (ESP32 PlatformIO)
- `firmware/arduino_sensor_node/` (NodeMCU Arduino)
- `firmware/platformio_sensor_node/src/` (NodeMCU PlatformIO)

For each file, record:
- File name and path
- Primary responsibility
- Dependencies (files included or called)
- Any `TODO`, `FIXME`, `HACK`, or `TEMP` comments
- All compile-time flags that alter behavior (`#define`, `#if`, `#ifdef`)

Cross-reference against the README to identify files that have been added, removed, or changed since the last documented review.

## 0.2 Pin Assignment Verification

**Deliverable:** Verified pin assignment table for both microcontrollers, cross-checked against firmware constants and hardware documentation.

**Rationale:** Pin assignments have changed at least once (NodeMCU ECHO moved from D2/GPIO4 to D0/GPIO16; ESP32 RS-485 RX changed from GPIO16 to GPIO25). Undocumented changes produce incorrect wiring docs and potential GPIO conflicts.

Extract all `#define PIN_*`, `#define RS485_*`, `#define RELAY_PIN` and equivalent pin constants from both firmware projects. Compare against:
- `hardware/wiring_notes.md`
- `hardware/bom.md`
- `hardware/enclosure_layout.md`

Flag any discrepancy as a finding that must be resolved before Phase 2.

Verified pin table format:

| Signal | ESP32 GPIO | NodeMCU GPIO | Hardware doc GPIO | Match? |
|--------|-----------|-------------|-------------------|--------|
| Relay output | — | — | — | — |
| RS-485 TX | — | — | — | — |
| RS-485 RX | — | — | — | — |
| RS-485 DE/RE | — | — | — | — |
| Ultrasonic TRIG | — | — | — | — |
| Ultrasonic ECHO | — | — | — | — |
| Flow sensor | — | — | — | — |

## 0.3 Firebase Schema Audit

**Deliverable:** Complete, accurate Firebase RTDB schema map with all current read/write paths.

Read `pushFirebaseStatus()`, `readFirebaseControl()`, and `readDeviceConfigFromFirebase()` in their current form. Extract every field name, data type, write condition, and read condition. Produce a canonical schema table in three sections:

- `/pump_system/status` — all fields written by ESP32
- `/pump_system/control` — all fields read by ESP32
- `/pump_system/config/device` — all fields read by ESP32

Note any field present in firmware but absent from the dashboard, and any dashboard control that writes a field not read by the firmware.

## 0.4 Dashboard Stack Confirmation

**Deliverable:** Confirmed technology stack, component structure, Firebase SDK version, feature completeness.

Confirm:
- Framework (Next.js App Router, Next.js Pages Router, plain HTML/JS, or other)
- Firebase SDK version and auth method
- List of all currently implemented dashboard panels, controls, and data displays
- Any `manifest.json` / PWA configuration present
- Current color palette and typography

## 0.5 Known Bug Verification

**Deliverable:** Triage table confirming current status of every previously identified bug.

Verify each of the following:

| Bug ID | Description | Verify status |
|--------|-------------|---------------|
| C-01 | Missing `void setup()` declaration in main `.ino` | Present / Fixed |
| C-02 | `waterLevelPct` initialized to `0` before first valid RS-485 frame | Present / Fixed |
| H-01 | No log verbosity levels — all Serial output flat | Present / Fixed |
| H-02 | Level plausibility filter discards silently with no counter or error promotion | Present / Fixed |
| H-03 | Flow discard debug print reads zeroed global `flowPulseDiscardCount` instead of local `disc` | Present / Fixed |
| H-04 | Flow error flag non-hysteretic — single sample flips `snFlowError` every second | Present / Fixed |
| H-05 | Overflow protection fires and stops pump in MANUAL mode (should warn only) | Present / Fixed |
| H-06 | Crash loop counter clears at 60 s — too short for full boot sequence | Present / Fixed |
| H-07 | No `AUTO_COOLDOWN` run mode when motor off-timer is active | Present / Fixed |
| M-01 | Two overlapping level timestamps (`levelLastValidMs` and `levelLastUpdateMs`) | Present / Fixed |
| M-02 | `cfgBypassFlowSensor` has no runtime Firebase control path | Present / Fixed |
| M-03 | RS-485 partial frame receives no inter-byte timeout — receiver never resets on stall | Present / Fixed |
| M-05 | `runMode` initialized to `"OFF"` instead of `"AUTO_STANDBY"` | Present / Fixed |
| M-06 | `is_idle_mode` not pushed to Firebase status | Present / Fixed |

Add any new bugs discovered during this phase to the table with severity: **Critical / High / Medium / Low**.

## 0.6 ISR Safety Audit (New — Added in v2.0)

Verify the following across both firmware projects:

- Flow sensor pulse counter: confirm `volatile` qualifier on the ISR-modified variable
- Confirm the main-loop read of the pulse counter uses a safe atomic pattern (disable interrupts, read, re-enable) and does not read the counter directly
- Confirm no other ISR-modified variable is read from the main loop without protection

## 0.7 Phase 0 Deliverable

Produce `docs/audit/refactor_audit_2026.md` with all of the following before proceeding:

1. File manifest (§0.1)
2. Pin assignment table with discrepancy flags (§0.2)
3. Firebase schema table (§0.3)
4. Dashboard stack and feature completeness (§0.4)
5. Bug triage table with current status (§0.5)
6. ISR safety findings (§0.6)
7. Revised scope for Phases 1–7 based on findings (what was fixed, what is new, what changed)

**Phase 0 exit criterion:** All seven deliverables are complete. No other phase begins until this criterion is met.

---

---

# Phase 1 — Debug Infrastructure

## Objective

Build the observability layer that all subsequent firmware phases depend on. No module is refactored before this infrastructure is in place. This phase has zero functional behavior changes — it is purely additive.

## Engineering Basis

IEC 61508 Part 7 (Annex D) identifies diagnostic coverage as a key metric for programmable safety systems. Diagnostic coverage requires that failures can be detected and reported within a bounded time window. Without structured, leveled logging, failures that do not immediately trip a safety function remain invisible in the field.

Reference: Storey, N. (1996). *Safety-Critical Computer Systems*. Addison-Wesley. Chapter 10 — Fault Detection and Diagnosis.

## 1.1 Log Level Architecture

Define five severity levels as integer constants in both firmware projects. Place in the shared header (ESP32) and node shared header (NodeMCU):

| Level | Constant | Production default | Description |
|-------|----------|--------------------|-------------|
| 0 | `LOG_ERROR` | Always output | Safety trips, hardware failures, crash detection |
| 1 | `LOG_WARN` | Always output | Degraded states, comm loss, approaching limits |
| 2 | `LOG_INFO` | Always output | State transitions, mode changes, boot events |
| 3 | `LOG_DEBUG` | Off by default | Per-cycle sensor readings, RS-485 frame details |
| 4 | `LOG_VERBOSE` | Off by default | State machine internals, raw ISR counts, timer values |

**Compile-time floor** (`LOG_COMPILE_FLOOR`): a `#define` in the build config. Calls below this level are removed by the preprocessor — zero binary size overhead, zero runtime cost.

- Development builds: `LOG_COMPILE_FLOOR = LOG_DEBUG`
- Release-optimized builds: `LOG_COMPILE_FLOOR = LOG_INFO`

**Runtime ceiling** (`gLogLevel`): a `uint8_t` global initialized from NVS (ESP32) or compile-time define (NodeMCU). Calls above `gLogLevel` are skipped at runtime. On ESP32, this value is readable and writable via Firebase config — enables field verbosity increases without reflashing.

## 1.2 Log Format — Structured Prefix

All log calls produce output in this exact format:

```
[L][MODULE][MS] message content
```

Where:
- `L` = single character level code: `E`, `W`, `I`, `D`, `V`
- `MODULE` = 4–6 character uppercase tag: `PUMP`, `RS485`, `WIFI`, `FIREBASE`, `SENSOR`, `BOOT`, `NVS`, `SAFETY`
- `MS` = `millis()` as a 10-digit zero-padded decimal

Examples:
```
[E][PUMP][0045231] DRY_RUN lockout. flow=0.08LPM < 1.0LPM for 30s. Relay OFF.
[W][RS485][0046002] Frame timeout attempt 2/3. Retrying.
[I][BOOT][0001240] NVS config loaded. mode=AUTO cycles=142 run_sec=8820
[D][SENSOR][0047100] lvl=82% dist=45.2cm flow=8.30LPM err=0 seq=142
[V][SAFETY][0047105] DryRun timer: 2100ms / 30000ms. flow=0.08LPM
```

## 1.3 LOG Macro Implementation

```cpp
// In shared header — ESP32
#define LOG_ERROR   0
#define LOG_WARN    1
#define LOG_INFO    2
#define LOG_DEBUG   3
#define LOG_VERBOSE 4

#ifndef LOG_COMPILE_FLOOR
  #define LOG_COMPILE_FLOOR LOG_DEBUG
#endif

extern uint8_t gLogLevel;

static const char LOG_LEVEL_CHAR[] = { 'E', 'W', 'I', 'D', 'V' };

#define LOG(level, module, fmt, ...) \
  do { \
    if ((level) <= LOG_COMPILE_FLOOR && (level) <= gLogLevel) { \
      Serial.printf("[%c][%s][%010lu] " fmt "\n", \
        LOG_LEVEL_CHAR[level], module, millis(), ##__VA_ARGS__); \
    } \
  } while(0)
```

For the NodeMCU, the macro routes to `Serial` (bench mode) or `Serial1` (production mode) based on `DEBUG_USB_MODE`.

## 1.4 ESP32 Debug Transport

The ESP32 uses UART0 via USB for all debug output. UART2 is reserved for RS-485. No transport conflict exists. No special handling needed.

**Remote log level control:** `/pump_system/config/device/debug_log_level` accepts an integer 0–4. `readDeviceConfigFromFirebase()` reads and applies this value to `gLogLevel` at runtime. No reflashing required for field verbosity changes.

**Push current log level to Firebase status:** Include `debug_log_level: gLogLevel` in `pushFirebaseStatus()` so the dashboard always reflects the active log level.

## 1.5 NodeMCU Debug Transport

The NodeMCU UART0 (GPIO1 TX / GPIO3 RX) is shared between RS-485 and USB flashing. This is the fundamental hardware constraint. Two transport modes are selected at compile time via `DEBUG_USB_MODE`.

**Production mode (`DEBUG_USB_MODE = 0`) — field deployment:**
- UART0 (GPIO1/3): RS-485 to MAX485
- Serial1 (GPIO2 TX-only): debug output
- Hardware required for monitoring: USB-TTL adapter, adapter RX → NodeMCU GPIO2, shared GND
- The NodeMCU onboard LED is wired to GPIO2 and will flicker during Serial1 output — expected, not a fault

**Bench / flash mode (`DEBUG_USB_MODE = 1`):**
- UART0: USB Serial for output and flashing
- MAX485 DI and RO: physically disconnected from NodeMCU TX/RX
- Serial1: disabled
- Used only during development, bench testing, and reflashing

```cpp
// In sensor node shared header
#ifndef DEBUG_USB_MODE
  #define DEBUG_USB_MODE 0
#endif

#if DEBUG_USB_MODE == 1
  #warning "DEBUG_USB_MODE=1: RS-485 DISABLED. Do not flash to deployed device."
  #define SN_SERIAL_DEBUG Serial
  #define SN_SERIAL_RS485 // disabled — physically disconnect DI/RO
#else
  #define SN_SERIAL_DEBUG Serial1  // GPIO2
  #define SN_SERIAL_RS485 Serial   // UART0: GPIO1/3
#endif

#define LOG_SN(level, module, fmt, ...) \
  do { \
    if ((level) <= SN_LOG_COMPILE_FLOOR && (level) <= snLogLevel) { \
      SN_SERIAL_DEBUG.printf("[%c][%s][%010lu] " fmt "\n", \
        LOG_LEVEL_CHAR[level], module, millis(), ##__VA_ARGS__); \
    } \
  } while(0)
```

**Field debugging without reflash procedure** (document in `firmware/README.md`):
1. Connect USB-TTL adapter: adapter RX → NodeMCU GPIO2, shared GND
2. Open terminal at 115200 baud
3. Debug output streams via GPIO2 without disrupting RS-485 on GPIO1/3

## 1.6 Serial Output Triage — Migration Rules

Migrate every existing `Serial.printf` and `Serial.println` call to `LOG()` using this classification:

| Original content | Assigned level | Rationale |
|---|---|---|
| Safety trips, hardware failures, crash detection | `LOG_ERROR` | Always visible |
| Degraded states, comm loss, approaching limits | `LOG_WARN` | Always visible |
| State transitions, mode changes, boot events | `LOG_INFO` | Always visible |
| Per-cycle sensor readings, RS-485 frame details | `LOG_DEBUG` | Gated in production |
| State machine internals, timer values, raw ISR counts | `LOG_VERBOSE` | Developer only |

**Rate limiting:** WARN messages that repeat on the same sustained condition (e.g., WiFi reconnection attempts, Firebase write failures) must be rate-limited to once per 60 seconds using a `lastWarnMs` timestamp pattern.

**Target production Serial volume:** In `LOG_INFO` mode (default), serial output is limited to boot sequence and error/warning events only. Per-cycle sensor readings, RS-485 frame bytes, Firebase payloads, and timer internals are suppressed. Expected reduction from baseline: 80–90% in steady-state normal operation.

## 1.7 Phase 1 Exit Criteria

- `LOG()` macro implemented in both firmware projects
- `LOG_SN()` macro implemented in sensor node with `DEBUG_USB_MODE` routing
- All existing `Serial.printf` and `Serial.println` calls migrated to `LOG()` with correct level
- `gLogLevel` remote control via Firebase config working on ESP32
- `debug_log_level` field present in Firebase status push
- `#warning` directive fires when `DEBUG_USB_MODE = 1` is compiled
- Serial output volume in `LOG_INFO` mode confirmed ≥ 80% reduction from baseline
- No functional behavior changes — this phase is purely observability

---

---

# Phase 2 — Slave Node Bug Fixes (NodeMCU V2)

## Objective

Fix confirmed bugs in the sensor node firmware. Fix only what is confirmed present in Phase 0. Do not touch code that is working correctly.

## 2.1 Fix H-03: Flow Discard Debug Print Bug

**Problem:** The debug print for `pulses_discarded` reads the global `flowPulseDiscardCount` after it has already been zeroed into the local variable `disc`. The debug output always shows 0 regardless of actual discards.

**Fix:** Change the debug log call to use the local `disc` variable. One-line change. Comment: `// REFACTOR [H-03]: use local disc, not zeroed global`

**Verification:** With `DEBUG_USB_MODE = 1` and a noise source on the flow sensor wire, confirm `disc` shows non-zero in debug output.

## 2.2 Fix H-02: Level Discard Filter — Observability

**Problem:** The level plausibility filter discards readings silently with no counter, no log, and no error promotion. A failed sensor that rejects all samples is indistinguishable from a working one.

**Fix:**
- Add `uint16_t snLevelDiscardCount` to node state
- Increment on every plausibility filter rejection
- Log at `LOG_WARN` on first discard; rate-limit to once per 60 seconds
- Reset at the start of each measurement window
- If all samples in a window are rejected (e.g., 5/5 discarded), set `snLevelError = true`
- Include `snLevelDiscardCount` in the RS-485 response frame as the `LDSC` field (see Phase 4.1)

## 2.3 Fix H-04: Flow Error Flag Hysteresis

**Problem:** `snFlowError = (disc > 50)` is non-hysteretic and can oscillate true/false every second on a borderline sensor.

**Fix:** Replace with a two-stage counter system:

```cpp
// Assert after 3 consecutive seconds above threshold
if (disc > 50) {
  flowErrorAssertCount++;
  flowErrorClearCount = 0;
  if (flowErrorAssertCount >= 3) snFlowError = true;
} else if (disc <= 20) {
  // Clear after 5 consecutive seconds below clear threshold
  flowErrorClearCount++;
  flowErrorAssertCount = 0;
  if (flowErrorClearCount >= 5) snFlowError = false;
} else {
  // Hysteresis band — hold current state
  flowErrorAssertCount = 0;
  flowErrorClearCount = 0;
}
```

Assert threshold: `disc > 50`. Clear threshold: `disc <= 20`. Assert dwell: 3 s. Clear dwell: 5 s.

## 2.4 Fix M-03: RS-485 Partial Frame Inter-Byte Timeout

**Problem:** If a frame arrives with a mid-frame byte loss, `rxPos > 0` indefinitely. The receiver never resets and subsequent valid frames are silently ignored.

**Fix:**
```cpp
// Track last byte received timestamp in rs485_slave_poll()
static uint32_t lastByteMs = 0;

if (Serial.available()) {
  lastByteMs = millis();
  // ... existing byte read logic
}

// Stall detection: partial frame with no new bytes for > 20ms → reset
if (rxPos > 0 && (millis() - lastByteMs) > 20) {
  LOG_SN(LOG_DEBUG, "RS485", "Partial frame stall — resetting. rxPos=%d", rxPos);
  rxPos = 0;
}
```

## 2.5 Phase 2 Exit Criteria

- H-02, H-03, H-04, M-03 confirmed fixed via bench test
- `snLevelDiscardCount` increments correctly and is visible in debug output
- Flow error flag does not oscillate in borderline conditions
- Partial frame receiver resets within 20 ms of stall
- `LDSC` field present in RS-485 response frame (see Phase 4 for format)
- Clean compilation with no new warnings

---

---

# Phase 3 — Master Node Bug Fixes (ESP32)

## Objective

Fix confirmed bugs in the ESP32 master firmware. Fix only what is confirmed present in Phase 0.

## 3.1 Fix C-01: Missing `void setup()` Declaration

If the main `.ino` is missing the `void setup()` declaration, add it. This is a structural integrity fix.

Comment: `// REFACTOR [C-01]: explicit setup() declaration`

## 3.2 Fix C-02: `waterLevelPct` Initial Value

**Problem:** `waterLevelPct = 0` on startup. The ESP32 pushes 0% to Firebase before the first valid RS-485 frame arrives, which can trigger false AUTO pump start if start level is above 0%.

**Fix:**
- Initialize `waterLevelPct = -1` (sentinel value indicating "not yet known")
- In `pushFirebaseStatus()`, omit `water_level_percent` from the JSON payload when value is `-1`
- In `executePumpLogic()`, guard all level comparisons: `if (waterLevelPct < 0) return; // no valid level yet`

## 3.3 Fix H-05: Overflow Protection in MANUAL Mode

**Problem:** Overflow protection stops the pump in MANUAL mode. In MANUAL mode, the operator has explicitly chosen to run the pump. Stopping it silently violates operator intent and removes the ability to use MANUAL for diagnostics.

**Engineering basis:** NEMA MG-1 section on duty cycles and operator override expectations.

**Fix:**
- Remove `"MANUAL"` from the overflow protection condition that stops the pump
- Instead, set a new non-latching `manual_runtime_warning` flag in Firebase status when manual runtime exceeds `cfgMaxPumpRuntimeMin`
- Log at `LOG_WARN`: "Manual runtime exceeded [X] min. Operator supervision recommended."
- Do not stop the pump. Information only.

## 3.4 Fix H-06: Crash Loop Counter Clear — Success-Based

**Problem:** The crash loop counter clears at 60 s from boot. A full boot sequence (WiFi + Firebase init) can take up to 90 s on a cold start, so a crash occurring at 70 s still reads as a clean boot. This underreports crash loops.

**Fix:**
- Replace time-based clear with success-based clear: clear the counter on the first successful Firebase push
- Keep a 180 s fallback automatic clear as a safety net
- Comment: `// REFACTOR [H-06]: success-based clear, 180s fallback`

## 3.5 Fix H-07: Add AUTO_COOLDOWN and MANUAL_COOLDOWN Run Modes

**Problem:** When the pump stops and the motor off-timer is active, `runMode` stays at its previous value. Dashboard has no way to display the cooldown state or its duration.

**Fix:**
- Add `AUTO_COOLDOWN` and `MANUAL_COOLDOWN` as valid `runMode` string values
- Set in `executePumpLogic()` when the pump is OFF and the off-timer is counting down
- Add `pump_cooldown_remaining_sec` to `pushFirebaseStatus()`: integer, 0 when not in cooldown

```cpp
// In executePumpLogic() — when pump is forced off by off-timer
if (offTimerActive) {
  runMode = (controlMode == MODE_MANUAL) ? "MANUAL_COOLDOWN" : "AUTO_COOLDOWN";
  pumpCooldownRemainingSec = (offTimerEndMs - millis()) / 1000;
} else {
  pumpCooldownRemainingSec = 0;
}
```

## 3.6 Fix ISR Safety: Flow Sensor Pulse Counter

**Problem (from Phase 0 §0.6):** If `flowPulseCount` is not `volatile`, the compiler may optimize away the ISR-modified value. If the main loop reads it directly without disabling interrupts, a torn read is possible on 32-bit values.

**Fix:**
```cpp
volatile uint32_t g_flowPulseCount = 0;

void IRAM_ATTR onFlowPulse() {
  g_flowPulseCount++;  // Only ISR touches this
}

// Safe accessor — call from main loop only
uint32_t readAndResetFlowPulses() {
  portDISABLE_INTERRUPTS();
  uint32_t count = g_flowPulseCount;
  g_flowPulseCount = 0;
  portENABLE_INTERRUPTS();
  return count;
}
```

Replace all direct reads of the pulse counter in the main loop with `readAndResetFlowPulses()`.

## 3.7 Fix M-01: Level Timestamp Consolidation

**Problem:** `levelLastValidMs` and `levelLastUpdateMs` have overlapping semantics and are used inconsistently across safety gates and dashboard reporting. Inconsistent usage can cause stale-level checks to pass when they should fail.

**Fix:**
- Retain `levelLastUpdateMs` as the single authoritative timestamp
- Update it in `pollRemoteSensorNode()` when a valid frame arrives AND `(remoteSensorLastErrCode & 0x01) == 0` (no ultrasonic error in the received frame)
- Remove all references to `levelLastValidMs` and replace with `levelLastUpdateMs`
- Apply `levelLastUpdateMs` consistently in `checkDryRunProtection()`, `executePumpLogic()`, and `pushFirebaseStatus()`

## 3.8 Fix M-02: `bypass_flow_sensor` Runtime Control

**Problem:** `cfgBypassFlowSensor` exists in firmware but has no Firebase control path. It cannot be changed at runtime without reflashing.

**Fix:**
- Add `bypass_flow_sensor` field to `readFirebaseControl()`, mirroring the existing `bypass_level_sensor` handling
- Persist value to NVS on change (same pattern as other persisted configs)
- Add `bypass_flow_sensor: cfgBypassFlowSensor` to `pushFirebaseStatus()`
- Dashboard: add toggle to Advanced panel (Phase 6)

## 3.9 Fix M-05: `runMode` Initial Value

**Problem:** `runMode` is initialized to `"OFF"` instead of `"AUTO_STANDBY"`. On first boot before any control message, the dashboard shows "OFF" which is misleading — the controller is active in AUTO mode.

**Fix:** Initialize `runMode = "AUTO_STANDBY"`.

## 3.10 Fix M-06: `is_idle_mode` in Firebase Status

**Problem:** `isIdleMode` is maintained internally but never pushed to Firebase. The dashboard cannot explain why sensor data updates are slow during idle periods.

**Fix:** Add `statusJson.set("is_idle_mode", isIdleMode)` to `pushFirebaseStatus()`.

## 3.11 Update: `DRY_RUN_THRESHOLD_LPM` Default

**Engineering basis:** YF-G1 1-inch flow sensor working range per manufacturer specification: 1–60 L/min. At sub-1 L/min, Hall-effect pulse intervals become erratic and detection becomes unreliable against ISR timing noise. The current default of 0.5 L/min is below the reliable detection floor.

**Fix:** Update `DRY_RUN_THRESHOLD_LPM` from `0.5f` to `1.0f`. Add comment:
```cpp
// 1.0 L/min: YF-G1 minimum reliable detection threshold per manufacturer spec.
// Verify and adjust after bucket calibration on installed system.
```

## 3.12 Add: `LDSC` Field Parsing from RS-485 Frame

The NodeMCU now includes `LDSC:<n>` in its response frame (Phase 2.2, Phase 4.1). Update the ESP32 frame parser.

**Fix:** In `parseSensorFrameStrict()` (or equivalent), add optional parsing of the `LDSC:` field. If absent, default to 0. Store as `remoteSensorLevelDiscardCount`. Include in Firebase status as `remote_level_discard_count`. This is backward compatible — old NodeMCU firmware without `LDSC` field continues to work.

## 3.13 Add: Firebase Write Error Backoff

**Problem:** If Firebase writes fail consecutively, the system may retry without delay, generating network congestion and UART pressure.

**Fix:** If not already present, add exponential backoff to the Firebase write retry path:

```cpp
uint32_t backoffMs = min(1000UL * (1UL << consecutiveFirebaseFailures), 30000UL);
// delay(backoffMs); — or use non-blocking millis() gate
```

Log at `LOG_WARN` after first failure. Log at `LOG_ERROR` after 5 consecutive failures.

## 3.14 Replace `String` Heap Fragmentation in Hot Paths

Audit all uses of the Arduino `String` class in the main loop body. The Arduino `String` class allocates on the heap and causes fragmentation over long runtimes (days/weeks of continuous operation).

**Fix:** Replace `String` concatenation in hot paths (loop body, ISR helpers, per-cycle functions) with fixed-size `char[]` buffers and `snprintf`. Reserve Arduino `String` only for one-time setup operations (boot banner, NVS reads during init).

## 3.15 Phase 3 Exit Criteria

- C-01, C-02, H-05, H-06, H-07, M-01, M-02, M-05, M-06 all confirmed via bench test
- ISR safety fix applied and verified (no direct read of pulse counter in main loop)
- `AUTO_COOLDOWN` state visible in Firebase and serial log when pump stops and off-timer is active
- `DRY_RUN_THRESHOLD_LPM = 1.0` confirmed in compile-time default and NVS load/save paths
- `water_level_percent` omitted from Firebase status on first push (before valid RS-485 frame)
- `remote_level_discard_count` present in Firebase status
- `debug_log_level` present in Firebase status reflecting current `gLogLevel`
- Clean compilation with no new warnings

---

---

# Phase 4 — Protocol & Firebase Contract

## Objective

Formalize the RS-485 protocol and Firebase schema so both nodes and the dashboard share a single authoritative contract. This phase verifies the implementations from Phases 2–3 match the spec and documents the complete contract.

## 4.1 RS-485 Protocol Specification

**Frame format — NodeMCU → ESP32 (response):**

```
STX LVL:<pct>;DIST:<cm>;FLOW:<lpm>;ERR:<code>;LDSC:<n>;SEQ:<seq>;CRC:<hex4> ETX
```

Where `STX = 0x02`, `ETX = 0x03`.

| Field | Type | Range | Description |
|-------|------|-------|-------------|
| `LVL` | int | 0–100 | Water level percentage |
| `DIST` | float (1 dp) | 2.0–300.0 | Raw ultrasonic distance in cm |
| `FLOW` | float (2 dp) | 0.00–100.00 | Flow rate in L/min |
| `ERR` | int (bitmask) | 0–3 | Bit 0 = ultrasonic error, Bit 1 = flow error |
| `LDSC` | int | 0–255 | Level reading discard count since last frame |
| `SEQ` | uint8 | 0–255 | Wrapping sequence number |
| `CRC` | hex4 | 0000–FFFF | CRC16-Modbus (poly 0xA001) over payload up to and including `SEQ:<n>;` |

**Command — ESP32 → NodeMCU (request):**

```
REQ\n
```

**Half-duplex timing:**

| Parameter | Value | Basis |
|---|---|---|
| Frame response timeout | 250 ms | Round-trip RS-485 + sensor read latency |
| Max retries | 3 | 3 × 250 ms = 750 ms max stall before marking failed |
| Turnaround guard (ESP32) | 80 µs | RS-485 bus turnaround per TIA-485-A |
| Turnaround guard (NodeMCU) | 60 µs | |
| Inter-byte stall reset | 20 ms | Phase 2.4 fix |

**CRC:** CRC16-Modbus. Polynomial `0xA001`. Initial value `0xFFFF`. Computed over the ASCII payload bytes between `STX` and `CRC:` field (inclusive of all semicolons, exclusive of STX/ETX and the `CRC:` field itself).

**Backward compatibility:** `LDSC` is a new field. The ESP32 parser treats it as optional. If absent, `remoteSensorLevelDiscardCount = 0`. Old NodeMCU firmware without `LDSC` remains compatible with new ESP32 firmware during a rolling update.

## 4.2 Firebase Schema — Complete Canonical Contract

### `/pump_system/status` — Written by ESP32

| Field | Type | Notes |
|-------|------|-------|
| `water_level_percent` | int \| absent | Omitted when `waterLevelPct == -1` |
| `is_running` | bool | Relay state |
| `flow_rate_lpm` | float | From RS-485 `FLOW` field |
| `run_mode` | string | See Run Mode table |
| `pump_cooldown_remaining_sec` | int | 0 when not in cooldown — **new** |
| `is_error` | bool | DRY_RUN lockout active |
| `is_sensor_error` | bool | Ultrasonic sensor failure |
| `is_flow_sensor_error` | bool | Flow sensor error |
| `is_overflow_error` | bool | Max runtime exceeded |
| `is_idle_mode` | bool | Slow-poll mode active — **was missing, now added** |
| `is_sleeping` | bool | Scheduled sleep active |
| `emergency_stop_latched` | bool | |
| `manual_desired` | bool | |
| `bypass_level_sensor` | bool | |
| `bypass_flow_sensor` | bool | **new** |
| `remote_sensor_stable` | bool | 3 consecutive valid frames |
| `level_fresh` | bool | Level age < staleness threshold |
| `manual_runtime_warning` | bool | Manual run exceeded max runtime — **new** |
| `countdown_remaining_sec` | int | 0 when not in countdown |
| `last_fault_code` | string | See Fault Code table |
| `last_fault_message` | string | Human-readable fault detail |
| `remote_level_discard_count` | int | From RS-485 `LDSC` field — **new** |
| `flow_volume_added_l` | float | |
| `wifi_rssi` | int | dBm |
| `uptime_minutes` | int | |
| `last_boot_reason` | string | |
| `debug_log_level` | int | Current active `gLogLevel` — **new** |
| `total_pump_cycles` | int | NVS-persisted |
| `total_pump_run_min` | int | NVS-persisted |
| `ultrasonic_cycles_ok` | int | Lifetime counter |
| `ultrasonic_cycles_timeout` | int | Lifetime counter |
| `free_heap_bytes` | int | |
| `min_free_heap_observed_bytes` | int | |
| `firebase_consecutive_failures` | int | |

### Run Mode Values

| Value | Condition | Dashboard label |
|-------|-----------|----------------|
| `AUTO_STANDBY` | AUTO, pump off, level OK | AUTO — Standby |
| `AUTO` | AUTO, pump running | AUTO — Running |
| `AUTO_COOLDOWN` | AUTO, pump off, off-timer active — **new** | AUTO — Cooldown Xs |
| `MANUAL_ON` | MANUAL, pump running | MANUAL — On |
| `MANUAL_OFF` | MANUAL, pump off | MANUAL — Off |
| `MANUAL_COOLDOWN` | MANUAL, pump off, off-timer active — **new** | MANUAL — Cooldown Xs |
| `COUNTDOWN` | Countdown running | Countdown |
| `STOPPED` | Emergency stop latched | Emergency Stop |

### Fault Code Values

| Code | Trigger | Recovery |
|------|---------|---------|
| `DRY_RUN` | Flow < threshold for > timeout | `clear_error: true` + verify water |
| `OVERFLOW` | Runtime > max in AUTO/COUNTDOWN | `clear_error: true` |
| `E_STOP` | Emergency stop triggered | `clear_error: true` then `reset_stop: true` |
| `COMM_LOSS` | RS-485 link unstable | Auto-clears on link recovery |
| `STALE_LEVEL` | Level data age > threshold | Auto-clears when fresh data arrives |
| `LEVEL_SENSOR` | Ultrasonic error | Auto-clears on sensor recovery |
| `FLOW_SENSOR` | Flow sensor stuck-high while pump OFF | Auto-clears on recovery |
| `SAFE_MODE` | Crash loop detected | Power cycle or 1-hour auto-clear |

### `/pump_system/control` — Read by ESP32

| Field | Type | Behavior |
|-------|------|---------|
| `mode` | string | Valid: `AUTO`, `MANUAL`, `COUNTDOWN` |
| `manual_desired` | bool | Persistent operator intent in MANUAL |
| `emergency_stop` | bool | One-shot. Firmware resets to false |
| `reset_stop` | bool | One-shot. Blocked if lockout active |
| `clear_error` | bool | One-shot. Clears DRY_RUN and OVERFLOW |
| `countdown_start` | bool | One-shot |
| `countdown_duration_min` | int | 1–120 |
| `countdown_add_time` | bool | One-shot |
| `countdown_add_min` | int | Minutes to add |
| `bypass_level_sensor` | bool | Persistent |
| `bypass_flow_sensor` | bool | Persistent — **new** |
| `reboot_request_id` | int | Increment to trigger reboot |

### `/pump_system/config/device` — Read by ESP32

| Field | Type | Range | Description |
|-------|------|-------|-------------|
| `tank_empty_cm` | int | 5–200 | |
| `tank_full_cm` | int | 1–199 | |
| `pump_start_level` | int | 0–100 | |
| `pump_stop_level` | int | 0–100 | |
| `dry_run_threshold_lpm` | float | 0.1–10.0 | Default: 1.0 |
| `dry_run_timeout_sec` | int | 10–300 | |
| `max_pump_runtime_min` | int | 30–480 | |
| `flow_calibration_factor` | float | 0.1–20.0 | |
| `debug_log_level` | int | 0–4 | **new** — remote log level control |
| `sleep_enabled` | bool | | |
| `sleep_start_hour` | int | 0–23 | PHT |
| `sleep_end_hour` | int | 0–23 | PHT |
| `sleep_emergency_level` | int | 0–100 | |
| `sensor_failure_threshold` | int | | |
| `idle_sensor_interval_ms` | int | | |
| `idle_firebase_interval_ms` | int | | |

## 4.3 Protocol Spec Document

Produce `docs/specs/rs485_protocol.md` containing the frame format, timing parameters, CRC method, and backward compatibility rules from §4.1. This document is the authoritative reference for both firmware implementations and must be kept in sync with any future protocol changes.

## 4.4 Phase 4 Exit Criteria

- `docs/specs/rs485_protocol.md` created and version-controlled
- Actual `pushFirebaseStatus()` and `readFirebaseControl()` match the schema tables exactly (verified by code review, not assumption)
- All new fields from Phases 2–3 present in the schema
- No field exists in firmware that is absent from the schema, and vice versa

---

---

# Phase 5 — Test Firmware Suite

## Objective

Provide standalone, self-contained test sketches for each node that can verify hardware and communication integrity in isolation. These are not modified production firmware — they are separate, independent sketches.

## 5.1 Design Principles

- Each test sketch is a complete, standalone firmware
- Tests print structured output to USB Serial at 115200 baud
- Each test case has a clear `[PASS]` / `[FAIL]` verdict with reason
- Tests do not require Firebase or WiFi (except WiFi and Firebase tests themselves)
- Tests are stored in a `test/` directory within each firmware project

## 5.2 Test Runner Pattern

Both test suites use a common runner pattern:

```cpp
void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("=== SmartFlow Node Hardware Test ===");
  Serial.println("Build: " __DATE__ " " __TIME__);
  Serial.println("");

  runTest("TC-01: Hardware sanity", test_hw_sanity);
  // ... additional tests

  Serial.println("");
  Serial.println("=== Test complete ===");
}

void loop() {}  // All tests run once in setup()

void runTest(const char* name, bool (*fn)()) {
  Serial.printf("[ RUN ] %s\n", name);
  bool ok = fn();
  Serial.printf("%s %s\n", ok ? "[ PASS]" : "[ FAIL]", name);
}
```

## 5.3 NodeMCU Sensor Node — Test Suite

**Location:** `firmware/platformio_sensor_node/test/`

**TC-S-01: Hardware Sanity**
- Confirm `PIN_RS485_DE_RE` can be driven HIGH and LOW without exception
- Confirm `PIN_US_TRIG` can output a 10 µs pulse
- Confirm `PIN_FLOW_INPUT` reads HIGH with nothing connected (INPUT_PULLUP)
- Confirm Serial1 (GPIO2) outputs text (verify with USB-TTL adapter)
- Output: `[PASS] GPIO sanity` or `[FAIL] GPIO: <reason>`

**TC-S-02: Ultrasonic Sensor**
- Fire 20 trigger pulses with 200 ms spacing
- Record distance for each ping
- Report: valid count, timeout count, min, max, median
- PASS: ≥ 15/20 valid readings, all valid readings in 2–300 cm, (max − min) < 5 cm over 20 readings
- Output: `[PASS] Ultrasonic: median=82.3cm valid=20/20` or `[FAIL] Ultrasonic: valid=3/20`

**TC-S-03: Flow Sensor**
- Enable interrupt on `PIN_FLOW_INPUT`
- Count pulses for 10 seconds with volatile ISR counter
- Report: total pulses, computed Hz, computed L/min
- If zero pulses: report `[INFO] Flow: 0 pulses — no flow present. Manually activate flow to verify.`
- PASS: non-zero count when water flowing, 0 when stopped. Not auto-failed on no-flow condition.
- Output: `[PASS] Flow: 75 pulses/10s = 7.5Hz = 1.00L/min`

**TC-S-04: RS-485 Slave Echo Server**
- Initialize UART0 at 115200 for RS-485, enable DE/RE control
- Listen for incoming bytes
- When `REQ\n` is received, compute and respond with a valid hardcoded test frame:
  `STX LVL:50;DIST:61.0;FLOW:5.00;ERR:0;LDSC:0;SEQ:0;CRC:<computed> ETX`
- Print to Serial1 (GPIO2): `[TEST] REQ received — test frame sent`
- PASS: confirmed when ESP32 TC-M-02 receives and validates this frame

**TC-S-05: CRC Self-Test**
- Compute CRC16-Modbus on a known byte sequence
- Compare against a pre-computed expected value
- Expected: `LVL:50;DIST:61.0;FLOW:5.00;ERR:0;LDSC:0;SEQ:0;` → CRC computed at build time and hardcoded
- Output: `[PASS] CRC self-test` or `[FAIL] CRC: got=XXXX expected=YYYY`

## 5.4 ESP32 Master Node — Test Suite

**Location:** `firmware/platformio_smart_water_pump_controller/test/` (or Arduino: `firmware/test_master_node/`)

**TC-M-01: GPIO and Relay**
- Drive `RS485_DE_RE_PIN` HIGH and LOW — confirm no exception
- **Safety warning:** Print before relay activation:
  `[WARN] Relay will activate for 500ms. Confirm pump has water or disconnect pump before continuing. Press ENTER to proceed.`
- Wait for Serial input before proceeding
- Activate relay for 500 ms, then deactivate
- Confirm GPIO readback matches expected state
- Output: `[PASS] GPIO and relay` or `[FAIL] GPIO: <reason>`

**TC-M-02: RS-485 Master Poll**
- Initialize UART2 on RS-485 pins at 115200
- Send `REQ\n` every 1 second for 30 seconds
- For each response: validate CRC, parse all fields, print parsed values
- Report: requests sent, valid frames, CRC errors, timeouts
- PASS: success rate ≥ 90% over 30 seconds
- Prerequisite: NodeMCU running TC-S-04 (echo server) or production firmware
- Output: `[PASS] RS485 master: 29/30 frames valid (96.7%)`

**TC-M-03: WiFi Connection**
- Attempt connection using credentials from `secrets.h`
- Report: connection time, IP address, RSSI
- Ping `8.8.8.8` three times and report round-trip time
- PASS: connection within 20 s, ≥ 2/3 pings succeed
- Output: `[PASS] WiFi: IP=192.168.1.45 RSSI=-62dBm ping=12ms`

**TC-M-04: Firebase Read/Write**
- Connect to Firebase using `secrets.h` credentials
- Write: `/pump_system/test/ping_at` = current timestamp
- Read it back and verify value matches
- Delete the test node
- PASS: write confirmed and read-back matches within 10 s
- Output: `[PASS] Firebase: ping=OK latency=1240ms`

**TC-M-05: Full Round-Trip Integration**
- Requires: NodeMCU running production firmware or TC-S-04, WiFi available, Firebase accessible
- Send RS-485 request, parse response, push parsed sensor data to Firebase status
- Read back from Firebase and verify the value matches what was pushed
- PASS: end-to-end data path confirmed
- Output: `[PASS] Integration: RS485→Firebase→verify OK`

## 5.5 Test Suite File Structure

```
firmware/
  test_master_node/                   ← Arduino IDE version
    test_master_node.ino
    test_helpers.ino
    test_cases.ino
    README.md
  test_sensor_node/                   ← Arduino IDE version
    test_sensor_node.ino
    test_cases_slave.ino
    README.md
  platformio_smart_water_pump_controller/
    test/                             ← PlatformIO version (Unity framework)
      test_gpio_relay/
      test_rs485_master/
      test_wifi/
      test_firebase/
      test_integration/
  platformio_sensor_node/
    test/
      test_hw_sanity/
      test_ultrasonic/
      test_flow/
      test_rs485_slave/
      test_crc/
```

## 5.6 Phase 5 Exit Criteria

- All test sketches compile independently without production firmware files
- TC-S-01 through TC-S-05 PASS on NodeMCU hardware
- TC-M-01 through TC-M-05 PASS on ESP32 hardware
- Each test directory contains a `README.md` with prerequisites and expected output

---

---

# Phase 6 — Dashboard Redesign (SmartFlow)

## Objective

Apply the SmartFlow brand identity to the dashboard and implement all new UI components required by the Firebase schema additions from Phases 3–4. Resolve all dashboard bugs found in Phase 0.

## 6.1 SmartFlow Brand Identity

**Name:** SmartFlow
**Typography:** Geist (display/UI) + Geist Mono (values, hex, technical readouts). Self-hosted from `/public/fonts/` — no external font CDN request.
**Tone:** Industrial precision. Clean, data-forward, not decorative.

**Color design tokens — add to `tailwind.config.ts`:**

```typescript
colors: {
  sf: {
    blue:           '#185FA5',
    'blue-mid':     '#378ADD',
    'blue-light':   '#E6F1FB',
    teal:           '#0F6E56',
    'teal-light':   '#E1F5EE',
    amber:          '#BA7517',
    'amber-light':  '#FAEEDA',
    red:            '#A32D2D',
    'red-light':    '#FCEBEB',
    green:          '#3B6D11',
    'green-light':  '#EAF3DE',
    gray: {
      50:  '#F1EFE8',
      100: '#D3D1C7',
      200: '#B4B2A9',
      600: '#5F5E5A',
      900: '#2C2C2A',
    }
  }
}
```

**Semantic color mapping:**

| State | Background | Text / border |
|-------|-----------|---------------|
| Pump running | `sf-teal-light` | `sf-teal` |
| Standby / idle | `sf-gray-50` | `sf-gray-600` |
| Warning | `sf-amber-light` | `sf-amber` |
| Error / lockout | `sf-red-light` | `sf-red` |
| Sleeping | `sf-blue-light` | `sf-blue` |
| Bypass active | `sf-amber-light` | `sf-amber` |
| Cooldown | `sf-blue-light` | `sf-blue-mid` |

## 6.2 Rebranding String Substitution

Apply across all dashboard files and firmware files. **Do not rename the Arduino sketch folder or the primary `.ino` file** — the Arduino IDE requires them to share the same name.

| Old string | New string |
|---|---|
| `Smart Water Pump Controller` | `SmartFlow` |
| `smart_water_pump_controller` (in strings) | `smartflow` |
| Dashboard `<title>` | `SmartFlow` |
| Dashboard header/navbar | `SmartFlow` |
| PWA `manifest.json` `name` | `SmartFlow` |
| PWA `manifest.json` `short_name` | `SmartFlow` |
| Firmware boot banner | `SmartFlow vX.X` |
| All `docs/` document headers | `SmartFlow` |
| Root `README.md` title | `# SmartFlow` |

## 6.3 Layout Architecture

```
Header
  SmartFlow wordmark (Geist 700, sf-blue)
  WiFi RSSI indicator
  Last updated timestamp
  Dark / light theme toggle

Main dashboard (single route: /)
  ┌────────────────────────────────────────┐
  │  Tank Level Card                        │
  │  Animated SVG fill · % · cm            │
  │  Start / Stop level markers            │
  │  Level estimate indicator if active    │
  ├────────────────────────────────────────┤
  │  Pump Status Card                       │
  │  Run mode chip · Flow rate · Cooldown  │
  │  Running time · Uptime · Boot reason   │
  ├──────────────────┬─────────────────────┤
  │  Controls        │  Alerts             │
  │  Mode selector   │  Error cards        │
  │  E-stop button   │  Manual warning     │
  │  Countdown       │  Cooldown notice    │
  ├──────────────────┴─────────────────────┤
  │  Diagnostics (collapsible)              │
  │  Log level · Discard count · Heap      │
  │  Firebase failures · RS-485 stats      │
  └────────────────────────────────────────┘

Settings (route: /settings)
  Tank calibration
  Thresholds (dry-run, max runtime)
  Bypass controls (level + flow)
  Sleep schedule
  Notification preferences
  Advanced (log level control)
```

## 6.4 New Components Required by Phase 3–4 Changes

All new Firebase fields must be reflected in the dashboard. No field may be added to firmware without a corresponding dashboard display.

**Cooldown state:**
- Mode selector shows sub-label with countdown: `AUTO — Cooldown 47s` with `pump_cooldown_remaining_sec` counting down client-side using `setInterval`
- Pump toggle button shows "Cooldown" label and is disabled during cooldown
- Cooldown chip uses `sf-blue-light` background

**Manual runtime warning:**
- When `manual_runtime_warning: true`, show a non-blocking amber notification in the alerts card: "Manual run has exceeded [X] minutes. Operator supervision required."
- No pump action taken. Information only.
- Dismissible but reappears if still true on next Firebase update

**Flow sensor bypass:**
- Advanced panel: `bypass_flow_sensor` toggle mirroring existing level bypass toggle
- Status chip next to flow readout: `BYPASSED` in `sf-amber` when `bypass_flow_sensor: true`

**Idle mode:**
- Connectivity card: small amber badge `Idle Mode` when `is_idle_mode: true`
- Tooltip: "Sensor and Firebase polling reduced because tank is full and pump is off."

**Remote log level control:**
- Diagnostics section: dropdown or segmented control for `debug_log_level` (ERROR / WARN / INFO / DEBUG / VERBOSE)
- Writes to `/pump_system/config/device/debug_log_level`
- Amber warning when set above INFO: "Verbose logging may increase Firebase write volume"
- Current level reflects `debug_log_level` from Firebase status

**Level discard count:**
- Diagnostics section: `LVL_DISCARD` row showing `remote_level_discard_count`
- Shown per 60 s window. Non-zero values styled in `sf-amber`

**Level estimate visual:**
- When `level_estimate_active: true`:
  - Tank SVG: level number shows `~82%` with italic sub-label "Flow estimate"
  - Level history chart: dashed line in `sf-amber` for the estimate period
  - Transition point marked with a vertical dashed line labeled "Sensor bypass"

## 6.5 General Dashboard Bug Fixes (from Phase 0 Audit)

Apply all findings from the Phase 0 dashboard audit. At minimum:

- Unsubscribe all Firebase `onValue` listeners in `useEffect` cleanup functions
- Replace any `as any` Firebase data casts with typed interfaces matching the schema in §4.2
- Add null checks before accessing nested Firebase data: `data?.status?.water_level_percent ?? null`
- Settings form: validate `pump_start_level < pump_stop_level` before allowing save
- Settings form: validate `tank_full_cm < tank_empty_cm` before allowing save
- Disable mode controls while a Firebase write is in-flight (pending state)
- Add React error boundary around each major card — on catch: "Unable to load [section]" with retry button
- Skeleton loaders (`animate-pulse`) on all data-dependent components during initial load

## 6.6 PWA Manifest

```json
{
  "name": "SmartFlow",
  "short_name": "SmartFlow",
  "description": "Automated water pump controller — Leon, Iloilo",
  "theme_color": "#185FA5",
  "background_color": "#F1EFE8",
  "display": "standalone",
  "start_url": "/",
  "icons": [
    { "src": "/icons/icon-192.png", "sizes": "192x192", "type": "image/png" },
    { "src": "/icons/icon-512.png", "sizes": "512x512", "type": "image/png" }
  ]
}
```

## 6.7 Responsive Layout Requirements

- Render correctly at 1280 px (desktop) and 375 px (mobile)
- Dark and light themes fully functional, persisted in `localStorage`
- Emergency stop button always visible on mobile — must not be hidden at any scroll position; pin to bottom of viewport on mobile
- All text readable at 100% zoom in both themes

## 6.8 Phase 6 Exit Criteria

- SmartFlow branding applied throughout (wordmark, colors, typography)
- All new Firebase fields from §4.2 are displayed somewhere in the dashboard
- Cooldown state renders correctly when `run_mode` is `AUTO_COOLDOWN` or `MANUAL_COOLDOWN`
- Flow sensor bypass toggle works end-to-end
- Idle mode badge appears and disappears correctly
- Remote log level control writes to Firebase and ESP32 serial reflects the change
- Level estimate visual applied when `level_estimate_active: true`
- Dashboard renders at 1280 px and 375 px
- Dark and light themes both functional
- E-stop always visible on mobile
- No browser console errors on initial load

---

---

# Phase 7 — Integration & Validation

## Objective

Confirm the complete system — both firmware nodes, RS-485 link, Firebase integration, and dashboard — operates correctly as a whole under all defined operational states.

## 7.1 Run Test Sketches First

Before running integration tests, execute the full test suites from Phase 5 with production hardware. All TC-S-xx and TC-M-xx tests must PASS before loading production firmware.

## 7.2 System Integration Test Protocol

**Environment:** Full hardware — JSN-SR04T in tank, YF-G1 in water line, pump connected to relay, both nodes powered, WiFi available.

| # | Test | Condition | Expected result | Pass criteria |
|---|------|-----------|----------------|---------------|
| I-01 | Boot sequence | Cold power-on both nodes | RS-485 link within 10 s, Firebase connected | `remote_sensor_stable: true` within 15 s of boot |
| I-02 | Normal AUTO | Tank below start level, pump off | Pump starts | `is_running: true` within sensor interval |
| I-03 | AUTO stop | Tank reaches stop level | Pump stops | `is_running: false` within sensor interval |
| I-04 | Cooldown | Pump stops, level drops immediately | Pump stays off for cooldown period | `run_mode: AUTO_COOLDOWN` visible during cooldown |
| I-05 | Dry run | Run pump with valve closed | DRY_RUN fault after timeout | `is_error: true`, `last_fault_code: DRY_RUN` |
| I-06 | Error clear | After dry run | Clear via dashboard `clear_error` | Pump can restart in AUTO |
| I-07 | Emergency stop | E-stop button on dashboard | Pump stops immediately | `emergency_stop_latched: true`, `run_mode: STOPPED` |
| I-08 | E-stop reset | After E-stop | Reset via dashboard | Pump restarts normally |
| I-09 | MANUAL mode | Switch to MANUAL, start pump | Pump runs until manually stopped | `run_mode: MANUAL_ON` |
| I-10 | MANUAL overflow warning | Manual run > max runtime | Warning appears, pump continues | `manual_runtime_warning: true`, pump still running |
| I-11 | COUNTDOWN | Set 15-min countdown | Pump runs 15 min, reverts to AUTO | `run_mode: AUTO_STANDBY` after 15 min |
| I-12 | Comm loss | Disconnect CAT6 mid-run | Pump stops within offline timeout | `remote_sensor_stable: false` |
| I-13 | Level bypass | Enable level bypass | Pump operates on flow guard only | `bypass_level_sensor: true` in status and dashboard |
| I-14 | Flow bypass | Enable flow bypass | No DRY_RUN triggers | `bypass_flow_sensor: true` in status and dashboard |
| I-15 | Idle mode | Tank > 90%, pump off 5+ min | Slow poll enters idle | `is_idle_mode: true`, Firebase updates slow |
| I-16 | NVS first boot | Flash to blank NVS | All defaults load, no error on boot | Dashboard shows valid data within 30 s |
| I-17 | Production serial | Flash release build | Serial Monitor shows only boot + errors | No per-cycle data spam in serial output |
| I-18 | Debug serial | Set `gLogLevel` to DEBUG via dashboard | Verbose output in ESP32 serial | `[D]` messages visible in Serial Monitor |
| I-19 | Dashboard mobile | 375 px viewport | E-stop always visible | E-stop not hidden at any scroll position |
| I-20 | Theme switch | Toggle dark/light | Instant visual change, no flash | Correct colors in both themes |
| I-21 | 2-hour soak | Normal AUTO for 2 hours | No watchdog reset, no crash | `uptime_minutes` ≥ 120, heap stable |

## 7.3 Phase 7 Exit Criteria

- All 21 integration tests pass
- No regressions from previously working functionality
- Serial output at `LOG_INFO` is ≤ 20% of baseline volume in steady-state normal operation
- Dashboard shows no stale data indicators during normal operation (Firebase sync < 5 s)
- System runs ≥ 2 hours without watchdog reset in normal AUTO operation

## 7.4 Deployment Sign-Off Checklist

- [ ] Phase 0 audit report complete and reviewed
- [ ] All Critical and High bugs from triage table resolved
- [ ] Pin assignment table verified against physical wiring
- [ ] NVS config loaded and validated on production hardware
- [ ] `DRY_RUN_THRESHOLD_LPM` confirmed via bucket calibration (target ≈ 1.0 L/min)
- [ ] TOR dial set to motor FLA (8–9A for 1.5 HP 220 V single-phase)
- [ ] Pump cable earth continuity confirmed < 1 Ω
- [ ] CAT6 GND tied to both enclosure GNDs (shared reference for RS-485)
- [ ] Firebase security rules reviewed (out of scope but gated for deployment)
- [ ] Test suite results archived in `docs/audit/test_results_2026.md`
- [ ] Integration test protocol completed and all 21 tests passed
- [ ] Production firmware flashed (not debug build)
- [ ] `LOG_COMPILE_FLOOR` confirmed at `LOG_DEBUG` or `LOG_INFO` for production

---

---

# AI Agent Prompt

Copy the entire block below and use it as your prompt to an AI coding agent with access to the full SmartFlow source tree.

---

```
You are a senior embedded systems and full-stack engineer working on SmartFlow — a Smart
Water Pump Controller system running on an ESP32 main controller and a NodeMCU V2 sensor
node, with a Next.js Firebase RTDB dashboard. The system controls a 1.5HP jet pump and
660L water tank in Leon, Iloilo, Philippines.

You are executing the SmartFlow System Refactor Plan v2.0. You are methodical, precise,
and conservative. You fix what is broken. You do not add features that are not in scope.
You never change behavior that is currently working correctly.

=============================================================================
MANDATORY PHASE 0 — RESEARCH FIRST. DO NOT WRITE CODE UNTIL THIS IS COMPLETE.
=============================================================================

Before making any change to any file, complete all of the following research tasks
and report your findings in a structured audit report. Your findings supersede all
assumptions in the refactor plan.

TASK 0.1 — SOURCE INVENTORY
Read every source file in:
  - firmware/arduino_smart_water_pump_controller/
  - firmware/platformio_smart_water_pump_controller/src/
  - firmware/arduino_sensor_node/
  - firmware/platformio_sensor_node/src/
  - dashboard/app/, dashboard/components/, dashboard/lib/

For each file, record: path, responsibility, dependencies, any TODO/FIXME/HACK comments,
and all compile-time flags (#define, #if).

TASK 0.2 — PIN ASSIGNMENT VERIFICATION
Extract all pin constant definitions from both firmware projects. Compare against
hardware/wiring_notes.md. Produce a table. Flag any discrepancy explicitly.

TASK 0.3 — FIREBASE SCHEMA EXTRACTION
Read pushFirebaseStatus(), readFirebaseControl(), and readDeviceConfigFromFirebase()
in their current form. Produce a table of every field, type, write condition, and read
condition. Note any field in firmware missing from dashboard, or vice versa.

TASK 0.4 — DASHBOARD STACK CONFIRMATION
Confirm framework (Next.js App Router, Pages Router, plain HTML, or other), Firebase SDK
version, authentication method, component structure, and any PWA manifest present.

TASK 0.5 — BUG TRIAGE
Verify which of the following known bugs are currently present (not fixed), which are
already fixed, and identify any new bugs not in this list:

  C-01: Missing void setup() declaration in main .ino
  C-02: waterLevelPct initialized to 0 before first valid RS-485 frame
  H-01: No log verbosity levels — all Serial output flat and unfiltered
  H-02: Level plausibility filter discards readings silently with no counter
  H-03: Flow discard debug print reads zeroed global flowPulseDiscardCount
  H-04: Flow error flag non-hysteretic — single sample flips snFlowError
  H-05: Overflow protection fires and stops pump in MANUAL mode
  H-06: Crash loop counter cleared at 60s — too short for full boot sequence
  H-07: No AUTO_COOLDOWN runMode when motor off-timer is active
  M-01: Two overlapping level timestamps (levelLastValidMs and levelLastUpdateMs)
  M-02: cfgBypassFlowSensor has no runtime Firebase control path
  M-03: RS-485 receiver never resets on partial frame stall
  M-05: runMode initialized to "OFF" instead of "AUTO_STANDBY"
  M-06: is_idle_mode not pushed to Firebase status

TASK 0.6 — ISR SAFETY AUDIT
Confirm the flow sensor ISR counter variable has the volatile qualifier. Confirm there
are no unprotected reads of ISR-modified variables from the main loop.

TASK 0.7 — AUDIT REPORT
Produce docs/audit/refactor_audit_2026.md containing all six findings above, plus
a revised scope section listing what is still present, what is already fixed, and what
is new. Do not proceed to Phase 1 until this report is complete.

=============================================================================
IMPLEMENTATION SEQUENCE — EXECUTE IN ORDER AFTER PHASE 0
=============================================================================

After the audit report is complete and reviewed, implement phases in this exact order.
Complete and verify each phase before starting the next. State which phase you are in
at the start of each work block.

PHASE 1 — DEBUG INFRASTRUCTURE (implement this before any firmware changes)

Implement a 5-level structured log system:
  LOG_ERROR=0, LOG_WARN=1, LOG_INFO=2, LOG_DEBUG=3, LOG_VERBOSE=4

LOG() macro rules:
  - Compile-time floor via LOG_COMPILE_FLOOR define (no runtime cost below floor)
  - Runtime ceiling via gLogLevel global (uint8_t, initialized from NVS on ESP32)
  - Format: [L][MODULE][MS] message — single char level, uppercase module tag,
    millis() zero-padded to 10 digits

NodeMCU transport:
  - DEBUG_USB_MODE=0 (production): debug output to Serial1 (GPIO2), RS-485 on UART0
  - DEBUG_USB_MODE=1 (bench): debug output to Serial, disconnect MAX485 DI/RO
  - Add #warning when DEBUG_USB_MODE=1 is set

ESP32: gLogLevel readable/writable via /pump_system/config/device/debug_log_level
       Push current gLogLevel to Firebase status as debug_log_level

Migrate ALL existing Serial.printf and Serial.println calls to LOG() with this triage:
  Safety trips, hardware failures → LOG_ERROR
  Degraded states, comm issues → LOG_WARN (rate-limit repeated messages: once/60s)
  State transitions, boot events → LOG_INFO
  Per-cycle sensor readings, RS-485 frames → LOG_DEBUG
  State machine internals, raw ISR → LOG_VERBOSE

Target: production LOG_INFO mode = only boot + error/warning messages on serial.
No functional behavior changes in this phase.

PHASE 2 — SLAVE NODE BUG FIXES (only bugs confirmed present in Phase 0)

H-03: Use local disc variable in flow discard debug print, not zeroed global.
H-02: Add snLevelDiscardCount. Increment on filter rejection. LOG_WARN on first,
      rate-limit to once/60s. Promote to snLevelError if all samples in window rejected.
      Include in RS-485 response frame as LDSC:<n> field.
H-04: Replace snFlowError single-sample check with two-stage hysteresis:
      Assert after 3 consecutive seconds disc>50. Clear after 5 consecutive seconds
      disc<=20. Hold state in hysteresis band (20 < disc <= 50).
M-03: Track lastByteMs in receiver. If rxPos>0 and no new byte for >20ms, reset rxPos
      and log at LOG_DEBUG.

PHASE 3 — MASTER NODE BUG FIXES (only bugs confirmed present in Phase 0)

C-01: Add missing void setup() declaration if absent.
C-02: Initialize waterLevelPct = -1. Omit water_level_percent from Firebase push when -1.
      Guard all level comparisons in executePumpLogic() against negative value.
H-05: Remove MANUAL from overflow stop condition. Add manual_runtime_warning: true to
      Firebase status when manual runtime exceeds cfgMaxPumpRuntimeMin. Pump keeps running.
H-06: Replace 60s time-based crash counter clear with success-based clear on first
      successful Firebase push. Keep 180s fallback.
H-07: Add AUTO_COOLDOWN and MANUAL_COOLDOWN runMode values. Set when pump is off and
      off-timer active. Add pump_cooldown_remaining_sec to Firebase status push (int, 0
      when not in cooldown).
ISR:  Add volatile to pulse counter. Implement readAndResetFlowPulses() accessor with
      portDISABLE_INTERRUPTS / portENABLE_INTERRUPTS. Replace all direct reads.
M-01: Remove levelLastValidMs. Use levelLastUpdateMs exclusively. Update only when valid
      frame arrives AND ultrasonic error bit is clear. Apply consistently across all
      safety checks and dashboard push.
M-02: Add bypass_flow_sensor to readFirebaseControl() mirroring bypass_level_sensor.
      Persist to NVS. Add bypass_flow_sensor to Firebase status push.
M-05: Initialize runMode = "AUTO_STANDBY" not "OFF".
M-06: Add is_idle_mode to pushFirebaseStatus().
Also: Update DRY_RUN_THRESHOLD_LPM default from 0.5f to 1.0f with comment citing
      YF-G1 1-60 L/min working range.
Also: Add optional LDSC field parsing in RS-485 frame parser. Default 0 if absent.
      Store as remoteSensorLevelDiscardCount. Push as remote_level_discard_count.
Also: Add Firebase write error backoff. Exponential: min(1000 * 2^n, 30000) ms.
Also: Replace Arduino String concatenation in hot loop paths with char[] + snprintf.

PHASE 4 — PROTOCOL & SCHEMA
Verify actual pushFirebaseStatus() and readFirebaseControl() match the canonical schema.
Produce docs/specs/rs485_protocol.md documenting the RS-485 frame format, timing,
CRC method (CRC16-Modbus, poly 0xA001), and backward compatibility rules.

PHASE 5 — TEST FIRMWARE
NodeMCU tests in firmware/platformio_sensor_node/test/ (or arduino_sensor_node/test/):
  TC-S-01: Hardware sanity (GPIO drive, TRIG pulse, FLOW pullup, Serial1 output)
  TC-S-02: Ultrasonic (20 pings, PASS if >=15/20 valid, stable within 5cm)
  TC-S-03: Flow sensor (10s pulse count, PASS if non-zero when water flowing)
  TC-S-04: RS-485 echo server (respond to REQ with hardcoded valid test frame)
  TC-S-05: CRC self-test (known vector vs pre-computed expected value)

ESP32 tests in firmware/platformio_smart_water_pump_controller/test/:
  TC-M-01: GPIO and relay (with printed safety warning, wait for ENTER before relay fires)
  TC-M-02: RS-485 master (30s poll, PASS if >=90% valid frames)
  TC-M-03: WiFi (connection within 20s, 2/3 pings succeed)
  TC-M-04: Firebase read/write (write test node, read back, delete)
  TC-M-05: Full round-trip integration (RS485 → parse → Firebase push → verify)

All test sketches must compile independently. Each test directory has a README.md
with prerequisites and expected output.

PHASE 6 — DASHBOARD REDESIGN
Apply SmartFlow brand: Geist + Geist Mono (self-hosted), sf-blue primary (#185FA5),
full sf-* color token system in tailwind.config.ts.

Rebranding: Replace all "Smart Water Pump Controller" with "SmartFlow" in visible strings.
Do NOT rename the Arduino sketch folder or primary .ino filename.

New components required (all must display new Firebase fields):
  - Cooldown chip: AUTO — Cooldown Xs / MANUAL — Cooldown Xs with countdown
  - manual_runtime_warning: amber non-blocking alert, information only
  - bypass_flow_sensor toggle in Advanced panel
  - is_idle_mode badge in connectivity card with tooltip
  - debug_log_level segmented control in diagnostics (writes to Firebase config)
  - remote_level_discard_count row in diagnostics
  - Level estimate visual: ~82% prefix, dashed amber chart line when level_estimate_active

Apply all dashboard bug fixes from Phase 0 audit:
  - Unsubscribe Firebase listeners in useEffect cleanup
  - Replace 'as any' with typed interfaces matching §4.2 schema
  - Null-check all nested Firebase data access
  - Validate pump_start_level < pump_stop_level in settings form
  - Add React error boundaries to each major card
  - Add skeleton loaders to all data-dependent components

PWA manifest: name="SmartFlow", theme_color="#185FA5", background_color="#F1EFE8"
Responsive: 1280px and 375px. E-stop pinned to bottom on mobile. Dark + light themes.

PHASE 7 — INTEGRATION & VALIDATION
Run all 21 integration tests from the test protocol table.
Confirm 2-hour soak test passes. Confirm production serial volume ≥80% reduction.
Complete deployment sign-off checklist.

=============================================================================
UNIVERSAL RULES — APPLY TO EVERY PHASE
=============================================================================

1. READ BEFORE WRITING. Read every file you intend to modify in full before changing
   anything. Do not rely on prior context — read the actual current source.

2. COMMENT EVERY CHANGE. Each modified code block gets:
   // REFACTOR [BUG_ID]: one-line description of what changed and why

3. ONE CONCERN PER CHANGE. Do not batch unrelated fixes into a single edit.

4. SAFETY IS NON-NEGOTIABLE. Never weaken or disable dry-run lockout, overflow
   protection, TOR layer, or sensor failure detection. If a change touches any
   safety code path, explicitly state why it is safe before proceeding.

5. REPORT FINDINGS BEFORE ACTING. Phase 0 audit report must be complete before
   any code is written. Phase N exit criteria must be confirmed before Phase N+1 starts.

6. FLAG UNCERTAINTY. If you are not certain a change is correct, say so and describe
   what verification is needed before proceeding.

7. NO SCOPE CREEP. If you find something fixable that is not in the plan, document it
   in docs/audit/out_of_scope_findings.md and do not act on it.

8. BACKWARD COMPATIBILITY. New Firebase fields are additive only. Do not rename or
   remove any existing field. The new LDSC RS-485 frame field must be optional in the
   ESP32 parser so old NodeMCU firmware remains compatible during a rolling update.

=============================================================================
WHAT NOT TO DO
=============================================================================

- Do not add features not in the refactor plan
- Do not refactor code style, naming, or structure for aesthetic reasons
- Do not change Firebase security rules
- Do not add new hardware requirements
- Do not introduce external libraries not already in the project
- Do not change the RS-485 protocol in a way that breaks backward compatibility
  between the two nodes during a rolling update

=============================================================================
OUTPUT FORMAT
=============================================================================

For each file you create or modify:
  1. State the full file path
  2. Explain what changed and why (cite bug ID if applicable)
  3. Provide the complete new file content, or a precise diff for large files

When a phase is complete, output:
  PHASE [N] COMPLETE
  Files modified: [list]
  Exit criteria met: [list each criterion and confirmation]
  Out-of-scope findings: [list, or "none"]

Begin with Phase 0. Do not write any code until the audit report is complete.
```

---

*SmartFlow System Refactor Plan v2.0*
*Engineering basis: IEC 61508 Part 7 (diagnostic coverage), IEC 61511, NEMA MG-1, TIA-485-A (RS-485 timing), YF-G1 manufacturer datasheet (1–60 L/min working range), JSN-SR04T-2.0 datasheet*
*Storey, N. (1996). Safety-Critical Computer Systems. Addison-Wesley. Ch. 10.*
