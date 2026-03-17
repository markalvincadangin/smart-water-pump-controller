## Dashboard Specification (Current)

This document is the **current** (non-versioned) dashboard specification for the Smart Water Pump Controller system.

The dashboard is a safety-critical operator interface. Firmware remains the source of truth; the dashboard must not infer pump behavior beyond what the ESP32 publishes to RTDB.

### System responsibilities

- Subscribe to:
  - `/pump_system/status`
  - `/pump_system/control`
- Present:
  - Current tank level (%), flow (L/min), pump running state, policy mode and run mode.
  - Faults and recovery guidance (`last_fault_code`, `last_fault_message`).
  - Safety gates that explain blocked starts (`remote_sensor_stable`, `level_fresh`).
- Write control intents to `/pump_system/control` in a safe, idempotent manner.

### Source-of-truth contract (RTDB)

The dashboard TypeScript types in `dashboard/lib/types.ts` are the canonical UI contract.

**Status fields used by UI**

- `water_level_percent` (0–100)
- `flow_rate_lpm` (L/min)
- `is_running`
- `run_mode`:
  - `OFF` | `AUTO` | `AUTO_STANDBY` | `MANUAL_ON` | `MANUAL_OFF` | `COUNTDOWN` | `STOPPED`
- `is_error` (dry-run lockout)
- `is_overflow_error`
- `is_level_sensor_error` / `is_flow_sensor_error`
- `last_fault_code` / `last_fault_message`
- `manual_desired`
- `emergency_stop_latched`
- `remote_sensor_stable`
- `level_fresh`
- `wifi_rssi`, `uptime_minutes`, optional diagnostics

**Control fields written by UI**

- `mode`: `AUTO` | `MANUAL` | `COUNTDOWN`
- `manual_desired` (bool)
- `emergency_stop` (one-shot bool; UI sets true then clears)
- `reset_stop` (one-shot bool; UI sets true then clears)
- `countdown_duration_min` (int 1–120)
- `countdown_start` (one-shot bool)
- `countdown_add_min` + `countdown_add_time` (optional one-shot add-time behavior)
- `clear_error` (acknowledge/clear lockouts)
- `reboot_request_id` (admin-only)

### Control UX rules (safety-critical)

- **No unsafe bypass in UI**:
  - UI must not provide any “override safety” control.
  - All actions remain subject to firmware safety enforcement.
- **Emergency Stop**:
  - Only shown/enabled when it is meaningful (typically when the pump is running).
  - Produces a latched stopped state until reset.
- **MANUAL is intent-based**:
  - Use `manual_desired` ON/OFF; keep `mode="MANUAL"` while toggling intent.
- **Blocked start clarity**:
  - When `manual_desired=true` but pump isn’t running and either `remote_sensor_stable=false` or `level_fresh=false`,
    show a clear “blocked by safety” warning with recovery steps.
- **Offline behavior**:
  - When controller is offline (stale status), disable control writes and show clear connectivity banners.

### UI architecture

- `dashboard/app/page.tsx`: composition and top-level state.
- `dashboard/lib/usePumpData.ts`: RTDB subscriptions + control writers.
- Alerts:
  - `dashboard/lib/alertRanking.ts`: ranked alert generation.
  - Fault code UX mapping: `dashboard/lib/faultCodes.ts` (must cover firmware fault codes, with safe fallback).

### UX principles (operational)

- Calm baseline UI; reserve high-salience color for abnormal states.
- Every warning must be actionable: “what happened / risk / what to do”.
- Touch targets ≥44px and clear disabled-state reasons.

