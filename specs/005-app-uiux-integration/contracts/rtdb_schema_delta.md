# Firebase RTDB Schema Delta

## Additions / Mappings for Android App Integration

The Android application will now fully adopt the v2 schema that is assumed to be active on the firmware side. The app will act as a reader for `reported` and `telemetry`, and a writer for `desired` and `config`.

```json
{
  "devices": {
    "[device_id]": {
      "shadow": {
        "desired": {
          "mode": "MANUAL",
          "manual_desired": true,
          "emergency_stop": false,
          "reset_stop": false,
          "clear_error": false
        },
        "reported": {
          "run_mode": "MANUAL",
          "is_running": true,
          "is_error": false,
          "is_overflow_error": false,
          "emergency_stop_latched": false,
          "countdown_remaining_sec": 0
        }
      },
      "telemetry": {
        "water_level_percent": 45,
        "flow_rate_lpm": 12.5,
        "ultrasonic_last_good_cm": 110
      },
      "settings": {
        "pump_start_level_pct": 30,
        "pump_stop_level_pct": 100,
        "dry_run_threshold_lpm": 2.0,
        "max_pump_runtime_min": 45
      }
    }
  }
}
```

### Constraints
- The Android app must NEVER write directly to `shadow/reported` or `telemetry`.
- The Android app writes only to `shadow/desired` and `settings`.
