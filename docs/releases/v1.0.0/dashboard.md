## Dashboard Documentation — v1.0.0

### Scope

This document describes the Next.js dashboard subsystem as deployed in v1.0.0:

- Technology stack
- Data flow (RTDB subscription + writes)
- Safety-critical UX rules
- Operational behavior (offline handling, admin gating)

For the authoritative RTDB schema, see `rtdb-contract.md`.

---

## 1) System role and constraints

The dashboard is an operator interface for a safety-critical physical system.

Hard constraints:

- The dashboard is **not** a control system; it only writes **intent** to RTDB.
- The firmware is the **sole authority** for pump actuation and safety enforcement.
- The dashboard must never present ambiguous or misleading state (e.g., showing “Running” if `is_running=false`).

---

## 2) Technology stack

- Next.js (App Router) + TypeScript
- Tailwind CSS
- Firebase client SDK:
  - Auth (Google sign-in)
  - Realtime Database (status/control/config/audit)
- Icons: Lucide
- Optional PWA + service worker

---

## 3) Data flow

### 3.1 Inbound (status/control)

The dashboard subscribes to:

- `/pump_system/status`
- `/pump_system/control`

It derives:

- Controller online/offline based on stale status update time
- Current policy mode and run state
- Fault banners and guidance
- Safety gate banners:
  - `remote_sensor_stable=false` → link unstable
  - `level_fresh=false` → level stale

### 3.2 Outbound (control writes)

The dashboard writes to `/pump_system/control` using safe patterns:

- Validates `mode` strings before writing.
- Uses one-shot semantics for actions:
  - `emergency_stop`
  - `reset_stop`
  - `countdown_start`
  - `clear_error` (may be firmware-reset depending on implementation)
- Uses persistent intent for MANUAL:
  - `mode="MANUAL"`
  - `manual_desired=true|false`

All control/config writes are **admin-gated** based on:

```text
/pump_system/config/admins/{uid} = true
```

Enforced in both:

- UI gating (disabled buttons + explanatory copy)
- `database.rules.json`

---

## 4) Operator UX (safety-critical)

### 4.1 Emergency stop

- Must be visually high-salience.
- Must show latched status clearly (`emergency_stop_latched=true`).
- Must not allow “start” actions while stopped unless reset.

### 4.2 MANUAL mode (intent-based)

- MANUAL is not a “momentary run” button; it is a policy mode.
- The dashboard should keep `mode="MANUAL"` while toggling `manual_desired` ON/OFF.
- If the user requests ON but the pump does not run, the UI must surface:
  - hard lockouts (dry-run / overflow)
  - emergency stop latch
  - comm freshness/stability gates

### 4.3 Countdown mode

- Requires explicit start (`countdown_start` one-shot).
- Duration must be set (`countdown_duration_min`).
- Shows remaining time from `countdown_remaining_sec` when provided.

### 4.4 Offline behavior

- If the controller is offline (no fresh `/status`), disable control buttons and show clear messaging.
- Never imply a command succeeded unless status confirms it.

---

## 5) Operational features

- Fault code mapping with safe fallback (unknown faults still display safely).
- Audit logging (best-effort append-only) for operator actions.

