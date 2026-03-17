# Changelog · v2.0

Summary of key behavior and documentation changes introduced in the
Firmware–Dashboard Design **v2.0** compared to the original design.

For full technical detail, see:

- `docs/releases/v2.0/firmware-rtdb-spec.md`
- `docs/releases/v2.0/dashboard-ux-spec.md`

---

## Firmware & RTDB

- **Split sensor error flags**:
  - Replaced combined `is_sensor_error` boolean with explicit `is_level_sensor_error` and `is_flow_sensor_error`.
  - Dashboard and firmware both consume these new fields.
- **Bypass and maintenance fields clarified**:
  - `bypass_level_sensor` is the single canonical flag for level‑sensor bypass.
  - Deprecated `is_maintenance_active` (no longer used by dashboard).
- **`run_mode` semantics fixed**:
  - Introduced `AUTO_STANDBY` to represent “AUTO but pump off, waiting to start”.
  - Dashboard renders this as “AUTO (Standby)” to avoid confusion with OFF.
- **Manual stop behavior corrected**:
  - `manual_stop` now reverts policy mode to `AUTO` instead of `FORCE_OFF`.
  - Prevents accidental long‑term FORCE_OFF after stopping a Manual run.
- **Countdown mode**:
  - Added `COUNTDOWN` policy mode, `countdown_duration_min`, `countdown_add_time`, and `countdown_remaining_sec`.
  - On expiry, firmware switches `mode` back to `AUTO`.
- **Sensor resilience telemetry**:
  - Added `estimated_level_pct`, `level_estimate_active`, `flow_volume_added_l`, `level_last_valid_age_sec`, `level_sensor_health_pct`.
  - Dashboard uses these to display estimated level, sensor health, and stale‑data badges.
- **Fault code standardization**:
  - Defined `last_fault_code` and `last_fault_message` with a fixed set of codes (`DRY_RUN`, `OVERFLOW`, `LEVEL_SENSOR`, `FLOW_SENSOR`, `SAFE_MODE`).

---

## Dashboard UX

- **Layered layout (4 layers)**:
  - Layer 1: System state (StatusBar, tank, key stats).
  - Layer 2: Ranked alerts (offline, dry‑run, overflow, maintenance, sleep).
  - Layer 3: Run controls + Mode controls, separated but visually grouped.
  - Layer 4: Diagnostics (history chart, system info, activity log).
- **Alert ranking model implemented** (1–8) so the most critical issues appear first.
- **Mobile‑first header**:
  - Slim top StatusBar + compact header.
  - Overflow menu on mobile for Settings, Notifications, Restart, Sign out.
- **Emergency Override UX**:
  - Clear confirmation dialog and copy when entering FORCE_ON.
  - No Stop button in Run Controls while in Emergency Override; user stops via mode.
- **Manual vs Countdown vs AUTO clarified**:
  - Explicit run types in Run Controls.
  - `run_mode` badges show MANUAL / COUNTDOWN / AUTO / AUTO (Standby).
- **Estimated level visualization**:
  - When `level_estimate_active` is true, level is shown as `~XX%` in amber with dashed tank fill and “Estimated · ±5%” label.
- **History chart context**:
  - Added dashed reference lines at `pump_start_level` and `pump_stop_level`.

---

## Operational & Documentation

- Removed `presence/` node from RTDB layout and all presence‑related UI.
- Clarified admin model (`config/admins/{uid}`) and security rules.
- Documented one‑shot timing for `manual_start`, `manual_stop` (5‑second window) and `countdown_add_time` (firmware‑reset one‑shot).

