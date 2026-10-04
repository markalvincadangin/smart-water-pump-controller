# WP-06D — Firebase Backend Implementation Report

**Status:** Implemented — static repository verification complete; runtime Firebase/emulator verification remains outstanding.

## Scope

WP-06D implements the finalized v1.1 SmartFlow system contract for Firebase Realtime Database validation and Cloud Functions notification handling.

No Android behavior, firmware behavior, or physical-system behavior was changed in this work package.

## Changes

### 1. RTDB validation — `database.rules.json`

Added defense-in-depth `.validate` rules for:

- `pump_start_level_pct`: number, 0–100
- `pump_stop_level_pct`: number, 0–100, and greater than start level
- `dry_run_threshold_lpm`: number, 0.1–10.0 L/min
- `max_pump_runtime_min`: number, 30–120 minutes
- desired `mode`: `MANUAL`, `COUNTDOWN`, or documented future `AUTO`
- desired `countdown_duration_min`: number, 1–120 minutes

Invalid values are rejected rather than silently clamped.

Firmware remains the final physical safety authority.

### 2. Cloud Function telemetry alignment — `functions/src/index.ts`

Updated notification telemetry reads to the canonical contract names:

- `water_level_percent`
- `flow_rate_lpm`

This removes the previous `waterLevel`/`flowRate` mismatch that could cause incorrect low-level notifications.

### 3. Cloud Function event alignment

Updated `onDeviceEventCreated` to recognize only the canonical firmware event codes:

- `EVT_DRY_RUN_LOCKOUT`
- `EVT_MAX_RUNTIME_EXCEEDED`

Removed the dead `COUNTDOWN_FINISHED` notification path because countdown expiry is state-transition-only for MVP.

The maximum-runtime notification now uses the `maxRuntime` notification tag and canonical preference name `maxRuntimeAlert`.

The existing `overflowAlert` preference is retained as a legacy compatibility alias so existing Android preference data is not silently broken. Android migration is intentionally deferred to the Android work packages.

### 4. Notification throttle vocabulary — `functions/src/notifications.ts`

Renamed the notification throttle type from legacy `overflow` to canonical `maxRuntime`.

Added a regression test covering the canonical `maxRuntime` throttle key.

## Verification

Repository-content verification confirmed:

- RTDB rules contain the required safety-sensitive validation expressions.
- Desired mode validation matches the v1.1 contract, including documented future `AUTO`.
- Countdown validation is 1–120 minutes.
- Canonical `EVT_*` event codes are present in the Cloud Function implementation.
- `COUNTDOWN_FINISHED` is no longer used by the Cloud Function.
- Canonical telemetry field names are used by the Cloud Function.
- Notification test coverage includes `maxRuntime`.

### Runtime test limitation

The execution environment could not reach GitHub to clone the branch, so `npm test` and Firebase Emulator rules tests could not be executed in this environment. No claim of a passing runtime test suite is made here.

The repository's existing Jest suite should be run locally from `functions/` after pulling this branch:

```bash
npm test -- --runInBand
npm run build
```

A Firebase Emulator rules test should also be added/run before treating the RTDB validation rules as physically/runtime verified.

## Remaining WP-06 work

WP-06D does **not** close the entire Firebase/backend audit.

Still requiring follow-up:

1. Runtime verification of RTDB rules with the Firebase Emulator.
2. Runtime verification of Cloud Function event notifications.
3. Canonical FCM token authority migration/deprecation across all writers/readers.
4. Firmware reported-shadow addition of `last_fault_code`.
5. Android alignment with the v1.1 contract.
6. Deployment/infrastructure verification.

## Acceptance boundary

This work package establishes the backend implementation direction required by contract v1.1. It does not establish production safety certification, physical pump safety, Firebase deployment correctness, or end-to-end FCM delivery.
