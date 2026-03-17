# Firmware Technical Documentation · v2.0

**Smart Water Pump Controller — ESP32 firmware**

This document describes the firmware subsystem in detail: purpose, architecture, hardware interface, software design, and operational behavior. It is intended for developers, integrators, and maintainers. For the Firebase RTDB contract (status/control/config schemas), see [firmware-rtdb-spec.md](./firmware-rtdb-spec.md). For build and flash instructions, see `firmware/README.md`.

---

## 1. Scope and references

### 1.1 Scope

This documentation covers:

- Role of the firmware within the overall Smart Water Pump Controller system
- Software architecture (modules, scheduling, state)
- Hardware interface (GPIO, sensors, relay)
- Safety and protection logic
- Configuration, calibration, and offline behavior
- Integration with Firebase Realtime Database

It does not replace the RTDB spec (data shapes and write ownership) or the deployment guide ([deploy.md](./deploy.md)).

### 1.2 References

| Document | Description |
|----------|-------------|
| [firmware-rtdb-spec.md](./firmware-rtdb-spec.md) | RTDB layout, control/status/config schemas, modes |
| [dashboard-ux-spec.md](./dashboard-ux-spec.md) | Dashboard behavior that consumes status and drives control |
| `firmware/README.md` | Build environment, pinout, calibration, troubleshooting |
| `docs/operations/RS485_TANK_LINK.md` | RS-485 tank node protocol when level is read remotely |

---

## 2. System context

The Smart Water Pump Controller is a three-layer system:

```
[ESP32 Firmware]  ←──read──  [Firebase RTDB]  ←──write──  [Next.js Dashboard]
[ESP32 Firmware]  ──write──► [Firebase RTDB]  ──read──►   [Next.js Dashboard]
```

- **Firmware** runs on an ESP32 DevKit. It reads tank level (ultrasonic or RS-485), flow rate (hall-effect), runs the pump state machine and safety logic, and syncs status to Firebase every few seconds. It reads control and device config from Firebase; it does not communicate directly with the dashboard.
- **Dashboard** is a Next.js PWA. Operators sign in with Google, view live status, and issue commands. All commands go through Firebase.
- **Firebase RTDB** is the only channel between firmware and dashboard. Write ownership is strict: firmware writes only `/pump_system/status`; dashboard writes `/pump_system/control/*` and `/pump_system/config/device`.

---

## 3. Architecture overview

### 3.1 Design principles

- **Non-blocking:** No long blocking delays in `loop()`. Sensor reads, timing, and Firebase I/O are driven by millis-based intervals and state.
- **Safety-first:** Dry-run, overflow, and sensor-failure protections can force the pump off regardless of mode. Error acknowledgment is explicit (dashboard sets `clear_error`).
- **Offline-capable:** When WiFi is down, the device runs from last-known config (NVS) and continues to enforce safety. Firebase sync resumes when connectivity returns.
- **Single loop:** One `loop()` with no RTOS tasks; shared state is updated in a deterministic order to avoid race conditions.

### 3.2 Module layout

The firmware is organized into numbered tabs (Arduino concatenates alphabetically); the main sketch file is opened in the IDE so that `setup()` and `loop()` are in one place. Logical modules:

| Module | File(s) | Responsibility |
|--------|---------|----------------|
| **Config & globals** | `01_config.ino`, `smart_water_pump_controller_shared.h` | Includes, GPIO constants, calibration defaults, global variables, NVS namespaces |
| **Sensors** | `02_sensors.ino` | Ultrasonic level (median + EMA), flow interrupt and rate calculation, RS-485 parsing (when used), telemetry counters |
| **Safety & pump** | `03_safety_pump.ino` | Dry-run timer, overflow timer, sensor-failure detection, pump state machine (AUTO/FORCE_ON/FORCE_OFF/COUNTDOWN/MANUAL), relay output |
| **Persistence** | `04_persistence.ino` | NVS read/write for device config and state (e.g. last reboot request ID, pump cycles, runtime), crash-loop detection, safe mode |
| **Connectivity** | `05_connectivity_cloud.ino` | WiFi connect with backoff, Firebase init, NTP, status push, control/config read, token and cooldown handling |

Shared definitions (pins, timeouts, defaults) live in `smart_water_pump_controller_shared.h` so all tabs see the same constants and externs.

### 3.3 Execution flow

- **setup():** GPIO init (relay off), flow interrupt attach, NVS load, WiFi connect, Firebase auth, first config read from RTDB or NVS, then enter main loop.
- **loop():** On each iteration:
  1. Feed watchdog.
  2. Sensor cycle (every 1 s normally; idle/sleep intervals when applicable): read level and flow, update telemetry.
  3. Safety and pump logic: evaluate dry-run, overflow, sensor failure; apply mode and run state; set relay.
  4. Persistence: periodic NVS writes for config/state (rate-limited to reduce wear).
  5. Firebase cycle (every 3 s normally; idle interval when applicable): push status, read control and (every 30 s) device config; apply one-shots (clear_error, reboot, manual_start/stop, countdown_add_time).
  6. Sleep window: if scheduled sleep is enabled and within the sleep window, and level is above emergency threshold, enter light sleep and wake on timer.

---

## 4. Hardware interface

### 4.1 Pin mapping

| GPIO | Direction | Function | Notes |
|------|-----------|----------|--------|
| 4 | OUTPUT | Relay control | Active LOW — LOW = pump ON. Drives 5 V relay module. |
| 5 | OUTPUT | JSN-SR04T TRIG | 10 µs pulse to trigger ranging. |
| 18 | INPUT | JSN-SR04T ECHO | Via 1 kΩ / 2 kΩ voltage divider (5 V → 3.3 V). |
| 34 | INPUT | YF-G1 flow signal | Input-only; no internal pull. Via 1 kΩ / 2 kΩ divider. RISING edge interrupt for pulse count. |

GPIO 34 is input-only on the ESP32 and is used only for the flow interrupt.

### 4.2 Sensors

- **Tank level:** JSN-SR04T ultrasonic distance sensor. Measures distance to water surface; firmware converts to 0–100% using `tank_empty_cm` and `tank_full_cm`. Optional: level is provided by a remote RS-485 tank node (see `docs/operations/RS485_TANK_LINK.md`); main ESP32 then parses `LVL:<pct>;ERR:<flag>` and does not drive TRIG/ECHO locally.
- **Flow rate:** YF-G1 hall-effect flow sensor. Pulse frequency is converted to L/min using a configurable calibration factor (default 7.5). Used for dry-run detection (low flow while pump is on) and for flow-based level estimate when level sensor is failed/bypassed.

### 4.3 Relay and external safety

- The firmware drives a 5 V relay that controls the contactor coil. **TOR (thermal overload relay)** is in series in hardware; firmware does not read or control it. Manual bypass switch can energize the contactor regardless of firmware; software protections (dry-run, level cutoff) are bypassed in that case; TOR remains active.

---

## 5. Software design

### 5.1 Timing constants (defaults)

| Constant | Default | Purpose |
|----------|---------|---------|
| Sensor interval | 1000 ms | Period between level/flow reads. |
| Firebase interval | 3000 ms | Period between status push and control read. |
| Device config interval | 30000 ms | Period between config node read. |
| Dry-run timeout | 30 s | Low-flow duration before lockout. |
| Max pump runtime (AUTO) | 120 min | Overflow protection cutoff. |
| Level sensor failure threshold | 5 | Consecutive ultrasonic failures before `is_level_sensor_error`. |
| Flow stuck threshold | 5 s | Pump off + flow > 2 LPM before `is_flow_sensor_error`. |
| Watchdog timeout | 120 s | Reset if loop is blocked (e.g. network stall). |
| Crash-loop window | 5 reboots in 5 min | Enters safe mode (no pump, no Firebase) for 1 hour. |

Idle and sleep intervals (slower sensor and Firebase when tank is full and pump off) are configurable via device config.

### 5.2 State and mode hierarchy

- **Policy mode** (`control/mode`): `AUTO`, `FORCE_OFF`, `FORCE_ON`, `COUNTDOWN`. Read from Firebase every Firebase cycle.
- **Run mode** (internal / status): `OFF`, `AUTO_STANDBY`, `AUTO`, `MANUAL`, `COUNTDOWN`. Reflects actual run state; MANUAL and COUNTDOWN are triggered by one-shots and/or countdown duration.
- **Priority:** Hard safety (dry-run, overflow, sensor failure) can force pump off. Then bypass (maintenance/auto). Then manual/countdown policy. Then AUTO hysteresis (start at ≤ pump_start_level, stop at ≥ pump_stop_level).

### 5.3 Safety logic summary

| Protection | Trigger | Action | Reset |
|------------|---------|--------|--------|
| Dry-run | Flow &lt; threshold for timeout while pump on | Relay off, `is_error = true` | Dashboard sets `clear_error = true` |
| Overflow | AUTO runtime &gt; max_pump_runtime_min | Relay off, `is_overflow_error = true` | `clear_error` or normal stop |
| Level sensor failure | Consecutive ultrasonic timeouts ≥ threshold | Pump off in AUTO, `is_level_sensor_error = true` | Auto when valid readings resume; optional auto-bypass |
| Flow sensor failure | Pump off and flow &gt; 2 LPM for 5 s | `is_flow_sensor_error = true` | Auto when flow returns to normal |
| Crash loop | 5+ reboots in 5 min | Safe mode: no pump, no Firebase for 1 h | Power cycle |

---

## 6. Configuration and calibration

### 6.1 Source of truth

- **Online:** Device config is read from `/pump_system/config/device` every 30 s. Values are applied in memory and written to NVS for persistence.
- **Offline:** Config is loaded from NVS at boot. If NVS is empty, compiled defaults from the shared header are used.

Calibration and thresholds (tank_empty_cm, tank_full_cm, pump_start_level, pump_stop_level, dry_run_*, flow_calibration_factor, max_pump_runtime_min, sleep_*, level_sensor_failure_threshold, idle_*, auto_bypass_*) are all configurable via the dashboard Device config (gear icon); no reflash needed for changes.

### 6.2 Tank calibration

- **tank_empty_cm:** Distance from sensor to tank bottom (0%).
- **tank_full_cm:** Distance from sensor to water surface when full (100%).
- Level % = linear interpolation between these two distances using the current median distance. Sensor measures distance; smaller distance = higher level.

### 6.3 Flow calibration

- YF-G1: flow (L/min) = pulse frequency (Hz) / calibration factor. Default factor 7.5; tunable via device config. Verify with bucket-and-stopwatch test.

---

## 7. Firebase integration

- **Status:** Single JSON object pushed to `/pump_system/status` on each Firebase cycle. Contains level, flow, run state, errors, telemetry, uptime, etc. Schema is defined in [firmware-rtdb-spec.md](./firmware-rtdb-spec.md).
- **Control:** Firmware reads `/pump_system/control` each cycle and applies mode, clear_error, reboot_request_id, manual_start, manual_stop, countdown_duration_min, countdown_add_time, bypass_level_sensor. One-shots are consumed (e.g. clear_error and countdown_add_time reset by firmware after use).
- **Config:** Read from `/pump_system/config/device` every 30 s; results merged into in-memory config and saved to NVS.

Token and connection failures trigger a cooldown period during which Firebase is not called, to avoid tight auth loops. WiFi reconnection is handled by the Firebase library and/or explicit reconnect logic.

---

## 8. Offline and resilience

- **WiFi down:** Sensors and pump logic continue. Config from NVS. Firebase push/read skipped with log message. When WiFi returns, sync resumes.
- **NVS:** Used for device config cache, last reboot request ID, pump cycles/runtime, and any state that must survive reboot. Writes are rate-limited to reduce flash wear.
- **Watchdog:** Long stalls in loop (e.g. blocking network) can trigger reset; crash-loop detection then may force safe mode.
- **RS-485 tank node:** If level is supplied over RS-485, loss of frames is reflected in telemetry and can lead to level_sensor_error and optional auto-bypass after delay.

---

## 9. Build variants

- **Arduino:** Sketch lives in a folder named `smart_pump_controller/` (or `arduino_smart_water_pump_controller/`); open the `.ino` file. All `.ino` tabs are concatenated alphabetically (hence `01_...` prefixes). Use Arduino IDE 2.x with ESP32 board support and libraries per `firmware/README.md`.
- **PlatformIO:** Project under `platformio_smart_water_pump_controller/` with `platformio.ini` and `src/`; shared header in `include/`. Build with `pio run`; same logic as Arduino build.

For detailed build steps, board settings, and troubleshooting, see `firmware/README.md`. For end-to-end deployment, see [deploy.md](./deploy.md).

