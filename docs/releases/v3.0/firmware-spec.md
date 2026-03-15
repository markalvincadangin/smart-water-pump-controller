# Firmware Spec · v3.0

Implemented firmware behavior for version **3.0.0 — Phase 5: Hierarchical
Priority Model + Sensor Resilience** of the Smart Water Pump Controller.

This document captures the parts of `FIRMWARE_REDESIGN_PLAN_v3.0.md` that are
now implemented in `platformio_smart_water_pump_controller/src/smart_water_pump_controller.ino`.

For the earlier v2 design, see `docs/releases/v2.0/firmware-rtdb-spec.md`.

---

## 1. Naming & Telemetry Changes

### 1.1 Sensor error naming

Ambiguous names from earlier versions have been replaced with explicit level vs
flow sensor fields:

- `isSensorError` → `isLevelSensorError`.- `cfgSensorFailureThreshold` → `cfgLevelSensorFailureThreshold`.- `sensorFailCount` → `levelSensorFailCount`.- `checkSensorFailure()` → `checkLevelSensorFailure()`.- RTDB status: `is_sensor_error` → `is_level_sensor_error` + `is_flow_sensor_error`.
Dashboard code reads the explicit v3 fields and falls back to legacy names only
for backward compatibility.

### 1.2 RTDB status additions (sensor resilience)

New v3 status fields (see pushes in `pushFirebaseStatus()`):

- `estimated_level_pct` (flow‑based volume estimate).- `level_estimate_active` (true when using estimate instead of sensor).- `flow_volume_added_l` (litres added since last good ultrasonic reading).- `level_last_valid_age_sec` (age of last valid ultrasonic reading in seconds).- `level_sensor_health_pct` (0–100 health score based on failures and age).- `total_pump_cycles` and `total_pump_run_min` (lifetime usage metrics).
These are consumed by the v2.0 dashboard UX (estimated level, stale‑data badge,
sensor health indicator) but are part of the v3 firmware feature set.

---

## 2. Hierarchical Priority Model

The pump state machine now evaluates control rules from highest to lowest
priority. Higher‑priority rules always override lower ones.

| Priority | Name              | Description |
|----------|-------------------|-------------|
| **P1**   | Hard Safety       | Dry‑run lockout and overflow protection. Cannot be bypassed by any mode; latched until `clear_error`. |
| **P2**   | Maintenance Bypass| Admin‑only level‑sensor bypass. Ignores level data for AUTO/COUNTDOWN; Flow Guard (P1) remains active. |
| **P3**   | Manual Overrides  | `FORCE_OFF` (Emergency Stop) and `FORCE_ON` (Manual/Emergency run). Checked before timed/auto logic. |
| **P4**   | Timed Operation   | `COUNTDOWN` mode; runs for a set duration, or until tank full (if bypass is off), then reverts to AUTO. |
| **P5**   | Automation        | Standard AUTO hysteresis; blocked by level sensor error unless bypass is active; sleep window suppression. |

### 2.1 Pump logic summary

Implementation outline (see `executePumpLogic()`):

1. **P1 — Hard safety:**   - If `isDryRunError` or `isOverflowError` is true:     - Force pump OFF (`setPump(false)` if running).     - Return immediately (no other rules run).2. **P3a — Emergency Stop (`FORCE_OFF`)**:   - If `pumpMode == "FORCE_OFF"`:     - Force pump OFF and return.3. **P3b — Manual / Emergency run (`FORCE_ON`)**:   - If `pumpMode == "FORCE_ON"`:     - Force pump ON (subject to P1) and return.4. **P4 — Countdown (`COUNTDOWN`)**:   - If `pumpMode == "COUNTDOWN"` and `isCountdownActive`:     - If bypass is **off** and `waterLevelPct >= cfgPumpStopLevel`:       - Stop pump, clear countdown, and revert mode to `"AUTO"`.     - Otherwise, keep pump ON until timer expiry handled elsewhere.5. **P5 — AUTO with sleep & bypass**:   - If `isSleeping` (within configured sleep window):     - If pump is running and `waterLevelPct >= cfgPumpStopLevel`, stop pump.     - Otherwise, do not auto‑start; return.   - If `cfgBypassLevelSensor` is true (manual bypass):     - Do not use level to start/stop; pump state is maintained by previous calls
       and hard safety only.   - If `isLevelSensorError` is true and bypass is **off**:     - Fail‑safe: stop pump and return.   - Otherwise, run standard hysteresis:     - Start pump when `!isRunning && waterLevelPct <= cfgPumpStartLevel`.     - Stop pump when `isRunning && waterLevelPct >= cfgPumpStopLevel`.
---

## 3. Manual Bypass & Auto‑Bypass

### 3.1 Manual level‑sensor bypass

Configuration and behavior:

- Controlled by `/pump_system/control/bypass_level_sensor`.- Firmware reads this flag each control poll and updates `cfgBypassLevelSensor`.- Status includes `bypass_level_sensor` so the dashboard can display a
  “Maintenance active” banner.- Bypass state is persisted in NVS (`bypass_lvl` key) so it survives reboot.
While bypass is active:

- AUTO and COUNTDOWN no longer use ultrasonic level to stop early.- Hard safety (dry‑run, overflow) remains in effect.- Flow‑based level estimation (Section 4) is used to present approximate level
  to the user.

### 3.2 Auto‑bypass on sensor failure

Controlled by device config:

- `auto_bypass_on_sensor_fail` (boolean).- `auto_bypass_delay_sec` (10–300 seconds).Behavior:- When level sensor failures exceed threshold for longer than
  `auto_bypass_delay_sec` and auto‑bypass is enabled, firmware:  - Sets `auto_bypass_active = true`.  - Enables level bypass internally while still honoring P1 hard safety.- Status field `auto_bypass_active` is true so the dashboard can show an
  “Auto‑maintenance active” banner distinct from manual bypass.- Auto‑bypass clears automatically when the level sensor recovers.

---

## 4. Sensor Resilience & Level Estimation

The v3 firmware can continue operating in a degraded but safe mode when the
ultrasonic level sensor is unreliable.

### 4.1 Flow‑based volume estimation

While the pump is running and level sensor is bypassed or failing, the firmware
uses YF‑G1 flow data to estimate how much water has been added to the tank.

High‑level algorithm:

```text
every sensor cycle (e.g. 1s):
  estimated_volume_added_L += flowRateLpm * (elapsedSec / 60.0)
  estimatedLevelPct = lastKnownLevelPct + (estimated_volume_added_L / TANK_CAPACITY_L * 100)
```

Where:

- `TANK_CAPACITY_L` is configured (e.g. 660 L).- `lastKnownLevelPct` is the last good ultrasonic reading.- `estimatedLevelPct` is clamped to [0, 100].
Firmware exposes this via:- `estimated_level_pct` (int) and `level_estimate_active` boolean.- `flow_volume_added_l` and `level_last_valid_age_sec`.
The dashboard:- Shows `~XX%` in amber with “Estimated · ±5%” label when `level_estimate_active`.- Shows “Level Xs old” when `level_last_valid_age_sec` grows large.- Uses `level_sensor_health_pct` to drive a sensor health indicator.

### 4.2 Sensor health score

`level_sensor_health_pct` is derived from:- Consecutive failure count (`levelSensorFailCount`).- Time since last valid reading (`level_last_valid_age_sec`).Failures and long gaps reduce the score from 100 down to a floor of 0. The
dashboard renders a simple green/amber/red indicator based on this value.

---

## 5. Countdown Mode

The v3 firmware implements a robust countdown mode, replacing ad-hoc timed runs.

Key behavior:

- **Starting**: `mode = "COUNTDOWN"` with `countdown_duration_min = N` starts a countdown of N minutes, sets `isCountdownActive = true` and records `countdownEndMs`. A `countdownConsumed` flag prevents re-triggering from stale RTDB values. Duration is persisted to NVS for offline fallback; if Firebase is unavailable, the last-known duration is used.
- **While active**: pump runs unless hard safety (P1), tank full (if bypass off), or timer expires. Firmware updates `countdown_remaining_sec` in status. Overflow protection (P1) now guards COUNTDOWN runs, preventing indefinite operation.
- **Add time**: `countdown_add_time` is **edge-triggered** (rising-edge via `lastAddTime` flag). The firmware writes `false` back to Firebase after processing (firmware-reset one-shot). The dashboard also has a 5-second fallback timeout. `lastAddTime` is reset when a new countdown starts to prevent stale edge state.
- **Stale mode protection (`pendingModeWriteback`)**: When the firmware locally reverts `pumpMode` to `"AUTO"` (manual_stop, expiry, or P4 early-stop), a `pendingModeWriteback` flag suppresses stale Firebase mode reads from overwriting the locally-set value. The flag clears when Firebase confirms the new value; retries the write if it hasn't landed.
- **On termination** (expiry, manual stop, FORCE_OFF, early stop, or P1 safety): `isCountdownActive` is cleared, `pumpMode` reverts to `"AUTO"`, and `pendingModeWriteback = true` blocks stale reads.
- **NVS persistence**: `"COUNTDOWN"` is a valid NVS mode. On reboot during countdown, mode is restored and timer restarts from Firebase or last-known duration.
- **Dashboard belt-and-suspenders**: `stopRun()` writes both `manual_stop = true` AND `mode = "AUTO"` to Firebase, so the mode value updates immediately.

The v2.0 dashboard exposes this via "Start Countdown" and "Add 5 min" buttons
with busy/timeout handling.

---

## 6. Run Mode Derivation

`run_mode` is a firmware-derived status field that tells the dashboard the
current operational state. It is computed at the **top** of `executePumpLogic()`
before any early-return path, ensuring it is always correct regardless of which
priority rule takes effect.

Derivation order (first match wins):

| Condition | `run_mode` |
|-----------|-----------|
| `isDryRunError \|\| isOverflowError` | `"OFF"` |
| `pumpMode == "FORCE_OFF"` | `"OFF"` |
| `pumpMode == "FORCE_ON"` | `"MANUAL"` |
| `pumpMode == "COUNTDOWN" && isCountdownActive` | `"COUNTDOWN"` |
| `pumpMode == "AUTO" && isRunning` | `"AUTO"` |
| `pumpMode == "AUTO" && !isRunning` | `"AUTO_STANDBY"` |
| `!isRunning` (fallback) | `"OFF"` |
| default | `"AUTO"` |

---

## 7. Implementation Notes & Edge Cases

- `run_mode` derivation runs before any priority-path return statement to
  guarantee the dashboard always has the correct state. Previous versions had
  this block at the end of `executePumpLogic()` where it was unreachable for
  FORCE_ON, FORCE_OFF, COUNTDOWN, sleep, and bypass paths.
- When the user disables bypass via the dashboard, both `autoBypassActive` and
  `autoBypassWasEngaged` are cleared to prevent stale state on sensor recovery.
- Countdown early stop only relies on level when bypass is off; with bypass
  enabled, early stop at 100% is suppressed.
- During sleep windows, countdown is not automatically started; active countdown
  runs are still allowed but subject to P1 safety.
- Auto‑bypass does not remove P1 lockouts: a dry‑run or overflow still latches
  `isDryRunError` / `isOverflowError` until `clear_error` is asserted.
- **Control read**: `readFirebaseControl()` fetches `/pump_system/control` as a
  single `getJSON` payload and parses mode, manual_stop, countdown_duration_min,
  countdown_add_time, manual_start, bypass_level_sensor, clear_error, and
  reboot_request_id from it. One round-trip per Firebase cycle reduces
  timeouts on weak WiFi (e.g. -79 dBm) where multiple separate reads would
  often trigger cooldown.

## 8. WiFi & Firebase Reconnection

The firmware handles three network recovery scenarios:

### 8.1 WiFi Down at Boot (Power Outage)

During power outages, the router typically takes 30-90 seconds to restart while
the ESP32 boots in ~5 seconds. If WiFi is unavailable during `setup()`:

1. `connectWiFi()` tries for 20 seconds (40 × 500 ms) then returns.
2. NTP and Firebase initialization are skipped (logged).
3. The main loop's WiFi recovery block retries with exponential backoff
   (5 s → 10 s → 20 s → 40 s → 60 s cap, ±2 s jitter).
4. On first successful connection, the `!wifiWasConnected` transition fires:
   - **Firebase late init**: `initFirebase()` is called (sets `firebaseInitialized = true`).
   - **NTP sync**: `configTime()` + `getLocalTime()` re-synced so sleep windows work.
5. Normal Firebase sync resumes on the next `firebaseInterval` tick.

### 8.2 WiFi Drops Mid-Operation

When WiFi drops while the ESP32 is running:

1. `WiFi.status() != WL_CONNECTED` triggers the backoff retry loop.
2. `WiFi.disconnect(false)` resets the radio without clearing stored credentials,
   followed by `WiFi.mode(WIFI_STA)` and `WiFi.begin()` for a clean reconnection.
   `WiFi.persistent(false)` is set during initial connect to avoid unnecessary
   flash writes since credentials come from `secrets.h`.
3. On reconnection, `Firebase.refreshToken(&config)` is called to renew the
   auth token (which may have expired during the outage).
4. NTP is re-synced to correct any drift.
5. `firebaseCooldownUntilMs` is set to `millis() + 10s` (not 0) to give the
   token 10 seconds to refresh before the next Firebase operation. This prevents
   the immediate post-reconnect Firebase call from failing due to an unrefreshed
   token, which would otherwise trigger a longer 30s cooldown.
6. Network timeout cooldown is 30 seconds (reduced from 120s), but only engages
   after 3 consecutive failures. Single timeouts trigger a short 1-second retry
   to keep the dashboard current during brief network hiccups.

### 8.3 Dashboard-Initiated Restart

When the dashboard sends a reboot request:

1. The firmware acknowledges the request to Firebase, then calls `ESP.restart()`.
2. On reboot, `setup()` runs the full initialization sequence including WiFi
   and Firebase. If WiFi is available, the device reconnects within seconds.
3. If WiFi is temporarily unavailable (unlikely for dashboard restart), the
   late-init path from §8.1 handles recovery.

