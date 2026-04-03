## Dashboard Specification (Current)

**Refactor version:** 2.0 | Updated 2026-03-31 (Phases 1–3 complete)

This document is the **current** (non-versioned) dashboard specification for the SmartFlow system.
The dashboard is a safety-critical operator interface. Firmware is the source of truth; the dashboard
must not infer pump behavior beyond what the ESP32 publishes to RTDB.

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

| Field | Type | Notes |
|-------|------|-------|
| `water_level_percent` | int | Absent until first valid RS-485 frame |
| `flow_rate_lpm` | float | L/min |
| `is_running` | bool | Relay state |
| `run_mode` | string | See Run Mode table below |
| `pump_cooldown_remaining_sec` | int | 0 when not in cooldown |
| `is_error` | bool | Dry-run lockout active |
| `is_level_sensor_error` | bool | Ultrasonic failure |
| `is_flow_sensor_error` | bool | Flow sensor error |
| `is_overflow_error` | bool | Max runtime exceeded |
| `manual_runtime_warning` | bool | MANUAL run reached ~90% of configured max runtime (pump still on; overflow cutoff follows at 100%) |
| `is_idle_mode` | bool | Slow-poll mode active — **Phase 3** |
| `is_sleeping` | bool | Scheduled light sleep active |
| `bypass_level_sensor` | bool | |
| `bypass_flow_sensor` | bool | **Phase 1** |
| `auto_bypass_active` | bool | Auto-maintenance mode engaged |
| `last_fault_code` | string | See Fault Code table below |
| `last_fault_message` | string | Human-readable detail |
| `manual_desired` | bool | |
| `emergency_stop_latched` | bool | |
| `remote_sensor_stable` | bool | Safety gate |
| `level_fresh` | bool | Safety gate |
| `countdown_remaining_sec` | int | 0 when not in countdown |
| `remote_level_discard_count` | int | From RS-485 LDSC field — **Phase 3** (per ultrasonic measurement window snapshot; not cumulative) |
| `debug_log_level` | int 0–4 | Current gLogLevel — **Phase 1** |
| `wifi_rssi` | int dBm | |
| `uptime_minutes` | int | |
| `last_boot_reason` | string | |
| `total_pump_cycles` | int | NVS-persisted |
| `total_pump_run_min` | int | NVS-persisted |
| `free_heap_bytes` | int | Diagnostic |
| `min_free_heap_observed_bytes` | int | Diagnostic |
| `firebase_consecutive_failures` | int | Diagnostic |

**Run Mode values** (`run_mode`):

| Value | Condition | Dashboard label |
|-------|-----------|----------------|
| `AUTO_STANDBY` | AUTO, pump off, level OK | AUTO — Standby |
| `AUTO` | AUTO, pump running | AUTO — Running |
| `AUTO_COOLDOWN` | AUTO, off-timer active | AUTO — Cooldown Xs |
| `MANUAL_ON` | MANUAL, pump running | MANUAL — On |
| `MANUAL_OFF` | MANUAL, pump off | MANUAL — Off |
| `MANUAL_COOLDOWN` | MANUAL, off-timer active | MANUAL — Cooldown Xs |
| `COUNTDOWN` | Countdown running | Countdown |
| `STOPPED` | Emergency stop latched | Emergency Stop |

**Fault Code values** (`last_fault_code`):

| Code | Trigger | Recovery |
|------|---------|----------|
| `DRY_RUN` | Flow < threshold for > timeout | `clear_error: true` + verify water |
| `OVERFLOW` | Runtime > max in AUTO/COUNTDOWN | `clear_error: true` |
| `E_STOP` | Emergency stop triggered | `clear_error: true` then `reset_stop: true` |
| `COMM_LOSS` | RS-485 link unstable | Auto-clears on link recovery |
| `STALE_LEVEL` | Level data age > threshold | Auto-clears when fresh data arrives |
| `LEVEL_SENSOR` | Ultrasonic error | Auto-clears on sensor recovery |
| `FLOW_SENSOR` | Flow sensor stuck-high while pump off | Auto-clears on recovery |
| `SAFE_MODE` | Crash loop detected | Power cycle or 1-hour auto-clear |

**Control fields written by UI**

| Field | Type | Notes |
|-------|------|-------|
| `mode` | string | `AUTO` \| `MANUAL` \| `COUNTDOWN` |
| `manual_desired` | bool | Persistent operator intent |
| `emergency_stop` | bool | One-shot |
| `reset_stop` | bool | One-shot |
| `clear_error` | bool | One-shot — clears DRY_RUN and OVERFLOW |
| `countdown_duration_min` | int 1–120 | |
| `countdown_start` | bool | One-shot |
| `countdown_add_min` | int | Minutes to add |
| `countdown_add_time` | bool | One-shot add-time trigger |
| `bypass_level_sensor` | bool | Persistent |
| `bypass_flow_sensor` | bool | Persistent — **Phase 1** |
| `reboot_request_id` | int | Monotonic token (admin only) |

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

