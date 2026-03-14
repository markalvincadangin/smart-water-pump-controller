# Migration from v2.0 to v3.0

This document highlights the key differences between firmware **v2.0** and
**v3.0** and what they mean for the dashboard and operators.

It assumes familiarity with:

- `docs/releases/v2.0/firmware-rtdb-spec.md`
- `docs/releases/v3.0/firmware-spec.md`

---

## 1. RTDB Schema Changes

### 1.1 Sensor error fields

**Before (v2.0):**

- Combined error flag `is_sensor_error` in status.**After (v3.0):**

- Split into:  - `is_level_sensor_error` — ultrasonic level sensor failure.  - `is_flow_sensor_error` — flow sensor stuck‑high / abnormal.- Dashboard should rely on these explicit flags and treat any remaining
  `is_sensor_error` as legacy only.

### 1.2 Sensor resilience telemetry

New fields in v3.0 status:
- `estimated_level_pct`- `level_estimate_active`- `flow_volume_added_l`- `level_last_valid_age_sec`- `level_sensor_health_pct`- `total_pump_cycles`, `total_pump_run_min`
These are additive and backward‑compatible; existing clients can ignore them,
but the v2.0 dashboard uses them to render estimated level, stale‑data badge,
and sensor health indicators.

### 1.3 Config keys

Clarifications in v3.0:
- `level_sensor_failure_threshold` is the preferred config key for ultrasonic
  failure threshold.- Legacy `sensor_failure_threshold` is still accepted but should not be written
  by new clients.- New resilience‑related fields:  - `auto_bypass_on_sensor_fail`  - `auto_bypass_delay_sec`
---

## 2. Behavior & State Machine

### 2.1 Hierarchical priority model

v3.0 formalizes the priority order of pump decisions (P1–P5):
1. **P1 Hard safety** (dry‑run & overflow lockouts) — cannot be bypassed.2. **P2 Maintenance bypass** (`bypass_level_sensor`) — ignores level data, but
   still subject to P1.3. **P3 Manual overrides** (`FORCE_OFF` / `FORCE_ON`).4. **P4 Timed operation** (`COUNTDOWN`).5. **P5 Automation** (AUTO hysteresis + sleep rules).The v2.0 spec described these behaviors but v3.0 makes them explicit and
codified in `executePumpLogic()`.Dashboard implication: existing v2.0 UI remains valid; there is no change to
the public control contract, only clearer guarantees about which rule wins.

### 2.2 Level bypass & auto‑bypass

v3.0 extends level‑sensor bypass:
- Manual bypass (`bypass_level_sensor`) is persisted in NVS and remains until
  explicitly cleared.- When `auto_bypass_on_sensor_fail` is enabled and the sensor fails for longer
  than `auto_bypass_delay_sec`, firmware sets `auto_bypass_active = true` and
  behaves as if bypass is on.Dashboard should:
- Continue to display a “Maintenance active” banner when `bypass_level_sensor`.- Display a distinct “Auto‑maintenance active” banner when `auto_bypass_active`.
### 2.3 Countdown behavior

While countdown features existed conceptually in v2, v3.0 fully formalizes:
- Max countdown duration and add‑time behavior.- Early stop on full tank when bypass is off.- Auto‑revert to AUTO on expiry.Dashboard v2.0 already supports these semantics and requires no changes beyond
consuming `countdown_remaining_sec` and displaying add‑time state.

---

## 3. Dashboard Compatibility

The existing v2.0 dashboard implementation is **forward‑compatible** with v3.0
firmware provided that:

- It reads the new `is_level_sensor_error` / `is_flow_sensor_error` flags.- It treats `bypass_level_sensor` and `auto_bypass_active` as separate banners.- It uses the new telemetry fields where available to render estimated level,
  stale‑data age, and sensor health.Older dashboards that still rely solely on `is_sensor_error` will continue to
function but will:- Lose distinction between level vs flow sensor failures.- Not display the richer resilience UI.For new deployments, use the v2.0 dashboard code (or later) with v3.0 firmware.

