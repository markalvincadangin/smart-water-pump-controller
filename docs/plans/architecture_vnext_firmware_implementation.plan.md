## Firmware implementation plan — Architecture vNext

Targets:
- **ESP32 master**: `firmware/master_node/src/*` and `firmware/arduino_smart_water_pump_controller/*`
- **ESP8266 sensor node**: unchanged behavior (still provides LVL/FLOW/ERR over RS-485), only referenced for end-to-end tests

Source of truth:
- `docs/specs/firmware.md` and `docs/operations/firmware_config_from_database.md`
- `docs/operations/rs485_tank_link.md` for transport behavior and link reliability constraints

---

## 0) Current alignment scan (baseline)

### Current firmware control contract (observed)
- **Mode values accepted in firmware**: `"AUTO"`, `"MANUAL"`, `"COUNTDOWN"`, `"FORCE_ON"`, `"FORCE_OFF"` (control-mode branch in `connectivity/connectivity_cloud.cpp`)
- **Manual controls**: `manual_start` (one-shot), `manual_stop` (one-shot)
- **Countdown controls**: `countdown_duration_min`, `countdown_add_time`, `countdown_add_min`
- **Safety controls**: `clear_error`, `bypass_level_sensor`
- **Reboot**: `reboot_request_id`

### Required vNext firmware contract (from vNext doc)
- Only policy modes: `"AUTO"`, `"MANUAL"`, `"COUNTDOWN"`
- Replace “manual_start/stop” with **persistent intent**: `manual_desired` (true/false)
- Add **Emergency Stop**: `emergency_stop` (one-shot) and `reset_stop` (one-shot)
- Remove FORCE_ON/FORCE_OFF semantics entirely

### Compatibility gap summary
- Firmware currently relies on one-shot edges (`manual_start`, `manual_stop`) and supports FORCE modes.
- vNext requires **persistent intent** and a **latched stop** concept.

---

## 1) Design decisions to lock (must decide before coding)

1. **COUNTDOWN completion landing policy**
   - Option A (recommended): when countdown expires, `pumpMode = "AUTO"` and `manual_desired` remains unchanged.
   - Option B: when countdown expires, return to `pumpMode = "MANUAL"` with `manual_desired = false` (“MANUAL_OFF”).

2. **Emergency stop latch persistence**
   - Option A (recommended): latch in RAM + publish in status; clears on reboot (simple and predictable).
   - Option B: persist latch in NVS (more conservative after brownout).

3. **Backwards compatibility window**
   - Option A (recommended): keep reading old keys for one release, map safely to new semantics, then remove.

---

## 2) Data model changes (ESP32 master)

### 2.1 New/changed control keys (RTDB)
Add to `/pump_system/control`:
- `manual_desired: bool` (default false)
- `countdown_start: bool` (one-shot) **or** interpret `mode="COUNTDOWN"` as start (pick one; vNext doc prefers explicit)
- `emergency_stop: bool` (one-shot)
- `reset_stop: bool` (one-shot)

Deprecate (read for backward compatibility only):
- `manual_start`
- `manual_stop`
- `mode = FORCE_ON/FORCE_OFF` (map to safe behavior)

### 2.2 New status keys
Add to `/pump_system/status`:
- `emergency_stop_latched: bool`
- `manual_desired: bool` (recommended for UI truth)
- `remote_sensor_stable: bool` (recommended; helps UI explain blocked starts)
- `level_fresh: bool` (recommended; helps UI explain blocked starts)

### 2.3 Internal firmware state variables
Add to state module:
- `bool emergencyStopLatched`
- `bool manualDesired`

Update `runMode` strings:
- add `"MANUAL_ON"`, `"MANUAL_OFF"`, `"STOPPED"`

---

## 3) Control-plane implementation (Firebase → firmware)

### 3.1 Update Firebase control parser (`readFirebaseControl`)
Implementation steps:
- Read `control.mode`
  - Accept only `AUTO`, `MANUAL`, `COUNTDOWN`
  - If `FORCE_ON/FORCE_OFF` received: map to `AUTO` and optionally write back `"AUTO"` to converge dashboards
- Read `control.manual_desired` and apply to `manualDesired`
- Read `control.emergency_stop` (one-shot)
  - If true: set `emergencyStopLatched=true`, stop pump, write `emergency_stop=false`
- Read `control.reset_stop` (one-shot)
  - If true: clear `emergencyStopLatched` only if no hard lockouts active; write `reset_stop=false`
- COUNTDOWN start:
  - If `countdown_start` exists: use rising edge to start countdown and clear `countdown_start`
  - Else: keep current behavior (COUNTDOWN “arms” when mode enters) but document/standardize one approach for dashboards

### 3.2 Backward compatibility adapters (one release)
Map old keys:
- `manual_start=true` → set `pumpMode="MANUAL"` and `manualDesired=true`, then clear `manual_start`
- `manual_stop=true` → set `manualDesired=false`, then clear `manual_stop`

---

## 4) Pump state machine changes (ESP32 master)

### 4.1 Priority order enforcement
Ensure pump logic checks in this order:
1. **Emergency stop latched** → pump OFF, runMode `"STOPPED"` (highest)
2. **Hard safety lockouts** (dry-run/overflow) → pump OFF
3. **Sensor validity gates** (stable + fresh level when bypass is OFF) → stop/block
4. **MANUAL intent** (manualDesired true/false)
5. **COUNTDOWN** (active timer)
6. **AUTO** hysteresis

### 4.2 Behavior definitions
- In `MANUAL`:
  - `manualDesired=false` means pump OFF (mode remains MANUAL)
  - `manualDesired=true` requests pump ON (still gated by safety + freshness)
- In `COUNTDOWN`:
  - When active, pump ON (gated)
  - When expired, follow decision from §1 (Option A recommended)
- In `AUTO`:
  - unchanged hysteresis control (still gated by safety + freshness)

---

## 5) Status publishing changes (ESP32 master)

In `pushFirebaseStatus()` add:
- `manual_desired`
- `emergency_stop_latched`
- `remote_sensor_stable`
- `level_fresh`

Also adjust:
- `run_mode` semantics to new strings (`MANUAL_ON`, `MANUAL_OFF`, `STOPPED`)

---

## 6) Remove FORCE modes in firmware

Steps:
- Remove acceptance of `"FORCE_ON"` and `"FORCE_OFF"` in control mode parsing
- Remove any special-case logic that references FORCE modes
- Keep backward-compat mapping for one release (recommended)

---

## 7) Arduino + PlatformIO parity

For each change above:
- Apply to PlatformIO C++ modules **and** Arduino `.ino` modules in parallel:
  - `connectivity_cloud.cpp` ↔ `05_connectivity_cloud.ino`
  - `safety_pump.cpp` ↔ `03_safety_pump.ino`
  - `state/*` ↔ `01_config.ino` + shared header externs

---

## 8) Validation checklist (firmware)

### 8.1 Unit-level validation (serial logs)
- Emergency stop pressed while running → pump stops immediately and latches STOPPED
- reset_stop clears latch only when safe (no hard lockout)
- manual_desired toggles pump state only in MANUAL
- FORCE_ON/FORCE_OFF values written by old dashboard → safely mapped to AUTO

### 8.2 Failure-mode validation
- RS-485 unplugged: pump cannot start; if running, stops within stale timeout
- Sensor node reboot loop: pump start blocked until stable latch is true

### 8.3 Build validation
- `pio run` for ESP32 master and ESP8266 node

