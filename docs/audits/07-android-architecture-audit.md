# WP-07 — Android Architecture Audit

## Status

**Audit complete. Implementation not yet authorized by this document.**

Scope: Android state model, Firebase shadow/telemetry/settings mapping, command handling, event/notification integration, FCM token ownership, and client-side validation against SmartFlow system contract v1.1.

Baseline:
- Branch: `docs/smartflow-system-contract`
- Contract: `docs/specifications/smartflow-system-contract.md`
- Android verification baseline: `./gradlew.bat compileDebugSources` passed during WP-06E validation.

## Executive Summary

The Android application compiles, but its current domain/DTO layer still reflects an older SmartFlow contract in several places.

The most important findings are:

1. **P0 — Firebase settings model is not aligned with the canonical settings schema.**
   `DeviceConfig` uses legacy/local property names (`lowLevelThreshold`, `dryRunThresholdLmin`, `maxOverflowTimeoutMins`) while firmware and RTDB use `pump_start_level_pct`, `pump_stop_level_pct`, `dry_run_threshold_lpm`, and `max_pump_runtime_min`. The Android DTO has no `@PropertyName` mapping for these settings.
2. **P1 — Reported shadow does not consume the newly canonical `last_fault_code`.**
   Firmware now publishes it, but `ShadowReportedDto`, `ShadowReported`, and `DashboardUiState` expose only the human-readable fault message.
3. **P1 — Deprecated device-level FCM token writes remain in three Android paths.**
   `FirebaseDeviceRepository.registerFcmToken()`, `MainActivity`, and `SmartFlowMessagingService` write `devices/{deviceId}/fcmTokens`, which the v1.1 contract formally deprecated.
4. **P1 — Android safety terminology remains coupled to legacy overflow naming.**
   The UI/domain use `isOverflowError` and `maxOverflowTimeoutMins`, while the canonical contract uses max-runtime semantics and `EVT_MAX_RUNTIME_EXCEEDED`.
5. **P1 — Countdown client validation is incorrect.**
   The countdown slider uses `maxOverflowTimeoutMins` as its maximum, so the UI can cap countdown duration at 30 minutes by default even though the contract permits 1–120 minutes independently.
6. **P1 — Configuration input ranges are stale.**
   Dry-run UI permits 0–5 L/min and max-runtime UI permits 5–60 minutes, while the contract requires 0.1–10.0 L/min and 30–120 minutes.
7. **P1 — Safety state precedence is represented incorrectly in one UI derivation.**
   `DashboardViewModel` checks generic error before emergency-stop when deriving `PumpState`, although the system contract gives emergency stop the highest safety precedence.
8. **P2 — Android defaults still model AUTO and sensor bypasses as ordinary defaults.**
   `ShadowDesiredDto` and domain `ShadowDesired` default to AUTO and both bypasses true. Firmware startup defaults are now false, and AUTO is not an MVP mode.
9. **P2 — Command-state model is richer than the current implementation.**
   `CommandState` defines Accepted, Completed, TimedOut, and InterlockBlocked, but the current dashboard command derivation primarily produces Ready/Pending/Rejected/OfflineBlocked. This is not a compile defect, but the state machine needs explicit semantics before UI polish.
10. **P2 — Telemetry freshness is inferred from app/Firebase connection rather than device telemetry age.**
    The current implementation can mark data stale when the app is disconnected, but the contract distinguishes LIVE/DELAYED/STALE/UNAVAILABLE and firmware owns sensor freshness for safety decisions. Android should not imply device sensor freshness solely from app connection state.

## Findings

### A-01 — Settings schema mismatch

**Severity: P0**

Firmware reads:

- `pump_start_level_pct`
- `pump_stop_level_pct`
- `dry_run_threshold_lpm`
- `max_pump_runtime_min`

Android `DeviceConfig` currently exposes:

- `lowLevelThreshold`
- `dryRunThresholdLmin`
- `maxOverflowTimeoutMins`

The Firebase model does not provide explicit RTDB property mappings for these fields.

### Risk

The Android settings UI can present and write a structure that does not represent the authoritative Firebase settings contract. This is more serious than terminology drift because the write payload can omit or create the wrong keys.

### Required direction

Replace the legacy settings model with a contract-aligned model:

- `pumpStartLevelPct`
- `pumpStopLevelPct`
- `dryRunThresholdLpm`
- `maxPumpRuntimeMin`

Map them explicitly to the RTDB snake_case names. Do not preserve `maxOverflowTimeoutMins` as the domain authority.

The exact valid ranges must be enforced client-side and server-side:

- start: 0–100%
- stop: 0–100% and greater than start
- dry-run threshold: 0.1–10.0 L/min
- max runtime: 30–120 min

### A-02 — Missing reported fault code

**Severity: P1**

Firmware now serializes:

`shadow/reported/last_fault_code`

Android `ShadowReportedDto` currently maps `last_fault_message` but not `last_fault_code`.

### Risk

The UI must parse human-readable text to infer fault meaning, while the contract explicitly provides a machine-readable fault code.

### Required direction

Add `lastFaultCode` through:

`ShadowReportedDto → ShadowReported → DashboardUiState → presentation`

Use the code for stable UI behavior and the message only as explanatory detail.

### A-03 — Deprecated device-level FCM token writes

**Severity: P1**

The v1.1 contract defines:

`users/{uid}/notification_prefs/fcmTokens/{tokenId}`

as canonical and formally deprecates:

`devices/{deviceId}/fcmTokens/{tokenId}`

Current Android writes the deprecated path from:

- `FirebaseDeviceRepository.registerFcmToken()`
- `MainActivity.onCreate()`
- `SmartFlowMessagingService.onNewToken()`

### Required direction

WP-07 implementation should consolidate registration to the canonical user-level path. Remove device-level writes after confirming no active backend reader depends on them. The existing Cloud Functions implementation already uses the user-level token authority.

### A-04 — Legacy overflow terminology

**Severity: P1**

Android still exposes:

- `isOverflowError`
- `maxOverflowTimeoutMins`
- `maxOverflow`

The canonical safety contract now distinguishes maximum runtime from actual overflow/tank-full conditions. Firmware events already use `EVT_MAX_RUNTIME_EXCEEDED`.

### Required direction

Use max-runtime terminology in the Android domain/UI. Do not blindly rename every firmware field in this WP; firmware compatibility fields remain part of the cross-system migration boundary.

### A-05 — Countdown maximum incorrectly coupled to max runtime

**Severity: P1**

`ControlPanel` passes `maxOverflowTimeoutMins` as `maxRuntimeLimitMins` and uses it as the countdown slider's upper bound.

The system contract defines countdown duration independently at **1–120 minutes**. Maximum pump runtime is a safety setting, not the countdown UI's duration ceiling.

### Required direction

Introduce an explicit countdown maximum of 120 minutes in the Android control domain/UI. Do not derive it from max pump runtime.

The firmware remains the final authority and already rejects countdown values outside 1–120.

### A-06 — Stale configuration UI ranges

**Severity: P1**

Current UI ranges:

- dry-run: 0–5 L/min
- max continuous run: 5–60 min

Canonical ranges:

- dry-run: 0.1–10.0 L/min
- max runtime: 30–120 min

### Required direction

Align the UI controls, validation messages, labels, and model constraints with the contract. Do not merely widen the sliders; also validate cross-field constraints such as stop level > start level.

### A-07 — Emergency-stop precedence in UI state derivation

**Severity: P1**

Current `DashboardViewModel` derives `PumpState.Error` when `isError || isOverflowError` before checking `emergencyStopLatched`.

The system contract defines emergency stop as the highest-priority safety condition.

### Risk

A state containing both an error and an emergency-stop latch can be presented as a generic error rather than an explicit interlock.

### Required direction

Derive UI state with explicit precedence:

1. Offline
2. Emergency-stop interlocked
3. Safety/fault state
4. Running/idle/transition state

The UI should clearly distinguish an emergency interlock from an ordinary fault.

### A-08 — Unsafe/stale Android desired-state defaults

**Severity: P2**

`ShadowDesiredDto` and `ShadowDesired` currently default to:

- `mode = AUTO`
- `bypassLevelSensor = true`
- `bypassFlowSensor = true`

The current firmware startup defaults are both bypasses false, and AUTO is outside the MVP boundary.

### Required direction

Android model defaults should represent the safe MVP baseline:

- MANUAL as the normal initial mode unless an existing RTDB value is being decoded
- both sensor bypasses false

Important: this is a model/default correction, not permission to automatically overwrite an existing device shadow with these values.

### A-09 — Command-state semantics need a defined lifecycle

**Severity: P2**

The domain declares:

- Ready
- Pending
- Accepted
- Completed
- Rejected
- TimedOut
- OfflineBlocked
- InterlockBlocked

The current dashboard derivation does not establish how Accepted/Completed/TimedOut are reached.

### Required direction

Before UI animation/polish, define a minimal command lifecycle based on observable device state:

**Ready → Pending → Reported transition → Completed**

Failure paths:

**Pending → Rejected / OfflineBlocked / InterlockBlocked / TimedOut**

Do not claim command acceptance merely because an RTDB write succeeds.

### A-10 — Telemetry freshness semantics are incomplete

**Severity: P2**

The repository converts telemetry to `TelemetryValue.Available` using local receipt time, then DashboardViewModel changes it to stale when Firebase connection is disconnected.

This does not establish whether the device's actual sensor data is fresh. The contract distinguishes application data availability from firmware safety freshness.

### Required direction

Keep firmware safety freshness authoritative. Android should expose:

- LIVE
- DELAYED
- STALE
- UNAVAILABLE

based on observable telemetry/status timestamps or an explicitly documented approximation. Do not imply that a Firebase connection alone proves sensor freshness.

## WP-07 Implementation Boundary

### In scope

1. Contract-aligned Android Firebase DTOs/domain models.
2. `last_fault_code` propagation.
3. FCM token consolidation.
4. Max-runtime terminology in Android domain/UI.
5. Countdown 1–120 validation.
6. Settings range and cross-field validation.
7. Emergency-stop UI state precedence.
8. Command-state lifecycle cleanup where needed.
9. Android tests for the above mappings and state transitions.

### Not in scope

- Firmware safety logic.
- Firebase Rules changes already completed in WP-06.
- Physical pump behavior.
- AUTO feature completion.
- Notification delivery redesign beyond token ownership and event mapping.
- Visual redesign of the Android application.

Those belong to later Android notification/UI work after the state contract is corrected.

## Recommended Implementation Order

1. **WP-07A — Android model/schema correction**
   - Settings model
   - Shadow reported fault code
   - Safe desired-state defaults
   - RTDB property mappings
2. **WP-07B — Command/state derivation**
   - Emergency-stop precedence
   - max-runtime fault semantics
   - Pending/completed/rejected/timeout rules
   - countdown state handling
3. **WP-07C — Validation**
   - Settings ranges
   - stop > start
   - countdown 1–120
4. **WP-07D — FCM token cleanup**
   - Remove deprecated device-level writers
   - Retain canonical user-level registration
5. **WP-07E — Android tests and compile verification**
   - DTO mapping tests
   - state derivation tests
   - validation tests
   - `./gradlew.bat compileDebugSources`

## Acceptance Criteria

WP-07 should not be considered complete until:

- Android reads/writes the canonical settings schema.
- `last_fault_code` reaches the UI state.
- No new Android code writes `devices/{deviceId}/fcmTokens`.
- Android uses max-runtime terminology for the user-facing safety setting.
- Countdown UI accepts 1–120 minutes independently of max runtime.
- Settings UI accepts only the contract ranges.
- Stop level cannot be saved at or below start level.
- Emergency stop is represented as an interlock before generic fault state.
- Desired-state defaults are safe and do not silently overwrite persisted device state.
- Android command UI never equates RTDB write success with physical command completion.
- Android build remains clean after changes.
- Automated tests cover the new contract mappings and state transitions.

## Verification Boundary

This audit is based on repository source and the v1.1 SmartFlow system contract. It does not establish:

- physical pump response,
- real FCM delivery on a device,
- Firebase Emulator allow/deny behavior,
- Android foreground/background/terminated notification behavior,
- hardware sensor freshness,
- production Firebase deployment behavior.

Those remain separate verification activities.
