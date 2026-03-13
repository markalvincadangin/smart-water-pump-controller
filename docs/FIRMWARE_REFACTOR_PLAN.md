# Firmware Refactoring Plan — Organization, Readability, Maintainability

**Goal:** Make the firmware easier to navigate, read, and maintain **without** changing behavior, adding complexity, or risking correctness. No OOP or multi-file split required for Phase 1.

**Constraint:** Single `.ino` file remains the norm for this sketch; refactor is in-place (comments, grouping, optional reorder only).

---

## Current Structure (Summary)

The sketch is ~1408 lines with 18 numbered sections:

| Section | Content |
|--------|--------|
| 1 | Configuration (secrets.h) |
| 2 | Pin definitions |
| 3 | Tank & sensor calibration #defines |
| 4 | Safety & timing constants |
| 5 | Firebase/NVS objects (fbdo, auth, config, prefs) |
| 6 | Global state variables |
| 7 | Flow ISR (flowPulseISR) |
| 8 | Hardware control (setPump) |
| 9 | Ultrasonic sensor (readSingleUltrasonic, readUltrasonicSensor) |
| 10 | Flow rate (calculateFlowRate) |
| 11 | Safety checks (sensor failure, flow stuck, dry-run, overflow, checkSafetyCutoff) |
| 12 | Pump state machine (executePumpLogic) |
| 12b | Device config NVS (load/save, apply from Firebase JSON) |
| 12b2 | Sleep window helper (isInSleepWindow) |
| 12c | Firebase read device config |
| 13 | Firebase read control (mode, clear_error, reboot_request_id) |
| 14 | Firebase push status |
| 15 | WiFi (connectWiFi) |
| 16 | Firebase init, boot reason (initFirebase, getBootReasonString) |
| 16C | Crash loop (checkCrashLoop) |
| 16D | NVS state (loadStateFromNVS, persistStateToNVS) |
| 17 | setup() |
| 18 | loop() |

---

## Phase 1 — In-Place Organization (Recommended First)

All changes are **comments and optional reordering only**. No new files, no new build steps, no logic changes.

### 1.1 Add a table of contents at the top

Immediately after the opening file header (version, PIN MAPPING, SYSTEM STATES) and **before** the first `#include`, insert a comment block:

```c
// =============================================================================
// TABLE OF CONTENTS
// =============================================================================
// Part I: Configuration and global state
//   Section 1    Configuration (secrets.h)
//   Section 2    Pin definitions
//   Section 3    Tank & sensor calibration
//   Section 4    Safety & timing constants
//   Section 5    Firebase/NVS objects
//   Section 6    Global state variables
//
// Part II: Sensors and hardware
//   Section 7    Flow pulse ISR
//   Section 8    Pump relay (setPump)
//   Section 9    Ultrasonic level (readSingleUltrasonic, readUltrasonicSensor)
//   Section 10   Flow rate (calculateFlowRate)
//
// Part III: Safety and pump logic
//   Section 11   Safety checks (sensor failure, flow stuck, dry-run, overflow)
//   Section 12   Pump state machine (executePumpLogic)
//
// Part IV: Configuration and state persistence
//   Section 12b  Device config NVS (load, save, apply from Firebase)
//   Section 12b2 Sleep window (isInSleepWindow)
//   Section 16C  Crash loop (checkCrashLoop)
//   Section 16D  NVS state (loadStateFromNVS, persistStateToNVS)
//
// Part V: Firebase and WiFi
//   Section 12c  Firebase read device config
//   Section 13   Firebase read control
//   Section 14   Firebase push status
//   Section 15   WiFi (connectWiFi)
//   Section 16   Firebase init, boot reason
//
// Part VI: Entry points
//   Section 17   setup()
//   Section 18   loop()
// =============================================================================
```

**Benefit:** Anyone opening the file can jump to the right section by searching for "Section 12" or "Part III".

### 1.2 Add part headers above each logical group

Insert a single **Part** comment line immediately before the first section of each group (no code movement):

- Before **Section 1**: `// -------- Part I: Configuration and global state --------`
- Before **Section 7**: `// -------- Part II: Sensors and hardware --------`
- Before **Section 11**: `// -------- Part III: Safety and pump logic --------`
- Before **Section 12b** (device config NVS): `// -------- Part IV: Configuration and state persistence --------`
- Before **Section 12c** (Firebase read device config): `// -------- Part V: Firebase and WiFi --------`
- Before **Section 17** (setup): `// -------- Part VI: Entry points --------`

**Benefit:** Scrolling through the file shows clear "chapters" without changing any logic.

### 1.3 Optional: function index

Add a short **Function index** comment block after the table of contents (or at the end of the TOC block) listing the main functions and their section, e.g.:

```c
// Main functions: setPump(8), readUltrasonicSensor(9), calculateFlowRate(10),
// checkSensorFailure/checkFlowSensorStuck/checkDryRun/checkOverflowProtection/checkSafetyCutoff(11),
// executePumpLogic(12), loadDeviceConfigFromNVS/saveDeviceConfigToNVS(12b), isInSleepWindow(12b2),
// readDeviceConfigFromFirebase(12c), readFirebaseControl(13), pushFirebaseStatus(14),
// connectWiFi(15), initFirebase(16), getBootReasonString(16B), checkCrashLoop(16C),
// loadStateFromNVS/persistStateToNVS(16D).
```

**Benefit:** Quick lookup when searching for where something is implemented.

### 1.4 Do not change

- No reordering of sections (current order is already dependency-safe: config and globals first, then sensors/hardware, then safety/pump, then Firebase/NVS/WiFi, then setup/loop).
- No splitting of the file into multiple `.ino` or `.cpp` files in Phase 1.
- No introduction of classes or new abstractions.
- No changes to function bodies, globals, or control flow.

---

## Phase 2 — Optional multi-tab split (only if you want separate files)

If after Phase 1 you still want **physical file splits** (e.g. to reduce scroll length or to edit one area without loading the whole file), use the **Arduino multi-tab** approach: multiple `.ino` files in the same folder are concatenated **alphabetically** before compilation.

### 2.1 File list and order

Concatenation order must preserve: includes and config first, then globals, then functions that use those globals, then `setup()` and `loop()` last. Proposed names and contents:

| Order | Filename | Contents |
|-------|----------|----------|
| 1 | `01_config.ino` | All `#include`, `#define` (pins, tank, Section 4 constants), Firebase/Preferences objects (Section 5), and all global variables (Section 6). Must include `secrets.h`. |
| 2 | `02_sensors.ino` | Section 7 (ISR), 9 (ultrasonic), 10 (flow rate). |
| 3 | `03_safety_pump.ino` | Section 8 (setPump), 11 (safety checks), 12 (executePumpLogic). |
| 4 | `04_nvs.ino` | Section 12b, 12b2, 16C, 16D (device config NVS, sleep helper, crash loop, state load/save). |
| 5 | `05_firebase_wifi.ino` | Section 12c, 13, 14, 15, 16, 16B (Firebase read config/control, push status, WiFi, init, boot reason). |
| 6 | `smart_pump_controller.ino` | Section 17 (setup), Section 18 (loop) only. No duplicate includes or globals. |

**Critical:** The main sketch file that the IDE opens is `smart_pump_controller.ino`; it must come **last** alphabetically so that `setup()` and `loop()` are at the end of the merged file. The numeric prefix `01_` … `05_` ensures the correct order.

### 2.2 What to put in the main .ino

- Only **Section 17 (setup)** and **Section 18 (loop)**.
- A brief comment at the top: "Main entry points. Other sections are in 01_config.ino … 05_firebase_wifi.ino (concatenated alphabetically)."
- No `#include`, no `#define`, no global variables in the main file (they live in `01_config.ino`).

### 2.3 Risks and how to avoid them

- **Wrong order:** If you add a new file (e.g. `00_something.ino`), keep the numbering so that config/globals stay first and `smart_pump_controller.ino` stays last.
- **Duplicate symbols:** Ensure each global and each function is defined in exactly one file. No `#include` of other `.ino` files (they are merged, not included).
- **Build verification:** After the split, do a clean build and run the same regression checks (WiFi reconnect, sensors, Firebase, NVS, safe mode) to confirm behavior is unchanged.

### 2.4 When to do Phase 2

- Only if you actually want smaller files to work with (e.g. "sensors only" or "Firebase only").
- If the single file with Phase 1 (TOC + part headers) is enough, **stop after Phase 1** to avoid build/order issues.

---

## Summary

| Phase | Action | Risk | Outcome |
|-------|--------|------|---------|
| **1** | Add TOC, part headers, optional function index; no code or order change | None | Same behavior; much easier to navigate and maintain. |
| **2** (optional) | Split into 6 `.ino` files with strict alphabetical order | Low if order and single-definition rule are kept | Same behavior; smaller, focused files. |

**Recommendation:** Implement **Phase 1 only** first. Rebuild, run your usual tests, and use the firmware for a while. Introduce Phase 2 only if you still want physical file splits and are comfortable with the Arduino concatenation order.
