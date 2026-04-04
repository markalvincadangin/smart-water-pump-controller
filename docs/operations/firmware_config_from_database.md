# Firmware Device Configuration from Firebase

This document describes the implemented runtime configuration model for the ESP32 controller.

## Scope

- Source path: `/pump_system/config/device`
- Reader: ESP32 firmware (`readDeviceConfigFromFirebase()`)
- Write authority: Admin UIDs only (enforced in `database.rules.json`)
- Refresh interval: every 30 seconds while cloud is healthy
- Persistence: applied to runtime and persisted via NVS after valid update

## Why this matters

This model allows calibration and safety tuning without reflashing firmware while preserving safe fallback behavior during network issues.

## Effective behavior

1. Firmware starts with compile-time defaults from `config/config.h`.
2. On successful config read, values are validated and applied.
3. If payload is invalid/partial, firmware keeps the previous valid config.
4. On cloud outage, firmware continues using last valid in-memory config.
5. NVS stores accepted config for reboot continuity.

## Required validation rules (firmware-enforced)

- `tank_empty_cm`: 5 to 200
- `tank_full_cm`: 1 to `tank_empty_cm - 1`
- `pump_start_level`: 0 to 100
- `pump_stop_level`: 0 to 100 and strictly greater than `pump_start_level`
- `dry_run_threshold_lpm`: 0.1 to 10.0
- `dry_run_timeout_sec`: 10 to 300
- `flow_calibration_factor`: 0.1 to 20.0
- `max_pump_runtime_min`: 30 to 480
- `debug_log_level`: 0 to 4
- `sleep_start_hour`, `sleep_end_hour`: 0 to 23
- `sleep_emergency_level`: 0 to 100
- `sensor_failure_threshold`/`level_sensor_failure_threshold`: 3 to 20
- `idle_sensor_interval_ms`: 5000 to 60000
- `idle_firebase_interval_ms`: 10000 to 120000
- `auto_bypass_delay_sec`: 10 to 300

## Current key set in `/pump_system/config/device`

```json
{
  "tank_empty_cm": 122,
  "tank_full_cm": 8,
  "pump_start_level": 30,
  "pump_stop_level": 100,
  "dry_run_threshold_lpm": 1.0,
  "dry_run_timeout_sec": 30,
  "flow_calibration_factor": 7.5,
  "max_pump_runtime_min": 120,
  "debug_log_level": 2,
  "sleep_enabled": false,
  "sleep_start_hour": 23,
  "sleep_end_hour": 5,
  "sleep_emergency_level": 5,
  "sensor_failure_threshold": 5,
  "idle_sensor_interval_ms": 10000,
  "idle_firebase_interval_ms": 30000,
  "auto_bypass_on_sensor_fail": false,
  "auto_bypass_delay_sec": 60
}
```

Notes:

- `level_sensor_failure_threshold` is supported as an alternate key for sensor-failure threshold.
- Pin mappings, relay polarity, and other hardware constants remain firmware-side and are not DB-configurable.

## Access control expectations

From current `database.rules.json`:

- Read: any authenticated client can read `pump_system/config/device`.
- Write: only UIDs listed in `pump_system/config/admins/{uid} = true` can write `pump_system/config/device`.

## Operational checks

1. Update one config value in dashboard Device Config.
2. Wait up to 30 seconds.
3. Confirm behavior change in status/operation.
4. Reboot controller and verify setting persists.
5. Write an invalid value and confirm firmware rejects update (existing behavior remains unchanged).

## Failure handling

- If config fetch fails: firmware logs warning and retains last valid config.
- If payload validation fails: firmware rejects update and keeps current config.
- If NVS write fails: runtime config still applies; investigate persistence layer.
