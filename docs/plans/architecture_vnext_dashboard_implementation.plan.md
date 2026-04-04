## Dashboard implementation plan — Architecture vNext

Target:
- `dashboard/` (Next.js PWA)

Source of truth:
- `docs/specs/dashboard.md`
- `docs/operations/firmware_config_from_database.md` and `docs/operations/rs485_tank_link.md`

---

## 0) Current alignment scan (baseline)

### Current dashboard control model (observed)
From `dashboard/lib/usePumpData.ts` and existing UI wiring:
- Writes `/pump_system/control/mode` with values including `FORCE_ON`/`FORCE_OFF`
- Uses one-shots:
  - `manual_start` (true → reset false after 5s)
  - `manual_stop` (true → reset false after 5s)
- Countdown:
  - `countdown_duration_min`, `countdown_add_time`, `countdown_add_min`
- Safety:
  - `clear_error`
  - `bypass_level_sensor`

### Required vNext dashboard control model
- No FORCE modes shown or written
- Three modes only: `AUTO`, `MANUAL`, `COUNTDOWN`
- MANUAL must expose explicit ON/OFF intent via `manual_desired` (persistent bool)
- COUNTDOWN should use an explicit start action (either `countdown_start` one-shot or “enter mode starts timer” — must match firmware)
- Emergency Stop:
  - shown only when running
  - writes `emergency_stop` one-shot
  - “Reset Stop” writes `reset_stop` one-shot

---

## 1) UX contract changes (front-end)

### 1.1 Remove FORCE_ON / FORCE_OFF UI entirely
- Remove buttons/toggles for FORCE modes in:
  - Mode controls UI
  - Alert banners that reference “override/lockout” modes
  - Any admin gating tied specifically to FORCE modes

### 1.2 Introduce MANUAL intent control (primary)
Add a clear, explicit control:
- **Manual Run: ON/OFF**
  - ON sets `manual_desired=true` and `mode="MANUAL"`
  - OFF sets `manual_desired=false` (mode remains MANUAL unless user switches)

UX requirements:
- If firmware reports blocked start (stale level, unstable sensor, dry-run lockout), show the reason inline and keep the toggle visually “ON but blocked” only if you also show an explicit “Blocked by safety” state; otherwise force toggle back off after a write is rejected (requires status feedback).

### 1.3 COUNTDOWN as “semi-auto”
Choose the firmware-aligned start mechanism:
- Option A (preferred): `countdown_start` one-shot + `countdown_duration_min`
- Option B: switching mode to COUNTDOWN starts the timer (current behavior in legacy system)

Implement the UI accordingly:
- Duration selector (1–120)
- Start/Stop countdown
- Optional +time button(s) (still supported by firmware if desired)

### 1.4 Emergency Stop (contextual)
Add a dedicated Emergency Stop control:
- Visible only when `status.is_running == true`
- Action:
  - writes `emergency_stop=true` (one-shot)
  - UI immediately shows STOPPED latched state when status confirms

Add “Reset Stop”:
- Visible when `status.emergency_stop_latched == true`
- writes `reset_stop=true` (one-shot)

---

## 2) RTDB writer changes (`usePumpData`)

### 2.1 Update PumpControl type
Add keys:
- `manual_desired?: boolean`
- `emergency_stop?: boolean`
- `reset_stop?: boolean`
- `countdown_start?: boolean` (if chosen)

Remove/deprecate keys in UI:
- `manual_start`, `manual_stop`
- Force mode values for `mode`

### 2.2 Implement new writers
- `setMode(mode)` only allows `AUTO|MANUAL|COUNTDOWN`
- `setManualDesired(on: boolean)`
  - sets `/control/mode = "MANUAL"` (if on or off; optional) and `/control/manual_desired = on`
- `triggerEmergencyStop()`
  - one-shot write `true` then reset to `false` after a short delay (e.g., 3–5s)
- `resetEmergencyStop()`
  - one-shot write `true` then reset to `false`
- Countdown:
  - if using `countdown_start`: set duration then pulse start

### 2.3 Audit logging alignment
Update audit event names/details to match new actions:
- `control.manual_desired`
- `control.emergency_stop`
- `control.reset_stop`

---

## 3) UI component updates

### 3.1 `RunControls`
- Replace “manual_start/stop” style buttons with:
  - Manual ON/OFF intent toggle
  - Countdown start/stop controls
  - Emergency stop button (conditional on running)

### 3.2 `ModeControls`
- Remove FORCE_ON/FORCE_OFF selectors
- Only show AUTO / MANUAL / COUNTDOWN

### 3.3 Alerts / banners
Update alert rules to:
- Remove FORCE mode related alerts
- Add “Emergency stop latched” critical alert with Reset action
- Add “Blocked by stale/unstable sensor” warning/critical alert

---

## 4) Firmware-dashboard alignment checklist (must pass)

Before shipping:
- Dashboard never writes `FORCE_ON`/`FORCE_OFF`
- Dashboard uses the same COUNTDOWN start semantics as firmware
- Dashboard displays `run_mode` values including:
  - `MANUAL_ON`, `MANUAL_OFF`, `COUNTDOWN`, `STOPPED`
- Dashboard uses status booleans if added:
  - `emergency_stop_latched`
  - `remote_sensor_stable`
  - `level_fresh`

---

## 5) Validation plan (dashboard)

### 5.1 Functional tests
- Manual ON sets intent true; pump runs if safe
- Manual OFF stops pump and stays in MANUAL
- COUNTDOWN start → timer begins; stop ends it
- Emergency Stop while running → pump stops and latches STOPPED; Reset clears latch

### 5.2 Failure-mode UX tests
- RS-485 unplugged (controller blocks start): dashboard must show “blocked by stale level” and prevent repeated unsafe start attempts
- Dry-run lockout: dashboard shows clear lockout and correct reset path

---

