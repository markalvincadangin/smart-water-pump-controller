# SmartFlow — Firebase Schema Canonical Contract

All new fields are additive only. Never remove or rename existing fields.
New RS-485 frame fields (LDSC) must be optional in the parser.

---

## `/pump_system/status` — Written by ESP32

| Field | Type | Notes | New in refactor? |
|-------|------|-------|-----------------|
| `water_level_percent` | int \| absent | **Omit when `waterLevelPct == -1`** | — |
| `is_running` | bool | Relay state (active-low: LOW = ON) | — |
| `flow_rate_lpm` | float | From RS-485 FLOW field | — |
| `run_mode` | string | See Run Mode table below | Updated values |
| `pump_cooldown_remaining_sec` | int | 0 when not in cooldown | **NEW** |
| `is_error` | bool | DRY_RUN lockout active | — |
| `is_sensor_error` | bool | Ultrasonic sensor failure | — |
| `is_flow_sensor_error` | bool | Flow sensor error | — |
| `is_overflow_error` | bool | Max runtime exceeded | — |
| `is_idle_mode` | bool | Slow-poll mode active | **NEW (was missing)** |
| `is_sleeping` | bool | Scheduled sleep active | — |
| `emergency_stop_latched` | bool | | — |
| `manual_desired` | bool | | — |
| `bypass_level_sensor` | bool | | — |
| `bypass_flow_sensor` | bool | | **NEW** |
| `remote_sensor_stable` | bool | 3 consecutive valid frames | — |
| `level_fresh` | bool | Level age < staleness threshold | — |
| `manual_runtime_warning` | bool | Manual run exceeded max runtime | **NEW** |
| `countdown_remaining_sec` | int | 0 when not in countdown | — |
| `last_fault_code` | string | See Fault Code table | — |
| `last_fault_message` | string | Human-readable fault detail | — |
| `level_sensor_health_pct` | int | 0–100 | — |
| `remote_level_discard_count` | int | From RS-485 LDSC field | **NEW** |
| `level_estimate_active` | bool | Level estimated via flow while sensor bypassed | — |
| `estimated_level_pct` | int | Present only when bypass active | — |
| `flow_volume_added_l` | float | | — |
| `wifi_rssi` | int | dBm | — |
| `uptime_minutes` | int | | — |
| `last_boot_reason` | string | | — |
| `debug_log_level` | int | Current active gLogLevel (0–4) | **NEW** |
| `total_pump_cycles` | int | NVS-persisted | — |
| `total_pump_run_min` | int | NVS-persisted | — |
| `ultrasonic_cycles_ok` | int | Lifetime counter | — |
| `ultrasonic_cycles_timeout` | int | Lifetime counter | — |
| `ultrasonic_last_good_cm` | float | | — |
| `free_heap_bytes` | int | | — |
| `min_free_heap_observed_bytes` | int | | — |
| `max_alloc_heap_bytes` | int | | — |
| `firebase_consecutive_failures` | int | | — |
| `firebase_last_error` | string | | — |

---

## Run Mode Values

| Value | Condition | Dashboard label |
|-------|-----------|----------------|
| `AUTO_STANDBY` | AUTO mode, pump off, level OK | AUTO — Standby |
| `AUTO` | AUTO mode, pump running | AUTO — Running |
| `AUTO_COOLDOWN` | AUTO, pump off, off-timer active | AUTO — Cooldown Xs |
| `MANUAL_ON` | MANUAL mode, pump running | MANUAL — On |
| `MANUAL_OFF` | MANUAL mode, pump off | MANUAL — Off |
| `MANUAL_COOLDOWN` | MANUAL, pump off, off-timer active | MANUAL — Cooldown Xs |
| `COUNTDOWN` | Countdown running | Countdown |
| `STOPPED` | Emergency stop latched | Emergency Stop |

**Init rule:** `runMode` must initialize to `"AUTO_STANDBY"`, not `"OFF"` (Bug M-05).

---

## Fault Code Values

| Code | Trigger | Recovery |
|------|---------|---------|
| `DRY_RUN` | Flow < threshold for > timeout while running | `clear_error: true` + verify water |
| `OVERFLOW` | Runtime > max in AUTO/COUNTDOWN | `clear_error: true` |
| `E_STOP` | Emergency stop triggered | `clear_error: true` then `reset_stop: true` |
| `COMM_LOSS` | RS-485 link unstable | Auto-clears on link recovery |
| `STALE_LEVEL` | Level data age > threshold | Auto-clears when fresh data arrives |
| `LEVEL_SENSOR` | Ultrasonic error | Auto-clears on sensor recovery |
| `FLOW_SENSOR` | Flow sensor stuck-high while pump OFF | Auto-clears on recovery |
| `SAFE_MODE` | Crash loop detected | Power cycle or 1-hour auto-clear |

---

## `/pump_system/control` — Read by ESP32

| Field | Type | Behavior |
|-------|------|---------|
| `mode` | string | Valid: `AUTO`, `MANUAL`, `COUNTDOWN` |
| `manual_desired` | bool | Persistent operator intent in MANUAL mode |
| `emergency_stop` | bool | One-shot. Firmware resets to false |
| `reset_stop` | bool | One-shot. Blocked if lockout active |
| `clear_error` | bool | One-shot. Clears DRY_RUN and OVERFLOW |
| `countdown_start` | bool | One-shot |
| `countdown_duration_min` | int | 1–120 |
| `countdown_add_time` | bool | One-shot |
| `countdown_add_min` | int | Minutes to add |
| `bypass_level_sensor` | bool | Persistent |
| `bypass_flow_sensor` | bool | Persistent — **NEW** |
| `reboot_request_id` | int | Increment to trigger reboot |

---

## `/pump_system/config/device` — Read by ESP32

| Field | Type | Range | Notes |
|-------|------|-------|-------|
| `tank_empty_cm` | int | 5–200 | |
| `tank_full_cm` | int | 1–199 | |
| `pump_start_level` | int | 0–100 | Must be < pump_stop_level |
| `pump_stop_level` | int | 0–100 | Must be > pump_start_level |
| `dry_run_threshold_lpm` | float | 0.1–10.0 | Default: 1.0 (updated from 0.5) |
| `dry_run_timeout_sec` | int | 10–300 | |
| `max_pump_runtime_min` | int | 30–480 | |
| `flow_calibration_factor` | float | 0.1–20.0 | |
| `debug_log_level` | int | 0–4 | **NEW** — remote log level control |
| `sleep_enabled` | bool | | |
| `sleep_start_hour` | int | 0–23 | PHT (UTC+8) |
| `sleep_end_hour` | int | 0–23 | PHT |
| `sleep_emergency_level` | int | 0–100 | |
| `sensor_failure_threshold` | int | | |
| `idle_sensor_interval_ms` | int | | |
| `idle_firebase_interval_ms` | int | | |

---

## `/pump_system/config/notifications_by_user/$uid`

| Field | Type | Notes |
|-------|------|-------|
| `enabled` | bool | |
| `email` | string | |
| `fcmTokens` | array | Push notification tokens |
| `dryRunAlert` | bool | |
| `lowLevelAlert` | bool | |
| `lowLevelThreshold` | int | 0–100 |
| `pumpStartedAlert` | bool | |
| `overflowAlert` | bool | |

---

## Implementation Verification

After implementing Firebase schema changes, verify with this checklist:

- [ ] `pushFirebaseStatus()` includes every field in the status table
- [ ] `readFirebaseControl()` reads every field in the control table
- [ ] `readDeviceConfigFromFirebase()` reads every field in the config table
- [ ] `water_level_percent` is omitted when `waterLevelPct == -1`
- [ ] All new fields (`pump_cooldown_remaining_sec`, `manual_runtime_warning`,
      `bypass_flow_sensor`, `is_idle_mode`, `debug_log_level`,
      `remote_level_discard_count`) are present in the status push
- [ ] `bypass_flow_sensor` is read from control path and persisted to NVS
- [ ] `debug_log_level` is read from config path and applied to `gLogLevel`
- [ ] No existing field has been removed or renamed
