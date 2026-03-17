# Firmware & RTDB Spec · v2.0

Authoritative reference for how the ESP32 firmware and the Next.js dashboard
communicate via Firebase Realtime Database for version **2.0** of the system.

For additional background and audit notes, see the original design document
`docs/FIRMWARE_DASHBOARD_DESIGN_v2.md`.

---

## 1. Overview

**Three-layer architecture:**

```text
[ESP32 Firmware]  ←──read──  [Firebase RTDB]  ←──write──  [Next.js Dashboard]
[ESP32 Firmware]  ──write──►  [Firebase RTDB]  ──read──►   [Next.js Dashboard]
```

- **Firmware** (ESP32 / Arduino): reads sensors, runs the pump state machine,
  and syncs with Firebase every ~3 seconds.
- **Dashboard** (Next.js 14 PWA): operators sign in with Google, monitor status,
  and issue control commands.
- **Bridge** (Firebase RTDB): only communication channel between firmware and
  dashboard. Dashboard never talks directly to the ESP32.

**Write ownership:**

| Actor     | Writes to                             | Reads from                                  |
|----------|----------------------------------------|---------------------------------------------|
| Dashboard| `/control/*`, `/config/device`, `/audit/events` | `/status`, `/config/device`, `/config/admins` |
| Firmware | `/status`                              | `/control/*`, `/config/device`              |

The firmware writes to `/control/mode` only to revert it to `"AUTO"` when a
countdown run completes. All other control writes are one‑directional
dashboard → RTDB → firmware.

---

## 2. RTDB Layout

```text
/pump_system/
├── control/                          # Dashboard → ESP32 (polled every ~3s)
│   ├── mode                          # string: "AUTO"|"FORCE_OFF"|"FORCE_ON"|"COUNTDOWN"
│   ├── clear_error                   # boolean one-shot: clears dry-run + overflow lockouts
│   ├── reboot_request_id             # number: new value triggers ESP32 soft restart
│   ├── manual_start                  # boolean one-shot: starts a Manual run
│   ├── manual_stop                   # boolean one-shot: stops Manual run, reverts to AUTO
│   ├── countdown_duration_min        # number 1–120: duration when entering COUNTDOWN
│   ├── countdown_add_time            # boolean one-shot: adds 5 min to running countdown
│   └── bypass_level_sensor           # boolean: admin maintenance toggle
│
├── status/                           # ESP32 → Dashboard (single JSON push ~every 3s)
│   └── { see Status schema below }
│
├── config/
│   ├── device/                       # Shared calibration & thresholds
│   │   └── { see Device config schema below }
│   ├── admins/                       # { uid: true } — admin access list
│   └── notifications_by_user/{uid}/  # Per-user notification preferences
│       ├── on_dry_run_error          # boolean
│       ├── on_tank_full              # boolean
│       └── on_controller_offline     # boolean
│
├── audit/
│   └── events/                       # Append-only log; dashboard writes, dashboard reads
│       └── {push_id}/
│           ├── ts                    # number: Unix timestamp ms
│           ├── uid                   # string: operator uid
│           ├── action                # string: e.g. "mode_change", "manual_start"
│           └── detail                # string: human-readable description
│
└── presence/                         # removed in v2 (unused in implementation)
```

`presence/` is intentionally unused in firmware and dashboard. If real-time
presence is needed in future, it should be designed explicitly and documented
in a later versioned spec.

---

## 3. Control Path (Dashboard → RTDB → ESP32)

The dashboard writes to `/pump_system/control/*`. The firmware polls this node
on each Firebase cycle (~3 seconds) in `readFirebaseControl()`.

### 3.1 Control keys

| Key                     | Type      | Purpose |
|-------------------------|-----------|---------|
| `mode`                  | `string`  | Policy mode: `"AUTO"` \| `"FORCE_OFF"` \| `"FORCE_ON"` \| `"COUNTDOWN"`. |
| `clear_error`           | `boolean` | **One‑shot.** Clears dry‑run and overflow lockouts when `true`. Firmware resets it to `false` after applying. |
| `reboot_request_id`     | `number`  | Non‑zero value triggers ESP32 soft restart. Firmware persists last processed ID in NVS to avoid restart loops. |
| `manual_start`          | `boolean` | **One‑shot.** Starts a Manual run (`FORCE_ON` + `run_mode = "MANUAL"`). Firmware does not reset this flag; dashboard resets it. |
| `manual_stop`           | `boolean` | **One‑shot.** Stops a Manual run and **reverts `mode` to `"AUTO"`**. Firmware does not reset this flag; dashboard resets it. |
| `countdown_duration_min`| `number`  | Duration (1–120 min) for the next COUNTDOWN run. Set before writing `mode = "COUNTDOWN"`. |
| `countdown_add_time`    | `boolean` | **One‑shot.** Adds 5 minutes to the active countdown. Firmware resets it to `false` after applying. |
| `bypass_level_sensor`   | `boolean` | Admin maintenance toggle. Persists until explicitly cleared. |

### 3.2 One‑shot timing

- `manual_start` and `manual_stop` are **dashboard‑reset one‑shots**:
  - Dashboard writes `true`, disables the triggering button for **5 seconds**,
    then writes `false`.
  - This gives the firmware at least two poll cycles to observe the edge.
- `countdown_add_time` is a **firmware‑reset one‑shot**:
  - Dashboard writes `true` and shows a busy state.
  - Firmware processes the add‑time, then sets the flag back to `false`.
  - Dashboard clears the busy state once it sees `false` on the control path.

The dashboard must not allow a user to trigger the same one‑shot again until
the first command has been observed and confirmed (either by time or status).

---

## 4. Status Path (ESP32 → RTDB → Dashboard)

The firmware pushes a single JSON object to `/pump_system/status` on each
status cycle. The dashboard subscribes with `onValue` and derives a snapshot
plus a local `updatedAt` timestamp.

**Controller online detection:** If no status update is received within
**20 seconds**, the dashboard treats the controller as offline and disables
control buttons.

### 4.1 Status schema

| Field                    | Type      | Description |
|--------------------------|-----------|-------------|
| `water_level_percent`    | `number`  | 0–100. From ultrasonic sensor, or flow‑based estimate when bypass is active and sensor has failed. |
| `estimated_level_pct`    | `number`  | Flow‑based level estimate. `-1` if not yet initialized. Shown separately when `level_estimate_active` is true. |
| `level_estimate_active`  | `boolean` | `true` when flow‑based estimate is being used (bypass ON + sensor failed). Dashboard labels level as “Estimated”. |
| `flow_volume_added_l`    | `number`  | Litres added since last level sensor anchor. For diagnostics. |
| `level_last_valid_age_sec` | `number` | Seconds since last successful ultrasonic reading. Dashboard shows “Level Xs old” when this grows large. |
| `level_sensor_health_pct` | `number` | 0–100 sensor health score. Dashboard shows a sensor health indicator. |
| `is_running`             | `boolean` | Pump relay energized. |
| `run_mode`               | `string`  | `"OFF"` \| `"AUTO_STANDBY"` \| `"AUTO"` \| `"MANUAL"` \| `"COUNTDOWN"`. |
| `flow_rate_lpm`          | `number`  | Litres per minute from YF‑G1. |
| `countdown_remaining_sec`| `number`  | Seconds remaining in active countdown; `0` otherwise. |
| `is_error`               | `boolean` | Dry‑run lockout active. Pump will not run until cleared. |
| `is_level_sensor_error`  | `boolean` | Ultrasonic level sensor failure (consecutive timeouts). |
| `is_flow_sensor_error`   | `boolean` | Flow sensor stuck‑high (flow when pump is off). |
| `is_overflow_error`      | `boolean` | Max AUTO runtime exceeded. |
| `bypass_level_sensor`    | `boolean` | Level sensor bypass (maintenance) active. |
| `auto_bypass_active`     | `boolean` | Firmware auto‑engaged bypass due to sensor failure. |
| `is_sleeping`            | `boolean` | Within scheduled sleep window; AUTO start suppressed. |
| `last_fault_code`        | `string`  | See Fault code table below. |
| `last_fault_message`     | `string`  | Human‑readable description of the last fault. |
| `total_pump_cycles`      | `number`  | Lifetime pump start count. |
| `total_pump_run_min`     | `number`  | Lifetime pump runtime in minutes. |
| `wifi_rssi`              | `number`  | Wi‑Fi RSSI in dBm. |
| `uptime_minutes`         | `number`  | Minutes since last ESP32 boot. |
| `last_boot_reason`       | `string`  | Human‑readable boot reason (“Power‑on”, “Watchdog”, etc.). |

### 4.2 Fault codes

| `last_fault_code` | Meaning                                       | User message                               | Recovery                                      |
|-------------------|-----------------------------------------------|--------------------------------------------|-----------------------------------------------|
| `"DRY_RUN"`       | Low flow for N seconds while pump running     | “Dry‑run detected. Check water supply.”    | Fix water supply then tap **Clear Error**.    |
| `"OVERFLOW"`      | Max AUTO runtime exceeded                     | “Max runtime exceeded. Check tank sensor.” | Inspect tank/sensor then tap **Clear Error**. |
| `"LEVEL_SENSOR"`  | Consecutive ultrasonic timeouts ≥ threshold   | “Level sensor offline.”                    | Auto‑clears on recovery; enable bypass for interim. |
| `"FLOW_SENSOR"`   | Flow sensor stuck‑high                        | “Flow sensor reading abnormal.”            | Auto‑clears on recovery.                      |
| `"SAFE_MODE"`     | Crash loop detected (5+ reboots in 5 minutes) | “Controller in safe mode. Power cycle.”    | Full power cycle of controller.               |
| `""` (empty)     | No fault                                      | —                                          | —                                             |

---

## 5. Shared Device Config

Path: `/pump_system/config/device`  
Written by: dashboard (admins only)  
Read by: firmware (every ~30 seconds) and dashboard (for settings UI).

### 5.1 Fields

| Field                       | Type      | Description |
|----------------------------|-----------|-------------|
| `tank_empty_cm`            | `number`  | Distance (cm) from sensor to tank bottom (0% reference). |
| `tank_full_cm`             | `number`  | Distance (cm) from sensor to water when tank is full (100% reference). |
| `pump_start_level`         | `number`  | AUTO starts pump when level ≤ this % (default 30). |
| `pump_stop_level`          | `number`  | AUTO stops pump when level ≥ this % (default 100). |
| `dry_run_threshold_lpm`    | `number`  | Flow below this is considered “no flow” (default 0.5 L/min). |
| `dry_run_timeout_sec`      | `number`  | Seconds of low flow before dry‑run lockout triggers (default 30). |
| `flow_calibration_factor`  | `number`  | YF‑G1 K‑factor (default 7.5). |
| `max_pump_runtime_min`     | `number`  | Max continuous AUTO runtime before overflow protection triggers (default 120). |
| `sleep_enabled`            | `boolean` | Enable scheduled sleep window. |
| `sleep_start_hour`         | `number`  | Hour (0–23, local time) to begin sleep window. |
| `sleep_end_hour`           | `number`  | Hour (0–23) to end sleep window. |
| `sleep_emergency_level`    | `number`  | % below which sleep is overridden to prevent running out of water. |
| `level_sensor_failure_threshold` | `number` | Consecutive ultrasonic failures before `is_level_sensor_error` is set. |
| `idle_sensor_interval_ms`  | `number`  | Sensor poll interval during idle (tank full, pump off). |
| `idle_firebase_interval_ms`| `number`  | Firebase push interval during idle. |
| `auto_bypass_on_sensor_fail` | `boolean` | Enable auto‑bypass when level sensor fails. |
| `auto_bypass_delay_sec`    | `number`  | Delay (seconds) before auto‑bypass engages. |

The firmware accepts both `level_sensor_failure_threshold` and the legacy
`sensor_failure_threshold`, but new writes should prefer the explicit
`level_sensor_failure_threshold` key.

---

## 6. Modes and Run Types (Reference)

This section is a brief reference for how RTDB control and status fields map
to user‑visible run types. The full UX behavior is documented in
`docs/releases/v2.0/dashboard-ux-spec.md`.

- **Policy mode** (`control/mode`): `"AUTO"`, `"FORCE_OFF"`, `"FORCE_ON"`, `"COUNTDOWN"`.
- **Run mode** (`status/run_mode`): `"OFF"`, `"AUTO_STANDBY"`, `"AUTO"`, `"MANUAL"`, `"COUNTDOWN"`.

Manual run and Emergency Override both use `"FORCE_ON"` at the firmware
  level; the dashboard distinguishes them by which UI triggered the mode.

The hierarchical priority model (`Hard safety → Bypass → Manual → Countdown → AUTO`)
is implemented in firmware and documented in more detail in the v2 design
and v3 firmware spec.

