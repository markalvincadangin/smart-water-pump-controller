## Firmware Specification (Current)

This document is the **current** (non-versioned) firmware specification for the Smart Water Pump Controller system.

It supersedes the older archived specs and release notes. Historical documents remain under `docs/archive/` (including `docs/archive/releases/`).

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
LVL:<percent>;DIST:<cm>;FLOW:<lpm>;ERR:<code>;SEQ:<n>;CRC:<hex>
ETX (0x03)
```

**Field semantics**

- **`DIST`**: last good median ultrasonic distance (cm). This is the preferred primary measurement.
- **`LVL`**: water level percent (0–100). Provided for backward compatibility; master will prefer `DIST` when present to avoid calibration drift.
- **`FLOW`**: liters per minute (L/min).
- **`ERR` bitfield**:
  - bit0 (1): ultrasonic error (timeout/invalid)
  - bit1 (2): flow signal error (noise/floating heuristics)
- **`SEQ`**: monotonically increasing per response (uint8 on node; parsed as uint32 on master).
- **`CRC`**: CRC16 Modbus computed over the payload up to and including the trailing `;` after `SEQ`.

**Master acceptance rules (safety critical)**

- Reject frames with invalid framing, CRC mismatch, or out-of-range values.
- Maintain:
  - **freshness** (`levelLastUpdateMs`) updated only on validated frames
  - **stability latch** (`remoteSensorStable`) requiring N consecutive valid frames
- Fail-safe gating:
  - If data is stale or the link is unstable, the pump is **blocked from starting** and a running pump is **stopped** (unless in maintenance bypass).

### Control modes (vNext, current)

Policy mode is stored in `pumpMode` (RTDB: `/pump_system/control/mode`):

- **AUTO**: level-based hysteresis control.
- **MANUAL**: operator policy mode with persistent intent `manual_desired` (RTDB: `/pump_system/control/manual_desired`).
  - `manual_desired=true` requests pump ON (all safety still enforced).
  - `manual_desired=false` keeps pump OFF (mode stays MANUAL).
- **COUNTDOWN**: timed run.
  - Start is explicit via one-shot `/pump_system/control/countdown_start=true`.
  - Duration via `/pump_system/control/countdown_duration_min`.

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

Required core fields:

- `water_level_percent` (0–100)
- `flow_rate_lpm` (L/min)
- `is_running` (bool)
- `run_mode` (OFF/AUTO/AUTO_STANDBY/MANUAL_ON/MANUAL_OFF/COUNTDOWN/STOPPED)
- `last_fault_code` / `last_fault_message` (strings, when faulted)
- `manual_desired` (bool)
- `emergency_stop_latched` (bool)
- `remote_sensor_stable` (bool)
- `level_fresh` (bool)

**Control (cloud → ESP32)**: `/pump_system/control`

- `mode`: AUTO | MANUAL | COUNTDOWN
- `manual_desired`: bool (persistent intent)
- `emergency_stop`: bool (one-shot)
- `reset_stop`: bool (one-shot)
- `countdown_start`: bool (one-shot)
- `countdown_duration_min`: int (1–120)
- `clear_error`: bool (acknowledge/clear lockouts)
- `reboot_request_id`: int (monotonic request token)

### Hardware assumptions (deployment-critical)

- JSN‑SR04T **ECHO must be level shifted to 3.3V** on ESP8266.
- RS‑485 termination/biasing must be correct for cable length.
- Buck converter sizing and wiring must tolerate pump-induced transients; brownouts are expected and the system fails safe.

