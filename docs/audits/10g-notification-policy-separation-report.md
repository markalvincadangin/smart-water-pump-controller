# WP-10 N-FIX-09 — Android EventRegistry / Backend Policy Separation

**Status:** VERIFIED (audit + contract clarification)  
**Scope:** Event identity, Android presentation mapping, Cloud Functions push-delivery policy

## 1. Audit Result

The implementation already has the intended architectural separation:

- Android `EventRegistry` maps event codes to local presentation data:
  - category
  - title
  - log message
  - notification/presentation message
- Cloud Functions `notificationPolicy.ts` independently owns push-delivery eligibility.
- `index.ts` consults `NOTIFICATION_POLICIES` for event-driven push delivery.
- Derived notifications use `DERIVED_NOTIFICATION_POLICIES` and do not depend on Android's registry.

Therefore, no runtime Android/backend refactor was necessary for N-FIX-09.

## 2. Canonical Event Identity

The firmware `EVT_*` event code remains the stable cross-system identifier.

Currently verified event-driven push policies are:

- `EVT_DRY_RUN_LOCKOUT`
- `EVT_MAX_RUNTIME_EXCEEDED`

Other canonical firmware events can remain valid events without being push-enabled.

This is intentional: **event existence does not imply push delivery**.

## 3. Boundary Rules

The system contract now explicitly states:

1. Firmware event codes are canonical identifiers.
2. Android `EventRegistry` is presentation-only.
3. Android `EventRegistry` must never enable FCM delivery.
4. Cloud Functions owns push-delivery policy.
5. Backend notification policies must reference canonical firmware event codes.
6. Backend and Android may have different user-facing wording without changing the event code.

## 4. Verification

Source inspection confirmed:

- `EventRegistry.kt` contains local presentation mappings only.
- `notificationPolicy.ts` contains the backend delivery policy.
- `index.ts` checks `NOTIFICATION_POLICIES[code]` before event-driven push delivery.
- Derived low-tank/pump-started delivery uses `DERIVED_NOTIFICATION_POLICIES`.
- No backend import/use of Android `EventRegistry` exists.
- No Android code is responsible for invoking the backend notification policy.

The canonical system contract was updated accordingly.

## 5. Scope Boundary

N-FIX-09 does not attempt to merge the two registries. Doing so would create the coupling this fix is intended to prevent.

It also does not require every firmware event to have a push policy.

## 6. Verification Status

No runtime code changes were required. Existing verification remains:

- Cloud Functions: 43/43 tests passing, TypeScript build passing.
- Android: 39/39 unit tests passing, compilation passing.

N-FIX-09 is therefore **VERIFIED** at the architecture/source-contract level.
