---
name: smartflow
description: >
  Use for any task in the SmartFlow repository: firmware (ESP32 master, NodeMCU sensor node),
  RS-485 protocol, Firebase RTDB schema, Next.js dashboard, documentation, or deployment
  runbooks. Trigger on: SmartFlow, ESP32, NodeMCU, RS-485, JSN-SR04T, YF-G1, dry-run,
  overflow, run_mode, countdown, emergency_stop, RTDB, pump controller, sensor node,
  tank level, flow sensor, platformio, issue IDs M-xx / S-xx / X-xx, or any request
  touching firmware/dashboard/docs in this repo.
---

# SmartFlow — Copilot Skill

## 1. Product Identity

**SmartFlow** is a production IoT water pump controller deployed in Leon, Iloilo, Philippines.
It automates a 1.5 HP Lotus Jet Pump filling a 660 L Bestank WT660 tank, with real-time
monitoring and remote control via a Next.js PWA dashboard backed by Firebase RTDB.

The system is safety-critical. A pump fault can damage hardware or waste hundreds of liters
of water. Every code change must be evaluated against this physical reality.

**Public-facing identity:** SmartFlow (not "Smart Water Pump Controller"). Use this name in
all user-visible strings, documentation, and commit messages.

---

## 2. Non-Negotiable Safety Rules

These rules override all other instructions. Never violate them.

1. **Fail toward pump OFF.** Every fault path, timeout, and ambiguous state must call
   `setPump(false)` or leave the relay in its de-energized (HIGH) state. Never bias toward ON.
2. **Never weaken dry-run lockout.** Flow < `cfgDryRunThresholdLpm` for longer than
   `cfgDryRunTimeoutSec` → `isDryRunError = true` → `setPump(false)`. This path is
   non-negotiable and must survive every refactor.
3. **Never weaken overflow protection.** Pump runtime > `cfgMaxPumpRuntimeMin` → `isOverflowError = true`
   → `setPump(false)`. This applies to AUTO, COUNTDOWN, and MANUAL modes equally.
4. **TOR is independent.** The LR2-D13 Thermal Overload Relay at 8–9 A cuts power to the
   contactor at the hardware level. Software must never assume it has sole protection.
5. **E-stop is always reachable.** `readFirebaseControl()` must never return early in a way
   that prevents `reset_stop` or `clear_error` from being processed while the latch is active.
6. **Level freshness gate is mandatory.** Before any pump start decision, verify
   `(levelLastUpdateMs > 0) && elapsedMillis32(nowMs, levelLastUpdateMs) <= LEVEL_STALE_TIMEOUT_MS`.
   The `> 0` guard is essential — it prevents false-fresh reads at boot before the first
   RS-485 frame arrives.
7. **RS-485 and Firebase changes must be backward compatible.** New frame fields must be
   optional in the parser. New RTDB fields must be additive only.

---

## 3. Hardware Reference

```
Power chain (always active, independent of firmware):
  MCB → CJX2-2510 Contactor → LR2-D13 TOR (8–9 A dial) → 1.5 HP 220 V Lotus Jet Pump

Microcontrollers:
  ESP32 DevKit V1 (master)   — PlatformIO, UART2 for RS-485, GPIO4 relay
  NodeMCU V2 / ESP8266 (slave) — PlatformIO, UART0 for RS-485, GPIO2 debug TX-only

GPIO — Master (ESP32):
  RELAY_PIN       = GPIO4   (active-LOW: LOW = pump ON, HIGH = pump OFF)
  RS485_TX_PIN    = GPIO17
  RS485_RX_PIN    = GPIO25
  RS485_DE_RE_PIN = GPIO5   (LOW = RX, HIGH = TX)

GPIO — Sensor Node (NodeMCU):
  PIN_RS485_DE_RE = GPIO14  (D5)
  PIN_FLOW_INPUT  = GPIO12  (D6, INPUT_PULLUP, FALLING edge ISR)
  PIN_US_TRIG     = GPIO5   (D1)
  PIN_US_ECHO     = GPIO16  (D0)

Sensors:
  JSN-SR04T-2.0 waterproof ultrasonic — 20–600 cm range, mounted above tank
  YF-G1 1-inch hall-effect flow meter — 1–60 L/min, ~7.5 pulses/L

Tank calibration (Leon, Iloilo field values, 2026-04-03):
  TANK_US_DIST_EMPTY_CM = 120.0  (distance sensor→water surface when tank empty)
  TANK_US_DIST_FULL_CM  = 30.0   (distance sensor→water surface when tank full)
  TANK_US_DISTANCE_OFFSET_CM = -5.8  (field-measured trim)
  TANK_CAPACITY_L = 660
  tank_empty_cm (RTDB/NVS) = 122
  tank_full_cm  (RTDB/NVS) = 25–30  (calibrate on-site)
  flow_calibration_factor  = 7.5   (YF-G1 Hz/LPM — bucket-validate in field)

RS-485 cable: 40 m outdoor CAT6
Enclosure: IP65 ABS 30×40×20 cm
Location timezone: PHT = UTC+8
```

---

## 4. Repository Layout

```
smartflow/  (repo root)
├── firmware/
│   ├── platformio_smart_water_pump_controller/   ← ESP32 master (ACTIVE)
│   │   ├── src/
│   │   │   ├── main.cpp
│   │   │   ├── config/config.h                  ← all compile-time constants + LOG macro
│   │   │   ├── state/state.h + state.cpp        ← all global variables
│   │   │   ├── rs485/rs485_comm.h + .cpp
│   │   │   ├── safety/safety_pump.h + .cpp
│   │   │   ├── persistence/persistence.h + .cpp
│   │   │   ├── connectivity/connectivity_cloud.h + .cpp
│   │   │   └── utils/time_utils.h               ← elapsedMillis32, millisDeadlineReached, addMillisSaturated
│   │   └── platformio.ini
│   ├── platformio_sensor_node/                   ← NodeMCU sensor (ACTIVE)
│   │   ├── src/
│   │   │   ├── main.cpp
│   │   │   ├── config/config.h
│   │   │   ├── state/state.h + state.cpp
│   │   │   ├── sensors/sensors.h + .cpp
│   │   │   ├── rs485/rs485_slave.h + .cpp
│   │   │   ├── ota/ota_wifi.h + .cpp
│   │   │   └── utils/
│   │   └── platformio.ini
│   └── arduino_*/                               ← REFERENCE ONLY — do not edit
├── dashboard/                                    ← Next.js 14 App Router PWA
│   ├── app/                                      ← pages (App Router)
│   ├── components/                               ← UI components
│   └── lib/                                      ← Firebase client, hooks, types
├── functions/                                    ← Firebase Cloud Functions
├── hardware/                                     ← BOM, wiring, enclosure layout
└── docs/
    ├── audit/                                    ← firmware_known_issues_*.md
    ├── specs/                                    ← firebase_schema.md, rs485_protocol.md
    └── runbooks/                                 ← pre-flash checklist, field calibration
```

---

## 5. RS-485 Protocol

**Baud:** 115 200, 8N1, half-duplex, CRC-16 Modbus (poly 0xA001, init 0xFFFF)

**Request (Master → Sensor):**
```
REQ\n
```

**Response (Sensor → Master):**
```
STX  LVL:<pct>;DIST:<cm>;FLOW:<lpm>;ERR:<code>;LDSC:<n>;SEQ:<seq>;  CRC:<HEX4>  ETX
0x02                                                                              0x03
```

**Field definitions:**

| Field | Type   | Notes |
|-------|--------|-------|
| LVL   | int    | 0–100%, derived from DIST when present |
| DIST  | float  | cm, optional (absent in legacy frames) |
| FLOW  | float  | L/min, ≥ 0 |
| ERR   | int    | Bitmask: bit0 = level error, bit1 = flow error |
| LDSC  | uint8  | Per-window discard count, 0–255, optional |
| SEQ   | uint8  | Wrapping sequence number |
| CRC   | 4 hex  | CRC16-Modbus over payload bytes (after STX, before CRC: field) |

**Parser rules:**
- `LDSC` is optional — default to 0 if absent (backward compat with old sensor firmware)
- `DIST` is preferred over `LVL` for level calculation when present and in 1–500 cm range
- If `DIST` is absent or out of range, use transmitted `LVL` directly
- Frame rejected if CRC fails, required fields missing, or `lvl` outside 0–100

**Ping frame (optional health check):**
```
Request:  PING:<seq>\n
Response: STX HELLO;SEQ:<seq>;NODE_OK:1; CRC:<HEX4> ETX
```

---

## 6. Firebase RTDB Schema

**Root path:** `/pump_system/`

### `/pump_system/control` — Dashboard → Controller

| Field | Type | Default | Notes |
|-------|------|---------|-------|
| `mode` | string | `"AUTO"` | Valid: `AUTO`, `MANUAL`, `COUNTDOWN` only |
| `manual_desired` | bool | false | Persistent manual pump intent |
| `emergency_stop` | bool | false | One-shot; firmware self-clears after processing |
| `reset_stop` | bool | false | One-shot; clears e-stop latch |
| `clear_error` | bool | false | One-shot; clears dry-run/overflow errors |
| `countdown_start` | bool | false | One-shot trigger |
| `countdown_stop` | bool | false | One-shot; stops timer, mode stays COUNTDOWN |
| `countdown_add_time` | bool | false | One-shot |
| `countdown_add_min` | int | 5 | Minutes to add (1–120) |
| `countdown_duration_min` | int | 15 | Duration for next countdown start (1–120) |
| `bypass_level_sensor` | bool | false | Disables level-based gating |
| `bypass_flow_sensor` | bool | false | Disables dry-run protection |
| `reboot_request_id` | int | 0 | Non-zero triggers reboot; firmware persists last seen ID |

**One-shot semantic:** firmware processes the flag then writes `false` back. The Firebase
self-clear IS the idempotency guard — do not rely on edge-detection static flags for
`emergency_stop` or `reset_stop`.

**Deprecated (map to AUTO):** `FORCE_ON`, `FORCE_OFF` — firmware auto-maps these and
writes back `AUTO` via `pendingModeWriteback`.

### `/pump_system/status` — Controller → Dashboard (written every ~3 s)

**Pump & mode state:**
`is_running`, `run_mode`, `manual_desired`, `manual_runtime_warning`,
`countdown_active`, `countdown_remaining_sec`, `pump_cooldown_remaining_sec`,
`emergency_stop_latched`

**run_mode valid values:**
`AUTO_STANDBY`, `AUTO`, `AUTO_COOLDOWN`, `MANUAL_ON`, `MANUAL_OFF`,
`MANUAL_COOLDOWN`, `COUNTDOWN`, `STOPPED`

**Safety & faults:**
`is_error` (dry-run), `is_overflow_error`, `is_level_sensor_error`, `is_flow_sensor_error`,
`last_fault_code`, `last_fault_message`, `bypass_level_sensor`, `bypass_flow_sensor`,
`auto_bypass_active`

**Sensor & telemetry:**
`water_level_percent` (omitted when -1 at boot), `flow_rate_lpm`,
`level_fresh`, `level_last_valid_age_sec`, `level_sensor_health_pct`,
`remote_sensor_stable`, `remote_level_discard_count`,
`flow_volume_added_l`, `estimated_level_pct`, `level_estimate_active`,
`ultrasonic_cycles_ok`, `ultrasonic_cycles_timeout`, `ultrasonic_last_good_cm`

**Connectivity & timing:**
`wifi_rssi`, `uptime_minutes`, `last_boot_reason`,
`rs485_last_call_ms`, `loop_max_ms`,
`cloud_last_control_call_ms`, `cloud_last_status_call_ms`, `cloud_last_cycle_ms`,
`cloud_control_poll_stale`, `cloud_last_control_ok_age_sec`

**Firebase health:**
`firebase_consecutive_failures`, `firebase_timeout_count`,
`firebase_auth_error_count`, `firebase_not_ready_skip_count`, `firebase_last_error`

**Resource:**
`free_heap_bytes`, `min_free_heap_bytes`, `min_free_heap_observed_bytes`,
`max_alloc_heap_bytes`

**Runtime counters:**
`total_pump_cycles`, `total_pump_run_min` (rounded),
`flow_stuck_high_events`, `debug_log_level`

**`water_level_percent` must be omitted (not set to 0) when `waterLevelPct == -1`.**
This prevents the dashboard from showing 0% on first boot before any sensor data arrives.

### `/pump_system/config/device` — Tunable parameters

`tank_empty_cm`, `tank_full_cm`, `pump_start_level`, `pump_stop_level`,
`dry_run_threshold_lpm`, `dry_run_timeout_sec`, `flow_calibration_factor`,
`max_pump_runtime_min`, `sleep_enabled`, `sleep_start_hour`, `sleep_end_hour`,
`sleep_emergency_level`, `level_sensor_failure_threshold`,
`idle_sensor_interval_ms`, `idle_firebase_interval_ms`,
`auto_bypass_on_sensor_fail`, `auto_bypass_delay_sec`, `debug_log_level`

### `/pump_system/audit/events` — Append-only operator log

Each entry: `action`, `at` (ms epoch), `at_ms`, `deviceId`, `email`, `uid`, `meta`, `detail`

---

## 7. Pre-Flash Safety Checklist (RTDB values)

Before flashing master firmware, verify these values in Firebase Console:

```
/pump_system/control/emergency_stop    = false
/pump_system/control/countdown_stop   = false
/pump_system/control/bypass_level_sensor = false
/pump_system/control/bypass_flow_sensor  = false
/pump_system/control/mode             = "AUTO"
/pump_system/config/device/flow_calibration_factor = 7.5
/pump_system/config/device/tank_full_cm = 25–30 (field-calibrated)
```

If `emergency_stop` is `true`, the master will latch the e-stop on its first control poll
(~3 s after boot) and lock out the pump. If either bypass is `true`, dry-run or level
protection is disabled from first boot.

---

## 8. Coding Conventions

### Master (ESP32 — C++ PlatformIO)

**Timing — always use `time_utils.h` helpers:**
```cpp
// CORRECT — wrap-safe at the ~49-day millis() rollover
if (elapsedMillis32(millis(), lastEventMs) >= INTERVAL_MS) { ... }
if (millisDeadlineReached(millis(), deadlineMs)) { ... }
deadlineMs = addMillisSaturated(millis(), DURATION_MS);  // never raw +

// WRONG — vulnerable at rollover
if (millis() - lastEventMs >= INTERVAL_MS) { ... }      // unsafe subtraction
if (millis() >= deadlineMs) { ... }                      // unsafe comparison
deadlineMs = millis() + DURATION_MS;                     // unsafe addition
```

Exceptions (safe by construction): `millis() - t0` where `t0` is captured in the same
scope and the interval is always short (< 60 s); NVS write throttles; log rate limiters;
call-duration telemetry. Document the reasoning in a comment.

**Logging:**
```cpp
LOG(LOG_LEVEL_INFO,  "MODULE", "Normal operational message.");
LOG(LOG_LEVEL_WARN,  "MODULE", "Unexpected but non-critical: %s", detail);
LOG(LOG_LEVEL_ERROR, "MODULE", "Fault or failure: %d", code);
LOG(LOG_LEVEL_DEBUG, "MODULE", "Diagnostic: val=%d", val);
```
- Routine per-cycle telemetry (level, flow readings) → `LOG_LEVEL_INFO` or `LOG_LEVEL_DEBUG`
- Safety events and faults → `LOG_LEVEL_ERROR`
- Do not use `LOG_LEVEL_ERROR` for normal healthy sensor readings
- `LOG_COMPILE_FLOOR = LOG_LEVEL_VERBOSE` must stay in config.h so Firebase
  `debug_log_level` can raise runtime verbosity without a reflash

**Variable naming:** follow existing state.h conventions
(`cfgXxx` for config, `snXxx` for sensor node globals, `isXxx` for boolean flags,
`lastXxxMs` for timestamps, `xxxCount` for counters).

**ISR safety:**
```cpp
// Atomic read-and-zero pattern (ESP8266 and ESP32)
noInterrupts();
uint32_t count = volatileCounter;
volatileCounter = 0;
interrupts();

// ISR-incremented counters must be volatile
static volatile uint32_t flowPulseCount = 0;
```
All ISR-incremented variables must be read inside `noInterrupts()`/`interrupts()` in the
main loop. Never read a volatile ISR variable outside this guard for anything meaningful.

**One-shot Firebase flags (self-clear pattern):**
```cpp
// Process every time the flag is true — Firebase clear is the idempotency guard
if (jd.success && jd.boolValue) {
  if (!alreadyLatched) {
    // ... act ...
  }
  Firebase.RTDB.setBool(&fbdo, "/path/to/flag", false); // always clear
}
```
Do not rely on `!lastFlagValue` edge detection for `emergency_stop` or `reset_stop` —
a soft-reset that sees a stale `true` in Firebase must still process the flag correctly.

**setPump() is the single relay control point.** Never call `digitalWrite(RELAY_PIN, ...)` directly.

### Sensor Node (NodeMCU — C++ PlatformIO)

- Use `SENSOR_DBGF(...)` for debug output (routes to Serial1/GPIO2 in production, USB in bench mode)
- All ISR code must be `IRAM_ATTR`
- `flowRawEdgeCount` and `flowPulseCount` must increment on the same deglitch-passed path in the ISR
- Median calculation uses upper-median (`n/2`) by design for odd sample counts (US_SAMPLES=5)

---

## 9. Known Issue Registry (Current Status)

Copilot must not re-introduce any resolved issue. Reference this before suggesting changes.

| ID | Severity | Status | Summary |
|----|----------|--------|---------|
| M-13 | Critical | **Resolved** | Countdown expiry used unsafe `millis() >= endMs` → fixed with `millisDeadlineReached()` |
| M-14 | Critical | **Resolved** | Unsafe `millis() - X` in all safety-path timers → fixed with `elapsedMillis32()` |
| M-15/M-23 | High | **Resolved** | Boot-time freshness false-positive → fixed with `levelLastUpdateMs > 0` guard |
| M-16 | High | **Resolved** | `emergency_stop` / `countdown_start` edge-detection → fixed with Firebase self-clear as idempotency guard |
| M-18 | High | **Resolved** | `countdown_add_min` could wrap unsigned arithmetic → fixed with `addMin >= 1` gate + `addMillisSaturated()` |
| M-19 | Medium | **Resolved** | `countdown_remaining_sec` ambiguous at 0 → fixed by adding `countdown_active` boolean field |
| M-20 | High | **Resolved** | Firebase cooldown overflow `now + COOLDOWN` → fixed with `addMillisSaturated()` |
| M-21 | High | **Resolved** | RS-485 frame timeout unsafe subtraction → fixed with `elapsedMillis32()` |
| M-24 | High | **Resolved** | RS-485 parse failure used `Serial.print()` bypassing syslog → fixed with `LOG()` |
| M-26 | Medium | **Resolved** | `isLevelFresh` recomputed multiple times per loop → fixed with single `nowMsPump` snapshot |
| M-27 | Critical | **Resolved** | E-stop early return blocked `reset_stop`/`clear_error` → fixed by removing early return |
| M-28 | Medium | **Resolved** | Error log used boot-time monotonic clock → fixed with NTP epoch when synced |
| M-29 | Low | **Resolved** | `total_pump_run_min` truncated seconds → fixed with rounding `(sec + 30) / 60` |
| M-30 | Medium | **Resolved** | `LOG_COMPILE_FLOOR` was `LOG_LEVEL_INFO`, blocking DEBUG/VERBOSE → fixed, now `LOG_LEVEL_VERBOSE` |
| M-31 | High | **Resolved** | WDT registered after `connectWiFi()` → fixed, WDT now registered before WiFi |
| M-32 | Medium | **Resolved** | Offline branch in `rs485_comm.cpp` used raw subtraction → fixed with `elapsedMillis32()` |
| S-06/S-09 | High | **Resolved** | `flowRawEdgeCount` ISR race → fixed by moving increment inside deglitch gate |
| M-01 | High | **Mitigated** | RS-485 stalls starving Firebase → mitigated with 150 ms time budget cap |
| NEW-01 | Low | **Open** | `firebaseCooldownUntilMs = millis() + 10000UL` in main.cpp WiFi reconnect → use `addMillisSaturated()` |
| NEW-02 | Cosmetic | **Open** | Routine sensor readings logged at `LOG_LEVEL_ERROR` → change to `LOG_LEVEL_INFO` |
| NEW-03 | Low | **Open** | Sleep wake `nextWake = lastSensorMs + SLEEP_WAKE_INTERVAL_MS` unsafe addition → use `addMillisSaturated()` |
| M-03/M-04 | High | **Open** | Crash-loop safe mode leaves device unreachable; NTP-dependent recovery |
| M-06 | Medium | **Open** | Runtime timers volatile across reboot |
| S-04 | Medium | **Open** | `FLOW_MIN_PULSE_INTERVAL_US = 800 µs` marked "temporary" — needs field validation with installed YF-G1 |
| S-10/S-11 | Medium | **Open** | `remote_level_discard_count` is per-window (resets every ~1 s), not cumulative — document for dashboard |

---

## 10. Pump State Machine

```
Priorities (highest to lowest):
  1. Emergency stop latch (emergencyStopLatched)     → setPump(false), block all starts
  2. Hard lockouts (isDryRunError, isOverflowError)  → setPump(false), block all starts
  3. Sensor validity gate (remoteSensorStable && levelFreshOk && !isLevelSensorError)
     → if fails and pump is running: setPump(false) with COMM_LOSS or STALE_LEVEL fault
  4. Mode-specific logic (AUTO / MANUAL / COUNTDOWN)

Mode transitions:
  AUTO_STANDBY → AUTO   : level ≤ cfgPumpStartLevel
  AUTO         → AUTO   : running, level < cfgPumpStopLevel
  AUTO         → AUTO_STANDBY : level ≥ cfgPumpStopLevel → setPump(false)
  MANUAL_OFF   → MANUAL_ON   : manualDesired = true
  MANUAL_ON    → MANUAL_OFF  : manualDesired = false, OR level ≥ cfgPumpStopLevel
  COUNTDOWN (active) → pump ON while isCountdownActive
  COUNTDOWN (expired/stopped) → setPump(false), mode stays COUNTDOWN

Minimum off-time: MIN_PUMP_OFF_TIME_MS = 30 000 ms between pump starts (motor protection).
```

---

## 11. Dashboard (Next.js 14 App Router)

**Stack:** Next.js 14, TypeScript, Tailwind CSS, Recharts, Firebase Auth + RTDB

**Auth:** Google OAuth (dashboard users), Email/Password (ESP32 device)

**Brand tokens (use these, do not invent new colors):**
```
sf-blue:    #185FA5   (primary brand)
sf-teal:    #0F6E56   (success / running)
sf-amber:   #BA7517   (warning)
sf-red:     #A32D2D   (error / fault)
sf-gray-50: #F1EFE8   (background)
Font: Geist (UI) + Geist Mono (telemetry values)
```

**Key dashboard concerns:**
- `water_level_percent` may be absent on first boot — handle null/undefined, never default to 0
- `run_mode` drives the primary status indicator — validate against the full enum list in §6
- `countdown_active` + `countdown_remaining_sec` together determine countdown UI state
  (both false/0 can mean "never started" OR "expired" — `countdown_active` is the discriminator)
- `emergency_stop_latched` = true → show locked-out state, only `reset_stop` clears it
- `level_fresh` = false → stale data warning on level display
- `cloud_control_poll_stale` = true → connection health warning
- One-shot control writes: always write the flag, confirm receipt by watching it return to `false`
- Firebase writes from dashboard should be logged to `/pump_system/audit/events`

---

## 12. Documentation Standards

When writing or updating any doc in `docs/`, `hardware/`, or `README` files:

**Accuracy first.** Every value, pin, threshold, and mode must match the actual firmware.
Do not invent plausible-sounding values — use the constants in `config.h` and `state.h`
as the source of truth.

**Writing style for public-facing content:**
- Product name: SmartFlow (title case, no hyphen)
- Tone: clear, professional, concise — suitable for a GitHub README, technical blog post,
  or portfolio project
- Avoid internal jargon (`snWaterLevelPct`, `FORCE_OFF`) in user-facing docs;
  use descriptive equivalents ("water level percentage", "disabled mode")
- Audience tiers: operator guide (non-technical), developer guide (firmware/dashboard
  contributors), field technician guide (hardware installation and calibration)

**Issue/audit documents** (`docs/audit/`):
- Always include: ID, severity, issue description, impact, status, and verification method
- Status values: `Open`, `Confirmed`, `Mitigated`, `Resolved — verified in source`, `Closed — not a bug`
- Resolved items stay in the register — they are regression-detection anchors

**Runbook structure** (`docs/runbooks/`):
- Title, purpose, prerequisites, step-by-step procedure with exact commands/values,
  expected outcome, rollback procedure if applicable
- All RTDB paths must be exact strings (e.g. `/pump_system/control/emergency_stop`)
- All threshold values must match firmware constants

**Changelog / release notes:**
- Use conventional commit style for entries
- Group by: Safety, Features, Fixes, Refactors, Docs
- Safety items always listed first

---

## 13. Validation Before Submitting Changes

**Firmware:**
```bash
# Master (ESP32)
cd firmware/platformio_smart_water_pump_controller
pio run -e esp32dev

# Sensor node (NodeMCU)
cd firmware/platformio_sensor_node
pio run
```
Do not declare a firmware change complete without a successful compile.

**Dashboard:**
```bash
cd dashboard
npm run type-check   # or: npx tsc --noEmit
npm run build        # verify no build errors
```

**RS-485 frame changes:** manually verify CRC covers the correct byte range (payload from
after STX up to and including the trailing `;` after `SEQ:`).

**Schema changes:** update `docs/specs/firebase_schema.md` in the same PR.

---

## 14. Operational Context

- **Timezone:** PHT (UTC+8) — timestamps in logs and Firebase should reflect this
- **Sleep window:** 23:00–05:00 PHT default; emergency level override at ≤ 5%
- **Watchdog timeout:** 120 s — Firebase RTDB operations can block under poor WiFi;
  WDT is reset in the WiFi retry loop and before `esp_light_sleep_start()`
- **Crash-loop threshold:** 5 rapid reboots → safe mode (pump OFF, WiFi/Firebase disabled,
  60 s stable-uptime clears the counter)
- **NVS namespaces:** `pump_cfg` (device config), `pump_state` (mode, errors, counters)
- **OTA:** ArduinoOTA on sensor node only; master rebooted via `reboot_request_id` RTDB field

---

## 15. What Copilot Should Never Do

- Write `digitalWrite(RELAY_PIN, ...)` outside `setPump()`
- Use `millis() + X` for deadline calculation (use `addMillisSaturated()`)
- Use `millis() >= deadline` for expiry check (use `millisDeadlineReached()`)
- Set `water_level_percent` to 0 when `waterLevelPct == -1` at boot
- Return early from `readFirebaseControl()` in a way that skips `reset_stop` or `clear_error`
- Make `emergency_stop` processing depend on a rising-edge static flag across reboots
- Add a new RTDB field without updating `docs/specs/firebase_schema.md`
- Remove or reduce `LOG_COMPILE_FLOOR` below `LOG_LEVEL_VERBOSE`
- Increment ISR-shared counters outside the `noInterrupts()` guard in the main loop
- Use `FORCE_ON` or `FORCE_OFF` as valid pump modes in new code
- Log routine healthy sensor readings at `LOG_LEVEL_ERROR`
- Call `pushFirebaseErrorLog()` for non-error events