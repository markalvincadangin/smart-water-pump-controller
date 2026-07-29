---
status: current
version: 1.1
last-reviewed: 2026-07-25
source: hand-authored
---

# Firmware Specification (Current)

| Field | Value |
|---|---|
| Product | SmartFlow |
| Scope | ESP32 master + NodeMCU V2 (ESP8266) sensor node |
| Status | Current (non-versioned) |
| Last reviewed | 2026-07-25 |

This document is the **current** (non-versioned) firmware specification for the SmartFlow system.
It supersedes the older archived specs and release notes. Historical documents remain under `docs/archive/`.
For the full RS-485 protocol contract see [`docs/specs/rs485_protocol.md`](./rs485_protocol.md).
For implemented operational behavior (modes, safety rules, WiFi, restart/safe-mode) see [`docs/specs/firmware_operational_rules.md`](./firmware_operational_rules.md).

### System architecture

- **ESP32 master** (controller)
  - Refactored into a strict embedded layer hierarchy:
    - `hal/`: Hardware Abstraction Layer (GPIO isolation).
    - `drivers/`: Device control (Sensors, Pump).
    - `services/`: Domain logic (Water Level, Flow).
    - `core/`: State machine, Lifecycle, and Bootloader.
    - `safety/`: Independent E-Stop, Dry-run, and Overflow evaluation logic.
  - Controls pump relay (contactor coil via relay module).
  - Runs the full safety state machine.
  - Syncs status to Firebase RTDB and reads control/config.
  - Persists configuration/state in NVS.
- **ESP8266 (NodeMCU V2) sensor node**
  - Reads **flow** (YF‑G1) via interrupt pulse counting.
  - Reads **ultrasonic** (JSN‑SR04T) via non-blocking state machine and filtering.
  - Replies to the ESP32 over **RS‑485** (MAX485).

### Firmware build targets

- ESP32 master (PlatformIO): `firmware/master_node/`
- ESP8266 node (PlatformIO): `firmware/sensor_node/`

### Hardware interface (GPIO assignments)

Production pin assignments as defined in each module's `config.h`.
Changes to these pins require updating both the source constants and this table.

**ESP32 master** (`firmware/master_node/src/config/config.h`)

| Constant | GPIO | Direction | Function |
|----------|------|-----------|----------|
| `RELAY_PIN` | GPIO 4 | OUT | Pump relay coil — HIGH = de-energized = pump OFF |
| `RS485_TX_PIN` | GPIO 17 (UART2 TX2) | OUT | RS-485 transmit → MAX485 DI |
| `RS485_RX_PIN` | GPIO 25 (UART2 RX2) | IN | RS-485 receive ← MAX485 RO |
| `RS485_DE_RE_PIN` | GPIO 5 | OUT | RS-485 direction — LOW = RX, HIGH = TX |

**NodeMCU V2 sensor node** (`firmware/sensor_node/src/config/config.h`)

| Constant | GPIO | NodeMCU label | Direction | Function |
|----------|------|---------------|-----------|----------|
| `PIN_RS485_DE_RE` | GPIO 14 | D5 | OUT | RS-485 direction — DE+RE tied |
| `PIN_FLOW_INPUT` | GPIO 12 | D6 | IN (PULLUP) | YF-G1 flow sensor pulse — *marked temporary diagnostic reroute in source; verify against hardware* |
| `PIN_US_TRIG` | GPIO 5 | D1 | OUT | JSN-SR04T ultrasonic trigger |
| `PIN_US_ECHO` | GPIO 16 | D0 | IN | JSN-SR04T ultrasonic echo ¹ |

RS-485 UART0 (NodeMCU): TX = GPIO1, RX = GPIO3 — fixed hardware UART0 pins, selected implicitly when `Serial` is initialized in production mode; not `#define` constants.
Debug output: Serial1 TX-only on GPIO2 (production mode only).

¹ ECHO requires level-shifting to 3.3 V — see [§ Hardware assumptions](#hardware-assumptions-deployment-critical) below.

---

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

#### Stable device authorization

The master authenticates as the stable Firebase UID `device:{device_id}`. On each boot it obtains a Firebase custom token from the regional `bootstrapDevice` HTTPS function by presenting its immutable device ID, current NTP timestamp, single-use nonce, and a per-device HMAC proof. The device-specific bootstrap secret is compiled from ignored local configuration; its backend counterpart lives only in Google Cloud Secret Manager.

The firmware must fail closed when its bootstrap configuration, trust root, NTP time, HTTPS validation, or token exchange is unavailable: it may not fall back to anonymous sign-in or unauthenticated RTDB access. RTDB rules allow a device principal to read only its own configuration/control paths and write only its own telemetry, status, reported shadow, events, diagnostics, and permitted metadata. Ownership remains an Android-user concern and is separate from device authentication.

Device metadata uses `deviceAuthUid` (not a transient Firebase UID). The Cloud Function establishes that field before device writes; firmware may only retain its immutable MAC-derived device ID and does not persist a Firebase access/refresh token or anonymous UID in NVS.

#### Ownership pairing boundary

The authoritative owner is `/devices/{device_id}/ownership/ownerUid`; the legacy-compatible `metadata/claimedByUid` and `/users/{uid}/devices/{device_id}` index are written atomically only by trusted backend code. Android clients do not write those paths directly. After `get_token`, firmware creates a fresh BLE-local proof, publishes only its SHA-256 verifier at `/devices/{device_id}/pairing/current`, and never persists or logs the raw proof. The verifier is single-use and expires after five minutes. Android may claim only through the regional `claimDevice` callable after the final BLE `provisioned` status and only while authenticated as a non-anonymous, durable account (Google or verified email/password).

`provisioned` ends the BLE exchange, not necessarily the user-visible setup flow. The device may intentionally close BLE while it joins Wi-Fi and completes cloud registration. The Android app must show a bounded cloud-registration wait state and retry the secure callable claim for up to 45 attempts at two-second intervals (90 seconds). A retry during that window must reuse the in-memory pairing proof and must not restart BLE scanning; the user may explicitly begin a new provisioning attempt only after that path is exhausted. The app must not use a direct RTDB ownership/readiness preflight as a substitute for the callable claim.

The BLE `reset` command is a scoped local reprovisioning operation, not a factory erase. Its first state-changing action is `setPump(false)`; it clears only local Wi-Fi credentials and enrollment material, then restarts into BLE provisioning. It retains safety latches, pump configuration, counters, cloud ownership, and immutable device identity.

#### Production diagnostics

Production firmware publishes the `/devices/{device_id}/diagnostics` snapshot with `freeHeap`, `wifiRSSI`, and `restartReason`. Its cloud event history contains only WARN/ERROR records. The trusted `retainDeviceEvents` backend trigger atomically retains the newest 50 push-ID-ordered records, including repair of oversized histories left by an older firmware version. Firmware application code logs through the transport-independent `AppLogger`/`LogSink` boundary; development-only sinks may expose bounded local history and live diagnostics on a trusted LAN, but the TCP implementation is compile-time gated and is not a production support interface.

**Status (ESP32 → cloud)**: `/devices/{device_id}/telemetry` and `/devices/{device_id}/shadow/reported`

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

**Control (cloud → ESP32)**: `/devices/{device_id}/shadow/desired`

- `mode`: AUTO | MANUAL | COUNTDOWN
- `manual_desired`: bool (persistent intent)
- `emergency_stop`: bool (one-shot)
- `reset_stop`: bool (one-shot)
- `clear_error`: bool (one-shot — clears DRY_RUN and OVERFLOW lockouts)
- `countdown_start`: bool (one-shot)
- `countdown_duration_min`: int (1–120)
- `bypass_level_sensor`: bool (persistent)
- `bypass_flow_sensor`: bool (persistent — added Phase 1)
- `reboot_request_id`: int (monotonic token)

**Configuration (cloud → ESP32)**: `/devices/{device_id}/settings`

- `pump_start_level_pct`: int (threshold to turn ON)
- `pump_stop_level_pct`: int (threshold to turn OFF)
- `dry_run_threshold_lpm`: float
- `max_pump_runtime_min`: int

### Hardware assumptions (deployment-critical)

- JSN‑SR04T **ECHO must be level shifted to 3.3V** on ESP8266.
- RS‑485 termination/biasing must be correct for cable length.
- Buck converter sizing and wiring must tolerate pump-induced transients; brownouts are expected and the system fails safe.

