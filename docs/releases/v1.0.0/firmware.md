# Firmware Documentation - SmartFlow v1.0.0

### Scope

This document describes the firmware subsystems:

- **ESP32 master controller firmware**
- **ESP8266 sensor node firmware**

It focuses on behavior, safety design, timing, RS‑485 protocol, and RTDB integration.

For build/flash wiring instructions, also see `firmware/README.md`.

---

## 1) ESP32 Master (controller)

### 1.1 Responsibilities

- Drives the pump relay output (contactor coil control).
- Polls remote sensor node over RS‑485 and validates incoming telemetry.
- Computes tank level percent from sensor-node `DIST` and master calibration.
- Enforces safety invariants:
  - Emergency stop latch
  - Dry-run lockout
  - Overflow protection
  - Sensor/comm freshness + stability gates (failsafe)
- Synchronizes with Firebase RTDB:
  - Pushes `/pump_system/status`
  - Reads `/pump_system/control`
  - Reads `/pump_system/config/device`
- Persists config and critical state to NVS.

### 1.2 Timing model (high level)

The firmware loop is designed to be **non-blocking**:

- **RS‑485 polling**: periodic request/response with strict timeouts.
- **Sensor freshness timeout**: if last validated RS‑485 frame is older than `LEVEL_STALE_TIMEOUT_MS`, `level_fresh=false`.
- **Stability latch**: `remote_sensor_stable=true` only after N consecutive validated frames.
- **Firebase cycle**: periodic push/read (default ~3s), with cooldown/backoff on repeated failures.
- **Device config read**: slower period (default ~30s).

### 1.3 Control modes (policy) and run modes (actual)

Policy mode: `/pump_system/control/mode`:

- `AUTO`
- `MANUAL`
- `COUNTDOWN`

MANUAL intent: `/pump_system/control/manual_desired`:

- `true`: request pump ON (safety still enforced)
- `false`: request pump OFF (mode remains MANUAL)

Emergency stop:

- `/pump_system/control/emergency_stop=true` (one-shot) → latches stop
- `/pump_system/control/reset_stop=true` (one-shot) → clears latch

Run mode (published status, actual state):

- `AUTO`
- `AUTO_STANDBY`
- `AUTO_COOLDOWN`
- `MANUAL_ON`
- `MANUAL_OFF`
- `MANUAL_COOLDOWN`
- `COUNTDOWN`

Emergency-stop status is represented separately via `emergency_stop_latched`.

### 1.4 Safety logic (priority order)

The system uses an explicit priority model (highest wins):

1. **Emergency stop latch**: pump forced OFF; cannot run until reset.
2. **Hard lockouts**:
   - Dry-run lockout: sustained low flow while running
   - Overflow protection: max runtime exceeded
   These stop the pump and remain latched until `clear_error`.
3. **Freshness/stability gating**:
   - If RS‑485 data is stale or link is unstable, pump start is blocked.
   - If the pump is running and data becomes stale/unstable, pump is stopped (failsafe) unless bypass is active.
4. **Maintenance bypass** (optional): `bypass_level_sensor=true` ignores level-based gating. Dry-run and overflow lockouts remain active.
5. **Mode behavior**:
   - AUTO: hysteresis on level
   - MANUAL: obey `manual_desired` with safety + (optional) full-tank stop when level is valid
   - COUNTDOWN: timed run started only by `countdown_start`

### 1.5 RS‑485 master behavior (safety-critical)

Electrical assumptions:

- Half-duplex RS‑485, DE/RE direction controlled by a GPIO.
- Proper termination/biasing and a reliable reference ground are required.

Protocol:

- Request: master sends `REQ\n`
- Response: framed payload with CRC16 and SEQ:

```text
STX (0x02)
LVL:<percent>;DIST:<cm>;FLOW:<lpm>;ERR:<code>;SEQ:<n>;CRC:<hex>
ETX (0x03)
```

Master acceptance:

- Requires correct STX/ETX framing
- Requires CRC16 (Modbus) match
- Applies strict parsing and range checks
- Prefers `DIST` and derives level % using configured `tank_empty_cm` / `tank_full_cm`
- Tracks SEQ for duplicate detection

On comm faults:

- Invalid frames are rejected without updating freshness timers.
- The system maintains the last known good values, but will enforce freshness/stability gating for actuation.

---

## 2) ESP8266 Sensor Node (tank)

### 2.1 Responsibilities

- Reads ultrasonic sensor (JSN‑SR04T) near the tank with non-blocking acquisition and filtering.
- Reads flow sensor (YF‑G1) using an ISR pulse counter.
- Responds to RS‑485 requests with framed telemetry including:
  - Raw distance `DIST` (cm)
  - Flow `FLOW` (L/min)
  - Error bitfield `ERR`
  - Sequence number `SEQ`
  - CRC16 `CRC`

### 2.2 Flow calculation (conceptual)

- Count pulses over a measured time window \(dt\).
- Convert to frequency: \(Hz = pulses / dt_s\)
- Convert to flow: \(L/min = Hz / FLOW\_HZ\_PER\_LPM\)

This time-aware method reduces sensitivity to loop jitter and window drift.

### 2.3 Ultrasonic measurement (conceptual)

- Uses a non-blocking state machine (trigger → wait-for-echo → timeout).
- Collects multiple samples, rejects implausible values, uses a median for robustness.
- Publishes the last good median distance as `DIST`.

### 2.4 Error reporting (`ERR`)

`ERR` is a bitfield:

- bit0: ultrasonic error
- bit1: flow signal error

The master uses this to drive safety behavior and user-facing diagnostics.

