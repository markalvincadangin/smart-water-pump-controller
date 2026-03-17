## Dashboard UX Spec — v1.0.0

### Goal

Provide a calm, high-signal operator UI that:

- Makes the current system state obvious (level, flow, running state, mode)
- Surfaces safety-critical blockers (lockouts, e-stop latch, comm gates)
- Prevents unsafe or confusing user actions

---

## 1) Information hierarchy (recommended)

1. **System state (always visible)**
   - Controller connectivity and last update age
   - Policy mode + run mode
   - Emergency stop state
   - Key telemetry: level %, flow L/min, pump running
2. **Alerts / banners**
   - Offline, e-stop, dry-run lockout, overflow, comm unstable, stale level, sensor errors, maintenance bypass
3. **Controls**
   - Emergency stop + reset
   - Mode controls (AUTO / MANUAL / COUNTDOWN)
   - MANUAL intent toggle (ON/OFF)
   - Countdown duration + start + add time + stop behavior
4. **Diagnostics**
   - History
   - System info
   - Audit trail

---

## 2) State clarity rules

### 2.1 Never “infer” the pump state

- “Pump ON” is only when `status.is_running === true`.
- “Pump requested ON” is separate (MANUAL intent) and is shown via `manual_desired`.

### 2.2 Blocked-start messaging

If the user requests pump ON but `is_running` remains false, show explicit blockers in priority order:

1. `emergency_stop_latched=true` → “Emergency stop is latched. Reset stop to resume.”
2. `is_error=true` → “Dry-run lockout. Clear error after verifying water supply.”
3. `is_overflow_error=true` → “Overflow protection. Clear error after inspection.”
4. `remote_sensor_stable=false` → “Link unstable. Check RS‑485 wiring/noise.”
5. `level_fresh=false` → “Level stale. Check RS‑485 node power/link.”
6. `is_level_sensor_error=true` (when bypass off) → “Level sensor error. Inspect sensor/node.”

---

## 3) Control UX rules

### 3.1 Admin gating

- Any control write must be admin-gated.
- Non-admins should see the UI but with disabled controls and explanatory text.

### 3.2 Emergency stop

- Emergency stop is a one-shot write (`emergency_stop=true`) and should be treated as an “action”, not a mode.
- Provide a separate reset action (`reset_stop=true`).

### 3.3 MANUAL

- MANUAL is intent-based:
  - Toggle `manual_desired` ON/OFF.
  - Keep `mode="MANUAL"` while toggling.
- Do not provide legacy “manual_start/manual_stop” UX.

### 3.4 COUNTDOWN

- Starting a countdown requires:
  - `mode="COUNTDOWN"`
  - `countdown_duration_min` set
  - `countdown_start=true` one-shot
- Adding time should be explicit:
  - `countdown_add_min` set
  - `countdown_add_time=true` one-shot

---

## 4) Visual design rules (safety-oriented)

- Calm baseline; reserve red for e-stop/lockouts only.
- Avoid heavy animation except for “pump running” indication.
- Use consistent icons; avoid emojis for critical meanings.
- Maintain large touch targets (≥44px) and strong disabled-state clarity.

