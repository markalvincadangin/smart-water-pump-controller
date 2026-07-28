# Data Model: App UI/UX Integration

## 1. Firebase RTDB Entities (App Perspective)

### `DeviceShadow`
Represents the twin state of the pump hardware.
- **Fields**:
  - `desired` (Object): The state the user is requesting.
    - `mode` (String): `"AUTO"`, `"MANUAL"`, or `"COUNTDOWN"`.
    - `manual_desired` (Boolean): Persistent intent.
    - `countdown_start` (Boolean): One-shot to begin countdown.
    - `countdown_duration_min` (Number): Duration.
    - `emergency_stop` (Boolean): Request immediate stop.
    - `reset_stop` (Boolean): Reset E-stop.
    - `clear_error` (Boolean): Reset hard lockout.
    - `bypass_level_sensor` (Boolean): Persistent bypass.
    - `bypass_flow_sensor` (Boolean): Persistent bypass.
  - `reported` (Object): The actual confirmed state from the ESP32.
    - `run_mode` (String): ESP32's current mode.
    - `is_running` (Boolean): Pump is actually running.
    - `countdown_remaining_sec` (Number): Time remaining.
    - `is_error` (Boolean): Fault active.
    - `is_overflow_error` (Boolean): Overflow fault active.
    - `emergency_stop_latched` (Boolean): E-Stop active.

### `Telemetry`
Real-time sensor stream.
- **Fields**:
  - `water_level_percent` (Number): Percentage 0-100.
  - `flow_rate_lpm` (Number): Liters per minute.
  - `ultrasonic_last_good_cm` (Number): Raw ultrasonic sensor distance.

### `DeviceConfig`
User-configurable safety limits (Stored in `settings` node).
- **Fields**:
  - `pump_start_level_pct` (Number): Tank % at which pump turns ON in AUTO mode.
  - `pump_stop_level_pct` (Number): Tank % at which pump turns OFF in AUTO mode.
  - `dry_run_threshold_lpm` (Number): Minimum flow rate before triggering a dry-run fault.
  - `max_pump_runtime_min` (Number): Maximum allowed continuous run time.

---

## 2. App Local Domain Models (Kotlin)

### `DashboardUiState` (UI Model)
- `isPumpRunning: Boolean`
- `mode: ControlMode` (AUTO, MANUAL, COUNTDOWN)
- `lockoutActive: Boolean`
- `waterLevelPct: Int`
- `flowRateLpm: Float`
- `connectionStatus: ConnectionState`
- `countdownRemainingSec: Int`
