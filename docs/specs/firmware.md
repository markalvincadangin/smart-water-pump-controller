# Firmware Specification (Current)

| Field | Value |
|---|---|
| Product | SmartFlow |
| Scope | ESP32 master + ESP8266 sensor node |
| Status | Current (non-versioned) |
| Last updated | 2026-03-31 |

This document is the **current** (non-versioned) firmware specification for the SmartFlow system.
It supersedes the older archived specs and release notes. Historical documents remain under `docs/archive/`.
For the full RS-485 protocol contract see [`docs/specs/rs485_protocol.md`](./rs485_protocol.md).

### System architecture

- **ESP32 master** (controller)
  - Controls pump relay (contactor coil via relay module).
  - Runs the full safety state machine.
  - Syncs status to Firebase RTDB and reads control/config.
  - Persists configuration/state in NVS.
- **ESP8266 (NodeMCU V2) sensor node**
  - Reads **flow** (YF‑G1) via interrupt pulse counting.
  - Reads **ultrasonic** (JSN‑SR04T) via non-blocking state machine and filtering.
  - Replies to the ESP32 over **RS‑485** (MAX485).

### Firmware build targets

- **PlatformIO (recommended for CI/build validation)**
  - ESP32 master: `firmware/platformio_smart_water_pump_controller/`
  - ESP8266 node: `firmware/platformio_sensor_node/`
- **Arduino IDE (parity build, multi-tab sketches)**
  - ESP32 master: `firmware/arduino_smart_water_pump_controller/`
  - ESP8266 node: `firmware/arduino_sensor_node/`

### RS‑485 tank link (protocol contract)

**Electrical**

- Half duplex RS‑485 (MAX485-class transceiver), UART 115200 8N1.
- Master controls DE/RE (tied) via a GPIO.
- **Deployment requirement**: proper termination/biasing and solid ground reference (or isolated transceiver).

**Request**

- Master → node: `REQ\n`

**Response**

- Node → master: framed payload with CRC16 (Modbus) and sequence number:

```text
STX (0x02)
LVL:<percent>;DIST:<cm>;FLOW:<lpm>;ERR:<code>;LDSC:<n>;SEQ:<n>;CRC:<hex4>
ETX (0x03)
```

**Field semantics**

- **`LVL`**: water level percent (0–100). Provided for backward compatibility; master prefers `DIST`.
- **`DIST`**: last good median ultrasonic distance (cm). Preferred primary measurement (avoids calibration drift).
- **`FLOW`**: liters per minute (L/min), 0.00–100.00.
- **`ERR` bitfield**:
  - bit0 (1): ultrasonic error (timeout/invalid)
  - bit1 (2): flow signal error (noise/floating heuristics) — 2-stage hysteretic (Phase 2 H-04)
- **`LDSC`**: level reading discard count since last frame (0–255). Optional field added in Phase 2.
  ESP32 parser treats as optional — zero if absent (backward compatible with pre-Phase 2 NodeMCU firmware).
- **`SEQ`**: monotonically increasing uint8 (wraps at 255) per response.
- **`CRC`**: CRC16-Modbus over the payload bytes up to and including the trailing `;` after `SEQ`.

**Master acceptance rules (safety critical)**

- Reject frames with invalid framing, CRC mismatch, or out-of-range values.
- Maintain:
  - **freshness** (`levelLastUpdateMs`) updated only on validated frames
  - **stability latch** (`remoteSensorStable`) requiring N consecutive valid frames
- Fail-safe gating:
  - If data is stale or the link is unstable, the pump is **blocked from starting** and a running pump is **stopped** (unless in maintenance bypass).

### Control modes (current)

Policy mode is stored in `pumpMode` (RTDB: `/pump_system/control/mode`):

- **AUTO**: level-based hysteresis control.
- **MANUAL**: operator policy mode with persistent intent `manual_desired`.
  - `manual_desired=true` requests pump ON (all safety still enforced).
  - `manual_desired=false` keeps pump OFF (mode stays MANUAL).
- **COUNTDOWN**: timed run.
  - Start is explicit via one-shot `/pump_system/control/countdown_start=true`.
  - Duration via `/pump_system/control/countdown_duration_min`.

**Run mode values** (`runMode` → RTDB: `run_mode` in `/pump_system/status`):

| Value | Condition | Dashboard label |
|-------|-----------|----------------|
| `AUTO_STANDBY` | AUTO, pump off, level OK | AUTO — Standby |
| `AUTO` | AUTO, pump running | AUTO — Running |
| `AUTO_COOLDOWN` | AUTO, pump off, off-timer active | AUTO — Cooldown Xs |
| `MANUAL_ON` | MANUAL, pump running | MANUAL — On |
| `MANUAL_OFF` | MANUAL, pump off | MANUAL — Off |
| `MANUAL_COOLDOWN` | MANUAL, pump off, off-timer active | MANUAL — Cooldown Xs |
| `COUNTDOWN` | COUNTDOWN mode (active or idle timer state) | Countdown |

`emergency_stop_latched` is published separately and should be used as the canonical emergency-stop indicator.

**Emergency stop**

- One-shot `/pump_system/control/emergency_stop=true` latches `emergencyStopLatched`.
- Reset via one-shot `/pump_system/control/reset_stop=true`.

### Safety model (hard invariants)

The firmware must always satisfy:

- **Emergency stop latch** forces pump OFF until reset.
- **Dry-run lockout**: sustained low flow while running → pump OFF + lockout until cleared.
- **Overflow protection**: max runtime exceeded → pump OFF + lockout until cleared.
- **Sensor/comm failsafe**: stale or unstable remote data blocks starts; stale data stops a running pump.
- **Minimum off-time**: prevents rapid cycling of the pump motor.

### Cloud contract (Firebase RTDB)

**Status (ESP32 → cloud)**: `/pump_system/status`

Core fields:

- `water_level_percent` (0–100, omitted until first valid RS-485 frame)
- `flow_rate_lpm` (L/min)
- `is_running` (bool)
- `run_mode` (see Run Mode table above)
- `pump_cooldown_remaining_sec` (int, 0 when not in cooldown)
- `is_error` / `is_level_sensor_error` / `is_flow_sensor_error` / `is_overflow_error` (bools)
- `last_fault_code` / `last_fault_message` (strings, when faulted)
- `manual_desired` / `emergency_stop_latched` (bools)
- `remote_sensor_stable` / `level_fresh` (safety gate indicators)
- `bypass_level_sensor` / `bypass_flow_sensor` / `auto_bypass_active` (bool)
- `manual_runtime_warning` (bool — MANUAL run reached ~90% of max runtime; pump may still be on until hard overflow cutoff at 100%)
- `is_idle_mode` (bool — slow-poll mode active; added Phase 3)
- `is_sleeping` (bool — scheduled light sleep active)
- `remote_level_discard_count` (int — from RS-485 LDSC field; added Phase 3)
- `countdown_remaining_sec` (int)
- `debug_log_level` (int 0–4 — current gLogLevel; added Phase 1)
- `wifi_rssi` / `uptime_minutes` / `last_boot_reason`
- `free_heap_bytes` / `min_free_heap_bytes` / `min_free_heap_observed_bytes`
- `firebase_consecutive_failures` / `firebase_last_error`
- `total_pump_cycles` / `total_pump_run_min`
- `ultrasonic_cycles_ok` / `ultrasonic_cycles_timeout` / `ultrasonic_last_good_cm`
- `flow_discard_max_sane` / `flow_stuck_high_events`
- `estimated_level_pct` / `level_estimate_active` / `flow_volume_added_l`
- `level_last_valid_age_sec` / `level_sensor_health_pct` (dashboard diagnostic metrics)

**Control (cloud → ESP32)**: `/pump_system/control`

- `mode`: AUTO | MANUAL | COUNTDOWN
- `manual_desired`: bool (persistent intent)
- `emergency_stop`: bool (one-shot)
- `reset_stop`: bool (one-shot)
- `clear_error`: bool (one-shot — clears DRY_RUN and OVERFLOW lockouts)
- `countdown_start`: bool (one-shot)
- `countdown_duration_min`: int (1–120)
- `countdown_add_time` + `countdown_add_min`: one-shot time extension
- `bypass_level_sensor`: bool (persistent)
- `bypass_flow_sensor`: bool (persistent — added Phase 1)
- `reboot_request_id`: int (monotonic token)

### Hardware assumptions (deployment-critical)

- JSN‑SR04T **ECHO must be level shifted to 3.3V** on ESP8266.
- RS‑485 termination/biasing must be correct for cable length.
- Buck converter sizing and wiring must tolerate pump-induced transients; brownouts are expected and the system fails safe.

