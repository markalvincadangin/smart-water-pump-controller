# Firebase RTDB Contract - SmartFlow v1.0.0

### Purpose

This document defines the **exact Firebase Realtime Database schema and write ownership** used by:

- ESP32 firmware (master controller)
- Next.js dashboard (operator UI)
- Optional Cloud Functions (notifications)

It is a **safety-critical interface contract**. Any change here must be reflected in firmware and dashboard together.

---

## 1) Top-level layout

All data lives under:

```text
/pump_system/
  status/     (ESP32 → Cloud)
  control/    (Cloud → ESP32)
  config/
    device/
    admins/
    notifications_by_user/
    notification_last_sent/
  audit/
    events/
  presence/   (optional / best-effort)
```

---

## 2) Write ownership (hard rule)

| Path | Written by | Notes |
|------|------------|------|
| `/pump_system/status` | **ESP32** only | Dashboard must treat this as source of truth |
| `/pump_system/control/*` | **Admins only** | Admins are defined by `config/admins/{uid}=true` |
| `/pump_system/config/device` | **Admins only** | Calibration + thresholds |
| `/pump_system/config/admins/*` | **Admins only** | Admins can add/remove other admins |
| `/pump_system/config/notifications_by_user/{uid}` | That **user** only | Each user manages their own notification preferences |
| `/pump_system/audit/events/*` | Any signed-in user | Must only write entries whose `uid` matches `auth.uid` |
| `/pump_system/presence/*` | Any signed-in user | Best-effort; optional |

Implementation: see `database.rules.json`.

---

## 3) Control (Cloud → ESP32)

Path: `/pump_system/control`

### 3.1 Fields

| Key | Type | Required | Reset behavior | Meaning |
|-----|------|----------|----------------|---------|
| `mode` | string | yes | persistent | `AUTO` \| `MANUAL` \| `COUNTDOWN` |
| `manual_desired` | boolean | no | persistent | MANUAL intent: true=request ON, false=request OFF |
| `countdown_duration_min` | number | no | persistent | 1–120 minutes |
| `countdown_start` | boolean | no | **one-shot** | Start countdown (UI sets true then clears) |
| `countdown_add_min` | number | no | persistent | Minutes to add when `countdown_add_time` is triggered (commonly 5) |
| `countdown_add_time` | boolean | no | **one-shot** | Add time to running countdown |
| `countdown_stop` | boolean | no | **one-shot** | Stop an active countdown while mode remains `COUNTDOWN` |
| `emergency_stop` | boolean | no | **one-shot** | Latch emergency stop |
| `reset_stop` | boolean | no | **one-shot** | Clear emergency stop latch |
| `clear_error` | boolean | yes | **one-shot** | Clears dry-run and overflow lockouts |
| `reboot_request_id` | number | no | persistent | Monotonic token: new value triggers ESP32 soft restart |
| `bypass_level_sensor` | boolean | no | persistent | Maintenance: ignore level for start/stop (flow guard still active) |
| `bypass_flow_sensor` | boolean | no | persistent | Maintenance bypass for dry-run flow gate |

### 3.2 One-shot conventions

- **UI-reset one-shots**: UI writes `true`, waits a short window, then writes `false` (or clears field).
  - `countdown_start`
  - `countdown_stop`
  - `emergency_stop`
  - `reset_stop`
- **Firmware-reset one-shots**: Firmware may write `false` back after processing.
  - `clear_error`
  - `countdown_add_time` (implementation-dependent)

The dashboard should treat one-shots as **edge-triggered**, and provide a busy/disabled state to prevent repeated taps until confirmed.

---

## 4) Status (ESP32 → Cloud)

Path: `/pump_system/status`

### 4.1 Required core fields

| Key | Type | Meaning |
|-----|------|---------|
| `water_level_percent` | number | 0–100 (computed by master, preferably from `DIST`) |
| `flow_rate_lpm` | number | Liters per minute |
| `is_running` | boolean | True when relay energizes contactor coil |
| `run_mode` | string | AUTO \| AUTO_STANDBY \| AUTO_COOLDOWN \| MANUAL_ON \| MANUAL_OFF \| MANUAL_COOLDOWN \| COUNTDOWN |
| `is_error` | boolean | Dry-run lockout latched |
| `is_overflow_error` | boolean | Overflow protection latched |
| `last_fault_code` | string | Fault identifier (see below) |
| `last_fault_message` | string | Human-readable fault details |
| `manual_desired` | boolean | Echo of control intent (for UI clarity) |
| `emergency_stop_latched` | boolean | True when emergency stop latch is active |
| `remote_sensor_stable` | boolean | RS-485 stability latch: N consecutive good frames |
| `level_fresh` | boolean | Freshness gate: last good frame within timeout |

Current expected `run_mode` values for this release:

- `AUTO`
- `AUTO_STANDBY`
- `AUTO_COOLDOWN`
- `MANUAL_ON`
- `MANUAL_OFF`
- `MANUAL_COOLDOWN`
- `COUNTDOWN`

### 4.2 Recommended diagnostic fields

These improve observability and troubleshooting:

- `wifi_rssi`
- `uptime_minutes`
- `is_level_sensor_error`, `is_flow_sensor_error`
- `bypass_level_sensor`, `auto_bypass_active`
- Estimation telemetry (when bypass/sensor failure):
  - `estimated_level_pct`
  - `level_estimate_active`
  - `flow_volume_added_l`
  - `level_last_valid_age_sec`
  - `level_sensor_health_pct`
- Heap/network telemetry:
  - `free_heap_bytes`, `min_free_heap_bytes`, `max_alloc_heap_bytes`
  - `firebase_consecutive_failures`, `firebase_last_error`

### 4.3 Fault codes

Firmware can emit (not exhaustive; UI must have a safe fallback):

- `DRY_RUN`
- `OVERFLOW`
- `LEVEL_SENSOR`
- `FLOW_SENSOR`
- `COMM_LOSS`
- `STALE_LEVEL`
- `E_STOP`
- `SAFE_MODE`

---

## 5) Config

### 5.1 Device config

Path: `/pump_system/config/device` (admin-only write)

Contains calibration and thresholds, including:

- `tank_empty_cm`, `tank_full_cm`
- `pump_start_level`, `pump_stop_level`
- `dry_run_threshold_lpm`, `dry_run_timeout_sec`
- `flow_calibration_factor`
- `max_pump_runtime_min`
- sleep scheduling keys
- sensor failure thresholds + idle intervals
- auto-bypass settings

### 5.2 Admins map

Path: `/pump_system/config/admins/{uid} = true`

This is the sole source of truth for admin authorization in rules and UI gating.

