> **Superseded by:** [./releases/v2.0/firmware-rtdb-spec.md](./releases/v2.0/firmware-rtdb-spec.md) — canonical v2 firmware/RTDB spec.

---

# Firmware–Dashboard Design v2.0
### Smart Water Pump Controller — ESP32 + Next.js + Firebase RTDB

| Field | Value |
|---|---|
| **Version** | 2.0 — Audited & Redesigned |
| **Previous version** | FIRMWARE_DASHBOARD_DESIGN.md (original) |
| **Author** | Mark Alvin Cadangin |
| **Date** | March 2026 |

---

## Audit Report — Issues Found in v1.0

Before the redesign, a full audit was performed against the original document. All issues are catalogued here so each fix can be traced to a root cause.

### A. Errors

**A1 — Priority numbering gap (Section 7)**
The priority list jumps from P1 directly to P3 twice. P2 (Maintenance Bypass) is described in firmware documentation but is missing entirely from the dashboard design document's pump logic section. This is not just a typo — it means the document is structurally incomplete for a reader implementing the dashboard, who would not know that P2 exists or how to represent it in the UI.

**A2 — Redundant dual fields: `bypass_level_sensor` and `is_maintenance_active` (Sections 4, 8)**
Both fields are pushed in status and both mean the same thing. The document says so explicitly: *"Same as `bypass_level_sensor`"*. One should be the canonical field; the other should be removed. Having both creates a synchronisation risk — future firmware changes may update one and not the other, causing dashboard state divergence.

**A3 — `run_mode` update logic is incomplete (Section 7, last paragraph)**
The document says: *"run_mode is then updated for dashboard: MANUAL kept as-is, COUNTDOWN when in countdown, OFF when pump off, AUTO when in AUTO and running."*
This is missing the case where the pump is in AUTO mode but **not running** (waiting for level to drop to the start threshold). In that state, `run_mode = "AUTO"` is correct but the description implies AUTO only applies when running. A dashboard implementing this literally would show "OFF" when the pump is in standby AUTO, which is wrong — it should show "AUTO (Standby)".

**A4 — `manual_stop` sets `mode = "FORCE_OFF"` — unintended side effect**
Section 6.2 states: *"manual_stop → firmware sets pump off and mode = 'FORCE_OFF'"*. This means that after a user taps Stop on a Manual run, the system enters FORCE_OFF — meaning it will also block AUTO from running. The user must then manually switch back to AUTO to restore normal operation. This is not documented as a required step, so a user stopping a manual run would be confused when the tank doesn't fill automatically afterwards. Either the behavior should be changed (Stop on a Manual run reverts to AUTO, not FORCE_OFF), or it must be explicitly documented and surfaced in the UI.

**A5 — `last_fault_code` / `last_fault_message` not defined**
Section 4 lists these as status fields but neither their format, possible values, nor how the firmware sets them is specified anywhere. A dashboard developer cannot implement an error display without knowing what codes exist or what the strings look like.

### B. Ambiguities

**B1 — "Emergency override" has no Stop button — but this is not surfaced to the user**
Section 6.2 states: *"No Stop in 'Run Pump'; user stops by switching mode to FORCE_OFF or AUTO."* This is a significant UX gap. A non-technical user who sets Emergency Override and wants to stop has no obvious action. The document notes it as intended but does not specify how the dashboard should guide the user to stop it.

**B2 — One-shot reset timing is unspecified**
Section 3 says the dashboard sets `manual_start` or `countdown_add_time` to `true` *"then (after a short delay) resets to false"*. "Short delay" is not defined. If the delay is too short, the firmware (polling every ~3s) may not have read the value before it resets. If too long, the user could double-tap and trigger twice. A concrete value (e.g. 5 seconds) should be specified.

**B3 — Bypass behavior in COUNTDOWN mode is ambiguous**
Section 8 says bypass ignores level *"in AUTO"*. But Section 4 of the priority model (P4 — COUNTDOWN) says: *"pump on unless tank ≥ stop level (and bypass off)"*. So bypass also affects COUNTDOWN. The document describes bypass as "in AUTO" but it actually affects COUNTDOWN too. The scope of bypass is understated.

**B4 — `admins` key has no documented schema or access control enforcement**
Section 2 lists `/pump_system/config/admins` as `{ uid: true }` and notes it controls who can use FORCE_ON, restart, etc. But Section 11 (Dashboard Components) doesn't mention any admin-check logic in `usePumpData`, and the Firebase security rules are not referenced. A developer reading this cannot determine how admin gating is enforced client-side.

**B5 — `presence` node is described as "optional" with no further spec**
It appears in the RTDB layout with no description of schema, purpose, or who reads/writes it. Either document it or remove it from the layout.

**B6 — `sensor_failure_threshold` in config references old naming**
The firmware redesign plan renamed `cfgSensorFailureThreshold` to `cfgLevelSensorFailureThreshold`. The config field name `sensor_failure_threshold` is ambiguous — it is unclear whether it refers to the level sensor or the flow sensor. Should be `level_sensor_failure_threshold`.

### C. Inconsistencies

**C1 — `usePumpData` exposes `setMode` but Mode controls are described separately**
Section 11 lists `setMode` in `usePumpData`, but the Mode controls section says they set `control/mode`. The hook should be the single API for all control writes, but the document treats mode changes and run controls as separate concerns without clearly mapping which hook function is called for each action.

**C2 — Control table (Section 3) omits `run_mode` from control path but it appears in status**
`run_mode` is a status field (Section 4) that the firmware derives. But Section 7 says *"run_mode is then updated for dashboard"* as if the firmware writes it to a control-adjacent path. Clarify: `run_mode` is firmware-derived and lives only in `/status`, it is never written by the dashboard.

**C3 — "Auto-bypass" mentioned in Section 8 but not in Section 3 or 11**
Section 8 says firmware can auto-enable bypass but it's *"not exposed on dashboard"*. However, Section 4 lists `auto_bypass_active` as a status field (per the redesign plan). If the field exists in status, the dashboard must display it — even if the control is not exposed. The document is inconsistent on whether this is surfaced to the user at all.

**C4 — Firmware file reference `03_safety_pump.ino` does not match the project structure**
Section 7 references `03_safety_pump.ino` as where `executePumpLogic()` lives, but the agreed project structure uses a single `smart_pump_controller.ino` file. This is either outdated or refers to a future refactor that hasn't been documented.

### D. UX/UI Design Issues

**D1 — No visual hierarchy or grouping model defined**
The document lists components (Section 11) as a flat bullet list with no grouping, visual hierarchy, or information architecture. A dashboard for a safety-critical physical system needs a defined priority of what the user sees first (current state), second (controls), and third (diagnostics). Without this, implementations will vary and may bury critical status behind controls.

**D2 — Error and warning states have no defined dismissal or escalation model**
Multiple error states exist (`is_error`, `is_level_sensor_error`, `is_flow_sensor_error`, `is_overflow_error`, `is_maintenance_active`) but there is no specification of how they are visually ranked, whether they stack, whether they auto-dismiss, or what the user's recovery path is for each one. A user seeing three simultaneous error banners with no hierarchy would not know which to address first.

**D3 — No feedback model for latency**
All control writes go through Firebase → firmware poll cycle (~3s). The document has no specification for optimistic UI, loading states, or confirmation that the firmware received and applied a command. A user tapping "Stop" and seeing no immediate visual change for up to 3 seconds may assume the tap failed and tap again.

**D4 — No offline/disconnected state defined**
The document specifies "controller online" as status update within last 20s, but there is no spec for what the UI shows when: (a) the dashboard itself loses internet, (b) the ESP32 loses WiFi but the dashboard is online, or (c) both are online but Firebase is degraded.

**D5 — Mobile-first considerations absent**
The document is written with no reference to screen size, touch targets, or mobile layout. The system is deployed at a residential property in the Philippines — the primary access device is almost certainly a mobile phone. Touch targets for critical controls (Stop, Emergency Stop) need minimum 48×48px specification. Controls that may be tapped under stress (emergency conditions) need extra size and confirmation affordances.

---

## Redesigned Document

The redesigned document below fixes all issues above. Changes from v1.0 are marked with `[FIX Ax]`, `[FIX Bx]`, `[FIX Cx]`, `[FIX Dx]` referencing the audit findings.

---

# Firmware–Dashboard Design v2.0

This document is the authoritative reference for how the ESP32 firmware and Next.js dashboard communicate via Firebase Realtime Database (RTDB), and how the dashboard is structured to present that data clearly and safely to the operator.

---

## 1. Overview

**Three-layer architecture:**

```
[ESP32 Firmware]  ←──read──  [Firebase RTDB]  ←──write──  [Next.js Dashboard]
[ESP32 Firmware]  ──write──►  [Firebase RTDB]  ──read──►   [Next.js Dashboard]
```

- **Firmware** (ESP32 / Arduino): reads sensors (JSN-SR04T ultrasonic level, YF-G1 flow), controls the pump relay via magnetic contactor, and syncs with Firebase every ~3 seconds.
- **Dashboard** (Next.js 14 PWA): operators sign in with Google OAuth, monitor system status in real time, and issue control commands.
- **Bridge** (Firebase RTDB): the only communication channel between firmware and dashboard. No direct TCP connection exists.

**Write ownership:**

| Actor | Writes to | Reads from |
|---|---|---|
| Dashboard | `/control/*`, `/config/device`, `/audit/events` | `/status`, `/config/device`, `/config/admins` |
| Firmware | `/status` | `/control/*`, `/config/device` |

The firmware writes to `/control/mode` only to revert it to `"AUTO"` when a countdown expires. All other control writes are one-directional: dashboard → Firebase → firmware.

---

## 2. Firebase RTDB Layout

```
/pump_system/
├── control/                          # Dashboard → ESP32 (polled every ~3s)
│   ├── mode                          # string: "AUTO"|"FORCE_OFF"|"FORCE_ON"|"COUNTDOWN"
│   ├── clear_error                   # boolean one-shot: clears dry-run + overflow lockouts
│   ├── reboot_request_id             # number: new value triggers ESP32 soft restart
│   ├── manual_start                  # boolean one-shot: starts a Manual run
│   ├── manual_stop                   # boolean one-shot: stops Manual run, reverts to AUTO  [FIX A4]
│   ├── countdown_duration_min        # number 1–120: duration when entering COUNTDOWN
│   ├── countdown_add_time            # boolean one-shot: adds 5 min to running countdown
│   └── bypass_level_sensor           # boolean: Admin maintenance toggle
│
├── status/                           # ESP32 → Dashboard (pushed as single JSON ~every 3s)
│   └── { ... see Section 4 }
│
├── config/
│   ├── device/                       # Shared calibration & thresholds
│   │   └── { ... see Section 5 }
│   ├── admins/                       # { uid: true } — admin access list
│   └── notifications_by_user/{uid}/  # Per-user push notification preferences
│       ├── on_dry_run_error          # boolean
│       ├── on_tank_full              # boolean
│       └── on_controller_offline     # boolean
│
├── audit/
│   └── events/                       # Append-only log; dashboard writes, dashboard reads
│       └── {push_id}/
│           ├── ts                    # number: Unix timestamp ms
│           ├── uid                   # string: operator uid
│           ├── action                # string: e.g. "mode_change", "manual_start"
│           └── detail                # string: human-readable description
│
└── presence/                         # [REMOVED — undocumented, unused]  [FIX B5]
```

> **Removed:** The `presence/` node has been removed. It was listed in v1.0 with no schema, no purpose, and no reader/writer. If real-time presence is needed in future, it should be designed explicitly.

---

## 3. Control Path (Dashboard → Firebase → ESP32)

The dashboard writes to `/pump_system/control/*`. The firmware polls all control keys in `readFirebaseControl()` on each Firebase cycle (~3s).

| Key | Type | Purpose |
|---|---|---|
| `mode` | `string` | Policy mode: `"AUTO"` \| `"FORCE_OFF"` \| `"FORCE_ON"` \| `"COUNTDOWN"` |
| `clear_error` | `boolean` | **One-shot.** Clears dry-run and overflow lockouts when `true`. Firmware resets to `false` after applying. |
| `reboot_request_id` | `number` | Non-zero value triggers ESP32 soft restart. Firmware persists last processed ID in NVS to prevent restart loops on reboot. |
| `manual_start` | `boolean` | **One-shot.** Starts a Manual run (`FORCE_ON` + `run_mode = "MANUAL"`). Firmware does not reset this flag — dashboard resets after 5 seconds. |
| `manual_stop` | `boolean` | **One-shot.** Stops a Manual run and **reverts `mode` to `"AUTO"`** (not `FORCE_OFF`). [FIX A4] Firmware does not reset this flag — dashboard resets after 5 seconds. |
| `countdown_duration_min` | `number` | Duration (1–120 min) for the next COUNTDOWN run. Set before writing `mode = "COUNTDOWN"`. |
| `countdown_add_time` | `boolean` | **One-shot.** Adds 5 minutes to the active countdown. Firmware resets to `false` after applying. Dashboard should not allow tapping again until `false` is confirmed. |
| `bypass_level_sensor` | `boolean` | Admin maintenance toggle. Persists until explicitly cleared. Firmware reads and applies immediately. |

### One-Shot Timing [FIX B2]

One-shot flags (`manual_start`, `manual_stop`) use a **5-second reset delay** on the dashboard side. This provides two full firmware poll cycles (3s each) to guarantee the firmware reads the flag before it resets. The dashboard must disable the triggering button for the full 5 seconds to prevent double-fire.

```
User taps "Start"
  → Dashboard writes manual_start = true
  → Dashboard disables Start button (5s)
  → Firmware reads manual_start = true on next poll (~0–3s)
  → Firmware applies, sets run_mode = "MANUAL"
  → Dashboard writes manual_start = false at T+5s
  → Dashboard re-enables button (or updates to Stop state)
```

`countdown_add_time` resets are handled by the firmware (it sets the flag back to `false` after applying). The dashboard should disable the "Add 5 min" button until it receives the `false` value back from Firebase status, confirming the firmware has processed it.

---

## 4. Status Path (ESP32 → Firebase → Dashboard)

The firmware pushes a single JSON object to `/pump_system/status` on each cycle. The dashboard subscribes with Firebase `onValue` and derives a full UI snapshot from this single object plus a locally-tracked `updatedAt` timestamp.

**Controller online detection:** If no status update is received within **20 seconds**, the dashboard shows a "Controller offline" warning and disables all control buttons. This prevents sending commands into the void.

### 4.1 Full Status Schema

| Field | Type | Description |
|---|---|---|
| `water_level_percent` | `number` | 0–100. From ultrasonic sensor, or flow-based estimate when bypass is active and sensor has failed. |
| `estimated_level_pct` | `number` | Flow-based level estimate. `-1` if not yet initialized. Shown separately from `water_level_percent` when `level_estimate_active` is true. |
| `level_estimate_active` | `boolean` | `true` when flow-based estimate is being used (bypass ON + sensor failed). Dashboard should label the level display as "Estimated". |
| `flow_volume_added_l` | `number` | Litres added since last level sensor anchor. Shown in maintenance diagnostic panel. |
| `level_last_valid_age_sec` | `number` | Seconds since last successful ultrasonic reading. Dashboard shows "Level data Xs old" badge when > 30s. |
| `level_sensor_health_pct` | `number` | 0–100 sensor health score. Dashboard shows signal-bar indicator. |
| `is_running` | `boolean` | Pump relay is energized. |
| `run_mode` | `string` | `"OFF"` \| `"AUTO_STANDBY"` \| `"AUTO"` \| `"MANUAL"` \| `"COUNTDOWN"` — see Section 6.3. [FIX A3] |
| `flow_rate_lpm` | `number` | Litres per minute from YF-G1 flow sensor. |
| `countdown_remaining_sec` | `number` | Seconds left in active countdown. `0` when not in countdown mode. |
| `is_error` | `boolean` | Dry-run lockout active. Pump will not run until cleared. |
| `is_level_sensor_error` | `boolean` | JSN-SR04T ultrasonic failure (consecutive timeouts). |
| `is_flow_sensor_error` | `boolean` | YF-G1 stuck-high (reporting flow when pump is off). |
| `is_overflow_error` | `boolean` | MAX AUTO runtime exceeded. |
| `bypass_level_sensor` | `boolean` | Level sensor bypass (maintenance) active. [FIX A2 — `is_maintenance_active` removed as duplicate] |
| `auto_bypass_active` | `boolean` | `true` if firmware auto-engaged bypass due to sensor failure (not operator-toggled). Dashboard shows distinct "Auto-maintenance" indicator. |
| `is_sleeping` | `boolean` | Controller within scheduled sleep window. AUTO start is suppressed. |
| `last_fault_code` | `string` | See fault code table in Section 4.2. [FIX A5] |
| `last_fault_message` | `string` | Human-readable fault description. e.g. `"Dry-run detected after 30s. Pump stopped."` |
| `total_pump_cycles` | `number` | Lifetime pump start count. |
| `total_pump_run_min` | `number` | Lifetime pump runtime in minutes. |
| `wifi_rssi` | `number` | WiFi signal strength in dBm. |
| `uptime_minutes` | `number` | Minutes since last ESP32 boot. |
| `last_boot_reason` | `string` | Human-readable boot reason (e.g. `"Power-on"`, `"Watchdog"`, `"Software reset"`). |

> **Removed:** `is_maintenance_active` — duplicate of `bypass_level_sensor`. Dashboard reads `bypass_level_sensor` only. [FIX A2]

### 4.2 Fault Code Reference [FIX A5]

| `last_fault_code` | Meaning | User-facing message | Recovery |
|---|---|---|---|
| `"DRY_RUN"` | Flow below threshold for 30s while pump was running | "Dry-run detected. Check water supply." | Tap "Clear Error" after resolving |
| `"OVERFLOW"` | Max AUTO runtime exceeded without reaching stop level | "Max runtime exceeded. Check tank sensor." | Tap "Clear Error" after inspecting |
| `"LEVEL_SENSOR"` | Consecutive ultrasonic timeouts ≥ threshold | "Level sensor offline." | Auto-clears on recovery; enable bypass for interim |
| `"FLOW_SENSOR"` | YF-G1 stuck-high (flow when pump is off) | "Flow sensor reading abnormal." | Auto-clears on recovery |
| `"SAFE_MODE"` | Crash loop detected (5+ reboots in 5 min) | "Controller in safe mode. Power cycle to recover." | Full power cycle |
| `""` (empty) | No fault has occurred | — | — |

---

## 5. Device Config (Shared)

- **Path**: `/pump_system/config/device`
- **Written by**: Dashboard (admin users only — checked against `/config/admins`)
- **Read by**: Firmware (every 30s); also read by dashboard for display in settings modal

### 5.1 Config Fields

| Field | Type | Description |
|---|---|---|
| `tank_empty_cm` | `number` | Distance (cm) from sensor to tank bottom — represents 0% level |
| `tank_full_cm` | `number` | Distance (cm) from sensor to water when tank is full — represents 100% |
| `pump_start_level` | `number` | AUTO starts pump when level ≤ this % (default: 30) |
| `pump_stop_level` | `number` | AUTO stops pump when level ≥ this % (default: 100) |
| `dry_run_threshold_lpm` | `number` | Flow below this = dry-run condition (default: 0.5 L/min) |
| `dry_run_timeout_sec` | `number` | Seconds of low flow before lockout triggers (default: 30) |
| `flow_calibration_factor` | `number` | YF-G1 K-factor (default: 7.5). Verify with bucket test. |
| `max_pump_runtime_min` | `number` | Max continuous AUTO runtime before overflow protection triggers (default: 120) |
| `sleep_enabled` | `boolean` | Enable scheduled sleep window |
| `sleep_start_hour` | `number` | Hour (0–23, PHT) to begin sleep window |
| `sleep_end_hour` | `number` | Hour (0–23, PHT) to end sleep window |
| `sleep_emergency_level` | `number` | % below which sleep is overridden for emergency fill |
| `level_sensor_failure_threshold` | `number` | Consecutive timeouts before `isLevelSensorError` is set (default: 5) [FIX B6] |
| `idle_sensor_interval_ms` | `number` | Sensor poll interval during idle (tank full, pump off) |
| `idle_firebase_interval_ms` | `number` | Firebase push interval during idle |

---

## 6. Modes, Run Types, and `run_mode`

This section is the single authoritative reference for all pump operating states. The firmware, dashboard hook (`usePumpData`), and UI components must all derive state from this model.

### 6.1 Policy Mode (`control/mode`)

The policy mode is the firmware's current operating rule. It is written by the dashboard and readable by all clients.

| Mode | Firmware behavior | Who can set it |
|---|---|---|
| `"AUTO"` | Hysteresis control: start at ≤ `pump_start_level`, stop at ≥ `pump_stop_level`. Sleep rules apply. Sensor error causes fail-safe stop unless bypass is active. | Any signed-in user |
| `"FORCE_OFF"` | Pump off regardless of level. Blocks AUTO start. Used as emergency stop. | Any signed-in user |
| `"FORCE_ON"` | Pump on continuously. Subject to P1 (dry-run) only. Used for Emergency Override (admin). | Admin only |
| `"COUNTDOWN"` | Run for `countdown_duration_min`. Reverts to `"AUTO"` on expiry or tank full (if bypass off). | Any signed-in user |

### 6.2 Run Types (User-Facing)

These are the four ways a user initiates pump operation. They map to policy modes but with different UI semantics.

| Run Type | User action | What dashboard writes | How it stops | `run_mode` value |
|---|---|---|---|---|
| **AUTO run** | None — system handles it | — | Level reaches stop threshold | `"AUTO"` |
| **Manual run** | Tap "Quick Start" | `manual_start = true` | Tap "Stop" → `manual_stop = true` → reverts to `"AUTO"` [FIX A4] | `"MANUAL"` |
| **Countdown run** | Select duration, tap "Start Countdown" | `countdown_duration_min = N`, then `mode = "COUNTDOWN"` | Timer expires (auto-reverts to AUTO) or tank full | `"COUNTDOWN"` |
| **Emergency Override** | Tap "Emergency Override" (admin only) | `mode = "FORCE_ON"` | Tap "Emergency Stop" → `mode = "FORCE_OFF"`, then manually switch to AUTO | `"MANUAL"` |

> **Important:** Manual run and Emergency Override both use `FORCE_ON` in firmware; the firmware reports `run_mode = "MANUAL"` for both (dashboard never writes `run_mode`). Emergency Override does not show a "Stop" button in Run Controls — it shows "Emergency Stop" in the Mode Controls section, which sets `FORCE_OFF`. This must be clearly communicated to the user when they activate Emergency Override.

### 6.3 `run_mode` Values [FIX A3]

`run_mode` is derived by the firmware and pushed to `/status`. It is never written by the dashboard. The dashboard uses it for display only.

| `run_mode` value | Meaning | Pump state |
|---|---|---|
| `"OFF"` | Pump is off; system not in AUTO | Off |
| `"AUTO_STANDBY"` | Mode is AUTO; pump is off; waiting for level to drop to start threshold | Off |
| `"AUTO"` | Mode is AUTO; pump is on, filling tank | On |
| `"MANUAL"` | Pump on due to Manual run or Emergency Override | On |
| `"COUNTDOWN"` | Pump on; countdown timer is active | On |

> `"AUTO_STANDBY"` is new in v2.0. In v1.0, the pump-off AUTO state was ambiguously represented as either `"OFF"` or `"AUTO"`. A user seeing the tank at 45% with `run_mode = "AUTO"` but no pump running would be confused. `"AUTO_STANDBY"` makes the system's intent explicit: "I am watching — I'll start when the level drops to 30%."

---

## 7. Pump Priority Model (Firmware Reference)

`executePumpLogic()` evaluates states in strict priority order. Higher priority always wins. [FIX A1 — P2 added]

| Priority | Name | Condition | Pump action |
|---|---|---|---|
| **P1** | Hard Safety | `isDryRunError` OR `isOverflowError` | **OFF** — permanent until `clear_error`. Cannot be bypassed. |
| **P2** | Maintenance Bypass | `cfgBypassLevelSensor == true` AND mode is AUTO | Level data ignored for start/stop. P1 still applies. Pump state unchanged by P2 itself. |
| **P3a** | Emergency Stop | `pumpMode == "FORCE_OFF"` | **OFF** |
| **P3b** | Manual / Emergency Run | `pumpMode == "FORCE_ON"` | **ON** (P1 still applies) |
| **P4** | Countdown | `pumpMode == "COUNTDOWN"` AND `isCountdownActive` | **ON** until timer or tank full (if bypass off). Reverts to AUTO on expiry. |
| **P5** | AUTO Hysteresis | `pumpMode == "AUTO"` | Start when level ≤ start%; stop when level ≥ stop%. Fail-safe off on sensor error (unless P2 bypass active). Sleep suppresses auto-start. |

**Dashboard implication:** The dashboard does not need to re-implement this logic. It reads `run_mode`, `is_running`, and the error flags from `/status` and renders state. It does not infer pump state from mode alone.

---

## 8. Level Sensor Bypass

### 8.1 Manual Bypass (Operator-Toggled)

- **Written by**: Dashboard (`/control/bypass_level_sensor = true`) — admin users only
- **Read by**: Firmware (applies immediately on next poll); status reflects it as `bypass_level_sensor`
- **Scope**: Affects P2 (AUTO level decisions) and P4 (COUNTDOWN early-stop at tank full). Does NOT affect P1. [FIX B3]
- **Dashboard display**: Persistent amber "Maintenance Active" banner while `bypass_level_sensor` is `true`
- **Dashboard control**: Toggle in Device Settings → Maintenance tab (admin only)

### 8.2 Auto-Bypass (Firmware-Triggered) [FIX C3]

- **Triggered by**: Firmware when `cfgAutoBypassOnSensorFail` is enabled and level sensor fails for > `cfgAutoBypassDelaySec` seconds
- **Status field**: `auto_bypass_active: true` in `/status`
- **Dashboard display**: Distinct "Auto-Maintenance Active" banner (different color/icon from manual bypass) with explanation: "Level sensor offline. System switched to flow-only mode automatically."
- **Clearance**: Auto-clears when sensor recovers. Manual bypass does not auto-clear.
- **Dashboard control**: `cfgAutoBypassOnSensorFail` is exposed in Device Settings → Advanced as an admin toggle. Default: `false`.

---

## 9. Admin Access Model [FIX B4]

### 9.1 Admin Determination

Admin status is determined client-side by checking `/pump_system/config/admins/{uid}`. Firebase Security Rules enforce this server-side for write operations.

```typescript
// usePumpData hook
const isAdmin = admins[currentUser.uid] === true;
```

### 9.2 Feature Gating by Role

| Feature | Non-admin (signed in) | Admin |
|---|---|---|
| View status, level, flow | ✅ | ✅ |
| Switch mode: AUTO / FORCE_OFF | ✅ | ✅ |
| Quick Start (Manual run) | ✅ | ✅ |
| Countdown run | ✅ | ✅ |
| Add time to countdown | ✅ | ✅ |
| Clear error | ✅ | ✅ |
| Emergency Override (FORCE_ON) | ❌ | ✅ |
| Toggle bypass_level_sensor | ❌ | ✅ |
| Edit device config | ❌ | ✅ |
| Request reboot | ❌ | ✅ |
| View audit log | ✅ | ✅ |

Non-admin users who attempt admin actions should see a clear message: "Admin access required." Buttons they cannot use should be disabled (not hidden), with a tooltip explaining why.

### 9.3 Firebase Security Rules (Reference)

```json
{
  "rules": {
    "pump_system": {
      "status": {
        ".read": "auth != null",
        ".write": false
      },
      "control": {
        ".read": "auth != null",
        ".write": "auth != null",
        "bypass_level_sensor": {
          ".write": "root.child('pump_system/config/admins').child(auth.uid).val() === true"
        },
        "reboot_request_id": {
          ".write": "root.child('pump_system/config/admins').child(auth.uid).val() === true"
        }
      },
      "config": {
        "device": {
          ".read": "auth != null",
          ".write": "root.child('pump_system/config/admins').child(auth.uid).val() === true"
        },
        "admins": {
          ".read": "auth != null",
          ".write": false
        }
      },
      "audit": {
        "events": {
          ".read": "auth != null",
          ".write": "auth != null"
        }
      }
    }
  }
}
```

---

## 10. Restart, Errors, and Audit

### 10.1 Restart Sequence

```
Dashboard writes reboot_request_id = Date.now()
  → Firmware detects new ID (differs from NVS-persisted last ID)
  → Firmware persists new ID to NVS
  → Firmware writes status/last_reboot_request_id = new ID  (acknowledgement)
  → Firmware calls ESP.restart()
  → Dashboard sees status update stop → shows "Restarting..." state
  → Dashboard polls for status to resume (online detection: update within 20s)
  → On reconnect, uptime_minutes resets to 0 (confirms restart)
```

### 10.2 Error Acknowledgement Sequence

```
Error occurs (e.g. dry-run) → firmware sets is_error = true, last_fault_code = "DRY_RUN"
  → Dashboard shows error banner with fault message and "Clear Error" button
  → Operator resolves root cause (e.g. checks water supply)
  → Operator taps "Clear Error"
  → Dashboard writes clear_error = true
  → Firmware clears error flags, sets clear_error = false
  → Dashboard status updates: is_error = false
  → Error banner dismisses
```

### 10.3 Audit Events

Dashboard writes to `/pump_system/audit/events/{pushId}` for every control action. The firmware does not use audit.

**Audit event schema:**

```typescript
interface AuditEvent {
  ts: number;           // Unix timestamp ms (Date.now())
  uid: string;          // Operator's Firebase Auth UID
  action: string;       // Machine-readable action key
  detail: string;       // Human-readable description
}
```

**Standard audit action keys:**

| `action` | `detail` example |
|---|---|
| `"mode_change"` | `"Mode changed from AUTO to FORCE_OFF"` |
| `"manual_start"` | `"Manual run started"` |
| `"manual_stop"` | `"Manual run stopped"` |
| `"countdown_start"` | `"Countdown started: 15 min"` |
| `"countdown_add_time"` | `"5 min added to countdown (was 8:23 remaining)"` |
| `"error_cleared"` | `"Dry-run error acknowledged and cleared"` |
| `"bypass_enabled"` | `"Level sensor bypass enabled (maintenance mode)"` |
| `"bypass_disabled"` | `"Level sensor bypass disabled"` |
| `"reboot_requested"` | `"Controller reboot requested"` |
| `"config_saved"` | `"Device config updated: pump_start_level=25, pump_stop_level=100"` |

---

## 11. UX/UI Design Model [FIX D1–D5]

This section defines the interaction model, information hierarchy, visual states, and mobile-first layout for the dashboard. These are design requirements, not suggestions.

### 11.1 Information Hierarchy

The dashboard organizes information into four layers, presented top-to-bottom on mobile and in a grid on desktop. The user always sees the most critical information without scrolling.

```
┌─────────────────────────────────┐
│  LAYER 1: SYSTEM STATE          │  Always visible. Never behind a scroll.
│  Tank level + pump status       │  Primary visual. Largest element.
│  Run mode badge + online status │
├─────────────────────────────────┤
│  LAYER 2: ALERTS                │  Appears only when alerts exist.
│  Error banners (ranked)         │  Dismissal requires action, not a swipe.
│  Maintenance / sleep banners    │
├─────────────────────────────────┤
│  LAYER 3: CONTROLS              │  Run controls + mode controls.
│  Contextual — shows only what   │  Admin controls gated and labeled.
│  is valid in the current state  │
├─────────────────────────────────┤
│  LAYER 4: DIAGNOSTICS           │  Collapsible on mobile.
│  Flow rate, RSSI, uptime,       │  Expanded by default on desktop.
│  sensor health, audit log       │
└─────────────────────────────────┘
```

### 11.2 Visual State Model

Each system state maps to a distinct visual treatment applied to the Layer 1 tank indicator and status badge. States are mutually exclusive in their primary color — the user always knows at a glance what the system is doing.

| State | Tank indicator color | Badge text | Badge color |
|---|---|---|---|
| AUTO Standby | Blue (calm) | "AUTO — Standby" | Blue |
| AUTO Running (filling) | Animated blue fill rising | "Filling…" | Green |
| Manual Running | Solid green | "Manual Run" | Green |
| Countdown Running | Solid green + countdown ring | "Xs remaining" | Green |
| FORCE_OFF (idle) | Gray | "Stopped" | Gray |
| Sleeping | Indigo | "Sleeping — resumes HH:MM" | Indigo |
| Maintenance Active | Amber | "Maintenance Mode" | Amber |
| Level sensor error | Amber (not red — pump may still work) | "Level sensor offline" | Amber |
| Dry-run error | Red | "Dry-run lockout — pump off" | Red |
| Overflow error | Red | "Max runtime exceeded" | Red |
| Controller offline | Dark red / charcoal | "Controller offline" | Dark red |

**Color semantics** follow a traffic-light model with one extension:
- 🔴 **Red**: Pump is stopped due to a safety lockout. Operator action required.
- 🟡 **Amber**: Degraded state — system is operating but with reduced capability or monitoring.
- 🟢 **Green**: Pump is running normally.
- 🔵 **Blue**: System is in AUTO, healthy, not currently running.
- ⬛ **Gray**: System is deliberately stopped (FORCE_OFF).
- 🟣 **Indigo**: Sleep mode — scheduled, expected.

### 11.3 Alert Ranking and Display [FIX D2]

When multiple conditions are active simultaneously, alerts are displayed in ranked order. Each alert is a distinct card, not a combined message.

**Rank order (highest to lowest):**

1. 🔴 Controller offline
2. 🔴 Dry-run lockout (`is_error`)
3. 🔴 Overflow error (`is_overflow_error`)
4. 🟡 Auto-Maintenance active (`auto_bypass_active`)
5. 🟡 Maintenance active (`bypass_level_sensor`)
6. 🟡 Level sensor error (`is_level_sensor_error`)
7. 🟡 Flow sensor error (`is_flow_sensor_error`)
8. 🔵 Sleeping (`is_sleeping`)

Each alert card contains:
- **Icon + title** (e.g. "🛑 Dry-Run Lockout")
- **One-line description** (e.g. "No water flow detected for 30s. Pump has been stopped.")
- **Recovery action button** where applicable (e.g. "Clear Error", "Enable Bypass", "View diagnostics")

Red alerts (1–3) use a top-of-page fixed banner. Yellow alerts (4–7) use inline cards below the tank indicator. Blue alerts (8) use a subtle inline badge only.

### 11.4 Control Context Model [FIX D3]

Controls are **contextual** — only valid controls are shown at any time. This prevents invalid actions and reduces cognitive load.

**Control states by system state:**

| System state | Visible controls |
|---|---|
| AUTO Standby | "Quick Start", "Start Countdown", [Admin: "Emergency Override"] |
| AUTO Running | "Stop", "Switch to FORCE_OFF" |
| Manual Running | **"STOP" (large, prominent)**, + countdown timer if applicable |
| Countdown Running | **"STOP"**, "Add 5 min", countdown display |
| FORCE_OFF | "Resume AUTO", [Admin: "Emergency Override"] |
| Error active | "Clear Error" (after resolving cause), status read-only |
| Controller offline | All controls disabled, "Controller offline" message |

**Optimistic UI for latency [FIX D3]:**

Control actions should apply an optimistic UI state immediately, then confirm or rollback based on Firebase status:

```
User taps "Quick Start"
  → Button immediately shows loading spinner
  → run_mode badge shows "Starting…" (optimistic)
  → Dashboard writes manual_start = true
  → On next status update: if is_running = true → show "Manual Run" (confirmed)
  → If no confirmation within 8s → rollback to previous state + show "Command timed out"
```

The 8-second timeout covers two full Firebase cycles (3s each) plus margin.

### 11.5 Mobile-First Layout [FIX D5]

The primary use case is a mobile phone browser. All interactive elements must meet WCAG 2.1 AA touch target requirements.

**Touch target minimums:**

| Element | Minimum size |
|---|---|
| Stop button (primary action) | 64×64px |
| Quick Start button | 56×48px |
| Add 5 min button | 48×48px |
| Mode selector buttons | 48×48px each |
| Alert action buttons | 44×44px |
| Settings inputs | 44px height |

**Mobile layout (portrait, single column):**

```
┌─────────────────────────────┐
│  Header: system name + RSSI  │  Fixed top, 56px
├─────────────────────────────┤
│  Alert banners (if any)      │  Dynamic, ranked
├─────────────────────────────┤
│                              │
│    Tank visualization        │  240px min height
│    (animated fill)           │
│    Level % + mode badge      │
│                              │
├─────────────────────────────┤
│  Flow rate + sensor health   │  Two-column stat row
├─────────────────────────────┤
│  Run controls (contextual)   │  Full-width primary button
│                              │  Secondary buttons below
├─────────────────────────────┤
│  Mode controls               │  Segmented control or button row
├─────────────────────────────┤
│  ▼ Diagnostics (collapsed)   │  Tap to expand
│  ▼ Audit log (collapsed)     │
└─────────────────────────────┘
```

**Emergency Stop design:**
The Stop button — in Manual mode — uses maximum contrast (white text on red background), full screen width, and a minimum 64px height. It sits at the center of the screen without scrolling. This follows the principle of **Fitts's Law**: the most urgent action should be the largest and closest target.

### 11.6 Offline and Disconnected States [FIX D4]

Three distinct disconnection scenarios require different UI treatments:

| Scenario | Detection | UI response |
|---|---|---|
| **Dashboard loses internet** | Firebase `onValue` error callback fires | "You are offline. Reconnecting…" banner. All controls disabled. Last known status remains visible with timestamp. |
| **ESP32 offline, dashboard online** | Status not updated within 20s | "Controller offline — last seen Xs ago" banner. All controls disabled. Status data grayed out. |
| **Firebase degraded** | `onValue` receives no update AND no error (timeout) | After 30s, show "Connection issues — checking…". After 60s, treat as offline. |

The dashboard must distinguish between "I have no data" (gray everything out) and "I have stale data" (show data with a staleness badge). Stale data older than 20s should show a `"Last updated Xs ago"` label and all status values should be visually dimmed.

---

## 12. Dashboard Components (Detailed) [FIX D1, C1]

All control writes go through a single hook: `usePumpData`. No component writes directly to Firebase — components call hook methods only.

### 12.1 `usePumpData` Hook API

```typescript
interface UsePumpDataReturn {
  // Status (read-only)
  status: PumpStatus | null;
  control: PumpControl | null;
  isOnline: boolean;
  isAdmin: boolean;
  lastUpdatedAt: Date | null;

  // Run controls
  startManualRun: () => Promise<void>;    // writes manual_start = true (5s reset)
  stopRun: () => Promise<void>;           // writes manual_stop = true → reverts to AUTO [FIX A4]
  startCountdown: (minutes: number) => Promise<void>;
  addCountdownTime: () => Promise<void>;  // disabled until Firebase confirms false

  // Mode controls
  setModeAuto: () => Promise<void>;
  setModeForceOff: () => Promise<void>;
  setModeForceOn: () => Promise<void>;    // admin only; throws if !isAdmin

  // Error / maintenance
  clearError: () => Promise<void>;
  setBypassLevelSensor: (enabled: boolean) => Promise<void>;  // admin only

  // Admin
  requestReboot: () => Promise<void>;     // admin only
}
```

### 12.2 Component Map

| Component | Layer | Reads from hook | Writes via hook |
|---|---|---|---|
| `SystemStateCard` | 1 | `status.run_mode`, `status.water_level_percent`, `status.is_running`, `isOnline` | — |
| `AlertBanner` | 2 | All `is_*_error` flags, `bypass_level_sensor`, `auto_bypass_active`, `is_sleeping` | `clearError`, `setBypassLevelSensor` |
| `RunControls` | 3 | `status.run_mode`, `status.countdown_remaining_sec`, `isOnline` | `startManualRun`, `stopRun`, `startCountdown`, `addCountdownTime` |
| `ModeControls` | 3 | `control.mode`, `isAdmin`, `isOnline` | `setModeAuto`, `setModeForceOff`, `setModeForceOn` |
| `DiagnosticsPanel` | 4 | `status.flow_rate_lpm`, `status.level_sensor_health_pct`, `status.wifi_rssi`, `status.uptime_minutes`, `status.total_pump_cycles`, `status.level_last_valid_age_sec` | — |
| `AuditLog` | 4 | Firebase `/audit/events` (last 20) | — |
| `DeviceConfigModal` | Settings | `useDeviceConfig` | `useDeviceConfig.save()` |

### 12.3 `useDeviceConfig` Hook

Separate hook from `usePumpData`. Reads `/pump_system/config/device` once on mount (not real-time) and on explicit refresh. Writes on user save with optimistic UI.

```typescript
interface UseDeviceConfigReturn {
  config: DeviceConfig | null;
  isLoading: boolean;
  isDirty: boolean;
  save: () => Promise<void>;   // admin only; throws if !isAdmin
  reset: () => void;
}
```

Config is organized into logical tab groups in the modal:
1. **Tank Calibration** — `tank_empty_cm`, `tank_full_cm`
2. **Pump Thresholds** — `pump_start_level`, `pump_stop_level`, `max_pump_runtime_min`
3. **Dry-Run Protection** — `dry_run_threshold_lpm`, `dry_run_timeout_sec`, `flow_calibration_factor`
4. **Schedule** — `sleep_enabled`, `sleep_start_hour`, `sleep_end_hour`, `sleep_emergency_level`
5. **Advanced** — `level_sensor_failure_threshold`, `idle_sensor_interval_ms`, `idle_firebase_interval_ms`, `auto_bypass_on_sensor_fail`, `auto_bypass_delay_sec`
6. **Maintenance** — `bypass_level_sensor` toggle, reboot button

---

## 13. Data Flow Summary

| Direction | Firebase path | Writer | Reader | Latency |
|---|---|---|---|---|
| Dashboard → ESP32 | `/control/*` | Dashboard (via `usePumpData`) | Firmware (poll ~3s) | 0–6s |
| ESP32 → Dashboard | `/status` | Firmware (push ~3s) | Dashboard (`onValue`) | 0–3s |
| Shared config | `/config/device` | Dashboard (admin) | Firmware (poll ~30s), Dashboard | 0–30s |
| Admin list | `/config/admins` | Manual / Firebase console | Dashboard (on auth) | On sign-in |
| Audit | `/audit/events` | Dashboard | Dashboard (last 20 events) | On demand |

---

## 14. Change Log from v1.0

| Ref | Section | Change |
|---|---|---|
| FIX A1 | §7 | P2 (Maintenance Bypass) added to priority table. Priority numbering fixed. |
| FIX A2 | §4 | `is_maintenance_active` removed from status. `bypass_level_sensor` is the single source. |
| FIX A3 | §4, §6.3 | `"AUTO_STANDBY"` added as a `run_mode` value. `run_mode` values fully documented. |
| FIX A4 | §3, §6.2, §12 | `manual_stop` now reverts to `"AUTO"`, not `"FORCE_OFF"`. Documented stop sequence. |
| FIX A5 | §4.2 | Fault code table added with all codes, meanings, user messages, and recovery steps. |
| FIX B1 | §6.2 | Emergency Override stop path explicitly documented. UI must show guidance. |
| FIX B2 | §3 | One-shot reset delay specified as **5 seconds**. Double-fire prevention described. |
| FIX B3 | §8 | Bypass scope clarified: affects P2 (AUTO) and P4 (COUNTDOWN early-stop). |
| FIX B4 | §9 | Admin model fully documented: `/config/admins` schema, feature gate table, security rules. |
| FIX B5 | §2 | `presence/` node removed from RTDB layout. |
| FIX B6 | §5 | `sensor_failure_threshold` renamed to `level_sensor_failure_threshold`. |
| FIX C1 | §12 | All components map to hook methods. No direct Firebase writes from components. |
| FIX C2 | §4, §6.3 | `run_mode` clarified as firmware-derived, lives only in `/status`, never written by dashboard. |
| FIX C3 | §8.2 | Auto-bypass documented in UX section. `auto_bypass_active` exposed in dashboard with distinct UI. |
| FIX C4 | §7 | File reference `03_safety_pump.ino` removed. Logic lives in `smart_pump_controller.ino`. |
| FIX D1 | §11.1 | Four-layer information hierarchy defined. |
| FIX D2 | §11.3 | Alert ranking system defined. Dismissal model specified. |
| FIX D3 | §11.4 | Optimistic UI model defined. 8-second timeout specified. |
| FIX D4 | §11.6 | Three disconnection scenarios defined with distinct UI treatments. |
| FIX D5 | §11.5 | Mobile-first layout defined. Touch target minimums specified. Emergency Stop design specified. |

---

*End of Document — Firmware–Dashboard Design v2.0*
