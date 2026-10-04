---
status: proposed-authoritative
version: 1.1
last-reviewed: 2026-10-04
source: repository audit
---

# SmartFlow System Contract

## 1. Purpose

This document defines the authoritative application-level contract between the SmartFlow ESP32 controller, Firebase RTDB, Cloud Functions, and Android client.

It exists to prevent firmware, backend, Android, diagrams, and documentation from describing different versions of the same system.

**Contract rule:**

> Desired state expresses intent. Firmware decides whether that intent is safe. Reported state describes what the device actually accepted and executed.

This contract covers the **current MVP boundary**:

- Manual operation
- Countdown/timed operation
- Safety interlocks and fault handling
- Device telemetry and reported state
- Firebase authorization/ownership boundaries
- Android interpretation of device state

AUTO is retained as an implementation foundation but is **not part of the MVP acceptance contract** until separately validated.

---

## 2. Authority Model

| Concern | Authoritative source |
|---|---|
| Physical pump ON/OFF | ESP32 firmware |
| Safety decision | ESP32 firmware |
| E-stop latch | ESP32 firmware |
| Dry-run/max-runtime lockout | ESP32 firmware |
| Sensor/RS-485 validity | ESP32 firmware |
| User control intent | `shadow/desired` |
| Actual device state shown to clients | `shadow/reported` |
| Telemetry measurements | `telemetry` |
| Device ownership | `ownership/ownerUid` |
| User-to-device navigation index | `users/{uid}/devices/{deviceId}` |
| Notification delivery policy | Cloud Functions + Android FCM client |
| Human-readable activity history | `events` |

A desired value must never be interpreted by Android as proof that the pump physically changed state.

---

## 3. Device Tree

The current contract is centered on:

```text
devices/{deviceId}/
├── ownership/
│   ├── ownerUid
│   └── state
├── metadata/
├── pairing/
│   └── current/
├── shadow/
│   ├── desired/
│   └── reported/
├── telemetry/
├── settings/
├── status/
├── diagnostics/
├── events/
└── fcmTokens/        # deprecated; user-level FCM tokens are authoritative
```

User navigation data is separate:

```text
users/{uid}/devices/{deviceId}
users/{uid}/notification_prefs/
```

The device must authenticate as its stable device principal. Ownership must not be inferred from a device's Firebase authentication identity.

---

## 4. Operating Modes

### 4.1 MANUAL

Manual mode represents persistent operator intent.

- `mode = MANUAL`
- `manual_desired = true` requests operation.
- `manual_desired = false` requests OFF.
- Safety checks always take precedence.
- Manual ON is never a safety bypass.

Expected reported run modes include:

- `MANUAL_ON`
- `MANUAL_OFF`
- `MANUAL_COOLDOWN`

### 4.2 COUNTDOWN

Countdown mode represents a locally timed pump run.

- `mode = COUNTDOWN`
- `countdown_start = true` is a one-shot start request.
- `countdown_duration_min` specifies the duration.
- The ESP32 owns the timer.
- Android/Firebase connectivity is not required for the timer to continue after the command is accepted.
- Expiry must result in the pump being OFF and the countdown becoming inactive.
- Safety lockouts may terminate the countdown early.

The intended valid duration range is **1–120 minutes**. Firmware must reject invalid values rather than silently accepting an unsafe or ambiguous value.

Expected reported run mode:

- `COUNTDOWN`

### 4.3 AUTO — NOT MVP

AUTO exists in firmware and current specifications, but it is not a validated MVP feature.

AUTO must not be presented as a production-ready capability until its thresholds, hysteresis, sensor-failure behavior, cycling behavior, and physical operation have been validated.

Future AUTO work must preserve all MVP safety invariants.

---

## 5. Desired Shadow

Path:

```text
devices/{deviceId}/shadow/desired
```

Current intended fields:

| Field | Type | Semantics |
|---|---|---|
| `mode` | string | `MANUAL`, `COUNTDOWN`, or future `AUTO` |
| `manual_desired` | boolean | Manual ON/OFF intent |
| `countdown_start` | boolean | One-shot countdown start intent |
| `countdown_duration_min` | integer | Requested countdown duration; valid range 1–120 min |
| `emergency_stop` | boolean | One-shot E-stop activation request |
| `reset_stop` | boolean | E-stop reset request |
| `clear_error` | boolean | Fault-clear request |
| `bypass_level_sensor` | boolean | Maintenance/test bypass; must not be normal operating mode |
| `bypass_flow_sensor` | boolean | Maintenance/test bypass; must not be normal operating mode |
| `reboot_request_id` | integer | Monotonic reboot request token |

### Desired-state rules

1. Commands are requests, not guarantees.
2. The firmware may reject or override a request because of safety state.
3. One-shot commands must be edge-triggered and must not be retriggered indefinitely by a persistent true value.
4. Safety bypass fields are configuration controls, not ordinary pump controls.
5. Normal production operation requires sensor bypasses disabled.
6. Android must not treat a successful RTDB write as command completion.

---

## 6. Reported Shadow

Path:

```text
devices/{deviceId}/shadow/reported
```

The reported shadow is the client-facing source of truth for actual logical device state.

Current firmware-emitted fields:

| Field | Type | Meaning |
|---|---|---|
| `run_mode` | string | Current logical run state |
| `is_running` | boolean | Firmware's current physical pump command/state |
| `is_error` | boolean | Any active pump/safety fault |
| `is_overflow_error` | boolean | Current max-runtime/overflow-named fault |
| `emergency_stop_latched` | boolean | Canonical E-stop state |
| `countdown_remaining_sec` | integer | Remaining local countdown time |
| `last_fault_message` | string | Human-readable current fault |
| `last_fault_code` | string | Machine-readable current fault code; empty when no fault |

The wider firmware specification also defines additional reported fields such as sensor health, bypass state, runtime diagnostics, and heartbeat metrics. Those fields should be treated as contract fields only after their actual serialization and Android DTO mapping are verified.

### Required future normalization

The contract should add explicit machine-readable fault information:

- `last_fault_code`
- `fault_active`
- `fault_category`

Human-readable messages must not be the sole source used by Android to determine behavior.

---

## 7. Telemetry

Path:

```text
devices/{deviceId}/telemetry
```

Primary current telemetry fields:

| Field | Type | Unit |
|---|---|---|
| `water_level_percent` | number | % |
| `flow_rate_lpm` | number | L/min |
| `ultrasonic_last_good_cm` | number | cm |

Telemetry must carry sufficient freshness information for the Android client to distinguish:

- LIVE
- DELAYED
- STALE
- UNAVAILABLE

Firmware safety decisions must use its own validated sensor freshness/stability state. Android freshness is a presentation/state-awareness concern and must never become the device safety authority.

---

## 8. Safety Contract

Safety has higher priority than user intent.

Conceptual precedence:

```text
Emergency Stop
    ↓
Dry-Run Protection
    ↓
Maximum Runtime Protection
    ↓
Tank-Full / High-Level Protection
    ↓
Sensor / RS-485 Validity
    ↓
Minimum Pump OFF Time
    ↓
User Intent
    ↓
Physical Pump Command
```

### 8.1 Emergency stop

- E-stop is latched by the ESP32.
- It forces the pump OFF.
- It remains latched until an explicit reset.
- Android must display the latched state from reported device state.

### 8.2 Dry-run

Current implementation uses:

- default threshold: **1.0 L/min**
- default timeout: **30 seconds**

These values are implementation/configuration values, not claims that the physical installation has already been validated.

A sustained low-flow condition while running causes a dry-run lockout and pump shutdown.

### 8.3 Maximum runtime

Current implementation uses:

- default maximum runtime: **120 minutes**

The existing firmware uses the fault code/name `OVERFLOW` for this path even though its message identifies maximum runtime as the cause.

This terminology must be corrected in the contract and implementation:

- `MAX_RUNTIME` = maximum runtime exceeded
- `TANK_FULL` = level reached configured stop threshold
- `OVERFLOW/HIGH_HIGH` = separate high-high/overflow condition if physically implemented

These conditions must not be conflated.

### 8.4 Sensor and communication failure

Validated stale/unstable RS-485 data must fail safe:

- block pump starts
- stop a running pump

unless an explicitly authorized maintenance bypass is active.

The system must not automatically enable a sensor bypass to recover from communication failure.

### 8.5 Minimum OFF time

The controller enforces a minimum pump OFF interval to prevent rapid cycling.

The exact configured interval is firmware-owned and must be documented from the authoritative configuration source.

---

## 9. Fault Vocabulary

The following machine-readable vocabulary is recommended as the authoritative contract:

| Code | Meaning | Typical severity |
|---|---|---|
| `E_STOP` | Emergency stop latched | Critical |
| `DRY_RUN` | Dry-run protection triggered | Critical |
| `MAX_RUNTIME` | Maximum runtime exceeded | Critical |
| `TANK_FULL` | Tank reached normal stop threshold | Advisory/Normal |
| `OVERFLOW` | High-high/overflow condition | Critical |
| `LEVEL_SENSOR` | Level sensor invalid/unavailable | Critical |
| `FLOW_SENSOR` | Flow sensor invalid/unavailable | Critical |
| `COMM_LOSS` | RS-485/remote sensor communication unavailable | Critical |
| `STALE_LEVEL` | Level data exceeded freshness limit | Critical |
| `MIN_OFF_TIME` | Pump temporarily blocked by minimum OFF time | Advisory |
| `COMMAND_REJECTED` | Requested action rejected by controller | Warning |

The implementation must use machine-readable codes for application logic. Human-readable messages are presentation text.

---

## 10. Events

Path:

```text
devices/{deviceId}/events/{eventId}
```

Events should contain at least:

- timestamp
- severity
- category
- code
- message

The canonical event namespace is the firmware/Android `EVT_*` registry. Cloud Functions must consume the same identifiers.

Examples currently represented by the firmware event system include:

- `EVT_DRY_RUN_WARN`
- `EVT_DRY_RUN_LOCKOUT`
- `EVT_DRY_RUN_CLEARED`
- `EVT_MAX_RUNTIME_EXCEEDED`
- `EVT_FAIL_SAFE_STOP`
- `EVT_RS485_TIMEOUT`
- `EVT_RS485_INVALID`

The event code is the stable identifier. Android notification titles/messages may change without changing the code. A separate `COUNTDOWN_FINISHED` event is **not part of the MVP contract**; countdown completion is represented by the reported state transition and desired-state cleanup.

---

## 11. Ownership and Authorization

The authoritative owner is:

```text
devices/{deviceId}/ownership/ownerUid
```

The user device index:

```text
users/{uid}/devices/{deviceId}
```

is a navigation/index structure, not the ownership authority.

Ownership changes must occur through trusted backend functions. Android must not directly write the authoritative ownership fields.

Device authentication is separate from user ownership.

Firebase RTDB rules must enforce:

- device principals can access only their own device paths
- owners can control only devices they own
- clients cannot directly mutate ownership
- reported/device telemetry writes are device-only
- settings writes require ownership

---

## 12. Notification Contract

Notifications should be generated from authoritative device events/fault codes, not inferred only from changing telemetry values.

The notification pipeline is:

```text
ESP32 event
  → Firebase event
  → Cloud Function
  → FCM
  → Android notification
```

Android foreground/background/terminated behavior must be verified separately.

Notification semantics must distinguish:

- critical safety event
- warning
- informational event
- cleared/recovery event

The canonical FCM token authority is:

```text
users/{uid}/notification_prefs/fcmTokens/{tokenId}
```

The device-level `devices/{deviceId}/fcmTokens` path is deprecated and must not be used by new code. Existing readers/writers must be migrated or removed during backend/Android implementation work.

An FCM token identifies an app installation/device instance for a user; it is not the ownership authority for a SmartFlow pump.

---

## 13. Android State Interpretation

Android should derive UI state from the combination of:

1. connection state
2. reported shadow
3. telemetry freshness
4. active fault state
5. pending command state

A control write should follow:

```text
Ready
  → Pending
  → Reported state changes
  → Completed
```

If the device rejects the request, times out, becomes unavailable, or enters a safety interlock:

```text
Pending
  → Rejected / TimedOut / OfflineBlocked / InterlockBlocked
```

The app must never display “Pump ON” solely because the user pressed the ON control.

The dashboard should distinguish at least:

- Connected + Running
- Connected + Stopped
- Connected + Starting/Pending
- Connected + Safety Interlocked
- Connected + Fault
- Device Offline
- Data Stale
- Data Unavailable

---

## 14. Bypass Policy

Sensor bypasses are maintenance/testing controls.

They are **not** normal recovery mechanisms.

Normal production state:

```text
bypass_level_sensor = false
bypass_flow_sensor = false
```

Current Android domain defaults contain both bypasses as `true`. Those defaults are **not authoritative production defaults** and must be corrected during the implementation audit.

The firmware's persisted NVS values and first-boot initialization must also be normalized so that a fresh installation does not unintentionally start with safety bypasses enabled.

Any UI exposing bypass controls must:

- clearly label them as maintenance/testing
- explain which safety protection is disabled
- require deliberate confirmation
- show the active bypass state prominently
- prevent bypass state from being mistaken for healthy sensor state

---

## 15. Configuration Ownership

The firmware owns enforcement of safety limits.

Cloud settings may provide user-configurable values only within firmware-validated safe bounds.

For every safety-sensitive setting:

1. Android validates for user experience.
2. Firebase rules/backend protect authorization.
3. Firmware validates the actual value before applying it.
4. Invalid values are rejected, not silently accepted.

This is especially required for:

- countdown duration
- dry-run threshold
- dry-run timeout
- maximum runtime
- pump start/stop thresholds
- future AUTO hysteresis/deadband settings

---

## 16. Countdown Contract

Countdown is local to the ESP32.

Required behavior:

1. Android writes the desired mode and countdown request.
2. ESP32 validates the duration.
3. ESP32 starts the local countdown.
4. ESP32 reports `COUNTDOWN` and remaining seconds.
5. Android may disconnect without cancelling the timer.
6. At expiry, ESP32 stops the pump and clears the countdown.
7. A higher-priority safety event may terminate the countdown early.
8. Android learns the final state from reported state/events.

The countdown duration boundary is **1–120 minutes** unless a future validated configuration changes this contract.

### Countdown expiry semantics

Countdown completion is a **state-transition-only MVP behavior**. The ESP32 must stop the pump, clear the countdown desired state, and transition the desired operating state toward `MANUAL`/OFF. The MVP does not require a dedicated `COUNTDOWN_FINISHED` event or push notification.

### Concurrent countdown requests

If a new countdown start request arrives while a countdown is already active, the controller must not silently restart or extend the active timer. The MVP behavior is to require the active countdown to finish/stop before accepting a new countdown start. Changing `countdown_duration_min` alone does not restart an active countdown.

---

## 17. Recovery Semantics

### E-stop

```text
Normal → E_STOP_LATCHED → explicit reset → normal eligible state
```

### Dry-run / maximum-runtime fault

```text
Running → fault → pump OFF → explicit clear/recovery → normal eligible state
```

### Communication/sensor failure

```text
Running/Start requested
  → invalid or stale sensor state
  → pump OFF / start blocked
  → valid stable data
  → normal control eligibility
```

The system must not silently clear a safety fault merely because connectivity returns.

---

## 18. Explicit MVP Boundary

### Included

- MANUAL
- COUNTDOWN
- E-stop
- dry-run protection
- maximum runtime protection
- tank-full stopping
- sensor/RS-485 fail-safe behavior
- minimum OFF time
- Firebase desired/reported shadow
- telemetry
- events
- Android authoritative-state presentation

### Not accepted as MVP

- fully validated AUTO control
- adaptive/predictive pump control
- AI-based pump decisions
- production-grade remote diagnostics beyond the documented contract
- unvalidated automatic sensor bypass/recovery

---

## 19. Firebase Validation and Backend Contract

Firebase RTDB rules provide defense in depth and must validate safety-sensitive client inputs in addition to authorization. Firmware remains the final safety authority.

At minimum, rules/backend validation must enforce:

- `pump_start_level_pct` is 0–100
- `pump_stop_level_pct` is 0–100 and greater than `pump_start_level_pct`
- dry-run threshold is within the firmware-supported range
- maximum runtime is within the firmware-supported range
- `countdown_duration_min` is 1–120 when supplied for a countdown request
- desired `mode` uses an allowed value
- expected primitive types are respected

Invalid values must be rejected rather than silently clamped or accepted.

## 20. Known Repository Mismatches Requiring Follow-up

This contract intentionally records mismatches instead of hiding them.

### P1 — Android model defaults

`ShadowDesired` currently defaults to:

- `mode = AUTO`
- `bypassLevelSensor = true`
- `bypassFlowSensor = true`

These values conflict with the intended MVP/safety contract and must be corrected.

### P1 — Countdown boundary

Firmware documentation states 1–120 minutes, but the current desired-state handler accepts positive durations without enforcing the documented upper bound at the boundary.

### P1 — Fault naming

Maximum-runtime protection is currently surfaced internally as `OVERFLOW`. This must be separated from a genuine tank overflow/high-high condition.

### P1 — Shadow completeness

The firmware's current serialized reported shadow is smaller than the broader firmware specification. The contract must be reconciled with actual DTOs and cloud consumers before implementation cleanup.

### P1 — Legacy RTDB paths

Some older documentation still references `/pump_system/control` and `/pump_system/status`, while current specifications use `/devices/{deviceId}/shadow` and `/devices/{deviceId}/status`.

Current implementation should be treated as authoritative only after the Firebase audit confirms all writers/readers.

### P1 — Notification telemetry fields

Cloud notification code has historically referenced telemetry names such as `waterLevel` and `flowRate`, while current firmware uses `water_level_percent` and `flow_rate_lpm`. WP-06 must trace this end-to-end.

---

## 21. Contract Status

This is a **proposed authoritative contract**, not proof that the complete physical system is verified.

The following still require independent verification:

- physical E-stop
- relay/contactor fail-safe behavior
- physical dry-run detection
- physical maximum-runtime shutdown
- tank-full behavior under real sensor conditions
- RS-485 EMI/noise behavior
- power-loss/reboot recovery
- production Firebase security rules
- FCM delivery in foreground/background/terminated Android states
- complete Android DTO mapping
- OTA and provisioning end-to-end behavior

The contract is now the working cross-system authority for WP-06 implementation. Its remaining physical/infrastructure verification items do not block documenting the software contract, but they do block claims of complete system validation.
